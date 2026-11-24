#include "factory.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "control_pannel.h"
#include "hamming74_encoder.h"
#include "huffman_encoder.h"
#include "logger.h"
#include "sht30_driver.h"
#include "tx.h"

#define SHT30_BATCH_SAMPLE_COUNT 128U
#define SHT30_SAMPLE_INTERVAL_MS 1000U
#define HUFFMAN_STREAM_BUF_SIZE 256U
#define CODEBOOK_BUF_SIZE 520U
#define HAMMING_STREAM_BUF_SIZE 512U
#define LN2 0.6931471805599453f
#define LOG_STAGE_START_FMT "[LOG_STAGE_START] batch=%u stage=%s\n"
#define LOG_STAGE_END_FMT "[LOG_STAGE_END] batch=%u stage=%s\n"

typedef struct {
  uint8_t huffman_bitstream[HUFFMAN_STREAM_BUF_SIZE];
  uint8_t serialized_codebook[CODEBOOK_BUF_SIZE];
  uint8_t hamming_bitstream[HAMMING_STREAM_BUF_SIZE];
  huffman_codebook_t codebook;
  huffman_metrics_t h_metrics;
  hamming74_metrics_t ham_metrics;
  size_t book_bytes;
  huffman_status_t h_stat;
  hamming74_status_t ham_stat;
  uint32_t elapsed_huff_us;
  uint32_t elapsed_ham_us;
} stream_enc_t;

typedef struct {
  uint8_t temp[SHT30_BATCH_SAMPLE_COUNT];
  uint8_t humd[SHT30_BATCH_SAMPLE_COUNT];
  stream_enc_t enc_temp;
  stream_enc_t enc_humd;
  uint32_t batch_id;
  uint16_t tx_seq_id;
  bool has_batch;
  bool has_encoded;
  float avg_temp;
  float avg_rh;
  float entropy_temp;
  float entropy_humd;
  uint32_t dropped_samples;
} factory_ctx_t;

static factory_ctx_t s_ctx;
static uint16_t s_ent_freq[HUFFMAN_MAX_SYMBOLS];

static uint8_t round_clamp_u8(float val, uint8_t max_val) {
  if (val <= 0.0f) {
    return 0U;
  }
  float max_f = (float)max_val;
  if (val >= max_f) {
    return max_val;
  }
  uint32_t rounded = (uint32_t)(val + 0.5f);
  return (rounded > (uint32_t)max_val) ? max_val : (uint8_t)rounded;
}

static float compute_shannon_entropy(const uint8_t *data, size_t len) {
  if (data == NULL || len == 0U) {
    return 0.0f;
  }

  memset(s_ent_freq, 0, sizeof(s_ent_freq));
  for (size_t i = 0U; i < len; i++) {
    s_ent_freq[data[i]]++;
  }

  float entropy = 0.0f;
  for (size_t i = 0U; i < HUFFMAN_MAX_SYMBOLS; i++) {
    if (s_ent_freq[i] > 0U) {
      float p = (float)s_ent_freq[i] / (float)len;
      entropy -= p * (logf(p) / LN2);
    }
  }
  return entropy;
}

static bool require_batch_data(void) {
  if (!s_ctx.has_batch) {
    printk("\n[WARN] Chua co du lieu cam bien! Vui long nhap lenh COLLECT truoc.\n");
    return false;
  }
  return true;
}

static void collect_sht30_batch(void) {
  size_t valid_count = 0U;
  float sum_temp = 0.0f;
  float sum_rh = 0.0f;
  sht30_stats_t stats_before;

  sht30_get_stats(&stats_before);
  printk("\n[SHT30] Dang thu thap lo mau #%u (%u mau hop le, chu ky %u ms)...\n", (unsigned)s_ctx.batch_id, SHT30_BATCH_SAMPLE_COUNT, SHT30_SAMPLE_INTERVAL_MS);

  while (valid_count < SHT30_BATCH_SAMPLE_COUNT) {
    sht30_sample_t sample;
    sht30_status_t st = sht30_read_sample(&sample);

    if (st == SHT30_OK) {
      float t_c = sht30_raw_to_celsius(sample.t_raw);
      float rh_pct = sht30_raw_to_humidity(sample.rh_raw);
      uint8_t t_u8 = round_clamp_u8(t_c, 255U);
      uint8_t h_u8 = round_clamp_u8(rh_pct, 100U);

      s_ctx.temp[valid_count] = t_u8;
      s_ctx.humd[valid_count] = h_u8;
      sum_temp += t_c;
      sum_rh += rh_pct;
      valid_count++;

      if (valid_count == 1U || (valid_count % 8U == 0U) || valid_count == SHT30_BATCH_SAMPLE_COUNT) {
        printk("  [Mau %3u/%3u] T=%.2f C -> %3u (0x%02X) | RH=%.2f %% -> %3u (0x%02X)\n", (unsigned)valid_count, SHT30_BATCH_SAMPLE_COUNT, (double)t_c, (unsigned)t_u8, t_u8, (double)rh_pct, (unsigned)h_u8, h_u8);
      }

      if (valid_count < SHT30_BATCH_SAMPLE_COUNT && SHT30_SAMPLE_INTERVAL_MS > SHT30_MEAS_DELAY_MS) {
        k_msleep(SHT30_SAMPLE_INTERVAL_MS - SHT30_MEAS_DELAY_MS);
      }
    } else {
      printk("  [SHT30 ERROR] Loi doc cam bien (ma loi: %d) -> Thu lai...\n", (int)st);
      k_msleep(500);
    }
  }

  sht30_stats_t stats_after;
  sht30_get_stats(&stats_after);

  s_ctx.avg_temp = sum_temp / (float)SHT30_BATCH_SAMPLE_COUNT;
  s_ctx.avg_rh = sum_rh / (float)SHT30_BATCH_SAMPLE_COUNT;
  s_ctx.dropped_samples = stats_after.i2c_errors - stats_before.i2c_errors;
}

static void run_huffman_on_stream(const uint8_t *in_data, stream_enc_t *enc) {
  uint32_t start = k_cycle_get_32();
  enc->h_stat = huffman_encode(in_data, SHT30_BATCH_SAMPLE_COUNT, enc->huffman_bitstream, sizeof(enc->huffman_bitstream), &enc->codebook, &enc->h_metrics);
  if (enc->h_stat == HUFFMAN_OK) {
    int book_len = huffman_serialize_codebook(&enc->codebook, enc->serialized_codebook, sizeof(enc->serialized_codebook));
    enc->book_bytes = (book_len > 0) ? (size_t)book_len : 0U;
  } else {
    enc->book_bytes = 0U;
  }
  enc->elapsed_huff_us = k_cyc_to_us_floor32(k_cycle_get_32() - start);
}

static void run_hamming_on_stream(stream_enc_t *enc) {
  memset(&enc->ham_metrics, 0, sizeof(enc->ham_metrics));
  enc->elapsed_ham_us = 0U;
  enc->ham_stat = HAMMING74_OK;

  if (enc->h_stat != HUFFMAN_OK) {
    enc->ham_stat = HAMMING74_ERR_PARAM;
    return;
  }

  size_t out_bits = 0U;
  uint32_t start = k_cycle_get_32();
  enc->ham_stat = hamming74_encode(enc->huffman_bitstream, enc->h_metrics.coded_valid_bits, enc->hamming_bitstream, sizeof(enc->hamming_bitstream), &out_bits, &enc->ham_metrics);
  enc->elapsed_ham_us = k_cyc_to_us_floor32(k_cycle_get_32() - start);
}

static void build_total_huffman_metrics(huffman_metrics_t *out_m, uint16_t *out_syms, size_t *out_book_bytes) {
  const huffman_metrics_t *mt = &s_ctx.enc_temp.h_metrics;
  const huffman_metrics_t *mh = &s_ctx.enc_humd.h_metrics;

  size_t raw_total = SHT30_BATCH_SAMPLE_COUNT * 2U;
  size_t total_bits = mt->coded_valid_bits + mh->coded_valid_bits;
  size_t total_bytes = mt->coded_bytes_count + mh->coded_bytes_count;
  size_t total_book = s_ctx.enc_temp.book_bytes + s_ctx.enc_humd.book_bytes;
  float avg_entropy = 0.5f * (mt->entropy + mh->entropy);
  float avg_len = (float)total_bits / (float)raw_total;

  out_m->raw_bytes_count = raw_total;
  out_m->coded_valid_bits = total_bits;
  out_m->coded_bytes_count = total_bytes;
  out_m->codebook_bytes_size = total_book;
  out_m->entropy = avg_entropy;
  out_m->avg_code_len = avg_len;
  out_m->coding_efficiency = (avg_len > 0.0f) ? (avg_entropy / avg_len) : 0.0f;
  out_m->payload_comp_ratio = (total_bits > 0U) ? (((float)raw_total * 8.0f) / (float)total_bits) : 0.0f;
  out_m->net_comp_ratio = ((total_bytes + total_book) > 0U) ? ((float)raw_total / (float)(total_bytes + total_book)) : 0.0f;

  *out_syms = (uint16_t)(s_ctx.enc_temp.codebook.used_symbols_count + s_ctx.enc_humd.codebook.used_symbols_count);
  *out_book_bytes = total_book;
}

static void handle_start_mode(void) {
  logger_init();
  if (control_pannel_init() != CP_TRUE) {
    printk("[ERROR] Khong the khoi tao UART Console cho Control Panel!\n");
  }
  if (tx_init() != TX_OK) {
    printk("[ERROR] Khong the khoi tao UART1 TX (GPIO17)!\n");
  }
  logger_print_banner();
  logger_print_csv_header();

  printk("\n[HARDWARE] Khoi tao cam bien SHT30 qua I2C (SDA=GPIO8, SCL=GPIO9, Addr=0x44): ");
  while (sht30_init() != SHT30_OK) {
    printk("FAILED! Kiem tra dau noi I2C SHT30, thu lai sau 2 giay...\n");
    k_sleep(K_SECONDS(2));
  }
  printk("READY!\n");
}

static void handle_collect_mode(void) {
  s_ctx.batch_id++;
  printk(LOG_STAGE_START_FMT, (unsigned)s_ctx.batch_id, "collect");

  collect_sht30_batch();
  s_ctx.entropy_temp = compute_shannon_entropy(s_ctx.temp, SHT30_BATCH_SAMPLE_COUNT);
  s_ctx.entropy_humd = compute_shannon_entropy(s_ctx.humd, SHT30_BATCH_SAMPLE_COUNT);
  s_ctx.has_batch = true;
  s_ctx.has_encoded = false;

  printk("\n[COLLECT OK] Lo #%u (%u mau | T_tb=%.2f C, H(T)=%.4f | RH_tb=%.2f %%, H(RH)=%.4f)\n", (unsigned)s_ctx.batch_id, SHT30_BATCH_SAMPLE_COUNT, (double)s_ctx.avg_temp, (double)s_ctx.entropy_temp, (double)s_ctx.avg_rh, (double)s_ctx.entropy_humd);

  logger_print_stream_collected("TEMP", "C", s_ctx.batch_id, s_ctx.temp, SHT30_BATCH_SAMPLE_COUNT);
  logger_print_probability_table("TEMP", s_ctx.temp, SHT30_BATCH_SAMPLE_COUNT);

  logger_print_stream_collected("HUMD", "%%RH", s_ctx.batch_id, s_ctx.humd, SHT30_BATCH_SAMPLE_COUNT);
  logger_print_probability_table("HUMD", s_ctx.humd, SHT30_BATCH_SAMPLE_COUNT);

  printk("\n             Hay chon ENCODER de ma hoa Huffman + Hamming(7,4).\n");
  printk(LOG_STAGE_END_FMT, (unsigned)s_ctx.batch_id, "collect");
}

static bool handle_encoder_mode(void) {
  if (!require_batch_data()) {
    return false;
  }

  printk(LOG_STAGE_START_FMT, (unsigned)s_ctx.batch_id, "encoder");
  printk("\n[STATE: ENCODER] Thuc thi ENCODER (Canonical Huffman + Hamming 7,4 theo LTTT06) cho Lo #%u...\n", (unsigned)s_ctx.batch_id);
  run_huffman_on_stream(s_ctx.temp, &s_ctx.enc_temp);
  run_huffman_on_stream(s_ctx.humd, &s_ctx.enc_humd);

  if (s_ctx.enc_temp.h_stat == HUFFMAN_OK) {
    logger_print_huffman_table("TEMP", &s_ctx.enc_temp.codebook, &s_ctx.enc_temp.h_metrics, s_ctx.enc_temp.huffman_bitstream);
  }
  if (s_ctx.enc_humd.h_stat == HUFFMAN_OK) {
    logger_print_huffman_table("HUMD", &s_ctx.enc_humd.codebook, &s_ctx.enc_humd.h_metrics, s_ctx.enc_humd.huffman_bitstream);
  }

  hamming74_params_t params;
  hamming74_compute_params(&params);
  logger_print_hamming_theory(&params);

  run_hamming_on_stream(&s_ctx.enc_temp);
  run_hamming_on_stream(&s_ctx.enc_humd);

  if (s_ctx.enc_temp.ham_stat == HAMMING74_OK) {
    logger_print_hamming_stream_steps("TEMP", s_ctx.enc_temp.huffman_bitstream, s_ctx.enc_temp.h_metrics.coded_valid_bits, s_ctx.enc_temp.hamming_bitstream, &s_ctx.enc_temp.ham_metrics, s_ctx.enc_temp.elapsed_ham_us);
  }
  if (s_ctx.enc_humd.ham_stat == HAMMING74_OK) {
    logger_print_hamming_stream_steps("HUMD", s_ctx.enc_humd.huffman_bitstream, s_ctx.enc_humd.h_metrics.coded_valid_bits, s_ctx.enc_humd.hamming_bitstream, &s_ctx.enc_humd.ham_metrics, s_ctx.enc_humd.elapsed_ham_us);
  }

  huffman_metrics_t total_m;
  uint16_t total_syms = 0U;
  size_t total_book = 0U;
  build_total_huffman_metrics(&total_m, &total_syms, &total_book);

  size_t total_ham_in_bits = s_ctx.enc_temp.ham_metrics.in_bits_count + s_ctx.enc_humd.ham_metrics.in_bits_count;
  size_t total_ham_out_bits = s_ctx.enc_temp.ham_metrics.out_bits_count + s_ctx.enc_humd.ham_metrics.out_bits_count;
  hamming74_metrics_t total_ham = {
      .in_bits_count = total_ham_in_bits,
      .in_nibbles_count = s_ctx.enc_temp.ham_metrics.in_nibbles_count + s_ctx.enc_humd.ham_metrics.in_nibbles_count,
      .out_bits_count = total_ham_out_bits,
      .out_bytes_count = s_ctx.enc_temp.ham_metrics.out_bytes_count + s_ctx.enc_humd.ham_metrics.out_bytes_count,
      .code_rate = (total_ham_out_bits > 0U) ? ((float)total_ham_in_bits / (float)total_ham_out_bits) : 0.0f
  };

  uint32_t us_t = s_ctx.enc_temp.elapsed_huff_us + s_ctx.enc_temp.elapsed_ham_us;
  uint32_t us_h = s_ctx.enc_humd.elapsed_huff_us + s_ctx.enc_humd.elapsed_ham_us;
  int st_t = (s_ctx.enc_temp.h_stat == HUFFMAN_OK && s_ctx.enc_temp.ham_stat == HAMMING74_OK) ? 0 : -1;
  int st_h = (s_ctx.enc_humd.h_stat == HUFFMAN_OK && s_ctx.enc_humd.ham_stat == HAMMING74_OK) ? 0 : -1;
  int st_total = (st_t == 0 && st_h == 0) ? 0 : -1;

  logger_log_encoder(s_ctx.batch_id, ENCODER_STREAM_TEMP, SHT30_BATCH_SAMPLE_COUNT, s_ctx.dropped_samples, s_ctx.avg_temp, s_ctx.avg_rh, SHT30_BATCH_SAMPLE_COUNT, s_ctx.enc_temp.codebook.used_symbols_count, s_ctx.enc_temp.book_bytes, &s_ctx.enc_temp.h_metrics, &s_ctx.enc_temp.ham_metrics, us_t, st_t);
  logger_log_encoder(s_ctx.batch_id, ENCODER_STREAM_HUMD, SHT30_BATCH_SAMPLE_COUNT, s_ctx.dropped_samples, s_ctx.avg_temp, s_ctx.avg_rh, SHT30_BATCH_SAMPLE_COUNT, s_ctx.enc_humd.codebook.used_symbols_count, s_ctx.enc_humd.book_bytes, &s_ctx.enc_humd.h_metrics, &s_ctx.enc_humd.ham_metrics, us_h, st_h);
  logger_log_encoder(s_ctx.batch_id, ENCODER_STREAM_TOTAL, SHT30_BATCH_SAMPLE_COUNT, s_ctx.dropped_samples, s_ctx.avg_temp, s_ctx.avg_rh, SHT30_BATCH_SAMPLE_COUNT * 2U, total_syms, total_book, &total_m, &total_ham, us_t + us_h, st_total);

  s_ctx.has_encoded = (st_total == 0);
  printk(LOG_STAGE_END_FMT, (unsigned)s_ctx.batch_id, "encoder");
  return s_ctx.has_encoded;
}

static void send_stream_over_uart(encoder_stream_t stream, const stream_enc_t *enc) {
  if (enc->h_stat != HUFFMAN_OK || enc->ham_stat != HAMMING74_OK) {
    printk("[TX_UART ERROR] Luong %s chua co du lieu Hamming hop le!\n", logger_stream_to_str(stream));
    return;
  }

  tx_frame_t frame = {0};
  frame.mode = (uint8_t)ENCODER_MODE_DEFAULT;
  frame.stream = (uint8_t)stream;
  frame.seq_id = ++s_ctx.tx_seq_id;
  frame.sample_count = (uint16_t)SHT30_BATCH_SAMPLE_COUNT;
  frame.codebook_len = (uint16_t)enc->book_bytes;
  frame.codebook = enc->serialized_codebook;
  frame.payload_bit_count = (uint32_t)enc->ham_metrics.out_bits_count;
  frame.payload_len = (uint16_t)enc->ham_metrics.out_bytes_count;
  frame.payload = enc->hamming_bitstream;

  tx_status_t st = tx_send_frame(&frame);
  if (st != TX_OK) {
    printk("[TX_UART ERROR] Loi gui frame %s (status=%d)\n", logger_stream_to_str(stream), (int)st);
    return;
  }
  tx_print_sent_bits(&frame);
}

static void handle_transmit_uart_mode(void) {
  if (!require_batch_data()) {
    return;
  }
  if (!s_ctx.has_encoded) {
    printk("\n[WARN] Lo #%u chua duoc ma hoa! Vui long chay ENCODER truoc.\n", (unsigned)s_ctx.batch_id);
    return;
  }

  printk(LOG_STAGE_START_FMT, (unsigned)s_ctx.batch_id, "transmit_uart");
  printk("\n[STATE: TRANSMIT_UART] Dang gui du lieu ENCODER (Lo #%u) qua UART1 (TX=GPIO17, 115200bps)...\n", (unsigned)s_ctx.batch_id);
  send_stream_over_uart(ENCODER_STREAM_TEMP, &s_ctx.enc_temp);
  send_stream_over_uart(ENCODER_STREAM_HUMD, &s_ctx.enc_humd);
  printk(LOG_STAGE_END_FMT, (unsigned)s_ctx.batch_id, "transmit_uart");
}

void factory_run(void) {
  control_panel_t current_state = START;
  cp_menu_stage_t active_menu = CP_MENU_IDLE;
  char raw_cmd[64] = {0};

  while (1) {
    switch (current_state) {
    case START:
      handle_start_mode();
      current_state = IDLE;
      continue;

    case IDLE:
      active_menu = CP_MENU_IDLE;
      printk("\n[STATE: IDLE] He thong dang o che do cho.\n");
      control_pannel_print_menu(active_menu);
      break;

    case COLLECT:
      handle_collect_mode();
      active_menu = CP_MENU_IDLE;
      control_pannel_print_menu(active_menu);
      break;

    case ENCODER:
      active_menu = handle_encoder_mode() ? CP_MENU_POST_ENCODER : CP_MENU_IDLE;
      control_pannel_print_menu(active_menu);
      break;

    case TRANSMIT_UART:
      handle_transmit_uart_mode();
      current_state = IDLE;
      continue;

    case ERROR:
    default:
      printk("\n[STATE: ERROR] Lenh Serial \"%s\" KHONG HOP LE!\n", raw_cmd);
      control_pannel_print_menu(active_menu);
      break;
    }

    if (active_menu == CP_MENU_IDLE) {
      printk("\n[CONTROL PANEL] Nhap lenh (COLLECT | ENCODER): ");
    } else {
      printk("\n[CONTROL PANEL] Nhap lenh (IDLE | TRANSMIT_UART): ");
    }
    current_state = read_serial_line(active_menu, raw_cmd, sizeof(raw_cmd));
  }
}
