# File Tom tat lai /sht30_driver

## Cac hang so phan cung

- `SHT30_CMD_MEAS_HIGH_NO_CS(0x2400U)`: do don(single-shot), do lap lai cao.
- `SHT30_CMD_SOFT_RESET(0X30A2U)`: Software reset.
- `SHT30_MEAS_DELAY_MS(20U)`: Thoi gian cho do.
- `SHT30_RAW_READ_SIZE(6U)`: So byte doc tho tu SHT30(`2 byte T + 1 byte CRC_T + 2 byte RH + 1 byte CRC_RH`).
- `SHT30_PAYLOAD_SAMPLE_SIZE(4U)`: So byte du lieu sach dong goi gui di sau khi kiem tra CRC(`[T_MSB, T_LSB, RH_MSB, RH_LSB]`).

## Cac trang thai cua Sensor `sht30_status_t`

- `SHT30_OK` = 0
- `SHT30_ERR_NOT_READY` = -1, Bus I2C san sang
- `SHT30_ERR_I2C` = -2, Loi ack/nack, timeout
- `SHT30_ERR_CRC` = -3, Sai ma kiem tra crc - 8 
- `SHT30_ERR_PARAM` = -4 tham so con tro NULL

## Mot mau tho 16-bit `sht30_sample_t`

- `t_raw`: nhiet do 
- `rh_raw`: do dam

## Thong ke van hanh `sht30_stats_t`

- `total_reads`
- `valid_samples`
- `i2c_errors`
- `crc_errors`

## Cac ham data convert

### Quy doi sang do C

```cpp
    float sht30_raw_to_celsius(uint16_t t_raw) {
        return -45.0f + (175.0f * ((float)t_raw / 65535.0f));
    }
```
### Quy doi sang RH

```cpp
    float sht30_raw_to_humidity(uint16_t rh_raw) {
        return 100.0f * ((float)rh_raw / 65535.0f);
    }
```

## Ham init sht30 - `sht30_init(void)`

- Kiem tra neu sht30 chua khoi dong hoc i2c false thi  tra ve `SHT30_ERR_NOT_READY`.

- Khai bao cam bien neu loi thi cnt i2c_errors

## Gui lenh do `sht30_read_sample()`

- Kiem tra lai mot lan nua xem i2c da sang sang chua va sample da duong tro toi dau chua.
- Dem lan doc.
- Neu loi thi cnt++ cho i2c_errors.
- Xu ly crc8 cua `t` va `rh`. Neu noi ta cnt++ `crc_errors`.

- luu du lieu vao `t_raw` va `rh_raw`.

## Ham doc va dat lai bo dem thong ke noi bo `sht30_get_stats()`
## Dong goi `sht30_pack_sample_be()`

Dong goi ta ca vao mang buf
## Ham reset - `sht30_reset_stats(void)`

Dat lai ca 4 bo dem trong `g_stats` ve `0U`.
