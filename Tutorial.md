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

> **Lưu ý:** Không mở `west espressif monitor` cùng lúc với `collect_logs.py` trên cùng một cổng `COMx` để tránh xung đột cổng Serial. Script `collect_logs.py` đã hỗ trợ sẵn giao tiếp 2 chiều (vừa xem log, vừa gõ lệnh điều khiển và tự động lưu file).

---

## Hướng dẫn thực thi dự án và kiểm thử Khối 1 với cảm biến SHT30 (TX Encoder)

### 1. Đấu nối phần cứng Khối 1 (ESP32-S3 #1 - TX)

Theo cấu hình trong `firmware/encoder/app.overlay`:

#### Cảm biến SHT30 qua I²C (`i2c0`, 100 kHz, địa chỉ `0x44`)

| Chân SHT30 | Chân ESP32-S3 | Ghi chú |
| :---: | :---: | :--- |
| **VCC / VIN** | **3V3** | Cấp nguồn 3.3V |
| **GND** | **GND** | Nối mass chung |
| **SDA** | **GPIO8** | Dữ liệu I²C (`i2c0`) |
| **SCL** | **GPIO9** | Xung nhịp I²C (`i2c0`) |

#### Giao tiếp UART1 (`uart1`, `115200 bps`) sang Board 2 (Channel)

| Chân ESP32-S3 #1 (TX) | Chức năng | Kết nối sang Board 2 |
| :---: | :---: | :--- |
| **GPIO17** | `UART1 TX` | Nối vào chân **RX** của Board 2 |
| **GPIO18** | `UART1 RX` | Dự phòng (nối **TX** của Board 2 nếu cần) |
| **GND** | `GND` | Nối mass chung giữa 2 board |

---

### 2. Kiến trúc các module thực thi trong Khối 1

1. **`sht30_driver`**: Gửi lệnh đo Single-Shot `0x2400`, đọc đủ 6 byte từ SHT30 qua I²C (không kiểm tra CRC-8, chỉ kiểm tra lỗi giao tiếp I²C). Trong `factory.c`, nhiệt độ ($^{\circ}\text{C}$) và độ ẩm ($\%\text{RH}$) được làm tròn thành số nguyên dương 8-bit và lưu tách riêng vào 2 mảng: `temp[128]` (`128 B`, kẹp `[0, 255]`) và `humd[128]` (`128 B`, kẹp `[0, 100]`).
2. **`huffman_encoder`**: Nhận độc lập từng luồng $N = 128$ byte (`Temp` và `Humd`), tính Entropy $H(X)$, xây dựng cây Canonical Huffman theo `LTTT05`, nén bitstream MSB-first và trích xuất `Codebook` rút gọn (`2 + 2*K` byte).
3. **`hamming74_encoder`**: Thực hiện mã hóa kênh tuyến tính Hamming(7,4) theo `LTTT06` dưới dạng hệ thống $c = [a \mid p_1]$ ($l = 7$, $k = 4$, $r = l - k = 3$, tỷ số mã $R = k/l = 4/7 \approx 0.5714$, $d_{\min} = 3$, phát hiện tối đa $2$ bit lỗi, sửa tối đa $1$ bit lỗi). Đọc lần lượt nhóm 4 bit từ bitstream Huffman của `Temp` rồi `Humd`, nhân $c = a \cdot G$ và xuất chuỗi nhị phân `hamming_bitstream`.
4. **`control_pannel`**: Điều phối giao tiếp lệnh Serial Console theo 2 bước:
   - Trạng thái chờ (`IDLE`): chỉ hiển thị và chấp nhận `COLLECT` (thu 128 mẫu mới) và `ENCODER` (mã hóa Canonical Huffman + Hamming 7,4).
   - Ngay sau khi chạy `ENCODER`: chỉ hiển thị và chấp nhận `IDLE` (quay lại chờ) và `TRANSMIT_UART` (truyền dữ liệu sang Board 2).
5. **`Tx`**: Đóng gói từng luồng (`TEMP`, `HUMD`) với header 16 byte Big-Endian (`Magic 0xAA 0x55`, `mode`, `stream`, `seq_id`, `sample_count`, `codebook_len`, `payload_bit_count`, `payload_len`) + `Codebook` + `hamming_bitstream`, gửi qua `UART1` (`GPIO17`, `115200 bps`) và in toàn bộ chuỗi bit nhị phân `0`/`1` đã truyền ra Serial Console.
6. **`logger`**: In ma trận 128 mẫu, bảng phân bố xác suất tăng dần, bảng từ mã Canonical Huffman, các tham số & bước tính Hamming(7,4), dòng `[CSV_DATA]` (chế độ duy nhất `ENCODER`), đồng thời phát cặp marker `[LOG_STAGE_START]` / `[LOG_STAGE_END]` cho 3 trạng thái `collect`, `encoder`, `transmit_uart`.

---

### 3. Quy trình chạy và thu thập log theo từng lô

#### Bước 1: Build và Flash nạp firmware vào ESP32-S3

Chạy trong PowerShell đã kích hoạt môi trường Zephyr:

```powershell
Set-Location C:\Users\ADMIN\Documents\Decoder_and_Encoder

# Build firmware
west build -p auto -b esp32s3_devkitc/esp32s3/procpu -s firmware/encoder -d build/encoder

# Flash nạp code vào board (thay COMx bằng cổng thực tế, ví dụ COM5)
west flash -d build/encoder --esp-device COMx
```

#### Bước 2: Chạy script Python thu log 2 chiều theo lô

Mở cửa sổ Terminal và chạy:

```powershell
Set-Location C:\Users\ADMIN\Documents\Decoder_and_Encoder

# Cài đặt pyserial nếu máy chưa cài
pip install pyserial

# Tự động quét cổng COM, giao tiếp 2 chiều và tự tách log theo từng lô
python firmware/encoder/sourcepy/collect_logs.py
```

> **Các tùy chọn thường dùng:**
> ```powershell
> # Chỉ định cổng COM và baudrate cụ thể
> python firmware/encoder/sourcepy/collect_logs.py --port COM3 --baud 115200
>
> # Ghi đè lại từ lô đầu (lo1) thay vì nối tiếp các thư mục lo<N> đã có
> python firmware/encoder/sourcepy/collect_logs.py --port COM3 --overwrite
> ```

*(Hoặc chạy qua wrapper `python scripts/collect_logs.py --port COM5`.)*

#### Bước 3: Điều khiển chu trình đo trên Terminal

Ngay trong cửa sổ đang chạy `collect_logs.py`, thực hiện tuần tự cho mỗi lô dữ liệu:
1. Gõ **`COLLECT`** → ESP32-S3 đọc 128 mẫu SHT30 (`temp[128]`, `humd[128]`), in ma trận mẫu và bảng phân bố xác suất tăng dần.
2. Gõ **`ENCODER`** → ESP32-S3 chạy Canonical Huffman + Hamming(7,4) độc lập cho `TEMP` và `HUMD`, in bảng mã, tham số LTTT06, bitstream nhị phân và các dòng `[CSV_DATA]`.
3. Gõ **`TRANSMIT_UART`** *(hoặc **`IDLE`** để bỏ qua gửi)* → Khối `Tx` gửi lần lượt 2 frame `TEMP` và `HUMD` qua `UART1` (`GPIO17`), in toàn bộ chuỗi nhị phân đã gửi lên Serial rồi tự động quay lại `IDLE`.

Mỗi lô dữ liệu sẽ được lưu tự động thành một thư mục riêng trong `firmware/encoder/results/lo<N>/` (ví dụ `firmware/encoder/results/lo1/`) gồm đúng 3 file log theo 3 trạng thái:
- `collect.log` — Toàn bộ log thu thập 128 mẫu và bảng phân bố xác suất của lô đó
- `encoder.log` — Toàn bộ log bảng mã Huffman, tham số & bước Hamming(7,4), bitstream và `[CSV_DATA]`
- `transmit_uart.log` — Thông tin frame và chuỗi bit nhị phân `0`/`1` thực tế đã phát qua `UART1`

Nhấn **`Ctrl + C`** để dừng chương trình và đóng cổng Serial an toàn.

---

### 4. Giải thích 23 cột trong dòng `[CSV_DATA]` (lưu trong `results/lo<N>/encoder.log`)

| Cột | Ý nghĩa |
| :--- | :--- |
| `seq_id` | Số thứ tự bản ghi log phát ra từ khi khởi động |
| `batch_id` | Số thứ tự lô mẫu cảm biến SHT30 (`1, 2, ...`) |
| `mode` | Chế độ mã hóa duy nhất hiện tại: `ENCODER` |
| `stream` | Luồng dữ liệu của bản ghi: `TEMP`, `HUMD`, hoặc `TOTAL` |
| `valid_samples` | Số mẫu SHT30 đọc hợp lệ trong lô ($N = 128$) |
| `dropped_samples` | Số lần đọc bị lỗi giao tiếp I²C trong quá trình thu thập lô |
| `avg_temp_c` | Nhiệt độ trung bình của lô mẫu ($^{\circ}\text{C}$) |
| `avg_rh_pct` | Độ ẩm trung bình của lô mẫu ($\%\text{RH}$) |
| `raw_bytes` | Kích thước dữ liệu 8-bit trước nén (`128` byte cho `TEMP`/`HUMD`, `256` byte cho `TOTAL`) |
| `entropy` | Entropy nguồn tin thực tế $H(X)$ (bit/ký hiệu) |
| `avg_code_len` | Độ dài từ mã Huffman trung bình $\bar{l}$ (bit/ký hiệu) |
| `efficiency` | Hiệu suất mã hóa nguồn $\eta = H(X) / \bar{l}$ |
| `huff_symbols` | Số ký hiệu phân biệt có trong `Codebook` |
| `codebook_bytes` | Kích thước `Codebook` tuần tự hóa (`2 + 2*K` byte) |
| `huff_bits` | Tổng số bit hợp lệ của bitstream sau nén Huffman |
| `huff_bytes` | Số byte đóng gói của bitstream Huffman (`ceil(huff_bits / 8)`) |
| `payload_ratio` | Tỷ lệ nén payload Huffman: $\frac{\text{raw\_bytes} \times 8}{\text{huff\_bits}}$ |
| `net_ratio` | Tỷ lệ nén toàn phần (tính cả Codebook): $\frac{\text{raw\_bytes}}{\text{huff\_bytes} + \text{codebook\_bytes}}$ |
| `ham_bits` | Tổng số bit đầu ra sau mã hóa kênh Hamming(7,4) |
| `ham_bytes` | Số byte đóng gói của bitstream Hamming(7,4) (`ceil(ham_bits / 8)`) |
| `ham_rate` | Tỷ số mã kênh Hamming(7,4) thực tế ($R \approx 0.5714$) |
| `encode_us` | Thời gian thực thi mã hóa trên ESP32-S3 (micro-giây) |
| `status` | Trạng thái mã hóa (`OK` hoặc `ERR`) |
