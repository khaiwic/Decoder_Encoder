#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

static uint32_t s_sequence_id = 0;

const char *logger_mode_to_str(encoder_mode_t mode) {
  switch (mode) {
  case ENCODER_MODE_RAW:
    return "RAW";
  case ENCODER_MODE_HUFFMAN:
    return "HUFFMAN";
  case ENCODER_MODE_HUFFMAN_HAMMING:
    return "HUFFMAN_HAMMING";
  default:
    return "UNKNOWN";
  }
}

uint32_t logger_calculate_crc32(const uint8_t *data, size_t len) {
  if (data == NULL || len == 0) {
    return 0;
  }

  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint32_t)data[i];
    for (uint8_t bit = 0; bit < 8U; bit++) {
      if ((crc & 1UL) != 0UL) {
        crc = (crc >> 1U) ^ 0xEDB88320UL;
      } else {
        crc >>= 1U;
      }
    }
  }
  return crc ^ 0xFFFFFFFFUL;
}

void logger_init(void) { k_msleep(50); }

void logger_print_banner(void) {
  printk("\n");
  printk("========================================================================\n");
  printk("       HE THONG TRUYEN TIN ESP32-S3 - KHOI 1: MAY PHAT (TX ENCODER)    \n");
  printk("       Mon hoc: Ly thuyet thong tin | Cam bien that: SHT30 (I2C)       \n");
  printk("========================================================================\n");
  printk(" Che do kiem thu:\n");
  printk("   [Mode A] RAW              : Duong co so khong nen / khong bao ve\n");
  printk("   [Mode B] HUFFMAN          : Ma hoa nguon Canonical Huffman\n");
  printk("   [Mode C] HUFFMAN + HAMMING: Ma hoa nguon Huffman + Ma kenh Hamming(7,4)\n");
  printk("------------------------------------------------------------------------\n");
  printk("Composer: Dươnng Quang Khải - B24DCDT152\n");
}

void logger_print_csv_header(void) {
  printk("%s seq_id,batch_id,mode,valid_samples,dropped_samples,avg_temp_c,avg_rh_pct,raw_bytes,raw_crc32,entropy,huff_symbols,codebook_bytes,huff_bits,huff_bytes,payload_ratio,net_ratio,ham_bits,ham_bytes,ham_rate,encode_us,status\n", LOGGER_CSV_TAG_HEADER);
}

void logger_log_record(const encoder_log_record_t *record) {
  if (record == NULL) {
    return;
  }

  const char *status_str = (record->status == 0) ? "OK" : "ERR";

  /* 1. Xuất dòng định dạng Machine-Readable dành cho Python thu thập */
  printk("%s %u,%u,%s,%u,%u,%.2f,%.2f,%u,0x%08X,%.4f,%u,%u,%u,%u,%.2f,%.2f,%u,%u,%.4f,%u,%s\n", LOGGER_CSV_TAG_DATA, record->seq_id, record->batch_id, logger_mode_to_str(record->mode), (unsigned)record->valid_samples, record->dropped_samples, (double)record->avg_temp_c, (double)record->avg_rh_pct, (unsigned)record->raw_bytes, record->raw_crc32, (double)record->entropy, (unsigned)record->huff_used_symbols, (unsigned)record->huff_codebook_bytes, (unsigned)record->huff_valid_bits, (unsigned)record->huff_bytes, (double)record->payload_comp_ratio, (double)record->net_comp_ratio, (unsigned)record->ham_valid_bits, (unsigned)record->ham_bytes, (double)record->ham_code_rate, (unsigned)record->encode_time_us, status_str);

  /* 2. Xuất thông tin ngắn gọn trực quan */
  printk("  -> [Seq #%03u | Batch #%u] Mode: %-15s | Mau hop le: %u (Loi: %u) | T_tb: %.2f C, RH_tb: %.2f %% | Raw: %u B (CRC: 0x%08X)\n", record->seq_id, record->batch_id, logger_mode_to_str(record->mode), (unsigned)record->valid_samples, record->dropped_samples, (double)record->avg_temp_c, (double)record->avg_rh_pct, (unsigned)record->raw_bytes, record->raw_crc32);

  if (record->mode == ENCODER_MODE_RAW) {
    printk("     Payload truyen: %u bytes | Thoi gian: %u us | Trang thai: %s\n", (unsigned)record->raw_bytes, (unsigned)record->encode_time_us, status_str);
  } else if (record->mode == ENCODER_MODE_HUFFMAN) {
    printk("     Huffman: H(X)=%.4f | %u bits (%u B) + Book %u B | Nen payload: %.2fx | Nen toan phan: %.2fx | %u us\n", (double)record->entropy, (unsigned)record->huff_valid_bits, (unsigned)record->huff_bytes, (unsigned)record->huff_codebook_bytes, (double)record->payload_comp_ratio, (double)record->net_comp_ratio, (unsigned)record->encode_time_us);
  } else if (record->mode == ENCODER_MODE_HUFFMAN_HAMMING) {
    printk("     Hamming: %u bits (%u B) | Code rate: %.4f | Tong sau ma hoa: %u B | %u us\n", (unsigned)record->ham_valid_bits, (unsigned)record->ham_bytes, (double)record->ham_code_rate, (unsigned)(record->huff_codebook_bytes + record->ham_bytes), (unsigned)record->encode_time_us);
  }
}

void logger_print_comparison_summary(const encoder_log_record_t *records, size_t count) {
  if (records == NULL || count == 0) {
    return;
  }

  printk("\n");
  printk("==============================================================================================================\n");
  printk("                     BANG TONG HOP SO SANH 3 CHE DO MA HOA TREN DU LIEU SHT30 THUC TE                        \n");
  printk("==============================================================================================================\n");
  printk("| Seq | Batch | Mode            | Mau(N) | T(C)  | RH(%%) | Raw(B) | Nen(B) | Book(B) | Ham(B) | PayR   | NetR  | Time(us) |\n");
  printk("|-----|-------|-----------------|--------|-------|-------|--------|--------|---------|--------|--------|-------|----------|\n");

  for (size_t i = 0; i < count; i++) {
    const encoder_log_record_t *r = &records[i];
    printk("| %3u | %5u | %-15s | %6u | %5.1f | %5.1f | %6u | %6u | %7u | %6u | %5.2fx | %4.2fx | %8u |\n", r->seq_id, r->batch_id, logger_mode_to_str(r->mode), (unsigned)r->valid_samples, (double)r->avg_temp_c, (double)r->avg_rh_pct, (unsigned)r->raw_bytes, (unsigned)r->huff_bytes, (unsigned)r->huff_codebook_bytes, (unsigned)r->ham_bytes, (double)r->payload_comp_ratio, (double)r->net_comp_ratio, (unsigned)r->encode_time_us);
  }
  printk("==============================================================================================================\n\n");
}

void logger_log_mode_a(uint32_t batch_id, size_t valid_samples, uint32_t dropped_samples, float avg_temp, float avg_rh, const uint8_t *raw_payload, size_t raw_bytes_size, uint32_t raw_crc32, float entropy, encoder_log_record_t *out_rec) {
  uint32_t start_a = k_cycle_get_32();
  volatile uint32_t crc_check = logger_calculate_crc32(raw_payload, raw_bytes_size);
  (void)crc_check;
  uint32_t elapsed_a_us = k_cyc_to_us_floor32(k_cycle_get_32() - start_a);

  encoder_log_record_t rec_a = {.seq_id = ++s_sequence_id, .batch_id = batch_id, .mode = ENCODER_MODE_RAW, .valid_samples = valid_samples, .dropped_samples = dropped_samples, .avg_temp_c = avg_temp, .avg_rh_pct = avg_rh, .raw_bytes = raw_bytes_size, .raw_crc32 = raw_crc32, .entropy = entropy, .huff_used_symbols = 0, .huff_codebook_bytes = 0, .huff_valid_bits = raw_bytes_size * 8U, .huff_bytes = raw_bytes_size, .payload_comp_ratio = 1.0f, .net_comp_ratio = 1.0f, .ham_valid_bits = 0, .ham_bytes = 0, .ham_code_rate = 0.0f, .encode_time_us = elapsed_a_us, .status = 0};

  logger_log_record(&rec_a);
  if (out_rec != NULL) {
    *out_rec = rec_a;
  }
}

void logger_log_mode_b(uint32_t batch_id, size_t valid_samples, uint32_t dropped_samples, float avg_temp, float avg_rh, size_t raw_bytes_size, uint32_t raw_crc32, const huffman_codebook_t *codebook, const huffman_metrics_t *h_metrics, int book_bytes, uint32_t elapsed_b_us, huffman_status_t h_stat, encoder_log_record_t *out_rec) {
  encoder_log_record_t rec_b = {.seq_id = ++s_sequence_id, .batch_id = batch_id, .mode = ENCODER_MODE_HUFFMAN, .valid_samples = valid_samples, .dropped_samples = dropped_samples, .avg_temp_c = avg_temp, .avg_rh_pct = avg_rh, .raw_bytes = raw_bytes_size, .raw_crc32 = raw_crc32, .entropy = (h_metrics != NULL) ? h_metrics->entropy : 0.0f, .huff_used_symbols = (codebook != NULL) ? codebook->used_symbols_count : 0, .huff_codebook_bytes = (book_bytes > 0) ? (size_t)book_bytes : 0, .huff_valid_bits = (h_metrics != NULL) ? h_metrics->coded_valid_bits : 0, .huff_bytes = (h_metrics != NULL) ? h_metrics->coded_bytes_count : 0, .payload_comp_ratio = (h_metrics != NULL) ? h_metrics->payload_comp_ratio : 0.0f, .net_comp_ratio = (h_metrics != NULL) ? h_metrics->net_comp_ratio : 0.0f, .ham_valid_bits = 0, .ham_bytes = 0, .ham_code_rate = 0.0f, .encode_time_us = elapsed_b_us, .status = (int)h_stat};

  logger_log_record(&rec_b);
  if (out_rec != NULL) {
    *out_rec = rec_b;
  }
}
