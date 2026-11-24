#include "sht30_driver.h"
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

static const struct i2c_dt_spec sht30_i2c = I2C_DT_SPEC_GET(DT_NODELABEL(sht30));
static sht30_stats_t g_stats = {0};

sht30_status_t sht30_init(void) {
  if (!i2c_is_ready_dt(&sht30_i2c)) {
    return SHT30_ERR_NOT_READY;
  }

  uint8_t reset_cmd[2] = {(uint8_t)(SHT30_CMD_SOFT_RESET >> 8U), (uint8_t)(SHT30_CMD_SOFT_RESET & 0xFFU)};
  if (i2c_write_dt(&sht30_i2c, reset_cmd, sizeof(reset_cmd)) != 0) {
    g_stats.i2c_errors++;
  }

  k_msleep(5);
  return SHT30_OK;
}

sht30_status_t sht30_read_sample(sht30_sample_t *sample) {
  if (sample == NULL) {
    return SHT30_ERR_PARAM;
  }
  if (!i2c_is_ready_dt(&sht30_i2c)) {
    return SHT30_ERR_NOT_READY;
  }

  g_stats.total_reads++;
  uint8_t cmd[2] = {(uint8_t)(SHT30_CMD_MEAS_HIGH_NO_CS >> 8U), (uint8_t)(SHT30_CMD_MEAS_HIGH_NO_CS & 0xFFU)};
  if (i2c_write_dt(&sht30_i2c, cmd, sizeof(cmd)) != 0) {
    g_stats.i2c_errors++;
    return SHT30_ERR_I2C;
  }

  k_msleep(SHT30_MEAS_DELAY_MS);

  uint8_t rx_buf[SHT30_RAW_READ_SIZE] = {0};
  if (i2c_read_dt(&sht30_i2c, rx_buf, sizeof(rx_buf)) != 0) {
    g_stats.i2c_errors++;
    return SHT30_ERR_I2C;
  }

  sample->t_raw = ((uint16_t)rx_buf[0] << 8U) | (uint16_t)rx_buf[1];
  sample->rh_raw = ((uint16_t)rx_buf[3] << 8U) | (uint16_t)rx_buf[4];
  g_stats.valid_samples++;
  return SHT30_OK;
}

void sht30_pack_sample_be(const sht30_sample_t *sample, uint8_t buf[4]) {
  if (sample == NULL || buf == NULL) {
    return;
  }
  buf[0] = (uint8_t)(sample->t_raw >> 8U);
  buf[1] = (uint8_t)(sample->t_raw & 0xFFU);
  buf[2] = (uint8_t)(sample->rh_raw >> 8U);
  buf[3] = (uint8_t)(sample->rh_raw & 0xFFU);
}

float sht30_raw_to_celsius(uint16_t t_raw) { return -45.0f + (175.0f * ((float)t_raw / 65535.0f)); }

float sht30_raw_to_humidity(uint16_t rh_raw) { return 100.0f * ((float)rh_raw / 65535.0f); }

void sht30_get_stats(sht30_stats_t *out_stats) {
  if (out_stats != NULL) {
    *out_stats = g_stats;
  }
}

void sht30_reset_stats(void) { g_stats = (sht30_stats_t){0}; }
