#ifndef _HAMMING74_ENCODER_H_
#define _HAMMING74_ENCODER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HAMMING74_L 7U
#define HAMMING74_K 4U
#define HAMMING74_R 3U
#define HAMMING74_N HAMMING74_L
#define HAMMING74_M HAMMING74_R
#define HAMMING74_CODE_RATE 0.57142857f
#define HAMMING74_LUT_SIZE 16U

typedef enum {
  HAMMING74_OK = 0,
  HAMMING74_ERR_PARAM = -1,
  HAMMING74_ERR_EMPTY = -2,
  HAMMING74_ERR_BUF_OVERFLOW = -3
} hamming74_status_t;

typedef struct {
  size_t in_bits_count;
  size_t in_nibbles_count;
  size_t out_bits_count;
  size_t out_bytes_count;
  float code_rate;
} hamming74_metrics_t;

typedef struct {
  uint8_t l;
  uint8_t k;
  uint8_t r;
  uint8_t m_codewords;
  float code_rate;
  uint8_t d_min;
  uint8_t detect_cap;
  uint8_t correct_cap;
  bool g_ht_zero;
} hamming74_params_t;

static inline size_t hamming74_calc_encoded_bits(size_t in_bits_count) {
  if (in_bits_count == 0U) {
    return 0U;
  }
  return ((in_bits_count + 3U) / 4U) * HAMMING74_L;
}

static inline size_t hamming74_calc_encoded_bytes(size_t in_bits_count) {
  return (hamming74_calc_encoded_bits(in_bits_count) + 7U) / 8U;
}

uint8_t hamming74_encode_nibble(uint8_t nibble);
uint8_t hamming74_calc_parity(uint8_t nibble);
uint8_t hamming74_encode_by_matrix(uint8_t nibble);
uint8_t hamming74_calc_syndrome(uint8_t codeword7);
void hamming74_get_matrices(uint8_t out_g[4][7], uint8_t out_h[3][7]);
void hamming74_compute_params(hamming74_params_t *out_params);

hamming74_status_t hamming74_encode(const uint8_t *in_data, size_t in_bits_count, uint8_t *out_buf, size_t max_out_len, size_t *out_bits_count, hamming74_metrics_t *metrics);
size_t hamming74_encode_stream(const uint8_t *in_data, size_t in_bits_count, uint8_t *out_buf, size_t *out_bits_count);
hamming74_status_t hamming74_encode_bytes(const uint8_t *in_bytes, size_t in_bytes_len, uint8_t *out_buf, size_t max_out_len, size_t *out_bits_count, hamming74_metrics_t *metrics);
bool hamming74_run_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
