#include "sht30_driver.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

//Marco cấu hình i2c đã setup tại app.overlay
static const struct i2c_dt_spec sht30_i2c = I2C_DT_SPEC_GET(DT_NODELABEL(sht30));

//g_stats: Biến toàn cục lưu lại số lần đọc thành công, số lần lỗi I²C, lỗi CRC.
static sht30_stats_t g_stats = {0};

//Kiem tra ma CRC-8
static uint8_t sht30_compute_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFFU;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; bit++) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1U) ^ 0x31U);
            } else {
                crc <<= 1U;
            }
        }
    }
    return crc;
}

sht30_status_t sht30_init(void) {
    if(!i2c_is_ready_dt(&sht30_i2c)) {
        return SHT30_ERR_NOT_READY;
    }

    uint8_t reset_cmd[2] = {
        (uint8_t)(SHT30_CMD_SOFT_RESET >> 8),
        (uint8_t)(SHT30_CMD_SOFT_RESET & 0xFFU)
    };

    int ret = i2c_write_dt(&sht30_i2c, reset_cmd, sizeof(reset_cmd));

    if(ret != 0) {
        g_stats.i2c_errors++;
    }

    k_msleep(5);
    return SHT30_OK;
}

sht30_status_t sht30_read_sample(sht30_sample_t *sample)
{
    if (sample == NULL) {
        return SHT30_ERR_PARAM;
    }
    if (!i2c_is_ready_dt(&sht30_i2c)) {
        return SHT30_ERR_NOT_READY;
    }
    g_stats.total_reads++;
    /* 1. Gửi lệnh đo Single-SShot- 0x2400*/
    uint8_t cmd[2] = {
        (uint8_t)(SHT30_CMD_MEAS_HIGH_NO_CS >> 8),
        (uint8_t)(SHT30_CMD_MEAS_HIGH_NO_CS & 0xFFU)
    };
    int ret = i2c_write_dt(&sht30_i2c, cmd, sizeof(cmd));
    if (ret != 0) {
        g_stats.i2c_errors++;
        return SHT30_ERR_I2C;
    }
    k_msleep(SHT30_MEAS_DELAY_MS);
    //Buffer luu ru lieu tra ve
    uint8_t rx_buf[SHT30_RAW_READ_SIZE] = {0};
    ret = i2c_read_dt(&sht30_i2c, rx_buf, sizeof(rx_buf));
    if (ret != 0) {
        g_stats.i2c_errors++;
        return SHT30_ERR_I2C;
    }
    uint8_t crc_t_calc  = sht30_compute_crc8(&rx_buf[0], 2U);
    uint8_t crc_rh_calc = sht30_compute_crc8(&rx_buf[3], 2U);
    if ((crc_t_calc != rx_buf[2]) || (crc_rh_calc != rx_buf[5])) {
        g_stats.crc_errors++;
        return SHT30_ERR_CRC;
    }
    /* 5. Ghép thành 2 số nguyên 16-bit gốc */
    sample->t_raw  = ((uint16_t)rx_buf[0] << 8U) | (uint16_t)rx_buf[1];
    sample->rh_raw = ((uint16_t)rx_buf[3] << 8U) | (uint16_t)rx_buf[4];
    g_stats.valid_samples++;
    return SHT30_OK;
}

void sht30_pack_sample_be(const sht30_sample_t *sample, uint8_t buf[4])
{
    if (sample == NULL || buf == NULL) {
        return;
    }
    /* Big-endian theo đúng mục 3.2 trong README: T_MSB, T_LSB, RH_MSB, RH_LSB */
    buf[0] = (uint8_t)(sample->t_raw >> 8U);
    buf[1] = (uint8_t)(sample->t_raw & 0xFFU);
    buf[2] = (uint8_t)(sample->rh_raw >> 8U);
    buf[3] = (uint8_t)(sample->rh_raw & 0xFFU);
}

float sht30_raw_to_celsius(uint16_t t_raw)
{
    return -45.0f + (175.0f * ((float)t_raw / 65535.0f));
}
float sht30_raw_to_humidity(uint16_t rh_raw)
{
    return 100.0f * ((float)rh_raw / 65535.0f);
}
void sht30_get_stats(sht30_stats_t *out_stats)
{
    if (out_stats != NULL) {
        *out_stats = g_stats;
    }
}

void sht30_reset_stats(void)
{
    g_stats.total_reads   = 0U;
    g_stats.valid_samples = 0U;
    g_stats.i2c_errors    = 0U;
    g_stats.crc_errors    = 0U;
}