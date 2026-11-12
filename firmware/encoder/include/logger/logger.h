#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "huffman_encoder.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOGGER_CSV_TAG_HEADER "[CSV_HEADER]"
#define LOGGER_CSV_TAG_DATA "[CSV_DATA]"

/* Ba chế độ mã hóa theo yêu cầu đề bài README.md */
typedef enum {
  ENCODER_MODE_RAW = 0,        /* Chế độ A: Không nén, không bảo vệ */
  ENCODER_MODE_HUFFMAN,        /* Chế độ B: Mã hóa nguồn Huffman */
  ENCODER_MODE_HUFFMAN_HAMMING /* Chế độ C: Nén Huffman + Kênh Hamming(7,4) */
} encoder_mode_t;

/* Cấu trúc lưu toàn bộ thông số thực nghiệm của 1 lô mẫu đo từ cảm biến SHT30 */
typedef struct {
  uint32_t seq_id;          /* Số thứ tự lượt phát / sequence ID */
  uint32_t batch_id;        /* Số thứ tự lô mẫu cảm biến SHT30 */
  encoder_mode_t mode;      /* Chế độ mã hóa (A, B, C) */
  size_t valid_samples;     /* Số mẫu SHT30 hợp lệ N (ví dụ 128) */
  uint32_t dropped_samples; /* Số mẫu bị loại do lỗi I2C hoặc CRC-8 */
  float avg_temp_c;         /* Nhiệt độ trung bình của lô mẫu (độ C) */
  float avg_rh_pct;         /* Độ ẩm trung bình của lô mẫu (%RH) */
  size_t raw_bytes;         /* Dung lượng dữ liệu thô (4N byte) */
  uint32_t raw_crc32;       /* Mã CRC-32 của payload gốc (4N byte) */
  float entropy;            /* Entropy nguồn tin H(X) (bit/symbol) */

  /* Thông số tầng mã hóa nguồn Huffman (Mode B, C) */
  uint16_t huff_used_symbols; /* Số ký hiệu xuất hiện trong Codebook */
  size_t huff_codebook_bytes; /* Kích thước Codebook tuần tự hóa (byte) */
  size_t huff_valid_bits;     /* Số bit nén hợp lệ thực tế */
  size_t huff_bytes;          /* Số byte sau đóng gói bitstream: ceil(bits/8) */
  float payload_comp_ratio;   /* Tỷ lệ nén payload: raw_bits / huff_valid_bits */
  float net_comp_ratio;       /* Tỷ lệ nén toàn phần: raw_bytes / (huff_bytes + codebook) */

  /* Thông số tầng mã hóa kênh Hamming(7,4) (Mode C) */
  size_t ham_valid_bits; /* Số bit sau mã hóa Hamming */
  size_t ham_bytes;      /* Số byte sau đóng gói Hamming */
  float ham_code_rate;   /* Tốc độ mã kênh R thực tế (xấp xỉ 4/7 ~ 0.5714) */

  /* Hiệu năng phần cứng */
  uint32_t encode_time_us; /* Thời gian mã hóa (micro-giây) */
  int status;              /* 0: OK, <0: Mã lỗi */
} encoder_log_record_t;

void logger_init(void);

void logger_print_banner(void);

void logger_print_csv_header(void);

void logger_log_record(const encoder_log_record_t *record);

void logger_print_comparison_summary(const encoder_log_record_t *records, size_t count);

const char *logger_mode_to_str(encoder_mode_t mode);

uint32_t logger_calculate_crc32(const uint8_t *data, size_t len);

void logger_log_mode_a(uint32_t batch_id, size_t valid_samples, uint32_t dropped_samples, float avg_temp, float avg_rh, const uint8_t *raw_payload, size_t raw_bytes_size, uint32_t raw_crc32, float entropy, encoder_log_record_t *out_rec);

void logger_log_mode_b(uint32_t batch_id, size_t valid_samples, uint32_t dropped_samples, float avg_temp, float avg_rh, size_t raw_bytes_size, uint32_t raw_crc32, const huffman_codebook_t *codebook, const huffman_metrics_t *h_metrics, int book_bytes, uint32_t elapsed_b_us, huffman_status_t h_stat, encoder_log_record_t *out_rec);

#ifdef __cplusplus
}
#endif

#endif /* _LOGGER_H_ */
