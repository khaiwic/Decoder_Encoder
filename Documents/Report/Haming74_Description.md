# THUẬT TOÁN MÃ HÓA KÊNH HAMMING(7,4) HỆ THỐNG (SYSTEMATIC HAMMING CODE)

> **Môn học:** Lý thuyết thông tin  
> **Dự án:** Hệ thống truyền thông tin qua 3 ESP32-S3 sử dụng cảm biến SHT30  
> **Khối phụ trách:** Máy phát (TX - Encoder) & Máy thu (RX - Decoder)  
---

## 1. Giới thiệu và Mục tiêu trong Hệ thống

### 1.1. Vị trí trong chuỗi truyền tin
Trong hệ thống truyền tin 3 khối (TX $\rightarrow$ CHANNEL $\rightarrow$ RX):
- **Huffman Encoder:** Đóng vai trò **Mã hóa nguồn** (Source Coding), nén dữ liệu thô để giảm số bit trung bình cần truyền.
- **Hamming(7,4) Encoder:** Đóng vai trò **Mã hóa kênh** (Channel Coding). Sau khi nén nguồn, dòng bit rất nhạy cảm với nhiễu (chỉ cần sai 1 bit có thể làm lệch toàn bộ giải mã Huffman phía sau). Hamming(7,4) bổ sung thêm các bit dư thừa có quy luật (parity bits) để phía thu có thể **tự phát hiện và tự sửa lỗi** khi truyền qua kênh truyền có nhiễu (BSC - Binary Symmetric Channel).

```text
[Buffer SHT30] ---> [Huffman Encoder] ---> [Hamming(7,4) Encoder] ---> [UART TX] ---> (Kênh BSC)
(Dữ liệu gốc)        (Dòng bit nén)        (Dòng bit có bảo vệ)
```

### 1.2. Các thông số kỹ thuật cốt lõi
- **Khối đầu vào ($k$):** $k = 4$ bit dữ liệu tin (Information bits: $d_1, d_2, d_3, d_4$).
- **Độ dài từ mã ($n$):** $n = 7$ bit sau mã hóa (Codeword: $c_1, c_2, c_3, c_4, c_5, c_6, c_7$).
- **Số bit kiểm tra chẵn lẻ ($m = n - k$):** $m = 3$ bit kiểm tra ($p_1, p_2, p_3$).
- **Tốc độ mã (Code Rate $R$):**
  $$R = \frac{k}{n} = \frac{4}{7} \approx 0.5714$$
  *Ý nghĩa:* Tăng thêm khoảng $75\%$ dung lượng bit cần truyền để đổi lấy khả năng chống nhiễu.
- **Khoảng cách Hamming tối thiểu ($d_{\min}$):** $d_{\min} = 3$.
- **Khả năng kiểm soát lỗi:**
  - Khả năng **sửa lỗi** ($t$): Sửa được tối đa $t = \left\lfloor \frac{d_{\min} - 1}{2} \right\rfloor = 1$ bit lỗi trên mỗi từ mã 7 bit.
  - Khả năng **phát hiện lỗi** ($e$): Phát hiện được $e = d_{\min} - 1 = 2$ bit lỗi.

---

## 2. Cơ sở Toán học của Hamming(7,4) Hệ thống

Mã tuyến tính được gọi là **hệ thống (systematic)** nếu các bit dữ liệu gốc xuất hiện nguyên vẹn ở một vị trí cố định trong từ mã. Trong dự án này, ta chọn quy ước chuẩn: **4 bit dữ liệu đi trước, 3 bit kiểm tra đi sau**:

$$c = [d_1, d_2, d_3, d_4, p_1, p_2, p_3] = [d | p]$$

### 2.1. Phương trình tính Parity Bits
Để bảo đảm khoảng cách Hamming $d_{\min} = 3$, mỗi bit kiểm tra bảo vệ một tập hợp con gồm 3 trong 4 bit dữ liệu theo phép cộng Modulo-2 (phép XOR $\oplus$):

$$\begin{cases}
p_1 = d_1 \oplus d_2 \oplus d_3 \\
p_2 = d_2 \oplus d_3 \oplus d_4 \\
p_3 = d_1 \oplus d_2 \oplus d_4
\end{cases}$$

### 2.2. Ma trận sinh $G$ (Generator Matrix)
Mã hóa một vector dữ liệu hàng $1 \times 4$: $d = [d_1, d_2, d_3, d_4]$ thành từ mã hàng $1 \times 7$: $c = d \cdot G$:

$$G = \begin{bmatrix} I_4 & \vert & P \end{bmatrix} = \begin{bmatrix}
1 & 0 & 0 & 0 & \vdots & 1 & 0 & 1 \\
0 & 1 & 0 & 0 & \vdots & 1 & 1 & 1 \\
0 & 0 & 1 & 0 & \vdots & 1 & 1 & 0 \\
0 & 0 & 0 & 1 & \vdots & 0 & 1 & 1
\end{bmatrix}$$

Khi nhân vector $d$ với $G$ trên trường Galois $\mathrm{GF}(2)$:
$$c = d \cdot G = [d_1, d_2, d_3, d_4, (d_1 \oplus d_2 \oplus d_3), (d_2 \oplus d_3 \oplus d_4), (d_1 \oplus d_2 \oplus d_4)]$$

---

## 3. Bảng tra cứu toàn bộ 16 từ mã (Codebook)

Vì đầu vào chỉ có $2^4 = 16$ giá trị khả dĩ (từ `0x0` đến `0xF`), ta có thể lập bảng ánh xạ trực tiếp (Lookup Table) giúp vi điều khiển ESP32-S3 mã hóa cực nhanh chỉ trong $O(1)$:

| Nibble gốc ($d_1 d_2 d_3 d_4$) | Hex gốc | Parity ($p_1 p_2 p_3$) | Codeword 7 bit ($d_1 d_2 d_3 d_4 p_1 p_2 p_3$) | Hex 7 bit |
| :---: | :---: | :---: | :---: | :---: |
| `0 0 0 0` | `0x0` | `0 0 0` | `0000000` | `0x00` |
| `0 0 0 1` | `0x1` | `0 1 1` | `0001011` | `0x0B` |
| `0 0 1 0` | `0x2` | `1 1 0` | `0010110` | `0x16` |
| `0 0 1 1` | `0x3` | `1 0 1` | `0011101` | `0x1D` |
| `0 1 0 0` | `0x4` | `1 1 1` | `0100111` | `0x27` |
| `0 1 0 1` | `0x5` | `1 0 0` | `0101100` | `0x2C` |
| `0 1 1 0` | `0x6` | `0 0 1` | `0110001` | `0x31` |
| `0 1 1 1` | `0x7` | `0 1 0` | `0111010` | `0x3A` |
| `1 0 0 0` | `0x8` | `1 0 1` | `1000101` | `0x45` |
| `1 0 0 1` | `0x9` | `1 1 0` | `1001110` | `0x4E` |
| `1 0 1 0` | `0xA` | `0 1 1` | `1010011` | `0x53` |
| `1 0 1 1` | `0xB` | `0 0 0` | `1011000` | `0x58` |
| `1 1 0 0` | `0xC` | `0 1 0` | `1100010` | `0x62` |
| `1 1 0 1` | `0xD` | `0 0 1` | `1101001` | `0x69` |
| `1 1 1 0` | `0xE` | `1 0 0` | `1110100` | `0x74` |
| `1 1 1 1` | `0xF` | `1 1 1` | `1111111` | `0x7F` |

---

## 4. Giải mã tại Máy thu (Syndrome Decoding)

Phía thu (RX) nhận được vector $r = [r_1, r_2, r_3, r_4, r_5, r_6, r_7] = c \oplus e$ (với $e$ là vector lỗi do kênh truyền).

### 4.1. Ma trận kiểm tra chẵn lẻ $H$ (Parity-Check Matrix)
$$H = \begin{bmatrix} P^T & \vert & I_3 \end{bmatrix} = \begin{bmatrix}
1 & 1 & 1 & 0 & \vdots & 1 & 0 & 0 \\
0 & 1 & 1 & 1 & \vdots & 0 & 1 & 0 \\
1 & 1 & 0 & 1 & \vdots & 0 & 0 & 1
\end{bmatrix}$$

### 4.2. Tính Syndrome $S = r \cdot H^T$
$$S = [s_1, s_2, s_3]$$
$$\begin{cases}
s_1 = r_1 \oplus r_2 \oplus r_3 \oplus r_5 \\
s_2 = r_2 \oplus r_3 \oplus r_4 \oplus r_6 \\
s_3 = r_1 \oplus r_2 \oplus r_4 \oplus r_7
\end{cases}$$

- Nếu $S = [0, 0, 0]$: **Không có lỗi**, lấy ngay 4 bit đầu làm dữ liệu: $[r_1, r_2, r_3, r_4]$.
- Nếu $S \neq [0, 0, 0]$: So sánh $S$ với các cột của ma trận $H$ để xác định vị trí bit bị lật:
  - $S = [1, 0, 1]^T \rightarrow$ Cột 1 ($r_1$ bị lỗi) $\rightarrow$ lật lại bit $r_1$.
  - $S = [1, 1, 1]^T \rightarrow$ Cột 2 ($r_2$ bị lỗi) $\rightarrow$ lật lại bit $r_2$.
  - $S = [1, 1, 0]^T \rightarrow$ Cột 3 ($r_3$ bị lỗi) $\rightarrow$ lật lại bit $r_3$.
  - $S = [0, 1, 1]^T \rightarrow$ Cột 4 ($r_4$ bị lỗi) $\rightarrow$ lật lại bit $r_4$.
  - $S = [1, 0, 0]^T \rightarrow$ Cột 5 ($r_5$ bị lỗi ở bit parity $p_1$).
  - $S = [0, 1, 0]^T \rightarrow$ Cột 6 ($r_6$ bị lỗi ở bit parity $p_2$).
  - $S = [0, 0, 1]^T \rightarrow$ Cột 7 ($r_7$ bị lỗi ở bit parity $p_3$).

Sau khi đảo bit sai tương ứng, ta thu hồi được chính xác 4 bit gốc $[d_1, d_2, d_3, d_4]$.

---

## 5. Quy tắc Đóng gói Bit (Bit Packing) khi Truyền UART

Mỗi từ mã dài **7 bit**, trong khi UART và bộ nhớ vi điều khiển làm việc theo đơn vị **Byte (8 bit)**. Có 2 cách tiếp cận:

### Cách khuyến nghị trong dự án: Đóng gói liên tục (Bitstream Packing)
Xếp liên tiếp các khối 7 bit vào mảng byte:
- 8 từ mã Hamming (mỗi từ 7 bit) = $8 \times 7 = 56$ bit = đúng **7 byte** tròn trịa.
- Nếu chuỗi bit kết thúc không tròn byte, ta chèn padding bit `0` ở byte cuối và ghi rõ tổng số bit có nghĩa (`coded_valid_bits`) trong Header gói tin.