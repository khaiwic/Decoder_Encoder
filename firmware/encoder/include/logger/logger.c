#include "logger.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define HAMMING_TRACE_LIMIT 4U

typedef struct {
  uint8_t symbol;
  uint16_t count;
} prob_entry_t;

static uint32_t s_sequence_id = 0;
static uint16_t s_prob_freq[HUFFMAN_MAX_SYMBOLS];
static prob_entry_t s_prob_items[HUFFMAN_MAX_SYMBOLS];

static void print_bits_u32(uint32_t val, uint8_t width) {
  for (int b = (int)width - 1; b >= 0; b--) {
    printk("%u", (unsigned)((val >> (uint32_t)b) & 1U));
  }
}

static void print_bitstream_bin(const char *prefix, const uint8_t *buf, size_t valid_bits) {
  if (buf == NULL || valid_bits == 0U) {
    return;
  }
  printk("%s (%u bits): ", prefix, (unsigned)valid_bits);
  for (size_t bit = 0U; bit < valid_bits; bit++) {
    uint8_t v = (buf[bit >> 3U] >> (7U - (bit & 7U))) & 1U;
    printk("%u", (unsigned)v);
  }
  printk("\n");
}

const char *logger_mode_to_str(encoder_mode_t mode) {
  switch (mode) {
  case ENCODER_MODE_DEFAULT:
    return "ENCODER";
  default:
    return "UNKNOWN";
  }
}

const char *logger_stream_to_str(encoder_stream_t stream) {
  switch (stream) {
  case ENCODER_STREAM_TEMP:
    return "TEMP";
  case ENCODER_STREAM_HUMD:
    return "HUMD";
  case ENCODER_STREAM_TOTAL:
    return "TOTAL";
  default:
    return "UNKNOWN";
  }
}

void logger_init(void) { k_msleep(50); }

void logger_print_banner(void) {
  printk("\n");
  printk("========================================================================\n");
  printk("       HE THONG TRUYEN TIN ESP32-S3 - KHOI 1: MAY PHAT (TX ENCODER)    \n");
  printk("       Mon hoc: Ly thuyet thong tin | Cam bien: SHT30 (2 luong 8-bit)  \n");
  printk("========================================================================\n");
  printk(" Che do hoat dong:\n");
  printk("   [ENCODER] Canonical Huffman + Hamming(7,4) doc lap Temp & Humd (LTTT06)\n");
  printk("------------------------------------------------------------------------\n");
  printk("Composer: Duong Quang Khai - B24DCDT152\n");
}

void logger_print_csv_header(void) {
  printk("%s seq_id,batch_id,mode,stream,valid_samples,dropped_samples,avg_temp_c,avg_rh_pct,raw_bytes,entropy,avg_code_len,efficiency,huff_symbols,codebook_bytes,huff_bits,huff_bytes,payload_ratio,net_ratio,ham_bits,ham_bytes,ham_rate,encode_us,status\n", LOGGER_CSV_TAG_HEADER);
}

void logger_print_stream_collected(const char *label, const char *unit, uint32_t batch_id, const uint8_t *data, size_t count) {
  if (data == NULL || count == 0U) {
    return;
  }

  printk("\n[BO TU MA %s (%s) - LO #%u] (%u mau 8-bit = %u bytes)\n", label, unit, (unsigned)batch_id, (unsigned)count, (unsigned)count);
  for (size_t i = 0U; i < count; i++) {
    if ((i % 16U) == 0U) {
      size_t end_idx = (i + 15U < count) ? (i + 15U) : (count - 1U);
      printk("  [%3u..%3u]: ", (unsigned)i, (unsigned)end_idx);
    }
    printk("%3u", (unsigned)data[i]);
    if (((i + 1U) % 16U) == 0U || (i + 1U) == count) {
      printk("\n");
    } else {
      printk(" ");
    }
  }

  size_t preview = (count < 4U) ? count : 4U;
  printk("  Mau 8-bit (%s[0..%u]): ", label, (unsigned)(preview - 1U));
  for (size_t i = 0U; i < preview; i++) {
    printk("%u=", (unsigned)data[i]);
    print_bits_u32(data[i], 8U);
    if ((i + 1U) < preview) {
      printk(" | ");
    }
  }
  printk("\n");
}

void logger_print_probability_table(const char *label, const uint8_t *data, size_t len) {
  if (data == NULL || len == 0U) {
    return;
  }

  memset(s_prob_freq, 0, sizeof(s_prob_freq));
  for (size_t i = 0U; i < len; i++) {
    s_prob_freq[data[i]]++;
  }

  size_t count = 0U;
  for (size_t sym = 0U; sym < HUFFMAN_MAX_SYMBOLS; sym++) {
    if (s_prob_freq[sym] > 0U) {
      s_prob_items[count].symbol = (uint8_t)sym;
      s_prob_items[count].count = s_prob_freq[sym];
      count++;
    }
  }

  for (size_t i = 1U; i < count; i++) {
    prob_entry_t key = s_prob_items[i];
    size_t j = i;
    while (j > 0U && (s_prob_items[j - 1U].count > key.count || (s_prob_items[j - 1U].count == key.count && s_prob_items[j - 1U].symbol > key.symbol))) {
      s_prob_items[j] = s_prob_items[j - 1U];
      j--;
    }
    s_prob_items[j] = key;
  }

  printk("\n[BANG PHAN BO XAC SUAT %s - TANG DAN] (Tong: %u mau, %u ky hieu)\n", label, (unsigned)len, (unsigned)count);
  printk("  | STT | Gia tri |  Hex | 8-bit goc | Tan suat | Xac suat p(x) |\n");
  printk("  |-----|---------|------|-----------|----------|---------------|\n");
  for (size_t i = 0U; i < count; i++) {
    double prob = (double)s_prob_items[i].count / (double)len;
    printk("  | %3u |   %3u   | 0x%02X |  ", (unsigned)(i + 1U), (unsigned)s_prob_items[i].symbol, s_prob_items[i].symbol);
    print_bits_u32(s_prob_items[i].symbol, 8U);
    printk(" |   %4u   |    %.4f     |\n", (unsigned)s_prob_items[i].count, prob);
  }
}

void logger_print_huffman_table(const char *label, const huffman_codebook_t *codebook, const huffman_metrics_t *metrics, const uint8_t *bitstream) {
  if (codebook == NULL || codebook->used_symbols_count == 0U || metrics == NULL) {
    return;
  }

  printk("\n[BANG MA SAU HUFFMAN - %s] (%u ky hieu | %u bits / %u bytes)\n", label, (unsigned)codebook->used_symbols_count, (unsigned)metrics->coded_valid_bits, (unsigned)metrics->coded_bytes_count);
  printk("  | STT | Gia tri |  Hex | 8-bit goc | l_i | Tu ma Huffman                    |\n");
  printk("  |-----|---------|------|-----------|-----|----------------------------------|\n");

  uint32_t cur_code = 0U;
  uint8_t prev_len = codebook->entries[0].len;

  for (uint16_t i = 0U; i < codebook->used_symbols_count; i++) {
    uint8_t sym = codebook->entries[i].symbol;
    uint8_t len = codebook->entries[i].len;
    if (len == 0U || len > HUFFMAN_MAX_CODE_LEN) {
      continue;
    }

    if (i > 0U && len >= prev_len) {
      cur_code = (cur_code + 1U) << (len - prev_len);
    }
    prev_len = len;

    printk("  | %3u |   %3u   | 0x%02X |  ", (unsigned)(i + 1U), (unsigned)sym, sym);
    print_bits_u32(sym, 8U);
    printk(" | %3u | ", len);
    print_bits_u32(cur_code, len);
    for (uint8_t pad = len; pad < HUFFMAN_MAX_CODE_LEN; pad++) {
      printk(" ");
    }
    printk(" |\n");
  }

  printk("  Thong so %s: H(X)=%.4f | l_tb=%.4f | Hieu suat eta=%.2f%% | PayR=%.2fx | NetR=%.2fx\n", label, (double)metrics->entropy, (double)metrics->avg_code_len, (double)(metrics->coding_efficiency * 100.0f), (double)metrics->payload_comp_ratio, (double)metrics->net_comp_ratio);
  print_bitstream_bin("  Huffman Bitstream", bitstream, metrics->coded_valid_bits);
}

void logger_print_hamming_theory(const hamming74_params_t *params) {
  if (params == NULL) {
    return;
  }

  uint8_t g[HAMMING74_K][HAMMING74_L];
  uint8_t h[HAMMING74_R][HAMMING74_L];
  hamming74_get_matrices(g, h);

  printk("\n[THAM SO MA KHOI TUYEN TINH HAMMING(%u,%u) - THEO LTTT06 | c = [a | p1]]\n", (unsigned)params->l, (unsigned)params->k);
  printk("  - Do dai tu ma (l)              : %u bit\n", (unsigned)params->l);
  printk("  - Do dai khoi tin (k)           : %u bit (So tu ma M = 2^k = %u)\n", (unsigned)params->k, (unsigned)params->m_codewords);
  printk("  - Do du thua (r = l - k)        : %u bit\n", (unsigned)params->r);
  printk("  - Ty so ma hoa (R = k/l)        : %.4f (%u/%u)\n", (double)params->code_rate, (unsigned)params->k, (unsigned)params->l);
  printk("  - Khoang cach toi thieu (d_min) : %u\n", (unsigned)params->d_min);
  printk("  - Phat hien loi toi da (d_min-1): %u bit\n", (unsigned)params->detect_cap);
  printk("  - Sua loi toi da floor((d-1)/2) : %u bit\n", (unsigned)params->correct_cap);

  printk("  Ma tran sinh he thong G = [I_%u | P] (%ux%u):\n", (unsigned)params->k, (unsigned)params->k, (unsigned)params->l);
  for (uint8_t r = 0U; r < HAMMING74_K; r++) {
    printk("    g%u = [", (unsigned)r);
    for (uint8_t c = 0U; c < HAMMING74_L; c++) {
      printk("%u%s", (unsigned)g[r][c], (c + 1U < HAMMING74_L) ? " " : "");
    }
    printk("]\n");
  }

  printk("  Ma tran kiem tra chan le H = [P^T | I_%u] (%ux%u):\n", (unsigned)params->r, (unsigned)params->r, (unsigned)params->l);
  for (uint8_t r = 0U; r < HAMMING74_R; r++) {
    printk("    h%u = [", (unsigned)r);
    for (uint8_t c = 0U; c < HAMMING74_L; c++) {
      printk("%u%s", (unsigned)h[r][c], (c + 1U < HAMMING74_L) ? " " : "");
    }
    printk("]\n");
  }
  printk("  Kiem chung G * H^T = 0 (mod 2)  : %s\n", params->g_ht_zero ? "PASS" : "FAIL");
}

void logger_print_hamming_stream_steps(const char *label, const uint8_t *in_huff_bits, size_t in_valid_bits, const uint8_t *out_ham_bits, const hamming74_metrics_t *ham_metrics, uint32_t encode_us) {
  if (in_huff_bits == NULL || out_ham_bits == NULL || ham_metrics == NULL) {
    return;
  }

  size_t trace_blocks = (ham_metrics->in_nibbles_count < HAMMING_TRACE_LIMIT) ? ham_metrics->in_nibbles_count : HAMMING_TRACE_LIMIT;

  printk("\n[MA HOA HAMMING(7,4) LUONG %s] (Minh hoa %u/%u khoi dau theo LTTT06: c = a*G)\n", label, (unsigned)trace_blocks, (unsigned)ham_metrics->in_nibbles_count);
  for (size_t i = 0U; i < trace_blocks; i++) {
    uint8_t a = 0U;
    for (size_t b = 0U; b < 4U; b++) {
      size_t bit_idx = (i * 4U) + b;
      if (bit_idx < in_valid_bits) {
        uint8_t bit_val = (in_huff_bits[bit_idx >> 3U] >> (7U - (bit_idx & 7U))) & 1U;
        a |= (uint8_t)(bit_val << (3U - b));
      }
    }

    uint8_t p1 = hamming74_calc_parity(a);
    uint8_t c = hamming74_encode_by_matrix(a);
    uint8_t syn = hamming74_calc_syndrome(c);

    printk("  Khoi #%u: a=[", (unsigned)(i + 1U));
    print_bits_u32(a, 4U);
    printk("] -> p1=[");
    print_bits_u32(p1, 3U);
    printk("] -> c=a*G=[");
    print_bits_u32(c, 7U);
    printk("] (0x%02X) | c*H^T=[", c);
    print_bits_u32(syn, 3U);
    printk("]\n");
  }

  printk("  Tong ket %s: %u khoi (k=4) -> %u bits (%u bytes) | R thuc te: %.4f | %u us\n", label, (unsigned)ham_metrics->in_nibbles_count, (unsigned)ham_metrics->out_bits_count, (unsigned)ham_metrics->out_bytes_count, (double)ham_metrics->code_rate, (unsigned)encode_us);
  print_bitstream_bin("  Hamming Bitstream", out_ham_bits, ham_metrics->out_bits_count);
}

void logger_log_record(const encoder_log_record_t *record) {
  if (record == NULL) {
    return;
  }

  const char *status_str = (record->status == 0) ? "OK" : "ERR";

  printk("%s %u,%u,%s,%s,%u,%u,%.2f,%.2f,%u,%.4f,%.4f,%.4f,%u,%u,%u,%u,%.2f,%.2f,%u,%u,%.4f,%u,%s\n", LOGGER_CSV_TAG_DATA, (unsigned)record->seq_id, (unsigned)record->batch_id, logger_mode_to_str(record->mode), logger_stream_to_str(record->stream), (unsigned)record->valid_samples, (unsigned)record->dropped_samples, (double)record->avg_temp_c, (double)record->avg_rh_pct, (unsigned)record->raw_bytes, (double)record->entropy, (double)record->avg_code_len, (double)record->coding_efficiency, (unsigned)record->huff_used_symbols, (unsigned)record->huff_codebook_bytes, (unsigned)record->huff_valid_bits, (unsigned)record->huff_bytes, (double)record->payload_comp_ratio, (double)record->net_comp_ratio, (unsigned)record->ham_valid_bits, (unsigned)record->ham_bytes, (double)record->ham_code_rate, (unsigned)record->encode_time_us, status_str);

  printk("  -> [Seq #%03u | Batch #%u | %-5s] Mode: %-15s | Mau: %u (I2C err: %u) | Raw: %u B | %u us | [%s]\n", (unsigned)record->seq_id, (unsigned)record->batch_id, logger_stream_to_str(record->stream), logger_mode_to_str(record->mode), (unsigned)record->valid_samples, (unsigned)record->dropped_samples, (unsigned)record->raw_bytes, (unsigned)record->encode_time_us, status_str);
}

void logger_log_encoder(uint32_t batch_id, encoder_stream_t stream, size_t valid_samples, uint32_t dropped_samples, float avg_temp, float avg_rh, size_t raw_bytes_size, uint16_t huff_used_symbols, size_t book_bytes, const huffman_metrics_t *h_metrics, const hamming74_metrics_t *ham_metrics, uint32_t elapsed_us, int status) {
  encoder_log_record_t rec = {.seq_id = ++s_sequence_id, .batch_id = batch_id, .mode = ENCODER_MODE_DEFAULT, .stream = stream, .valid_samples = valid_samples, .dropped_samples = dropped_samples, .avg_temp_c = avg_temp, .avg_rh_pct = avg_rh, .raw_bytes = raw_bytes_size, .entropy = (h_metrics != NULL) ? h_metrics->entropy : 0.0f, .avg_code_len = (h_metrics != NULL) ? h_metrics->avg_code_len : 0.0f, .coding_efficiency = (h_metrics != NULL) ? h_metrics->coding_efficiency : 0.0f, .huff_used_symbols = huff_used_symbols, .huff_codebook_bytes = book_bytes, .huff_valid_bits = (h_metrics != NULL) ? h_metrics->coded_valid_bits : 0U, .huff_bytes = (h_metrics != NULL) ? h_metrics->coded_bytes_count : 0U, .payload_comp_ratio = (h_metrics != NULL) ? h_metrics->payload_comp_ratio : 0.0f, .net_comp_ratio = (h_metrics != NULL) ? h_metrics->net_comp_ratio : 0.0f, .ham_valid_bits = (ham_metrics != NULL) ? ham_metrics->out_bits_count : 0U, .ham_bytes = (ham_metrics != NULL) ? ham_metrics->out_bytes_count : 0U, .ham_code_rate = (ham_metrics != NULL) ? ham_metrics->code_rate : 0.0f, .encode_time_us = elapsed_us, .status = status};

  logger_log_record(&rec);
}
