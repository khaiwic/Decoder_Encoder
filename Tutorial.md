# Hướng dẫn sử dụng Zephyr (Đã cài trên máy)

## Cài các công cụ còn thiếu

- Mở và kích hoạt môi trường and SDK:

```powershell
    Set-Location C:\zephyrproject

    Set-ExecutionPolicy -Scope Process -ExecutionPolicy RemoteSigned

    .\.venv312\Scripts\Activate.ps1

    $env:ZEPHYR_BASE = 'C:\zephyrproject\zephyr'
    $env:ZEPHYR_SDK_INSTALL_DIR = 'C:\zephyr-sdk-1.0.1'
    $env:ZEPHYR_TOOLCHAIN_VARIANT = 'zephyr'
```

- Chạy và kiểm tra môi trường:

```powershell
    python --version
    west --version
    west topdir
    west sdk list
```

- Mở dự án trong VScode:

```powershell
    code -n C:\Users\ADMIN\Documents\Decoder_and_Encoder
```

## Hướng dẫn sử dụng các công cụ build và flash của Zephyr

### Build

- Mở Terminal và chạy lệnh sau:

```powershell
    west build -p auto `
    -b esp32s3_devkitc/esp32s3/procpu `
    -s C:\Users\ADMIN\Documents\Decoder_and_Encoder\firmware\encoder `
    -d C:\Users\ADMIN\Documents\Decoder_and_Encoder\build\encoder
```

Sau khi build xong sẽ có folder `build` xuất hiện (Nếu thành công).

### Flash

- Mở Terminal và chạy lệnh sau (thay `COMx` bằng cổng COM thực tế):

```powershell
    west flash `
    -d C:\Users\ADMIN\Documents\Decoder_and_Encoder\build\encoder `
    --esp-device COMx
```

### Tìm cổng COM

- Mở máy tính và thao tác như sau:

Mở **Device Manager** -> **Ports (COM & LPT)** -> Ta sẽ nhìn thấy cổng kết nối của ESP32 với máy tính, sau đó điền tên cổng (ví dụ `COM3`, `COM5`) vào `x`.

### Serial Monitor cơ bản

- Xem danh sách cổng COM của board:

```powershell
    python -m serial.tools.list_ports
```

- Mở Serial Monitor bằng công cụ của Zephyr/Espressif:

```powershell
    Set-Location C:\Users\ADMIN\Documents\Decoder_and_Encoder\build\encoder
    west espressif monitor -p COMx -b 115200
```

---

## Hướng dẫn thực thi dự án và kiểm thử Khối 1 với cảm biến SHT30 (TX Encoder)

### 1. Đấu nối phần cứng SHT30 với ESP32-S3 #1 (TX)

Theo cấu hình trong `firmware/encoder/app.overlay` và `README.md`:

| Chân SHT30 | Chân ESP32-S3 | Ghi chú |
| :---: | :---: | :--- |
| **VCC / VIN** | **3V3** | Cấp nguồn 3.3V |
| **GND** | **GND** | Nối mass chung |
| **SDA** | **GPIO8** | Dữ liệu I²C (`i2c0`) |
| **SCL** | **GPIO9** | Xung nhịp I²C (`i2c0`, tốc độ 100 kHz) |

*(Địa chỉ I²C mặc định: `0x44`)*.

---

### 2. Kiến trúc các module thực thi trong Khối 1

1. **`sht30_driver`**: Gửi lệnh đo Single-Shot `0x2400`, đọc 6 byte từ SHT30 qua I²C, kiểm tra độc lập 2 mã CRC-8 cho nhiệt độ và độ ẩm, đóng gói mẫu hợp lệ thành 4 byte Big-Endian `[T_MSB, T_LSB, RH_MSB, RH_LSB]`.
2. **`huffman_encoder`**: Nhận lô $N = 128$ mẫu hợp lệ ($512$ byte raw), tính Entropy $H(X)$, xây dựng cây Canonical Huffman, nén dữ liệu và trích xuất Codebook.
3. **`hamming74_encoder`**: Nhận dòng bit sau nén Huffman, chia thành các nhóm 4-bit và mã hóa thành các từ mã 7-bit dạng hệ thống $c = [d_1, d_2, d_3, d_4, p_1, p_2, p_3]$, đóng gói MSB-first.
4. **`logger`**: Xuất bảng đối chiếu 3 chế độ (A: RAW, B: HUFFMAN, C: HUFFMAN+HAMMING) ra Serial Monitor và phát các dòng `[CSV_DATA]` để máy tính lưu tự động.

---

### 3. Quy trình chạy và thu thập dữ liệu thực nghiệm ra file `.csv`

#### Bước 1: Build và Flash nạp firmware vào ESP32-S3

Chạy trong PowerShell đã kích hoạt môi trường Zephyr:

```powershell
Set-Location C:\Users\ADMIN\Documents\Decoder_and_Encoder

# Build firmware
west build -b esp32s3_devkitc/esp32s3/procpu -s firmware/encoder -d build/encoder

# Flash nạp code vào board (thay COMx bằng cổng thực tế, ví dụ COM5)
west flash -d build/encoder --esp-device COMx
```

#### Bước 2: Chạy script Python thu thập log Serial và xuất CSV

Mở cửa sổ Terminal và chạy:

```powershell
Set-Location C:\Users\ADMIN\Documents\Decoder_and_Encoder

# Cài đặt pyserial nếu máy chưa cài
pip install pyserial

# Tự động quét cổng COM và bắt đầu ghi log từ SHT30
python firmware/encoder/sourcepy/collect_logs.py
```

> **Hoặc chỉ định cổng COM cụ thể:**
> ```powershell
> python firmware/encoder/sourcepy/collect_logs.py --port COM5 --baud 115200
> ```

Khi chương trình chạy:
- ESP32-S3 đọc từng mẫu nhiệt độ / độ ẩm thực tế từ SHT30 (in tiến độ trực tiếp lên màn hình).
- Khi gom đủ lô $N = 128$ mẫu hợp lệ ($512$ byte), vi điều khiển thực thi cả 3 chế độ mã hóa (A, B, C) trên cùng lô dữ liệu đó và gửi kết quả về máy tính.
- Script Python tự động lưu dữ liệu vào:
  - File CSV tích lũy: `firmware/encoder/results/tx_metrics.csv`
  - File CSV phiên chạy: `firmware/encoder/results/tx_metrics_<ngày_giờ>.csv`
  - File log Serial thô: `firmware/encoder/results/raw_logs/tx_serial_<ngày_giờ>.log`

Nhấn **`Ctrl + C`** để kết thúc thu thập dữ liệu.

---

### 4. Giải thích các cột trong file `results/tx_metrics.csv`

| Cột | Ý nghĩa |
| :--- | :--- |
| `seq_id` | Số thứ tự bản ghi |
| `batch_id` | Số thứ tự lô mẫu cảm biến SHT30 (mỗi lô gồm 3 chế độ A, B, C) |
| `mode` | Chế độ mã hóa: `RAW`, `HUFFMAN`, `HUFFMAN_HAMMING` |
| `valid_samples` | Số mẫu SHT30 đọc hợp lệ trong lô ($N = 128$) |
| `dropped_samples` | Số mẫu bị loại bỏ do lỗi đường truyền I²C hoặc sai CRC-8 cảm biến |
| `avg_temp_c` | Nhiệt độ trung bình của lô mẫu ($^{\circ}\text{C}$) |
| `avg_rh_pct` | Độ ẩm trung bình của lô mẫu ($\%\text{RH}$) |
| `raw_bytes` | Kích thước dữ liệu gốc ($4N = 512$ bytes) |
| `raw_crc32` | Mã CRC-32 (IEEE 802.3) của 512 bytes dữ liệu gốc |
| `entropy` | Entropy nguồn tin thực tế $H(X)$ (bit/symbol) |
| `payload_ratio` | Tỷ lệ nén payload: $\frac{\text{raw\_bits}}{\text{huff\_valid\_bits}}$ |
| `net_ratio` | Tỷ lệ nén toàn phần (tính cả Codebook): $\frac{\text{raw\_bytes}}{\text{huff\_bytes} + \text{codebook\_bytes}}$ |
| `ham_rate` | Tốc độ mã kênh Hamming(7,4) ($R \approx 0.5714$) |
| `encode_us` | Thời gian thực thi mã hóa trên ESP32-S3 (micro-giây) |
