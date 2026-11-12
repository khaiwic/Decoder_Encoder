#ifndef _SHT30_DRIVER_H_
#define _SHT30_DRIVER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SHT30_CMD_MEAS_HIGH_NO_CS 0x2400U
#define SHT30_CMD_SOFT_RESET 0x30A2U
#define SHT30_MEAS_DELAY_MS 20U
#define SHT30_RAW_READ_SIZE 6U
#define SHT30_PAYLOAD_SAMPLE_SIZE 4U

typedef enum {
  SHT30_OK = 0,
  SHT30_ERR_NOT_READY = -1, // Bus I2C chua san sang
  SHT30_ERR_I2C = -2,       // Loi ack/nack, timeout
  SHT30_ERR_CRC = -3,       // Sai ma kiem tra crc-8
  SHT30_ERR_PARAM = -4      // Tham so con tro NULL
} sht30_status_t;

typedef struct {
  uint16_t t_raw;  // Temperature 16-bit
  uint16_t rh_raw; // Do am 16-bit
} sht30_sample_t;

typedef struct {
  uint32_t total_reads;   // Sum la doc cam bien
  uint32_t valid_samples; // So mau hop le thanh cong - CRC dung
  uint32_t i2c_errors;
  uint32_t crc_errors;
} sht30_stats_t;

sht30_status_t sht30_init(void);
sht30_status_t sht30_read_sample(sht30_sample_t *sample);
// wrapp[[[]]]
void sht30_pack_sample_be(const sht30_sample_t *sample, uint8_t buf[4]);

// Convert
float sht30_raw_to_celsius(uint16_t t_raw);
float sht30_raw_to_humidity(uint16_t rh_raw);
// Lấy số liệu lỗi ra ngoài
void sht30_get_stats(sht30_stats_t *out_stats);
// đặt lại bộ đếm về 0
void sht30_reset_stats(void);

#endif