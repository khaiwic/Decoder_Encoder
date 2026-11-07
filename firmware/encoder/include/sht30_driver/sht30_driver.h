#ifndef _SHT30_DRIVER_H_
#define _SHT30_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
SHT30_CMD_MEAS_HIGH_NO_CS: Lệnh đo chế độ Single-Shot (đo 1 lần khi có lệnh), 
    độ lặp lại cao (High Repeatability), 
    không kéo dài xung clock (No Clock Stretching). 
    Đây là chế độ đo chuẩn, tin cậy nhất được đề bài quy định.
SHT30_CMD_SOFT_RESET (0x30A2U): Lệnh khởi động lại cảm biến qua phần mềm (Software Reset), 
    đưa SHT30 về trạng thái ban đầu sạch sẽ khi khởi động board.
SHT30_MEAS_DELAY_MS (20U): Sau khi gửi lệnh đo 0x2400, 
    SHT30 cần tối đa ~15.5 ms để mạch ADC bên trong đo xong nhiệt độ và độ ẩm. 
    Chọn nghỉ 20 ms để đảm bảo cảm biến đã đo xong trước khi đọc dữ liệu về.
SHT30_RAW_READ_SIZE (6U): Mỗi lần đọc, SHT30 trả về đúng 6 byte qua I²C:
    T_MSB ∣ T_LSB ∣ CRC_T ∣ RH_MSB ∣ RH_LSB ∣ CRC_RH
SHT30_PAYLOAD_SAMPLE_SIZE (4U): Kích thước 1 mẫu sau khi đã kiểm tra CRC xong và loại bỏ 2 byte CRC: gồm 2 byte nhiệt độ + 2 byte độ ẩm = 4 byte/mẫu.
*/
#define SHT30_CMD_MEAS_HIGH_NO_CS   0x2400U
#define SHT30_CMD_SOFT_RESET        0x30A2U
#define SHT30_MEAS_DELAY_MS         20U
#define SHT30_RAW_READ_SIZE         6U
#define SHT30_PAYLOAD_SAMPLE_SIZE   4U

typedef enum {
    SHT30_OK = 0,
    SHT30_ERR_NOT_READY = -1, // Bus I2C chua san sang
    SHT30_ERR_I2C       = -2, // Loi ack/nack, timeout
    SHT30_ERR_CRC       = -3, // Sai ma kiem tra crc-8
    SHT30_ERR_PARAM     = -4  // Tham so con tro NULL
} sht30_status_t;

typedef struct
{
    uint16_t t_raw;  // Temperature 16-bit
    uint16_t rh_raw; // Do am 16-bit
} sht30_sample_t;

/*
total_reads: Tổng số lần đọc cảm biến.
valid_samples: Số mẫu hợp lệ thành công - CRC(đúng)
i2c_errors: Số lần lỗi i2c
crc_erros: Số lần sai mã kiểm tra CRC-8
*/
typedef struct 
{
    uint32_t total_reads;
    uint32_t valid_samples;
    uint32_t i2c_errors;
    uint32_t crc_errors; 
} sht30_stats_t;

/*Cau hinh SHT30 cho du an: Khởi tạo sensor, kiểm tra kết nối và thử soft reset*/
sht30_status_t sht30_init(void);
/*Doc cac example: Gửi lệnh 0x2400U, chờ 20ms, đọc 6byte, Kiểm tra CRC-8 và lưu vào sample*/
sht30_status_t sht30_read_sample(sht30_sample_t *sample);
/*Đóng gói mẫu 16bit thành 4byte*/
void sht30_pack_sample_be(const sht30_sample_t *sample, uint8_t buf[4]);

//Convert
float sht30_raw_to_celsius(uint16_t t_raw);
float sht30_raw_to_humidity(uint16_t rh_raw);
//Lấy số liệu lỗi ra ngoài
void sht30_get_stats(sht30_stats_t *out_stats);
//đặt lại bộ đếm về 0
void sht30_reset_stats(void);

#endif