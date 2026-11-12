#ifndef _HUFFMAN_ENCODER_H_
#define _HUFFMAN_ENCODER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HUFFMAN_MAX_SYMBOLS 256U
#define HUFFMAN_MAX_CODE_LEN 32U

typedef enum {
  HUFFMAN_OK = 0,
  HUFFMAN_ERR_PARAM = -1,
  HUFFMAN_ERR_EMPTY = -2,
  HUFFMAN_ERR_BUF_OVERFLOW = -3
} huffman_status_t;

/* Cặp (symbol, code_length) dùng để tái tạo cây Canonical Huffman tại RX */
typedef struct {
  uint8_t symbol; /* Giá trị byte ký hiệu (0..255) */
  uint8_t len;    /* Độ dài từ mã Huffman nhị phân (bit) */
} huffman_book_entry_t;

typedef struct {
  uint16_t used_symbols_count;
  huffman_book_entry_t entries[HUFFMAN_MAX_SYMBOLS];
} huffman_codebook_t;

typedef struct {
  size_t raw_bytes_count;
  size_t coded_valid_bits;
  size_t coded_bytes_count;
  size_t codebook_bytes_size;
  float entropy;
  float avg_code_len;
  float payload_comp_ratio;
  float net_comp_ratio;
} huffman_metrics_t;

huffman_status_t huffman_encode(const uint8_t *in_data, size_t in_len, uint8_t *out_bitstream, size_t max_out_len, huffman_codebook_t *out_book, huffman_metrics_t *metrics);
int huffman_serialize_codebook(const huffman_codebook_t *book, uint8_t *out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif
