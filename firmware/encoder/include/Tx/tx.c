#include "tx.h"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

#define TX_UART_NODE DT_NODELABEL(uart1)
#define TX_BIT_PRINT_CHUNK 64U

static const struct device *const s_tx_uart = DEVICE_DT_GET(TX_UART_NODE);

static void build_frame_header(uint8_t hdr[TX_FRAME_HEADER_SIZE], const tx_frame_t *frame) {
  hdr[0] = TX_FRAME_MAGIC_0;
  hdr[1] = TX_FRAME_MAGIC_1;
  hdr[2] = frame->mode;
  hdr[3] = frame->stream;
  hdr[4] = (uint8_t)(frame->seq_id >> 8U);
  hdr[5] = (uint8_t)(frame->seq_id & 0xFFU);
  hdr[6] = (uint8_t)(frame->sample_count >> 8U);
  hdr[7] = (uint8_t)(frame->sample_count & 0xFFU);
  hdr[8] = (uint8_t)(frame->codebook_len >> 8U);
  hdr[9] = (uint8_t)(frame->codebook_len & 0xFFU);
  hdr[10] = (uint8_t)((frame->payload_bit_count >> 24U) & 0xFFU);
  hdr[11] = (uint8_t)((frame->payload_bit_count >> 16U) & 0xFFU);
  hdr[12] = (uint8_t)((frame->payload_bit_count >> 8U) & 0xFFU);
  hdr[13] = (uint8_t)(frame->payload_bit_count & 0xFFU);
  hdr[14] = (uint8_t)(frame->payload_len >> 8U);
  hdr[15] = (uint8_t)(frame->payload_len & 0xFFU);
}

static void uart_send_bytes(const uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    uart_poll_out(s_tx_uart, buf[i]);
  }
}

static void print_bytes_as_bits(const uint8_t *buf, size_t len) {
  char chunk[TX_BIT_PRINT_CHUNK + 1U];
  size_t pos = 0U;

  for (size_t i = 0; i < len; i++) {
    uint8_t b = buf[i];
    for (int bit = 7; bit >= 0; bit--) {
      chunk[pos++] = ((b >> (uint8_t)bit) & 1U) ? '1' : '0';
      if (pos == TX_BIT_PRINT_CHUNK) {
        chunk[pos] = '\0';
        printk("%s", chunk);
        pos = 0U;
      }
    }
  }
  if (pos > 0U) {
    chunk[pos] = '\0';
    printk("%s", chunk);
  }
}

tx_status_t tx_init(void) {
  return device_is_ready(s_tx_uart) ? TX_OK : TX_ERR_NOT_READY;
}

tx_status_t tx_send_frame(const tx_frame_t *frame) {
  if (!device_is_ready(s_tx_uart)) {
    return TX_ERR_NOT_READY;
  }
  if (!frame || (frame->codebook_len > 0U && !frame->codebook) || (frame->payload_len > 0U && !frame->payload)) {
    return TX_ERR_INVALID_ARG;
  }

  uint8_t hdr[TX_FRAME_HEADER_SIZE];
  build_frame_header(hdr, frame);

  uart_send_bytes(hdr, TX_FRAME_HEADER_SIZE);
  if (frame->codebook_len > 0U) {
    uart_send_bytes(frame->codebook, frame->codebook_len);
  }
  if (frame->payload_len > 0U) {
    uart_send_bytes(frame->payload, frame->payload_len);
  }

  return TX_OK;
}

void tx_print_sent_bits(const tx_frame_t *frame) {
  if (!frame) {
    return;
  }

  uint8_t hdr[TX_FRAME_HEADER_SIZE];
  build_frame_header(hdr, frame);

  size_t total_bytes = (size_t)TX_FRAME_HEADER_SIZE + (size_t)frame->codebook_len + (size_t)frame->payload_len;
  printk("[TX_UART] Seq=%u | Mode=%u | Stream=%u | Header=%uB | Codebook=%uB | Payload=%uB (%u bits) | Total=%uB (%u bits):\n", (unsigned)frame->seq_id, (unsigned)frame->mode, (unsigned)frame->stream, (unsigned)TX_FRAME_HEADER_SIZE, (unsigned)frame->codebook_len, (unsigned)frame->payload_len, (unsigned)frame->payload_bit_count, (unsigned)total_bytes, (unsigned)(total_bytes * 8U));
  print_bytes_as_bits(hdr, TX_FRAME_HEADER_SIZE);
  if (frame->codebook_len > 0U && frame->codebook) {
    print_bytes_as_bits(frame->codebook, frame->codebook_len);
  }
  if (frame->payload_len > 0U && frame->payload) {
    print_bytes_as_bits(frame->payload, frame->payload_len);
  }
  printk("\n");
}
