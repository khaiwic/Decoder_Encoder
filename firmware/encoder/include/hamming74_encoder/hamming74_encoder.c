#include "hamming74_encoder.h"
#include <string.h>

/* G = [I4 | P], c = [a | p1], bit 6 = a0 .. bit 0 = p3 */
static const uint8_t s_gen_rows[HAMMING74_K] = {0x45U, 0x27U, 0x16U, 0x0BU};

/* H = [P^T | I3] */
static const uint8_t s_chk_rows[HAMMING74_R] = {0x74U, 0x3AU, 0x69U};

static const uint8_t s_hamming74_lut[HAMMING74_LUT_SIZE] = {
    0x00U, 0x0BU, 0x16U, 0x1DU, 0x27U, 0x2CU, 0x31U, 0x3AU,
    0x45U, 0x4EU, 0x53U, 0x58U, 0x62U, 0x69U, 0x74U, 0x7FU
};

static uint8_t popcount7(uint8_t val) {
  uint8_t v = val & 0x7FU;
  uint8_t w = 0U;
  for (uint8_t i = 0U; i < HAMMING74_L; i++) {
    w += (uint8_t)((v >> i) & 1U);
  }
  return w;
}

uint8_t hamming74_encode_nibble(uint8_t nibble) { return s_hamming74_lut[nibble & 0x0FU]; }

uint8_t hamming74_calc_parity(uint8_t nibble) {
  uint8_t d1 = (nibble >> 3U) & 1U;
  uint8_t d2 = (nibble >> 2U) & 1U;
  uint8_t d3 = (nibble >> 1U) & 1U;
  uint8_t d4 = (nibble >> 0U) & 1U;

  uint8_t p1 = d1 ^ d2 ^ d3;
  uint8_t p2 = d2 ^ d3 ^ d4;
  uint8_t p3 = d1 ^ d2 ^ d4;

  return (uint8_t)((p1 << 2U) | (p2 << 1U) | p3);
}

uint8_t hamming74_encode_by_matrix(uint8_t nibble) {
  uint8_t c = 0U;
  for (uint8_t i = 0U; i < HAMMING74_K; i++) {
    if (((nibble >> (3U - i)) & 1U) != 0U) {
      c ^= s_gen_rows[i];
    }
  }
  return c & 0x7FU;
}

uint8_t hamming74_calc_syndrome(uint8_t codeword7) {
  uint8_t s = 0U;
  uint8_t c = codeword7 & 0x7FU;
  for (uint8_t i = 0U; i < HAMMING74_R; i++) {
    uint8_t bit = popcount7(c & s_chk_rows[i]) & 1U;
    s |= (uint8_t)(bit << (2U - i));
  }
  return s;
}

void hamming74_get_matrices(uint8_t out_g[4][7], uint8_t out_h[3][7]) {
  if (out_g != NULL) {
    for (uint8_t row = 0U; row < HAMMING74_K; row++) {
      for (uint8_t col = 0U; col < HAMMING74_L; col++) {
        out_g[row][col] = (uint8_t)((s_gen_rows[row] >> (6U - col)) & 1U);
      }
    }
  }
  if (out_h != NULL) {
    for (uint8_t row = 0U; row < HAMMING74_R; row++) {
      for (uint8_t col = 0U; col < HAMMING74_L; col++) {
        out_h[row][col] = (uint8_t)((s_chk_rows[row] >> (6U - col)) & 1U);
      }
    }
  }
}

void hamming74_compute_params(hamming74_params_t *out_params) {
  if (out_params == NULL) {
    return;
  }

  uint8_t min_weight = HAMMING74_L;
  for (uint8_t a = 1U; a < HAMMING74_LUT_SIZE; a++) {
    uint8_t w = popcount7(hamming74_encode_by_matrix(a));
    if (w < min_weight) {
      min_weight = w;
    }
  }

  bool ht_zero = true;
  for (uint8_t row = 0U; row < HAMMING74_K; row++) {
    if (hamming74_calc_syndrome(s_gen_rows[row]) != 0U) {
      ht_zero = false;
      break;
    }
  }

  out_params->l = HAMMING74_L;
  out_params->k = HAMMING74_K;
  out_params->r = HAMMING74_L - HAMMING74_K;
  out_params->m_codewords = (uint8_t)(1U << HAMMING74_K);
  out_params->code_rate = (float)HAMMING74_K / (float)HAMMING74_L;
  out_params->d_min = min_weight;
  out_params->detect_cap = (min_weight > 0U) ? (uint8_t)(min_weight - 1U) : 0U;
  out_params->correct_cap = (min_weight > 0U) ? (uint8_t)((min_weight - 1U) / 2U) : 0U;
  out_params->g_ht_zero = ht_zero;
}

hamming74_status_t hamming74_encode(const uint8_t *in_data, size_t in_bits_count, uint8_t *out_buf, size_t max_out_len, size_t *out_bits_count, hamming74_metrics_t *metrics) {
  if (in_data == NULL || out_buf == NULL) {
    return HAMMING74_ERR_PARAM;
  }
  if (in_bits_count == 0U) {
    return HAMMING74_ERR_EMPTY;
  }

  size_t in_nibbles = (in_bits_count + 3U) / 4U;
  size_t total_out_bits = in_nibbles * HAMMING74_L;
  size_t total_out_bytes = (total_out_bits + 7U) / 8U;

  if (total_out_bytes > max_out_len) {
    return HAMMING74_ERR_BUF_OVERFLOW;
  }

  memset(out_buf, 0, total_out_bytes);

  for (size_t i = 0; i < in_nibbles; i++) {
    uint8_t nibble = 0U;
    for (size_t b = 0; b < 4U; b++) {
      size_t in_bit_idx = (i * 4U) + b;
      if (in_bit_idx < in_bits_count) {
        uint8_t bit_val = (in_data[in_bit_idx >> 3U] >> (7U - (in_bit_idx & 7U))) & 1U;
        nibble |= (uint8_t)(bit_val << (3U - b));
      }
    }

    uint8_t codeword = s_hamming74_lut[nibble];
    for (int b = 6; b >= 0; b--) {
      if (((codeword >> (uint8_t)b) & 1U) != 0U) {
        size_t out_bit_idx = (i * HAMMING74_L) + (size_t)(6 - b);
        out_buf[out_bit_idx >> 3U] |= (uint8_t)(1U << (7U - (out_bit_idx & 7U)));
      }
    }
  }

  if (out_bits_count != NULL) {
    *out_bits_count = total_out_bits;
  }
  if (metrics != NULL) {
    metrics->in_bits_count = in_bits_count;
    metrics->in_nibbles_count = in_nibbles;
    metrics->out_bits_count = total_out_bits;
    metrics->out_bytes_count = total_out_bytes;
    metrics->code_rate = (float)in_bits_count / (float)total_out_bits;
  }

  return HAMMING74_OK;
}

size_t hamming74_encode_stream(const uint8_t *in_data, size_t in_bits_count, uint8_t *out_buf, size_t *out_bits_count) {
  if (in_data == NULL || out_buf == NULL || in_bits_count == 0U) {
    if (out_bits_count != NULL) {
      *out_bits_count = 0U;
    }
    return 0U;
  }

  size_t total_out_bytes = hamming74_calc_encoded_bytes(in_bits_count);
  if (hamming74_encode(in_data, in_bits_count, out_buf, total_out_bytes, out_bits_count, NULL) != HAMMING74_OK) {
    if (out_bits_count != NULL) {
      *out_bits_count = 0U;
    }
    return 0U;
  }
  return total_out_bytes;
}

hamming74_status_t hamming74_encode_bytes(const uint8_t *in_bytes, size_t in_bytes_len, uint8_t *out_buf, size_t max_out_len, size_t *out_bits_count, hamming74_metrics_t *metrics) {
  if (in_bytes == NULL) {
    return HAMMING74_ERR_PARAM;
  }
  if (in_bytes_len == 0U) {
    return HAMMING74_ERR_EMPTY;
  }
  return hamming74_encode(in_bytes, in_bytes_len * 8U, out_buf, max_out_len, out_bits_count, metrics);
}

bool hamming74_run_self_test(void) {
  for (uint8_t nibble = 0U; nibble < HAMMING74_LUT_SIZE; nibble++) {
    uint8_t expected = (uint8_t)((nibble << 3U) | hamming74_calc_parity(nibble));
    if (s_hamming74_lut[nibble] != expected || hamming74_encode_by_matrix(nibble) != expected) {
      return false;
    }
    if (hamming74_calc_syndrome(expected) != 0U) {
      return false;
    }
  }

  static const uint8_t test_in[4] = {0x12, 0x34, 0x56, 0x78};
  static const uint8_t test_expected_out[7] = {0x16, 0x58, 0xEA, 0x75, 0x8C, 0x5D, 0x45};
  uint8_t test_out[7] = {0};
  size_t out_bits = 0U;

  size_t written_bytes = hamming74_encode_stream(test_in, 32U, test_out, &out_bits);
  if (written_bytes != 7U || out_bits != 56U || memcmp(test_out, test_expected_out, 7U) != 0) {
    return false;
  }
  return true;
}
