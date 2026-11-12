# README cho module thử nhất
> Máy phát: Đọc SHT30 qua I2C --> gom mẫu --> Mã hóa nguồn Huffman --> Mã hóa kênh Hamming(7, 4) dạng  hệ thống --> Gửi UART.

## Các chế độ thực thi 
| Chế độ | Mã hóa nguồn | Mã hóa kênh| Mục tiêu so sánh|
|---|---|---|---|
|**A - RAW**| Không | Không | Đường cơ sở không nén/ không bảo vệ | 
|**B - Huffman**| Huffman| Không | Hiệu quả  và chi phí nén của nguồn|
|**C - Huffman + Hamming**| Huffman | Hamming(7, 4) | Khả năng sửa lỗi và overhead bảo vệ dữ liệu |

## Tiêu chỉ học thuật

- Phân biệt **Mã hóa nguồn** (giảm số bit trung bình) và **Mã hóa kênh**.
- Chứng minh bằng số liệu thực rằng Huffman **có thể không giảm** dung lượng **toàn gói** trên bản tin ngắn do overhead của codebook và metadata.
- So sánh tỷ lệ lỗi trước và sau khi mã hóa kênh, độ trễ tỷ lệ khôi phục thành công. 
- Đối chiếu với lý thuyết.

## Kiến trúc hệ thống Module 1

```text
SHT30(T, RH) --> Kiểm tra CRC - 8 --> Huffman encoder --> Hamming encoder --> Uart
```

## Kết nối phần cứng đề xuất

**SHT30 → ESP32-S3 TX:**

| Tín hiệu SHT30 | ESP32-S3 #1 | Ghi chú |
| --- | --- | --- |
| VCC / VIN | 3V3 | Cấp 3,3 V|
| GND | GND | Nối mass chung |
| SDA | GPIO8 | Dữ liệu I²C|
| SCL | GPIO9 | Clock I²C|

## Đặc tả dữ liệu SHT30 - điểm thay đổi quan trọng so với DHT11

### Dữ liệu I2C SHT30

Một kết quả đo single-shot SHT30 gồm **6byte** theo thứ tự:

```text
T_MSB | T_LSB | CRC_T | RH_MSB | RH_LSB | CRC_RH
  2 byte nhiệt độ           2 byte độ ẩm
```
