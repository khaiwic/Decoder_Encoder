#ifndef TX_H
#define TX_H

#include <stddef.h>
#include <stdint.h>

#define TX_FRAME_MAGIC_0 0xAAU
#define TX_FRAME_MAGIC_1 0x55U
#define TX_FRAME_HEADER_SIZE 16U

typedef enum {
  TX_OK = 0,
  TX_ERR_NOT_READY,
  TX_ERR_INVALID_ARG
} tx_status_t;

typedef struct {
  uint8_t mode;
  uint8_t stream;
  uint16_t seq_id;
  uint16_t sample_count;
  uint16_t codebook_len;
  uint32_t payload_bit_count;
  uint16_t payload_len;
  const uint8_t *codebook;
  const uint8_t *payload;
} tx_frame_t;

tx_status_t tx_init(void);
tx_status_t tx_send_frame(const tx_frame_t *frame);
void tx_print_sent_bits(const tx_frame_t *frame);

#endif
