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
  SHT30_ERR_NOT_READY = -1,
  SHT30_ERR_I2C = -2,
  SHT30_ERR_PARAM = -3
} sht30_status_t;

typedef struct {
  uint16_t t_raw;
  uint16_t rh_raw;
} sht30_sample_t;

typedef struct {
  uint32_t total_reads;
  uint32_t valid_samples;
  uint32_t i2c_errors;
} sht30_stats_t;

sht30_status_t sht30_init(void);
sht30_status_t sht30_read_sample(sht30_sample_t *sample);
void sht30_pack_sample_be(const sht30_sample_t *sample, uint8_t buf[4]);
float sht30_raw_to_celsius(uint16_t t_raw);
float sht30_raw_to_humidity(uint16_t rh_raw);
void sht30_get_stats(sht30_stats_t *out_stats);
void sht30_reset_stats(void);

#endif
