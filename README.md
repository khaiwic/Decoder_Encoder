# HỆ THỐNG TRUYỀN THÔNG TIN CƠ BẢN TRÊN 3 ESP32-S3 SỬ DỤNG SHT30

> **Tài liệu:** README dự án và phân công nhiệm vụ (phiên bản 4.0 — SHT30)  
> **Cập nhật:** **/**_/____
> **Môn học:** Lý thuyết thông tin  
> **Thành viên:** 03 sinh viên
> **Phần cứng:** 03 ESP32-S3, 01 cảm biến nhiệt độ/độ ẩm SHT30, cáp UART, máy tính để quan sát/log  
> **Mục tiêu:**   (1) Thực hiện tìm hiểu, khảo sát.
> (2) Tham khảo hoặc tự thiết kế, sử dụng các mô-đun ESP32, STM32 hoặc các mô-đun tương đương khác.
> (3) Phân tích và thực hiện hệ thống với tối thiểu gồm 3 khối: Máy phát - Kênh - Máy thu với các thao tác:

## 1. Mục tiêu và giới hạn dự án

### 1.1. Bài toán

Thiết kế và **hiện thực trên phần cứng** hệ thống truyền dữ liệu đo **nhiệt độ, độ ẩm từ SHT30** đi qua ba khối độc lập:

1. **Máy phát (TX):** đọc SHT30 qua I²C → gom mẫu → mã hóa nguồn Huffman → mã hóa kênh Hamming(7,4) dạng hệ thống → gửi UART.
2. **Kênh (CHANNEL):** nhận dữ liệu đã mã hóa → giả lập nhiễu bit theo mô hình BSC với xác suất `p` → chuyển tiếp UART.
3. **Máy thu (RX):** nhận dữ liệu → giải mã Hamming → giải mã Huffman → khôi phục mẫu → hiển thị, ghi log và tính chỉ số.

Cần trình diễn **ít nhất ba cấu hình** để chứng minh tác dụng của từng khối:

| Chế độ | Mã hóa nguồn | Mã hóa kênh | Mục tiêu so sánh |
| --- | --- | --- | --- |
| **A — RAW** | Không | Không | Đường cơ sở không nén/không bảo vệ |
| **B — HUFFMAN** | Huffman | Không | Hiệu quả và chi phí của nén nguồn |
| **C — HUFFMAN + HAMMING** | Huffman | Hamming(7,4) | Khả năng sửa lỗi và overhead bảo vệ dữ liệu |

**Không làm trong phạm vi bắt buộc:** camera, OpenCV, truyền ảnh, AI, ROS/micro-ROS, Wi-Fi, FPGA, giao diện đồ họa phức tạp. **RAW + Hamming** là thí nghiệm bổ sung nếu còn thời gian.

### 1.2. Tiêu chí học thuật

- Phân biệt **mã hóa nguồn** (giảm số bit trung bình) và **mã hóa kênh** (thêm dư thừa có cấu trúc để sửa lỗi).
- Chứng minh bằng số liệu thực rằng Huffman **có thể không giảm** dung lượng **toàn gói** trên bản tin ngắn do overhead của codebook và metadata.
- So sánh tỷ lệ lỗi trước và sau mã hóa kênh, độ trễ, tỷ lệ khôi phục thành công.
- Đối chiếu mô hình BSC và lý thuyết Hamming(7,4) với kết quả thu được trên ba ESP32-S3.

---

## 2. Kiến trúc hệ thống

```text
 NGƯỜI 1: TX / ESP32-S3 #1      NGƯỜI 2: CHANNEL / ESP32-S3 #2      NGƯỜI 3: RX / ESP32-S3 #3
 ┌───────────────────────┐      ┌──────────────────────────┐       ┌───────────────────────────┐
 │ SHT30 (T, RH)         │      │ UART nhận frame          │       │ UART nhận frame           │
 │       │ I²C            │      │          │               │       │           │               │
 │ Kiểm CRC-8 cảm biến   │      │ Xác định vùng payload    │       │ Frame parser              │
 │       │               │      │          │               │       │           │               │
 │ Lưu raw T16 + RH16    │      │ BSC(p): lật bit độc lập  │       │ Hamming decoder (mode C)  │
 │       │               │      │          │               │       │           │               │
 │ Buffer N mẫu          │      │ Ghi log số bit lật       │       │ Huffman decoder (B, C)    │
 │       │               │      │          │               │       │           │               │
 │ Huffman encode (B,C)  │      │ UART chuyển tiếp         │       │ Khôi phục raw T16 + RH16   │
 │       │               │      └──────────────────────────┘       │           │               │
 │ Hamming encode (C)    │                   │                     │ Tính T °C và RH %          │
 │       │               │                   │                     │ Kiểm CRC frame, log/metrics│
 │ Đóng frame + UART TX  │                   │                     └───────────────────────────┘
 └───────────────────────┘                   │
            │ UART                          │ UART
            └──────────────────────────────>└───────────────────────────────>

   Máy tính: cấu hình thí nghiệm / ghi log / tổng hợp CSV / biểu đồ (không thay ESP32 làm encode/decode).
```

**Nguyên tắc bắt buộc:** các thuật toán Huffman encoder, Hamming encoder phải chạy tại TX; các thuật toán giải mã tương ứng phải chạy tại RX. Máy tính chỉ hỗ trợ nạp bộ dữ liệu thử, ghi log, vẽ biểu đồ và hiển thị kết quả.

### 2.1. Kết nối phần cứng đề xuất

**SHT30 → ESP32-S3 TX:**

| Tín hiệu SHT30 | ESP32-S3 #1 | Ghi chú |
| --- | --- | --- |
| VCC / VIN | 3V3 | Cấp 3,3 V để tương thích mức logic ESP32-S3; kiểm tra đúng module đang dùng |
| GND | GND | Nối mass chung |
| SDA | GPIO8 (_đề xuất_) | Dữ liệu I²C; kiểm tra pin board và pull-up |
| SCL | GPIO9 (_đề xuất_) | Clock I²C; kiểm tra pin board và pull-up |

- Địa chỉ I²C 7-bit mặc định **`0x44`**, hoặc **`0x45`** khi chân ADDR được kéo lên mức cao. Một số breakout đã đặt ADDR cố định: cần quét I²C để xác minh.
- Nên bắt đầu I²C **100 kHz**; kiểm tra điện trở pull-up của module, tránh kéo SDA/SCL lên 5 V khi nối trực tiếp ESP32-S3.
- Đọc **single-shot**, ví dụ lệnh `0x2400` (high repeatability, **không** clock stretching). Chờ thời gian đo theo datasheet hoặc thăm dò sẵn sàng rồi mới đọc; không đọc ngay sau khi gửi lệnh.

**UART giữa ba ESP32-S3:**

```text
ESP32-S3 #1 (TX)       ESP32-S3 #2 (CHANNEL)      ESP32-S3 #3 (RX)
UART_TX  ────────────> UART_RX
GND      ────────────  GND
                        UART_TX  ────────────────> UART_RX
                        GND      ─────────────────  GND
```

Thống nhất UART phần cứng `115200, 8N1` để bắt đầu; pin UART có thể chọn phù hợp board, **không được lấy GPIO8/9 đang dùng I²C** trên TX. Nếu 3 board cùng được cấp nguồn từ USB, vẫn cần GND chung cho các đường UART. Không nối TX–TX hay RX–RX.

---

## 3. Đặc tả dữ liệu SHT30 — điểm thay đổi quan trọng so với DHT11

### 3.1. Dữ liệu I²C SHT30

Một kết quả đo single-shot SHT30 gồm **6 byte** theo thứ tự:

```text
T_MSB | T_LSB | CRC_T | RH_MSB | RH_LSB | CRC_RH
  2 byte nhiệt độ           2 byte độ ẩm
```

Mỗi CRC-8 bảo vệ **chính hai byte dữ liệu đứng trước nó**. Thông số CRC cảm biến theo Sensirion: polynomial `0x31`, giá trị khởi tạo `0xFF`, không phản chiếu bit, `xorout=0x00`.

**Cách xử lý trên TX:**

1. Đọc đủ 6 byte từ SHT30.
2. Xác minh `CRC_T` và `CRC_RH`.
3. Nếu CRC sai / I²C lỗi: tăng bộ đếm `sensor_read_error`, bỏ mẫu hoặc đánh dấu mẫu thiếu; **không** đưa mẫu sai vào bộ dữ liệu sạch.
4. Nếu CRC đúng: lưu `T_raw` và `RH_raw` nguyên dạng unsigned 16-bit.

> CRC-8 ở đây dùng kiểm tra đường I²C **cảm biến → TX**, **không phải** CRC của frame UART và cũng **không phải** mã sửa lỗi Hamming.

### 3.2. Quy ước payload truyền đi

**Mỗi mẫu hợp lệ = 4 byte** (`T_raw` 16-bit + `RH_raw` 16-bit), lưu theo **big-endian**:

```text
Sample #0: T_MSB, T_LSB, RH_MSB, RH_LSB
Sample #1: T_MSB, T_LSB, RH_MSB, RH_LSB
...
Sample #N-1
```

CRC-8 của cảm biến **được kiểm tại TX rồi loại khỏi payload ứng dụng**. Metadata như `sample_count`, `sequence_id`, kỳ lấy mẫu được đặt vào header, không tính là 4 byte/mẫu.

| Số mẫu hợp lệ N | Payload gốc | Dung lượng bit gốc |
| ---: | ---: | ---: |
| 1 | 4 byte | 32 bit |
| 64 | 256 byte | 2.048 bit |
| 128 | 512 byte | 4.096 bit |
| 256 | 1.024 byte | 8.192 bit |

**Lý do giữ 16-bit gốc:** tránh mất độ phân giải do làm tròn sớm; dễ đối chiếu chính xác từng byte giữa TX và RX. RX chỉ chuyển sang °C/%RH sau khi khôi phục, theo công thức:

\[
T\ (^{\circ}\mathrm{C})=-45+175\,\frac{T_{raw}}{65535}
\]

\[
RH\ (\%)=100\,\frac{RH_{raw}}{65535}
\]

**Không gửi số thực `float` trong payload**: cách đó dễ phát sinh sai khác biểu diễn và không cần thiết cho phép đo BER.

### 3.3. Chu kỳ đọc và dữ liệu phục vụ nghiên cứu

- Mặc định **1 mẫu/giây**, mỗi lô ưu tiên **N = 128 mẫu** (tương đương khoảng 128 giây ghi dữ liệu nếu đọc liên tục ở 1 Hz).
- TX lưu các lô mẫu đã đo thật để dùng lại khi chạy nhiều mức nhiễu, tránh phải chờ thu thập từ đầu mỗi phép thử.
- Chuẩn bị ít nhất **3 dataset**: (1) SHT30 đo thật trong điều kiện ổn định, (2) SHT30 đo thật có biến động môi trường, (3) dữ liệu tổng hợp để kiểm tra biên / thuật toán. Ghi rõ nguồn gốc từng dataset.
- Với `N=64,128,256`, kiểm tra hiệu quả nén **payload** và **toàn frame**. Dữ liệu SHT30 có độ phân giải cao, bit thấp có thể dao động nên **không có bảo đảm nén tốt hơn DHT11**.
- Nén Huffman theo byte 8-bit trên luồng `T_MSB,T_LSB,RH_MSB,RH_LSB`; nếu overhead lớn, phân tích và báo cáo. Việc thử mã hóa theo nhóm byte riêng hoặc làm tròn dữ liệu chỉ là mở rộng, không bắt buộc.

---

## 4. Ba chế độ truyền và thuật toán

### A. RAW

Đọc `N` mẫu → serial hóa đúng `4N` byte → truyền không nén/không mã hóa kênh. Chế độ này đo đường cơ sở: độ trễ và độ nhạy với nhiễu.

### B. HUFFMAN

- Tạo bảng tần suất byte (0–255), xây mã Huffman từ dataset của frame hoặc block.
- Xuất bitstream nén; lưu **số bit hợp lệ** để phân biệt padding ở byte cuối.
- Phải chuyển đủ dữ liệu tái tạo codebook cho RX (gợi ý: **canonical Huffman**, header chứa cặp `(symbol, code_length)` của các symbol có mặt; hoặc cơ chế đã thống nhất khác).
- Tính cả **codebook + header + CRC + padding** khi báo cáo tỷ lệ nén thực tế; với dataset ngắn hoàn toàn có thể nén âm.
- Khi bitstream bị lỗi, Huffman có thể giải mã thiếu/nhầm nhiều symbol; hệ thống phải kiểm tra độ dài và từ chối frame lỗi thay vì in ra dữ liệu sai như dữ liệu đúng.

### C. HUFFMAN + HAMMING(7,4)

- Lấy bitstream Huffman và chia thành nhóm **4 bit**; nếu chưa đủ thì pad cuối.
- Với mỗi nhóm, tạo codeword **7 bit** theo một quy ước Hamming(7,4) **dạng hệ thống** đã chốt: vị trí 4 bit dữ liệu và 3 bit parity; cách pack các codeword vào byte truyền.
- RX giải mã Hamming trước rồi mới giải Huffman; sau cùng cắt về độ dài bitstream Huffman hợp lệ.
- Hamming(7,4) **bảo đảm sửa đúng 1 bit lỗi mỗi codeword**, **không bảo đảm phát hiện/sửa đúng ≥2 lỗi**. Khi nhiễu mạnh có thể có lỗi sửa nhầm; CRC payload gốc và phép so sánh golden dataset giúp phát hiện kết quả cuối sai.

Tỷ lệ mã kênh:

\[
R=\frac{4}{7}\approx 0.5714,
\quad \text{bit mã hóa tăng khoảng }75\%\text{ so với đầu vào Hamming (chưa tính padding).}
\]

Với kênh BSC có xác suất bit lỗi độc lập `p`, xác suất **một codeword có từ 2 lỗi trở lên** là:

\[
P_{\ge 2}=1-(1-p)^7-7p(1-p)^6.
\]

Đây là **xác suất vượt quá khả năng bảo đảm sửa lỗi đơn**, **không đồng nhất** với BER sau giải mã.

---

## 5. Phân công nhiệm vụ cụ thể

### 5.1. Thành viên 1 — TX: SHT30 + Huffman Encoder + Hamming Encoder

**Phụ trách:** ESP32-S3 #1 và SHT30.

**Công việc phải làm:**

1. Đấu nối SHT30 I²C; xác định địa chỉ `0x44/0x45` và đọc sensor thành công.
2. Triển khai driver SHT30: phát lệnh đo, đọc 6 byte, kiểm CRC-8, xử lý timeout/NACK, thống kê lỗi.
3. Chuyển 6 byte I²C thành payload ứng dụng **4 byte/mẫu** (`uint16_t` raw T và RH), gom **N** mẫu; có khả năng phát lại dataset cố định.
4. Xây Huffman encoder và định dạng metadata/codebook RX có thể giải mã, thống kê entropy/chi phí mã.
5. Xây Hamming(7,4) encoder dạng hệ thống, kèm test vector.
6. Tạo ba chế độ A/B/C, đóng frame thống nhất, gửi UART đến CHANNEL.
7. Log: số mẫu hợp lệ/bị loại, tổng byte trước/sau nén, codebook size, bit Hamming, thời gian encode, sequence ID, CRC dữ liệu gốc.

**Sản phẩm bàn giao:** `sht30_driver`, `sample_buffer`, `huffman_encoder`, `hamming74_encoder`, `frame_tx`, dữ liệu golden TX, tài liệu API và unit tests.

**Nghiệm thu cá nhân:** TX phát được bộ mẫu cố định ở cả ba chế độ, file golden lưu đủ để RX so sánh; khi `p=0`, RX tham chiếu khôi phục đúng 100% bytes.

### 5.2. Thành viên 2 — CHANNEL: UART Bridge + BSC + Log thực nghiệm

**Phụ trách:** ESP32-S3 #2.

**Công việc phải làm:**

1. Nhận UART từ TX, kiểm tra cấu trúc frame và chuyển tiếp UART sang RX.
2. Thêm bộ giả lập nhiễu **BSC(p)**: mỗi bit trên **coded payload** hoặc RAW payload (tùy mode) được lật độc lập với xác suất `p`; **không** cố ý lật framing/header/codebook trong thử nghiệm chính.
3. Cung cấp cấu hình `p = 0%, 0.1%, 0.5%, 1%, 2%, 5%` và fixed PRNG seed để tái lập kết quả.
4. Ghi số bit chịu nhiễu, số bit bị lật, seed, `mode`, `sequence_id`, lỗi framing/UART; lưu log định dạng máy đọc được.
5. Phụ trách chuẩn hóa **giao thức chung**, timeout, framing, phân định vùng tin cậy và kịch bản thử nghiệm nhiều `p`.
6. Duy trì script tập hợp CSV và hỗ trợ chạy thử tập dataset lặp lại.

**Sản phẩm bàn giao:** `uart_bridge`, `bsc_channel`, `frame_parser`, `channel_config`, `logs`, `scripts/collect_logs.py`, đặc tả protocol chung đã được cả nhóm duyệt.

**Nghiệm thu cá nhân:** `p=0` chuyển tiếp chính xác, `p>0` làm biến đổi payload với tỷ lệ thống kê tiệm cận `p` khi số bit thí nghiệm đủ lớn; header không bị phá khiến mất đồng bộ ngoài ý muốn.

### 5.3. Thành viên 3 — RX: Hamming Decoder + Huffman Decoder + Metrics

**Phụ trách:** ESP32-S3 #3.

**Công việc phải làm:**

1. Nhận UART và parse frame đúng định dạng chung.
2. Giải mã Hamming(7,4) bằng syndrome (mode C); thống kê số codeword có syndrome khác 0 và thao tác sửa; **không** tuyên bố mọi sửa lỗi đều đúng.
3. Giải Huffman (mode B/C), kiểm số byte đầu ra mong đợi, codebook, độ dài bitstream, padding; xử lý dữ liệu hỏng an toàn.
4. Tái tạo cặp giá trị raw `T16, RH16` và hiển thị các số đo °C/%RH theo công thức SHT30.
5. Kiểm CRC dữ liệu gốc của TX, phân loại frame `OK`, `CRC_FAIL`, `DECODE_FAIL`, `TIMEOUT` ...; không coi frame không khôi phục được là dữ liệu đúng.
6. Tính và tổng hợp BER kênh, BER đầu cuối (khi có dataset TX tham chiếu và căn chỉnh hợp lệ), tỷ lệ frame thành công, throughput và latency.
7. Xuất CSV/đồ thị và viết phần phân tích, so sánh ba mode trong báo cáo.

**Sản phẩm bàn giao:** `hamming74_decoder`, `huffman_decoder`, `frame_rx`, `metrics`, `crc_check`, script vẽ đồ thị và dữ liệu nghiệm thu.

**Nghiệm thu cá nhân:** `p=0` khôi phục đúng 100% trên các dataset hợp lệ; khi có nhiễu, không treo, không mất đồng bộ kéo dài, có số liệu frame thành công/thất bại và hiển thị nhiệt độ/độ ẩm đúng đơn vị.

> **Cân bằng khối lượng:** Người 2 phụ trách thêm giao thức và công cụ kiểm thử/log. Người 1 và Người 3 đảm nhiệm cặp encode/decode tương ứng. Cả ba cùng duyệt protocol và hỗ trợ kiểm thử tích hợp.

---

## 6. Hợp đồng giao tiếp thống nhất trước khi viết code

**Bắt buộc** tạo tài liệu `docs/protocol.md` và chốt những mục dưới đây trong tuần đầu:

| Thành phần | Đề xuất ban đầu | Ghi chú |
| --- | --- | --- |
| UART | `115200, 8N1` | Thống nhất GPIO và timeout từng liên kết |
| Header | `magic`, `version`, `mode`, `sequence_id`, `sample_count`, `payload_length` | Field width và endian phải được chốt |
| Bit lengths | `huffman_valid_bits`, `coded_valid_bits` | Tránh hiểu nhầm padding ở mỗi tầng |
| Huffman dictionary | `used_symbol_count` + `(symbol, code_length)` | Tối ưu độ dài header theo số symbol xuất hiện |
| Kiểm header | Header CRC, ví dụ CRC-16 | Phải được kiểm tra trước khi đọc độ dài payload |
| Kiểm dữ liệu gốc | CRC-32 của `4N` byte raw từ TX | RX tính lại sau giải mã để phân loại đúng/sai |
| Vùng gây nhiễu | Chỉ **payload đã mã hóa** | Giữ header/codebook/CRC gốc ở vùng tin cậy trong thí nghiệm chính |
| Dữ liệu ứng dụng | `T_raw16` + `RH_raw16` big-endian | 4 byte/mẫu; không truyền `float` |
| Bit order | Quy định rõ MSB-first hoặc LSB-first | Áp dụng nhất quán từ Huffman tới Hamming/UART |
| Lỗi | `OK`, `CRC_FAIL`, `DECODE_FAIL`, `FORMAT_FAIL`, `TIMEOUT` | Log theo `sequence_id`, không tự suy diễn BER khi mất căn chỉnh |

**Quan trọng về thực nghiệm:** Header và codebook **không bị chèn nhiễu nhân tạo** để tránh lỗi framing che lấp tác dụng của thuật toán. Cần ghi rõ đây là **mô hình kênh thí nghiệm có vùng điều khiển tin cậy**; không mô tả sai rằng mọi bit trên đường UART đều chịu cùng xác suất `p`. Nếu muốn thử nhiễu trên toàn frame, đó là kịch bản nâng cao với framing/metadata được bảo vệ tương ứng.

**Bảo đảm thống nhất độ dài:**

```text
SHT30 đo thành công
   ↓
N mẫu × 4 byte raw  — tính CRC-32 trên dữ liệu gốc
   ↓
Mode A: giữ nguyên       Mode B: Huffman       Mode C: Huffman → Hamming
   ↓
Đóng frame: trusted header + [codebook nếu cần] + encoded payload
   ↓  UART
CHANNEL: lật bit trong encoded payload theo BSC(p)
   ↓  UART
RX: parse → Hamming decode (nếu C) → Huffman decode (nếu B/C)
   ↓
Kiểm số byte = 4N, CRC-32 và so với golden dataset theo sequence_id
   ↓
Chuyển raw16 sang °C/%RH; hiển thị và log kết quả
```

**Để đo BER đúng:**

- **BER kênh** = số bit thay đổi tại CHANNEL / số bit chịu nhiễu, dựa trên so sánh bitstream **trước và sau CHANNEL**; CHANNEL biết chính xác số bit đã lật.
- **BER đầu cuối** = số bit khác nhau giữa raw payload ở TX và raw payload đã khôi phục ở RX / tổng số bit raw gốc, **chỉ tính khi có dữ liệu căn chỉnh và độ dài hợp lệ**. Với frame giải mã thất bại, ghi `DECODE_FAIL`/`CRC_FAIL` và tỷ lệ frame fail riêng, **không tự gán BER = 0**.
- BER đầu cuối khi RX thu được đủ byte nhưng CRC sai có thể tính từ golden dataset trên PC để xác định số bit sai; CRC một mình không cho biết số bit lỗi.

---

## 7. Phương pháp thử nghiệm và các chỉ số

### 7.1. Tập thí nghiệm tối thiểu

1. **Driver:** kiểm `0x44/0x45`, CRC cảm biến, đọc SHT30 ở 1 Hz; so sánh T/RH hiển thị.
2. **Thuật toán thuần:** Huffman round-trip và Hamming round-trip, single-bit error correction, nhiều-bit-error behavior; test vector xác định.
3. **Tích hợp không nhiễu:** cả A/B/C truyền **cùng** `N` mẫu; `p=0` khôi phục đúng toàn bộ raw payload.
4. **Tích hợp có nhiễu:** thử `p=0%;0.1%;0.5%;1%;2%;5%`, cùng dataset cho ba chế độ, nhiều frame/seed cho mỗi tổ hợp.
5. **Số mẫu:** thử `N=64,128,256` (phát lại dataset đã thu) để đánh giá ảnh hưởng header/codebook; lượng bit thử phải đủ lớn để kết luận có ý nghĩa.
6. **Tình huống lỗi:** CRC SHT30 sai, I²C NACK, frame UART thiếu byte, Huffman stream hỏng, frame CRC fail; máy thu không treo và có thông báo rõ ràng.

### 7.2. Các metric phải có

| Metric | Định nghĩa và cách diễn giải |
| --- | --- |
| Entropy `H(X)` | Tính từ phân bố byte trong raw payload; so với độ dài mã Huffman trung bình |
| Payload compression ratio | `raw_bits / huffman_valid_bits`; **chỉ** đánh giá payload |
| Net compression ratio | `raw_frame_total_bits / huffman_frame_total_bits` hoặc thống kê số byte truyền thực tế trong UART, nói rõ mẫu số |
| Hamming code rate | `R = 4/7`; thêm khoảng 75% bit so với dòng vào encoder (trừ ảnh hưởng padding) |
| BER kênh | `flipped_bits / disturbed_bits` của payload qua CHANNEL |
| BER đầu cuối | Số raw bits sai khi đầu ra căn chỉnh hợp lệ / tổng raw bits tham chiếu |
| Frame success rate | `frame_crc_ok / total_frames_sent`, ghi riêng frame timeout/không giải mã được |
| Throughput hữu ích | Số bit raw **khôi phục đúng** / tổng thời gian truyền, nói rõ thời điểm bắt đầu/kết thúc |
| Latency | Tách `sensor acquisition`, `encoding`, `UART/CHANNEL`, `decoding`; thời gian đợi lấy 128 mẫu **không** trộn với độ trễ truyền |
| Overhead | Header + codebook + CRC + padding + Hamming parity; báo theo byte/bit |

Với Huffman nhị phân tối ưu theo symbol (trường hợp thông thường):

\[
H(X) \le \bar L < H(X)+1
\]

Nên lập bảng thống kê **theo cùng dataset** và đồ thị: (i) dung lượng toàn frame theo mode; (ii) BER đầu cuối / frame-success theo `p`; (iii) latency theo mode; (iv) tỷ lệ nén theo `N`.

> **Tránh kết luận sai:** (1) SHT30 chính xác hơn DHT11 không có nghĩa Huffman chắc chắn nén tốt hơn; (2) Hamming giảm xác suất lỗi dữ liệu nhưng tăng lượng bit truyền; (3) BER kênh của chế độ C vẫn xấp xỉ `p` vì lỗi được chèn **sau** encoder — lợi ích Hamming xuất hiện **sau decoder**.

---

## 8. Kế hoạch theo tuần đến cuối tháng 11/2026

| Giai đoạn | Người 1 — TX | Người 2 — CHANNEL | Người 3 — RX | Mốc bàn giao nhóm |
| --- | --- | --- | --- | --- |
| **27/10–02/11** | Đọc SHT30 + CRC-8; buffer raw16; Huffman/Hamming unit test | Chốt protocol UART/frame/BSC + parser tối thiểu | Test vector decoder; giao diện đọc raw16 | **Chốt giao thức & golden dataset** |
| **03/11–09/11** | Frame A/B/C, UART TX, log kích thước | Forward frame `p=0`, fixed seed config | RX A/B/C và CRC payload | **Round-trip đúng 100% khi p=0** |
| **10/11–16/11** | Tối ưu lỗi TX, replay dataset | Tích hợp BSC(p), log bit flip và thống kê | Giải mã khi có lỗi; classify frame fail | **Demo đủ 3 board và điều chỉnh p** |
| **17/11–23/11** | Entropy, net ratio, N biến đổi | Chạy batch thử nghiệm/CSV, cố định seed | BER/frame success, vẽ đồ thị | **Có bộ số liệu, công thức, đồ thị** |
| **24/11–28/11** | Hoàn thiện README phần TX, wiring | Chuẩn hóa log, kịch bản demo | Báo cáo, đồ thị, xử lý case xấu | **Đóng băng firmware & tổng duyệt** |
| **Cuối tháng 11** | Thuyết trình / demo | Thuyết trình / demo | Thuyết trình / demo | **Nộp theo lịch giảng viên** |

**Ưu tiên khi thiếu thời gian:** sensor + CRC → round-trip A/B/C tại `p=0` → BSC và BER → log/đồ thị → tối ưu. Không bổ sung thuật toán hoặc phần cứng mới trước khi đường truyền cơ bản hoạt động ổn định.

---

## 9. Cấu trúc thư mục Git đề xuất

```text
communication-system-sht30/
├── README.md
├── docs/
│   ├── requirements.md           # Diễn giải yêu cầu bài tập lớn
│   ├── protocol.md               # FRAME, UART, bit order, CRC, error codes
│   ├── wiring.md                 # Sơ đồ nối SHT30 + UART 3 ESP32-S3
│   ├── test_plan.md              # Test cases và cách lấy mẫu thống kê
│   └── report/
├── firmware/
│   ├── tx_sht30/
│   │   ├── sht30_driver/
│   │   ├── sample_buffer/
│   │   └── tx_app/
│   ├── channel_bsc/
│   └── rx_decoder/
├── common/
│   ├── frame_format/
│   ├── huffman/
│   ├── hamming74/
│   └── crc/
├── tests/
│   ├── golden_vectors/
│   ├── unit/
│   └── integration/
├── data/
│   ├── sht30_real/               # Dữ liệu do SHT30 đo thật
│   └── synthetic/                # Dữ liệu kiểm thử tổng hợp
├── scripts/
│   ├── collect_logs.py
│   ├── analyze_metrics.py
│   └── plot_results.py
└── results/
    ├── raw_logs/
    ├── metrics/
    └── figures/
```

**Môi trường lập trình:** chốt **một framework chung** (ESP-IDF hoặc Zephyr) cho cả nhóm, tránh mất thời gian tích hợp. Ưu tiên tận dụng môi trường và driver mà nhóm đã quen.

---

## 10. Checklist nghiệm thu hướng tới demo xuất sắc

### Bắt buộc theo đề bài

- [ ] Có **3 ESP32-S3** hoạt động độc lập, nối đúng UART TX → CHANNEL → RX.
- [ ] Có **SHT30 thật** đo nhiệt độ/độ ẩm, đọc qua I²C và xác minh CRC-8.
- [ ] TX triển khai **Huffman encoder** và **Hamming(7,4) encoder dạng hệ thống** trên vi điều khiển.
- [ ] CHANNEL triển khai BSC với xác suất `p`, chuyển tiếp giữa hai UART.
- [ ] RX triển khai giải mã Hamming, giải mã Huffman, khôi phục đầy đủ cặp `T_raw/RH_raw`.
- [ ] `p=0` giải mã đúng 100% dữ liệu hợp lệ; có log, báo cáo và demo.

### Để thể hiện năng lực nổi bật

- [ ] Có A/B/C và dùng **cùng dataset** khi so sánh.
- [ ] Có nhiều mức `p`, seed/cỡ mẫu và kết quả thống kê đủ rõ.
- [ ] Có entropy, độ dài mã Huffman trung bình, net compression ratio.
- [ ] Có BER trước/sau giải mã, frame-success, throughput, latency và overhead.
- [ ] Có phát hiện frame lỗi (CRC), xử lý giải mã thất bại và không treo.
- [ ] Có ít nhất ba đồ thị/bảng thể hiện lý thuyết và thực nghiệm.
- [ ] Có wiring diagram, mô tả frame, test vectors, README chạy thử, ảnh/video demo dự phòng.
- [ ] Cả **3 thành viên** đều giải thích được chuỗi TX–CHANNEL–RX và trách nhiệm của mình.

---

## 11. Kịch bản demo 5–8 phút

1. **Giới thiệu phần cứng:** chỉ rõ SHT30 + I²C tại TX; hai đường UART; vai trò của ba ESP32-S3.
2. **Đo thật:** đọc SHT30, CRC cảm biến hợp lệ; hiển thị số T/RH. Chuyển sang phát lại dataset đo sẵn để thử nghiệm nhanh và công bằng.
3. **Không nhiễu (`p=0`):** chạy A/B/C; RX khôi phục đúng `T_raw/RH_raw` và in ra °C/%RH.
4. **So sánh mã hóa:** minh họa byte gốc, bit Huffman, số bit sau Hamming, net bytes truyền. Giải thích trường hợp Huffman không nén ròng.
5. **Tăng nhiễu:** `p` điều chỉnh, cho xem lỗi kênh, số frame sửa/không sửa được; so sánh RAW, HUFFMAN, HUFFMAN+HAMMING.
6. **Báo cáo khoa học:** trình bày entropy, BER, frame-success, thời gian truyền và đối chiếu công thức Hamming với thống kê đo được.

**Kết luận hướng tới giảng viên:** đây không chỉ là demo đọc cảm biến rồi gửi UART; nhóm hiện thực và **đo kiểm được** các khâu mã hóa nguồn, mã hóa kênh, tác động của nhiễu và khả năng khôi phục thông tin trên ba vi điều khiển riêng biệt.

---

## 12. Tham khảo kỹ thuật và quyết định thiết kế

- **Sensirion — Datasheet SHT3x-DIS, Rev. 7 (12/2022):** <https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf>  
  Nguồn tra cứu về SHT30, giao thức I²C, lệnh đo, 6 byte đầu ra, CRC-8, địa chỉ `0x44/0x45` và công thức chuyển đơn vị.
- **Tài liệu ESP-IDF I²C/UART:** <https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2c.html> và <https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/uart.html>
- Các tham số **N=128**, **1 Hz**, **115200 baud**, **GPIO8/9**, cách tổ chức frame và danh sách mức `p` là **lựa chọn dự án đề xuất**, không phải thông số cứng bắt buộc của SHT30.

### Thay đổi so với bản DHT11

- `DHT11` → **`SHT30`**, đọc bằng **I²C** thay cho giao thức một dây chuyên biệt.
- Checksum DHT11 → **hai CRC-8 độc lập** trên 2 giá trị raw16 của SHT30.
- Payload đơn giản trước đây → **4 byte/mẫu**, gồm `T_raw16` và `RH_raw16`, tránh làm tròn.
- Lấy mẫu chậm 2 giây → **1 Hz đề xuất**, có dataset replay để không phải chờ thu dữ liệu mới.
- Các phần Huffman, Hamming(7,4), BSC, UART giữa ba ESP32-S3, phân công ba người, phép đo/đồ thị và lịch làm việc **giữ nguyên mục tiêu**.

**Nguyên tắc thực hiện:** hoàn thành bản chạy chắc chắn trước, dành thời gian cho kiểm thử có thể tái lập và báo cáo có số liệu thay vì mở rộng công nghệ ngoài đề bài.
