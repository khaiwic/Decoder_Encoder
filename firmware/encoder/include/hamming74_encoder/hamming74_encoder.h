#ifndef _HAMMING74_ENCODER_H_
#define _HAMMING74_ENCODER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file hamming74_encoder.h
 * @brief Bộ mã hóa kênh Hamming(7,4) hệ thống (Systematic Hamming Encoder)
 */

#define HAMMING74_K 4U                  /* Số bit dữ liệu đầu vào mỗi khối */
#define HAMMING74_N 7U                  /* Số bit từ mã đầu ra mỗi khối */
#define HAMMING74_M 3U                  /* Số bit kiểm tra parity (m = n - k = 3) */
#define HAMMING74_CODE_RATE 0.57142857f /* Tốc độ mã lý thuyết R = 4/7 */
#define HAMMING74_LUT_SIZE 16U          /* Số phần tử trong bảng tra mã (2^4 = 16) */

/* Mã trạng thái trả về của Hamming(7,4) Encoder */
typedef enum {
  HAMMING74_OK = 0,               /* Thành công */
  HAMMING74_ERR_PARAM = -1,       /* Con trỏ tham số NULL */
  HAMMING74_ERR_EMPTY = -2,       /* Số lượng bit đầu vào bằng 0 */
  HAMMING74_ERR_BUF_OVERFLOW = -3 /* Bộ đệm đầu ra không đủ dung lượng */
} hamming74_status_t;

/* Thống kê hiệu suất và chỉ số mã hóa kênh phục vụ Báo cáo */
typedef struct {
  size_t in_bits_count;    /* Số bit dữ liệu hợp lệ đầu vào */
  size_t in_nibbles_count; /* Số nhóm 4-bit (đã gồm 0-padding nếu in_bits_count không chia hết cho 4) */
  size_t out_bits_count;   /* Tổng số bit sau mã hóa = in_nibbles_count * 7 */
  size_t out_bytes_count;  /* Số byte thực tế sau đóng gói bitstream = ceil(out_bits_count / 8) */
  float code_rate;         /* Tốc độ mã thực tế: in_bits_count / out_bits_count */
} hamming74_metrics_t;

static inline size_t hamming74_calc_encoded_bits(size_t in_bits_count) {
  if (in_bits_count == 0) {
    return 0;
  }
  size_t nibbles = (in_bits_count + 3U) / 4U;
  return nibbles * 7U;
}

static inline size_t hamming74_calc_encoded_bytes(size_t in_bits_count) {
  size_t bits = hamming74_calc_encoded_bits(in_bits_count);
  return (bits + 7U) / 8U;
}

uint8_t hamming74_encode_nibble(uint8_t nibble);

uint8_t hamming74_calc_parity(uint8_t nibble);

hamming74_status_t hamming74_encode(const uint8_t *in_data, size_t in_bits_count, uint8_t *out_buf, size_t max_out_len, size_t *out_bits_count, hamming74_metrics_t *metrics);

size_t hamming74_encode_stream(const uint8_t *in_data, size_t in_bits_count, uint8_t *out_buf, size_t *out_bits_count);

hamming74_status_t hamming74_encode_bytes(const uint8_t *in_bytes, size_t in_bytes_len, uint8_t *out_buf, size_t max_out_len, size_t *out_bits_count, hamming74_metrics_t *metrics);

bool hamming74_run_self_test(void);

#ifdef __cplusplus
}
#endif

#endif /* _HAMMING74_ENCODER_H_ */
