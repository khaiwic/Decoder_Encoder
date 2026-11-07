# TÀI LIỆU TOÀN DIỆN VỀ CRC VÀ CRC-8 (CYCLIC REDUNDANCY CHECK 8-BIT)

> **Dự án:** Hệ thống truyền thông tin trên 3 ESP32-S3 sử dụng cảm biến SHT30  
> **Ứng dụng chính:** Kiểm tra toàn vẹn dữ liệu I²C từ SHT30 tới ESP32-S3 (TX)  
> **Tài liệu tham khảo code:** [sht30_driver.c](file:///c:/Users/ADMIN/Documents/Decoder_and_Encoder/firmware/encoder/include/sht30_driver/sht30_driver.c), [sht30_driver.h](file:///c:/Users/ADMIN/Documents/Decoder_and_Encoder/firmware/encoder/include/sht30_driver/sht30_driver.h)

---

## 1. CRC là gì? Tổng quan về CRC-8

### 1.1. Khái niệm
**CRC** (*Cyclic Redundancy Check* - Mã kiểm tra dư thừa vòng) là thuật toán mã hóa phát hiện lỗi (Error-Detecting Code) được sử dụng phổ biến nhất trong các giao thức truyền thông số (I²C, SPI, CAN, Ethernet, USB) và lưu trữ dữ liệu (HDD, flash).

Bản chất của CRC là sinh ra một chuỗi bit có độ dài cố định (gọi là giá trị CRC, Checksum hay Thẻ kiểm tra) dựa trên nội dung của khối dữ liệu cần truyền. Chuỗi này được đính kèm vào cuối bản tin. Khi phía nhận đọc được bản tin, nó tính lại mã CRC từ dữ liệu nhận được và so sánh với mã CRC gửi kèm:
* **Khớp nhau:** Khả năng rất cao dữ liệu toàn vẹn (không bị nhiễu bit làm hỏng).
* **Khác nhau:** Chắc chắn dữ liệu đã bị biến dạng trong quá trình truyền $\rightarrow$ hủy gói hoặc yêu cầu đọc lại.

### 1.2. Ý nghĩa của CRC-8
* Số **8** biểu thị kích thước của mã kiểm tra là **8-bit (đúng 1 byte)**.
* Đầu vào: Chuỗi dữ liệu có chiều dài bất kỳ (ví dụ: 1 byte, 2 byte, 100 byte...).
* Đầu ra: Luôn luôn là **1 byte (8 bit)** duy nhất nằm trong khoảng `0x00` đến `0xFF`.
* Overhead (chi phí băng thông): Cực kỳ nhỏ (chỉ thêm đúng 1 byte bảo vệ cho toàn bộ gói dữ liệu).

---

## 2. Cơ sở toán học của thuật toán CRC

### 2.1. Số học trên trường nhị phân $GF(2)$
Thuật toán CRC hoạt động trên trường hữu hạn hai phần tử Galois Field $GF(2)$:
1. Các hệ số chỉ có thể là `0` hoặc `1`.
2. Phép **cộng** và phép **trừ** trong $GF(2)$ là tương đương nhau và hoàn toàn trùng khớp với **phép toán logic XOR ($\oplus$)**:
   $$\begin{aligned}
   0 \oplus 0 &= 0 \\
   0 \oplus 1 &= 1 \\
   1 \oplus 0 &= 1 \\
   1 \oplus 1 &= 0 \quad (\text{không nhớ})
   \end{aligned}$$
3. Phép nhân tương đương với phép logic AND và phép dịch bit.
4. **Phép chia đa thức:** Chia nhị phân không mượn (modulo-2 division), trong đó phép trừ từng bước được thay thế hoàn toàn bằng phép XOR.

### 2.2. Biểu diễn dữ liệu dưới dạng đa thức
Mỗi chuỗi bit được xem như các hệ số của một đa thức theo biến $x$:
* Chuỗi 8 bit `10010001` tương ứng với:
  $$M(x) = 1 \cdot x^7 + 0 \cdot x^6 + 0 \cdot x^5 + 1 \cdot x^4 + 0 \cdot x^3 + 0 \cdot x^2 + 0 \cdot x^1 + 1 \cdot x^0 = x^7 + x^4 + 1$$

### 2.3. Đa thức sinh (Generator Polynomial - $G(x)$)
Mỗi chuẩn CRC được xác định bởi một **Đa thức sinh $G(x)$**.
Đối với **CRC-8**, đa thức sinh luôn có bậc là 8:
$$G(x) = x^8 + g_7 x^7 + g_6 x^6 + \dots + g_1 x + 1$$

* Bậc của $G(x)$ là 8, nghĩa là biểu diễn nhị phân của nó gồm **9 bit** (bit cao nhất $x^8$ luôn là 1).
* Khi biểu diễn dưới dạng số hex (thường viết là `Poly`), người ta **bỏ qua bit cao nhất $x^8$** (vì bit này luôn bằng 1), chỉ lấy 8 bit thấp ($g_7 \dots g_0$).
  > Ví dụ đa thức $x^8 + x^5 + x^4 + 1$:  
  > Dạng nhị phân 9-bit đầy đủ: `1 0011 0001`  
  > Bỏ bit $x^8$, lấy 8 bit thấp: `0011 0001` = **`0x31`**.

### 2.4. Nguyên lý tính phần dư (Remainder)
Cho khối dữ liệu cần bảo vệ là đa thức $M(x)$ có bậc $k$:
1. Dịch chuyển dữ liệu $M(x)$ sang trái 8 bit (tương đương nhân với $x^8$, thêm 8 số 0 vào cuối chuỗi bit):
   $$M(x) \cdot x^8$$
2. Thực hiện phép chia đa thức modulo-2 cho $G(x)$:
   $$\frac{M(x) \cdot x^8}{G(x)} = Q(x) \oplus \frac{R(x)}{G(x)}$$
   Trong đó:
   * $Q(x)$ là thương (bị loại bỏ, không quan tâm).
   * $R(x)$ là **phần dư (Remainder)** có bậc $\le 7$ (chính là chuỗi 8-bit CRC cần tìm).
3. Bản tin truyền đi bao gồm cả dữ liệu và mã CRC:
   $$T(x) = (M(x) \cdot x^8) \oplus R(x)$$

Khi bên nhận nhận được $T(x)$, nếu đem chia $T(x)$ cho $G(x)$:
$$\frac{T(x)}{G(x)} = \frac{M(x) \cdot x^8 \oplus R(x)}{G(x)} = Q(x) \oplus \frac{R(x) \oplus R(x)}{G(x)} = Q(x)$$
Phần dư thu được chắc chắn sẽ bằng **0** nếu không có bất kỳ bit nào bị lật (bị lỗi).

---

## 3. Các tham số cấu hình một chuẩn CRC (Mô hình tham số Rocksoft)

Không phải thuật toán CRC-8 nào cũng cho ra kết quả giống nhau. Một biến thể CRC-8 được định nghĩa chính xác bởi 6 tham số:

| Tham số | Ý nghĩa | Giải thích |
| :--- | :--- | :--- |
| **Width** | Độ rộng bit | Với CRC-8 thì `Width = 8`. |
| **Poly** | Đa thức sinh | Giá trị Hex của 8 bit thấp của $G(x)$ (ví dụ: `0x31`, `0x07`, `0xD5`). |
| **Init** | Giá trị khởi tạo | Giá trị nạp sẵn vào thanh ghi trước khi tính (thường là `0x00` hoặc `0xFF`). `Init = 0xFF` giúp phát hiện lỗi nếu bản tin bị chèn thêm các byte `0x00` ở đầu. |
| **RefIn** | Đảo ngược bit đầu vào | `False`: Xử lý MSB (bit trọng số lớn) trước. `True`: Đảo bit của từng byte (LSB trước). |
| **RefOut** | Đảo ngược bit đầu ra | `False`: Giữ nguyên MSB-first. `True`: Đảo ngược toàn bộ 8 bit kết quả trước khi XOR final. |
| **XorOut** | Giá trị XOR cuối | Giá trị XOR với kết quả trước khi trả về (thường là `0x00` hoặc `0xFF`). |

---

## 4. Trường hợp thực tế trong dự án: Chuẩn CRC-8 trên cảm biến Sensirion SHT30

Trong dự án của chúng ta (giao tiếp giữa SHT30 và vi điều khiển ESP32-S3 TX), cảm biến SHT30 sử dụng chuẩn CRC-8 của hãng **Sensirion**.

### 4.1. Bảng tham số chuẩn của SHT30
Theo datasheet chính thức của Sensirion SHT3x-DIS:

* **Đa thức sinh:** $P(x) = x^8 + x^5 + x^4 + 1$
* **Poly Hex:** `0x31` (nhị phân: `0011 0001`)
* **Initialization (Init):** `0xFF`
* **RefIn (Reflect In):** `False` (truyền MSB trước)
* **RefOut (Reflect Out):** `False` (kết quả MSB trước)
* **XorOut:** `0x00`

### 4.2. Khung truyền dữ liệu I²C của SHT30
Khi ESP32-S3 gửi lệnh đọc dữ liệu nhiệt độ và độ ẩm (ví dụ lệnh `0x2400`), SHT30 trả về đúng **6 bytes** liên tiếp:

```text
┌──────────────┬──────────────┬──────────────┬──────────────┬──────────────┬──────────────┐
│  Byte 0      │  Byte 1      │  Byte 2      │  Byte 3      │  Byte 4      │  Byte 5      │
├──────────────┼──────────────┼──────────────┼──────────────┼──────────────┼──────────────┤
│  T_MSB       │  T_LSB       │  CRC_T       │  RH_MSB      │  RH_LSB      │  CRC_RH      │
└──────────────┴──────────────┴──────────────┴──────────────┴──────────────┴──────────────┘
 └─── 2 byte Nhiệt độ ───┘   └── CRC-8 ──┘    └─── 2 byte Độ ẩm ────┘    └── CRC-8 ──┘
```

> [!IMPORTANT]
> **Đặc điểm sống còn cần lưu ý:**
> 1. Mỗi mã CRC-8 **chỉ bảo vệ đúng 2 byte dữ liệu đứng liền trước nó**:
>    * `CRC_T` (Byte 2) được tính riêng cho 2 byte: `[T_MSB, T_LSB]`.
>    * `CRC_RH` (Byte 5) được tính riêng cho 2 byte: `[RH_MSB, RH_LSB]`.
> 2. Đây **không phải** là CRC gộp chung cho cả 4 byte hay 6 byte.

---

## 5. Phân tích chi tiết mã nguồn: Thuật toán Bit-by-bit trong `sht30_driver.c`

Đây là hàm tính toán CRC-8 đang được triển khai trong dự án:

```c
static uint8_t sht30_compute_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFFU; /* 1. Khởi tạo thanh ghi với giá trị Init = 0xFF */
    
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];   /* 2. XOR byte dữ liệu tiếp theo vào thanh ghi CRC */
        
        for (uint8_t bit = 0; bit < 8U; bit++) { /* 3. Xử lý từng bit từ MSB đến LSB */
            if ((crc & 0x80U) != 0U) {           /* Nếu bit cao nhất (bit 7) là 1 */
                crc = (uint8_t)((crc << 1U) ^ 0x31U); /* Dịch trái 1 bit và XOR với Poly 0x31 */
            } else {                             /* Nếu bit cao nhất là 0 */
                crc <<= 1U;                      /* Chỉ dịch trái 1 bit */
            }
        }
    }
    return crc; /* 4. Trả về kết quả (XorOut = 0x00 nên không đổi) */
}
```

### Giải thích cơ chế từng bước hoạt động:
1. `uint8_t crc = 0xFFU;`: Nạp giá trị khởi tạo. Biến `crc` đóng vai trò như một thanh ghi dịch tuyến tính (LFSR - Linear Feedback Shift Register) 8-bit.
2. `crc ^= data[i];`: Do tính chất của phép chia modulo-2 trên $GF(2)$, việc cộng XOR dữ liệu vào thanh ghi dư thừa trước khi dịch tương đương với việc dịch từng bit dữ liệu vào qua phép chia.
3. Vòng lặp `for (uint8_t bit = 0; bit < 8U; bit++)`:
   * `(crc & 0x80U) != 0U`: Kiểm tra bit có trọng số lớn nhất (MSB - bit 7).
   * **Nếu MSB = 1:** Đa thức chia hết cho $x^8$ ở bước này $\rightarrow$ cần thực hiện phép trừ modulo-2 (tức là phép XOR) với đa thức chia `0x31`. Trước tiên dịch trái `crc << 1` (để loại bỏ bit 7 vừa chia), sau đó XOR với `0x31`.
   * **Nếu MSB = 0:** Đa thức không chia hết $\rightarrow$ chỉ cần dịch trái `crc << 1` để xét bit tiếp theo.
4. Lặp lại cho đến khi hết tất cả các byte dữ liệu trong buffer (với SHT30 là `len = 2`).

---

## 6. Ví dụ tính toán bằng tay (Tracer ví dụ thực tế)

Giả sử SHT30 đo được nhiệt độ trả về 2 byte:
* Byte 0: `0xBE` (`1011 1110` nhị phân)
* Byte 1: `0xEF` (`1110 1111` nhị phân)

### Bước 1: Khởi tạo
* `crc = 0xFF` (`1111 1111`)

### Bước 2: Nạp Byte 0 (`0xBE`)
* `crc = crc ^ 0xBE = 1111 1111 ^ 1011 1110 = 0100 0001` (`0x41`)
* Xử lý 8 bit của `0x41` (`0100 0001`):
  * **Bit 0:** MSB = 0 $\rightarrow$ Shift: `crc = 1000 0010` (`0x82`)
  * **Bit 1:** MSB = 1 $\rightarrow$ Shift: `0000 0100`, XOR `0x31` (`0011 0001`): `crc = 0011 0101` (`0x35`)
  * **Bit 2:** MSB = 0 $\rightarrow$ Shift: `crc = 0110 1010` (`0x6A`)
  * **Bit 3:** MSB = 0 $\rightarrow$ Shift: `crc = 1101 0100` (`0xD4`)
  * **Bit 4:** MSB = 1 $\rightarrow$ Shift: `1010 1000`, XOR `0x31`: `crc = 1001 1001` (`0x99`)
  * **Bit 5:** MSB = 1 $\rightarrow$ Shift: `0011 0010`, XOR `0x31`: `crc = 0000 0011` (`0x03`)
  * **Bit 6:** MSB = 0 $\rightarrow$ Shift: `crc = 0000 0110` (`0x06`)
  * **Bit 7:** MSB = 0 $\rightarrow$ Shift: `crc = 0000 1100` (`0x0C`)
* Kết thúc Byte 0: `crc = 0x0C`.

### Bước 3: Nạp Byte 1 (`0xEF`)
* `crc = crc ^ 0xEF = 0x0C ^ 0xEF = 0000 1100 ^ 1110 1111 = 1110 0011` (`0xE3`)
* Tiếp tục dịch và XOR 8 bit của `0xE3`:
  * Quá trình diễn ra tương tự, sau 8 nhịp ta thu được giá trị `crc` cuối cùng.
* Kết quả này sẽ được so sánh với Byte thứ 3 nhận được từ SHT30.

---

## 7. Tối ưu hóa: Thuật toán tra bảng (Lookup Table - LUT)

Trong các ứng dụng nhúng cần tốc độ cao hoặc xử lý khối dữ liệu lớn, việc lặp 8 lần cho mỗi byte có thể tốn chu kỳ CPU. Thay vào đó, người ta tính toán trước toàn bộ 256 giá trị kết quả có thể có của 1 byte thành một bảng hằng số (Lookup Table):

### Bảng tra trước (256 bytes trong Flash ROM):
```c
static const uint8_t crc8_sht30_table[256] = {
    0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97, 0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E,
    0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4, 0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
    /* ... 256 giá trị được tính sẵn ... */
};

uint8_t sht30_compute_crc8_fast(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc = crc8_sht30_table[crc ^ data[i]];
    }
    return crc;
}
```

### So sánh 2 cách tiếp cận:

| Tiêu chí | Bit-by-bit (Đang dùng trong dự án) | Lookup Table (Bảng tra) |
| :--- | :--- | :--- |
| **Tốc độ thực thi** | Chậm hơn (~60-80 chu kỳ lệnh / byte) | Cực nhanh (~3-5 chu kỳ lệnh / byte) |
| **Dung lượng Flash/ROM** | Rất nhỏ (< 50 bytes code) | Tốn thêm 256 bytes để lưu bảng |
| **Độ phức tạp code** | Đơn giản, trực quan | Cần tạo bảng hằng số trước |
| **Đánh giá trong dự án** | **Rất phù hợp:** SHT30 đọc ở tần số thấp (1 Hz - 10 Hz), mỗi lần chỉ tính cho 2 byte nên phương pháp Bit-by-bit là tối ưu nhất, tiết kiệm bộ nhớ vi điều khiển. |

---

## 8. Khả năng phát hiện lỗi của CRC-8

Tại sao không dùng phép cộng dồn Checksum đơn giản (như DHT11) mà lại dùng CRC-8?

### 8.1. Hạn chế của Checksum thông thường (cộng 8 bit)
* Cảm biến DHT11 sử dụng: `Checksum = Byte1 + Byte2 + Byte3 + Byte4`.
* Nhược điểm: Phép cộng có tính chất giao hoán ($A + B = B + A$). Nếu hai byte bị tráo đổi vị trí cho nhau, hoặc 1 bit tăng 1 và 1 bit giảm 1 (bù trừ), Checksum **hoàn toàn không phát hiện được**.

### 8.2. Ưu thế vượt trội của CRC-8
Nhờ cấu trúc đại số của đa thức $G(x) = x^8 + x^5 + x^4 + 1$:
1. **Phát hiện 100% tất cả lỗi 1-bit:** Bất kỳ 1 bit đơn lẻ nào bị lật do nhiễu đều làm thay đổi hoàn toàn mã CRC.
2. **Phát hiện 100% tất cả lỗi 2-bit độc lập:** Bất kỳ 2 bit lỗi nào trong phạm vi gói dữ liệu ngắn đều được phát hiện.
3. **Phát hiện 100% tất cả các lỗi cụm (burst error) có chiều dài $\le 8$ bit:** Ví dụ một xung nhiễu điện áp kéo dài làm hỏng liên tiếp 8 bit dữ liệu, CRC-8 đảm bảo bắt được 100%.
4. **Phát hiện $99.6\%$ các lỗi cụm ngẫu nhiên dài hơn 8 bit:** Xác suất một chuỗi lỗi ngẫu nhiên dài "may mắn" tạo ra CRC trùng khớp chỉ là:
   $$P_{\text{undetected}} = \frac{1}{2^8} = \frac{1}{256} \approx 0.39\%$$
   Nghĩa là tỷ lệ phát hiện thành công đạt **$99.61\%$**.

---

## 9. Phân định vai trò: CRC-8 vs Hamming(7,4) vs Huffman trong hệ thống

Trong toàn bộ hệ sinh thái dự án (3 ESP32-S3), sinh viên rất dễ nhầm lẫn vai trò của các khối mã hóa. Bảng dưới đây làm rõ vị trí độc lập của từng kỹ thuật:

```text
  [ Cảm biến SHT30 ]
         │
         │ (I²C Bus)  <-- Được bảo vệ bởi CRC-8 (Lọc bỏ mẫu nhiễu ngay tại phần cứng)
         ▼
  ┌─────────────────────────────────────────────────────────────┐
  │ ESP32-S3 #1: TX                                             │
  │  - Kiểm tra CRC-8: Nếu OK -> Lấy raw 4 byte                 │
  │  - Mã hóa Nguồn: HUFFMAN (Nén dữ liệu, giảm số bit)         │
  │  - Mã hóa Kênh: HAMMING(7,4) (Thêm bit chẵn lẻ để sửa lỗi)  │
  └─────────────────────────────────────────────────────────────┘
         │
         │ (UART Bus) <-- Giả lập kênh nhiễu BSC (lật bit ngẫu nhiên)
         ▼
  ┌─────────────────────────────────────────────────────────────┐
  │ ESP32-S3 #2: CHANNEL (Giả lập nhiễu p)                      │
  └─────────────────────────────────────────────────────────────┘
         │
         │ (UART Bus)
         ▼
  ┌─────────────────────────────────────────────────────────────┐
  │ ESP32-S3 #3: RX                                             │
  │  - Giải mã HAMMING(7,4): Sửa lỗi 1-bit trên từng codeword   │
  │  - Giải mã HUFFMAN: Khôi phục lại mẫu ban đầu               │
  │  - Frame CRC-32: Xác nhận toàn vẹn dữ liệu đầu cuối         │
  └─────────────────────────────────────────────────────────────┘
```

| Khối thuật toán | Bản chất | Vị trí hoạt động | Mục tiêu |
| :--- | :--- | :--- | :--- |
| **CRC-8** | **Error Detection** (Phát hiện lỗi, không sửa được) | Giao tiếp phần cứng I²C giữa SHT30 và vi điều khiển TX | Đảm bảo dữ liệu đọc từ cảm biến vào bộ đệm là "dữ liệu sạch". Nếu sai CRC-8 $\rightarrow$ vứt bỏ mẫu, tăng `crc_errors`. |
| **Huffman Coding** | **Source Coding** (Mã hóa nguồn) | Module phần mềm tại TX và RX | Nén dữ liệu nhằm tối ưu dung lượng truyền dẫn (giảm số bit trung bình). |
| **Hamming(7,4)** | **Channel Coding / FEC** (Mã hóa kênh sửa lỗi) | Module truyền thông UART giữa TX - Channel - RX | Tự động phát hiện và **sửa đúng 1 bit lỗi** trên mỗi khối 7 bit khi đi qua kênh truyền có nhiễu BSC. |
| **CRC Frame (CRC-16/32)**| **End-to-End Error Detection** | Khung truyền UART giữa TX và RX | Kiểm tra toàn bộ payload sau khi khôi phục ở RX xem có trùng khớp 100% với dữ liệu gốc của TX hay không. |

---

## 10. Tóm tắt nhanh khi bảo vệ đồ án / thuyết trình

1. **CRC-8 là gì?**  
   Là mã kiểm tra dư thừa vòng 8-bit, dùng toán học chia đa thức modulo-2 trên $GF(2)$ để sinh ra 1 byte kiểm tra tính toàn vẹn dữ liệu.
2. **Tại sao SHT30 dùng CRC-8?**  
   Để phát hiện nhiễu trên đường truyền I²C. Nếu đường dây I²C bị nhiễu do xung điện hoặc tiếp xúc kém, CRC-8 sẽ không khớp, giúp MCU không lưu các giá trị đo rác.
3. **Các tham số của CRC-8 SHT30:**  
   Poly = `0x31` ($x^8 + x^5 + x^4 + 1$), Init = `0xFF`, RefIn = `False`, RefOut = `False`, XorOut = `0x00`.
4. **CRC-8 có sửa được lỗi không?**  
   **Không.** CRC-8 chỉ có khả năng phát hiện lỗi (Detection), không có khả năng tự sửa lỗi (Correction) như mã Hamming. Khi CRC-8 lỗi, giải pháp duy nhất là loại bỏ mẫu và yêu cầu đọc lại từ cảm biến.
