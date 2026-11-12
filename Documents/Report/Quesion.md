# Ý nghĩa của mã hóa và giả mã bằng Huffman và Hamming
> Trong mô hình truyền thông tin kinh điện cuẩ Claude Shannon, hai thuật toán này đại diện cho hai nhiệu vụ cốt lõi và bổ trợ lẫn nhau là nén dữ liệu giảm dư thừa và thêm dư thừa có kiểm soát để chống nhiễu.

```text
[Nguồn SHT30] 
     │
     ▼
[Mã hóa Huffman]   ── (MÃ HÓA NGUỒN - Source Coding): Nén dữ liệu, giảm dư thừa
     │
     ▼
[Mã hóa Hamming]   ── (MÃ HÓA KÊNH - Channel Coding): Thêm dư thừa có kiểm soát để chống nhiễu
     │
     ▼
[Kênh truyền BSC]  ── (Nhiễu lật bit ngẫu nhiên)
     │
     ▼
[Giải mã Hamming]  ── Phát hiện và sửa bit bị lật (FEC)
     │
     ▼
[Giải mã Huffman]  ── Khôi phục lại bản tin gốc từ chuỗi bit biến thiên
     │
     ▼
[Bản tin thu được]
```

## Ý nghĩa của Huffman(Mã hóa nguồn - Source Coding):

- Tối ưu hóa băng thông & bộ nhớ: Dữ liệu tự nhiên luôn có độ dư thừa **Các giá trị nhiệt độ/độ ẩm phổ biến sẽ xuất hiện nhiều hơn các giá trị cực trị**. Huffman gán từ mã ngắn cho các giá trị xuất hiện nhiều và từ mã dài cho giá trị ít gặp.
- Giải mã không nhập nhằng(Prefix-free): Nhờ tính chất mã tiền tố **Không có từ mã nào là tiền tố của từ mã khác**, bộ thu có thể tách và giải nén chuỗi bit liên tục về đúng dữ liệu gốc mà không cần dấu phân các giữa các ký tự.

## Ý nghĩa của Hamming(Mã hóa kênh-Channel Coding):
- Tăng độ tin cậy trên kênh nhiễu: Kênh truyền thựcc tế (hoặc kênh giả lập BSC) luôn có rủi ro lật bit do nhiễu vật lý. **Hamming(7, 4)** thêm ba bit lẻ(parity) và bốn bit dữ liệu để tạo ra khoản các mã tối thiểu dmin = 3.
- Sửa lỗi chủ động(Forward Error Correction - FEC): Bên ghu giả mã bằng ma trận kiểm tra(Syndrome Decoding) để phát hiện và tự động sửa đúng 1 bit sai trong khối 7 bit mà không cần gửi gói tin yêu cầu tryền lại.

## Sự kết hợp giữa Huffman và Hamming(Nghịch lý bổ trợ)

- Huffman vắt kiệt bit dư thừa tự nhiên để gói tin ngắn nhất --> Giúp giản thời gian truyền  trên dây/sóng.
- Nhưng chuỗi bit nén Huffman **cực kì nhạy cảm với lỗi**(sai 1 bit có thể làm sai lệnh toàn bộ ranh giới từ mã phía sau).
- Vì vậy, Hamming chủ động bù lại các bit dư thừa toán học ở những vị trí chiến lược để bảo vệ chuỗi bit Huffman đó.

# Những câu hỏi cần trả lời được 

## Về giới hạn tài nguyên(Không quá quan trọng, chỉ là lớp nền cho bài báo cáo)

### Bộ nhớ(SRAM & Flash)

1. SRAM và Heap tiêu thụ
- Bảng tra mã (Look-up Table-LUT) hoặc cây Huffman tốn bao nhiêu byte RAM trên ESP32-S3?
- Khối mã hóa/giải mã tốn bao nhiêu byte>
- Bộ giả mã/mã hóa Huffman làm tăng dung lượng Flash thêm bao nhiêu KB?
- Kích thước buffer cần thiết để không bị tràn khi tốc độ nhận data từ SHT3x nhanh hơn tốc độ giải mã?

2. Thời gian thực thi và độ phức tạp của thuật toán.

- Thời gian xử lý: Mất bao nhiêu us hoặc chu kì xung nhịp để đọc và kiểm tra CRC-8 của SHT30 qua I2C? Nén một khối dữ liệu bằng Huffman? Mã hóa/Giải mã 1 block Hamming(7, 4)?
- Nút thắt cổ chai của hệ thống ở đâu(nếu có)?
- Tại sao giải mã Hamming có thể sử lý // nguyên khối rất nhanh trong khi giải mã Huffman lại là quá trình tuần tự từng bit(Bit-serial decoding) tốn nhiền chu kì máy hớn?

## Về kiến thức

### Mã hóa nguồn(Huffman)

1. Entropy của nguồn tin là bao nhiêu?
- Tính sác xuất p(xi) của các giá trị nhiệt độ/độ ẩm từ SHT30 như thế nào?
- Giá trị Entropy H(X)=−∑p(xi​)log2​p(xi​) (đv: bit/symbol) của tập dữ liệu đo được là bao nhiêu?
2. Hiệu suất nén và tính tối ưu: 
- Chiều dài từ mã trung bình bằng bao nhiêu?
- Hiệu suất mã hóa và độ dư thừa là bao nhiêu? Chiều dài trung bình có thỏa mãn định lý Shannon hay không?
3. Hiện tượng nén âm(Negative Compression) trên gói tin ngắn:
- Trên các bản tin cảm biến ngắn (vài mẫu T/RH), khi tính cả phần đỉnh gói(Header, Codebool/Trê definition, Padding bits), tổng số byte truyền đi của Mode B(Huffman) có thực sự nhỏ hơn Mode A(Raw) không? Nếu lớn hơn, tại sao và ngưỡng số lượng mẫu bằng bao nhiêu thì nén mới bắt đầu **lãi**?

### Vễ kệnh truyền và Mã hóa kênh(Hamming)

1. Đặc tính kênh BSC
- Kênh BSC hoạt động với xác xuất lật bit p độc lập như thế nào? Dung lượng kênh giảm ra sao khi p tăng?
2. Năng lực sửa lỗi của Hamming(7,4):
- Tại sao khoảng các Hamming tối thiểu dmin = 3 lại sửaa được t = [dmin - 1 / 2] = 1 bit và phát hiện được e = dmin - 1 = 2 bit lỗi?
- Ma trận sinh G và ma trận kiểm ta H được ánh xạ như thế nào? Hội chứng lỗi(Syndrome) S định bị but lỗi cụ thể ra sao?
- Tỷ lệ mã(Code rate). Mức Overhead +75% số bit có xứng đáng với xác suất kênh bị lỗi hay không?
3. Giới hạn và rủi ro của Hamming(7, 4):
- Điều gì xảy ra nếu trên 1 khối 7 bit có 2 bit bị lật cùng lúc?

### Tương tác hệ thống (Source-Channel Interaction)
1. Hiệu ứng lan truyền lỗi (Error Propagation):
- Trong Mode B (chỉ có Huffman, không có Hamming), khi kênh BSC làm lật 1 bit, bộ giải mã Huffman bị trượt đồng bộ (loss of synchronization) như thế nào? Một bit lỗi làm hỏng bao nhiêu giá trị đo phía sau?
2. So sánh định lượng giữa 3 chế độ (A - RAW, B - HUFFMAN, C - HUFFMAN + HAMMING):
- Tỷ lệ lỗi bit (BER - Bit Error Rate) trước và sau khối giải mã Hamming ở các mức xác suất 
𝑝 khác nhau (𝑝=0.001,0.01,0.05)?
- Tỷ lệ gói tin khôi phục thành công (Packet Success Rate)?
- Sai số toàn phương trung bình (MSE - Mean Squared Error) của nhiệt độ/độ ẩm khôi phục so với dữ liệu gốc?
	​