#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "control_pannel.h"
#include "huffman_encoder.h"
#include "logger.h"
#include "sht30_driver.h"
// #include "hamming74_encoder.h"

#define SHT30_BATCH_SAMPLE_COUNT 128U
#define SHT30_RAW_PAYLOAD_SIZE (SHT30_BATCH_SAMPLE_COUNT * SHT30_PAYLOAD_SAMPLE_SIZE)
#define SHT30_SAMPLE_INTERVAL_MS 1000U
#define MODES_PER_BATCH 2U

static uint8_t s_raw_payload[SHT30_RAW_PAYLOAD_SIZE];
static uint8_t s_huffman_bitstream[1024];
static uint8_t s_serialized_codebook[520];
// static uint8_t s_hamming_bitstream[2048];
static encoder_log_record_t s_batch_records[MODES_PER_BATCH];

/* Trạng thái dữ liệu của lô mẫu SHT30 hiện tại */
static uint32_t s_batch_id = 0;
static bool s_has_batch_data = false;
static bool s_done_mode_a = false;
static bool s_done_mode_b = false;
static float s_avg_temp = 0.0f;
static float s_avg_rh = 0.0f;
static uint32_t s_dropped_samples = 0;
static uint32_t s_raw_crc32 = 0;

static huffman_codebook_t s_codebook;
static huffman_metrics_t s_h_metrics;
static int s_book_bytes = 0;
static uint32_t s_elapsed_b_us = 0;
static huffman_status_t s_h_stat = HUFFMAN_OK;

// Thu thập mẫu từ cảm biến SHT30
static void collect_sht30_batch(float *out_avg_temp, float *out_avg_rh, uint32_t *out_dropped) {
  size_t valid_count = 0;
  float sum_temp = 0.0f;
  float sum_rh = 0.0f;

  sht30_stats_t stats_before;
  sht30_get_stats(&stats_before);

  printk("\n[SHT30] Dang thu thap lo mau #%u (%u mau hop le, chu ky %u ms)...\n", s_batch_id + 1U, SHT30_BATCH_SAMPLE_COUNT, SHT30_SAMPLE_INTERVAL_MS);

  while (valid_count < SHT30_BATCH_SAMPLE_COUNT) {
    sht30_sample_t sample;
    sht30_status_t st = sht30_read_sample(&sample);

    if (st == SHT30_OK) {
      //[T_MSB, T_LSB, RH_MSB, RH_LSB]
      sht30_pack_sample_be(&sample, &s_raw_payload[valid_count * SHT30_PAYLOAD_SAMPLE_SIZE]);

      float t_c = sht30_raw_to_celsius(sample.t_raw);
      float rh_pct = sht30_raw_to_humidity(sample.rh_raw);
      sum_temp += t_c;
      sum_rh += rh_pct;
      valid_count++;

      // In tiến độ
      if (valid_count == 1U || (valid_count % 8U == 0U) || valid_count == SHT30_BATCH_SAMPLE_COUNT) {
        printk("  [Mau %3u/%3u] T_raw=0x%04X (%.2f C) | RH_raw=0x%04X (%.2f %%)\n", (unsigned)valid_count, SHT30_BATCH_SAMPLE_COUNT, sample.t_raw, (double)t_c, sample.rh_raw, (double)rh_pct);
      }

      if (valid_count < SHT30_BATCH_SAMPLE_COUNT && SHT30_SAMPLE_INTERVAL_MS > SHT30_MEAS_DELAY_MS) {
        k_msleep(SHT30_SAMPLE_INTERVAL_MS - SHT30_MEAS_DELAY_MS);
      }
    } else {
      printk("  [SHT30 ERROR] Loi doc cam bien (ma loi: %d) -> Bo mau sai, thu lai...\n", (int)st);
      k_msleep(500);
    }
  }

  sht30_stats_t stats_after;
  sht30_get_stats(&stats_after);

  if (out_avg_temp != NULL) {
    *out_avg_temp = sum_temp / (float)SHT30_BATCH_SAMPLE_COUNT;
  }
  if (out_avg_rh != NULL) {
    *out_avg_rh = sum_rh / (float)SHT30_BATCH_SAMPLE_COUNT;
  }
  if (out_dropped != NULL) {
    *out_dropped = (stats_after.i2c_errors - stats_before.i2c_errors) + (stats_after.crc_errors - stats_before.crc_errors);
  }
}

/* Thực thi thu thập lô mẫu SHT30 (Chế độ COLLECT) */
static void handle_collect_mode(void) {
  collect_sht30_batch(&s_avg_temp, &s_avg_rh, &s_dropped_samples);
  s_batch_id++;

  const size_t raw_bytes_size = SHT30_RAW_PAYLOAD_SIZE;
  s_raw_crc32 = logger_calculate_crc32(s_raw_payload, raw_bytes_size);

  /* Tính trước thống kê nguồn/Huffman trên lô mẫu để có Entropy H(X) */
  uint32_t start_b = k_cycle_get_32();
  s_h_stat = huffman_encode(s_raw_payload, raw_bytes_size, s_huffman_bitstream, sizeof(s_huffman_bitstream), &s_codebook, &s_h_metrics);
  s_book_bytes = (s_h_stat == HUFFMAN_OK) ? huffman_serialize_codebook(&s_codebook, s_serialized_codebook, sizeof(s_serialized_codebook)) : 0;
  s_elapsed_b_us = k_cyc_to_us_floor32(k_cycle_get_32() - start_b);

  s_has_batch_data = true;
  s_done_mode_a = false;
  s_done_mode_b = false;

  printk("\n[COLLECT OK] Da thu thap xong Lo #%u (%u mau, %u bytes, CRC32=0x%08X, T_tb=%.2f C, RH_tb=%.2f %%).\n", s_batch_id, SHT30_BATCH_SAMPLE_COUNT, (unsigned)raw_bytes_size, s_raw_crc32, (double)s_avg_temp, (double)s_avg_rh);
  printk("             Hay nhap ENCODER_A hoac ENCODER_B de xuat ket qua ma hoa.\n");
}

/* Thực thi Chế độ A: RAW */
static void handle_encoder_a_mode(void) {
  if (!s_has_batch_data) {
    printk("\n[WARN] Chua co du lieu cam bien! Vui long nhap lenh COLLECT truoc.\n");
    return;
  }

  printk("\n[STATE: ENCODER_A] Thực thi Chế độ A (RAW) cho Lô #%u...\n", s_batch_id);
  logger_log_mode_a(s_batch_id, SHT30_BATCH_SAMPLE_COUNT, s_dropped_samples, s_avg_temp, s_avg_rh, s_raw_payload, SHT30_RAW_PAYLOAD_SIZE, s_raw_crc32, s_h_metrics.entropy, &s_batch_records[0]);
  s_done_mode_a = true;

  if (s_done_mode_a && s_done_mode_b) {
    logger_print_comparison_summary(s_batch_records, MODES_PER_BATCH);
  }
}

/* Thực thi Chế độ B: Canonical Huffman */
static void handle_encoder_b_mode(void) {
  if (!s_has_batch_data) {
    printk("\n[WARN] Chua co du lieu cam bien! Vui long nhap lenh COLLECT truoc.\n");
    return;
  }

  printk("\n[STATE: ENCODER_B] Thực thi Chế độ B (Canonical Huffman) cho Lô #%u...\n", s_batch_id);
  uint32_t start_b = k_cycle_get_32();
  s_h_stat = huffman_encode(s_raw_payload, SHT30_RAW_PAYLOAD_SIZE, s_huffman_bitstream, sizeof(s_huffman_bitstream), &s_codebook, &s_h_metrics);
  s_book_bytes = (s_h_stat == HUFFMAN_OK) ? huffman_serialize_codebook(&s_codebook, s_serialized_codebook, sizeof(s_serialized_codebook)) : 0;
  s_elapsed_b_us = k_cyc_to_us_floor32(k_cycle_get_32() - start_b);

  logger_log_mode_b(s_batch_id, SHT30_BATCH_SAMPLE_COUNT, s_dropped_samples, s_avg_temp, s_avg_rh, SHT30_RAW_PAYLOAD_SIZE, s_raw_crc32, &s_codebook, &s_h_metrics, s_book_bytes, s_elapsed_b_us, s_h_stat, &s_batch_records[1]);
  s_done_mode_b = true;

  if (s_done_mode_a && s_done_mode_b) {
    logger_print_comparison_summary(s_batch_records, MODES_PER_BATCH);
  }
}

int main(void) {
  control_panel_t current_state = START;
  char raw_cmd[64] = {0};

  while (1) {
    switch (current_state) {
    case START:
      logger_init();
      if (control_pannel_init() != CP_TRUE) {
        printk("[ERROR] Khong the khoi tao UART Console cho Control Panel!\n");
      }
      logger_print_banner();
      logger_print_csv_header();

      printk("\n[HARDWARE] Khoi tao cam bien SHT30 qua I2C (SDA=GPIO8, SCL=GPIO9, Addr=0x44): ");
      while (sht30_init() != SHT30_OK) {
        printk("FAILED! Kiem tra dau noi I2C SHT30, thu lai sau 2 giay...\n");
        k_sleep(K_SECONDS(2));
      }
      printk("READY!\n");

      control_pannel_print_menu();
      current_state = IDLE;
      continue;

    case IDLE:
      printk("\n[STATE: IDLE] He thong dang o che do cho.\n");
      break;

    case COLLECT:
      handle_collect_mode();
      break;

    case ENCODER_A:
      handle_encoder_a_mode();
      break;

    case ENCODER_B:
      handle_encoder_b_mode();
      break;

      // case ENCODER_C:
      //   /* Tạm khóa ở phiên bản hiện tại */
      //   break;

      // case TRANSMIT_UART:
      //   /* Tạm khóa ở phiên bản hiện tại */
      //   break;

    case ERROR:
    default:
      printk("\n[STATE: ERROR] Lenh Serial \"%s\" KHONG HOP LE!\n", raw_cmd);
      printk("               Cac lenh hop le: [IDLE, COLLECT, ENCODER_A, ENCODER_B]\n");
      control_pannel_print_menu();
      break;
    }

    printk("\n[CONTROL PANEL] Nhap lenh (IDLE | COLLECT | ENCODER_A | ENCODER_B): ");
    current_state = read_serial_line(raw_cmd, sizeof(raw_cmd));
  }

  return 0;
}
