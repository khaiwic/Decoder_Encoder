#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "hamming74_encoder.h"
#include "huffman_encoder.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOGGER_CSV_TAG_HEADER "[CSV_HEADER]"
#define LOGGER_CSV_TAG_DATA "[CSV_DATA]"

typedef enum {
  ENCODER_MODE_DEFAULT = 0
} encoder_mode_t;

typedef enum {
  ENCODER_STREAM_TEMP = 0,
  ENCODER_STREAM_HUMD,
  ENCODER_STREAM_TOTAL
} encoder_stream_t;

typedef struct {
  uint32_t seq_id;
  uint32_t batch_id;
  encoder_mode_t mode;
  encoder_stream_t stream;
  size_t valid_samples;
  uint32_t dropped_samples;
  float avg_temp_c;
  float avg_rh_pct;
  size_t raw_bytes;
  float entropy;
  float avg_code_len;
  float coding_efficiency;
  uint16_t huff_used_symbols;
  size_t huff_codebook_bytes;
  size_t huff_valid_bits;
  size_t huff_bytes;
  float payload_comp_ratio;
  float net_comp_ratio;
  size_t ham_valid_bits;
  size_t ham_bytes;
  float ham_code_rate;
  uint32_t encode_time_us;
  int status;
} encoder_log_record_t;

void logger_init(void);
void logger_print_banner(void);
void logger_print_csv_header(void);
const char *logger_mode_to_str(encoder_mode_t mode);
const char *logger_stream_to_str(encoder_stream_t stream);

void logger_print_stream_collected(const char *label, const char *unit, uint32_t batch_id, const uint8_t *data, size_t count);
void logger_print_probability_table(const char *label, const uint8_t *data, size_t len);
void logger_print_huffman_table(const char *label, const huffman_codebook_t *codebook, const huffman_metrics_t *metrics, const uint8_t *bitstream);
void logger_print_hamming_theory(const hamming74_params_t *params);
void logger_print_hamming_stream_steps(const char *label, const uint8_t *in_huff_bits, size_t in_valid_bits, const uint8_t *out_ham_bits, const hamming74_metrics_t *ham_metrics, uint32_t encode_us);

void logger_log_record(const encoder_log_record_t *record);
void logger_log_encoder(uint32_t batch_id, encoder_stream_t stream, size_t valid_samples, uint32_t dropped_samples, float avg_temp, float avg_rh, size_t raw_bytes_size, uint16_t huff_used_symbols, size_t book_bytes, const huffman_metrics_t *h_metrics, const hamming74_metrics_t *ham_metrics, uint32_t elapsed_us, int status);

#ifdef __cplusplus
}
#endif

#endif
