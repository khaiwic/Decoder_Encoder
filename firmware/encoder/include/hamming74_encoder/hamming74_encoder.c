#include "hamming74_encoder.h"
#include <string.h>

static const uint8_t s_hamming74_lut[HAMMING74_LUT_SIZE] = {
    0x00U, /* 0x0 (0000) -> 0000 000 */
    0x0BU, /* 0x1 (0001) -> 0001 011 */
    0x16U, /* 0x2 (0010) -> 0010 110 */
    0x1DU, /* 0x3 (0011) -> 0011 101 */
    0x27U, /* 0x4 (0100) -> 0100 111 */
    0x2CU, /* 0x5 (0101) -> 0101 100 */
    0x31U, /* 0x6 (0110) -> 0110 001 */
    0x3AU, /* 0x7 (0111) -> 0111 010 */
    0x45U, /* 0x8 (1000) -> 1000 101 */
    0x4EU, /* 0x9 (1001) -> 1001 110 */
    0x53U, /* 0xA (1010) -> 1010 011 */
    0x58U, /* 0xB (1011) -> 1011 000 */
    0x62U, /* 0xC (1100) -> 1100 010 */
    0x69U, /* 0xD (1101) -> 1101 001 */
    0x74U, /* 0xE (1110) -> 1110 100 */
    0x7FU  /* 0xF (1111) -> 1111 111 */
};

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

hamming74_status_t hamming74_encode(const uint8_t *in_data, size_t in_bits_count, uint8_t *out_buf, size_t max_out_len, size_t *out_bits_count, hamming74_metrics_t *metrics) {
  if (in_data == NULL || out_buf == NULL) {
    return HAMMING74_ERR_PARAM;
  }

  if (in_bits_count == 0) {
    return HAMMING74_ERR_EMPTY;
  }

  size_t in_nibbles = (in_bits_count + 3U) / 4U;
  size_t total_out_bits = in_nibbles * 7U;
  size_t total_out_bytes = (total_out_bits + 7U) / 8U;

  if (total_out_bytes > max_out_len) {
    return HAMMING74_ERR_BUF_OVERFLOW;
  }

  /* Khởi tạo toàn bộ bộ đệm đầu ra bằng 0 */
  memset(out_buf, 0, total_out_bytes);

  for (size_t i = 0; i < in_nibbles; i++) {
    /* 1. Trích xuất 4 bit cho nhóm i (thứ tự MSB-first) */
    uint8_t nibble = 0;
    for (size_t b = 0; b < 4U; b++) {
      size_t in_bit_idx = (i * 4U) + b;
      if (in_bit_idx < in_bits_count) {
        uint8_t bit_val = (in_data[in_bit_idx >> 3U] >> (7U - (in_bit_idx & 7U))) & 1U;
        nibble |= (uint8_t)(bit_val << (3U - b));
      }
    }

    /* 2. Tra cứu từ mã 7-bit tương ứng qua LUT */
    uint8_t codeword = s_hamming74_lut[nibble];

    /* 3. Ghi 7 bit của codeword vào out_buf (thứ tự MSB-first: từ bit 6 xuống bit 0) */
    for (int b = 6; b >= 0; b--) {
      if (((codeword >> (uint8_t)b) & 1U) != 0U) {
        size_t out_bit_idx = (i * 7U) + (size_t)(6 - b);
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
  if (in_data == NULL || out_buf == NULL || in_bits_count == 0) {
    if (out_bits_count != NULL) {
      *out_bits_count = 0;
    }
    return 0;
  }

  size_t in_nibbles = (in_bits_count + 3U) / 4U;
  size_t total_out_bits = in_nibbles * 7U;
  size_t total_out_bytes = (total_out_bits + 7U) / 8U;

  hamming74_status_t status = hamming74_encode(in_data, in_bits_count, out_buf, total_out_bytes, out_bits_count, NULL);
  if (status != HAMMING74_OK) {
    if (out_bits_count != NULL) {
      *out_bits_count = 0;
    }
    return 0;
  }

  return total_out_bytes;
}

hamming74_status_t hamming74_encode_bytes(const uint8_t *in_bytes, size_t in_bytes_len, uint8_t *out_buf, size_t max_out_len, size_t *out_bits_count, hamming74_metrics_t *metrics) {
  if (in_bytes == NULL) {
    return HAMMING74_ERR_PARAM;
  }
  if (in_bytes_len == 0) {
    return HAMMING74_ERR_EMPTY;
  }

  return hamming74_encode(in_bytes, in_bytes_len * 8U, out_buf, max_out_len, out_bits_count, metrics);
}

bool hamming74_run_self_test(void) {
  for (uint8_t nibble = 0; nibble < HAMMING74_LUT_SIZE; nibble++) {
    uint8_t expected_parity = hamming74_calc_parity(nibble);
    uint8_t expected_codeword = (uint8_t)((nibble << 3U) | expected_parity);
    if (s_hamming74_lut[nibble] != expected_codeword) {
      return false;
    }
  }

  static const uint8_t test_in[4] = {0x12, 0x34, 0x56, 0x78};
  static const uint8_t test_expected_out[7] = {0x16, 0x58, 0xEA, 0x75, 0x8C, 0x5D, 0x45};
  uint8_t test_out[7] = {0};
  size_t out_bits = 0;

  size_t written_bytes = hamming74_encode_stream(test_in, 32U, test_out, &out_bits);
  if (written_bytes != 7U || out_bits != 56U) {
    return false;
  }

  if (memcmp(test_out, test_expected_out, 7U) != 0) {
    return false;
  }

  return true;
}
