# ReadMe cua logger

## Cac che do cua ma hoa

|STT|Kieu ma hoa|Giai thich|
|---|---|---:|
|A|No Huffman No Hamming| Khong nen, khong them ma parity|
|B|Huffman No Hamming| Chi nen, khong them ma parity|
|C|Huffman & Hamming| Ca nen, ca ma hoa kenh|

## Struct

## Struct `encoder_log_record_t`

- `seq_id`: Stt luot phat.
- `batch_id`: Stt lo mau cam bien SHT3x.
- `mode`: Che do ma hoa A, B or C.
- `valid_samples`: So mau SHT3x hop le N.
- `dropped_samples`: So mau bi loai do `i2c_errors` && `crc_errors`.
- `avg_temp_c`: Nhiet do trung binh cua lo mau.
- `avg_rh_pct`: Do am trung binh cua lo mau.
- `raw_bytes`: Dung luong du lieu tho.
- `raw_crc32` Ma crc-32 cua payload goc.
- `entropy`: Entropy nguon tin H(x).

- `huff_used_symbols`: So ki hieu xuat hien trong codebook
- `huff_codebook_bytes`: Kich thuoc codebook
- `huff_valid_bit`: So bit nen hop le thuc te
- `huff_bytes`: So byte sau dong goi
- `payload_comp_ratio`: R
- `net_comp_ratio`: Ty le nen toan phan

- `ham_valid_bits`: So bit sau ma hoa Hamming
- `ham_bytes`: So byte sau dong goi Hamming
- `ham_code_rate`: Toc do ma hoa kenh thuc te

- `encode_time_us`:  Thoi gian ma hoa
- `status`: loi hay k loi

## Function

### logger_calculate_crc32()
### logger_init()
### logger_print_banner(void)

In ra banner cua **Khoi 1**.

### logger_print_csv_header(void)
### logger_log_record()
### logger_print_comparison_summary()
### logger_log_mode_a()
### logger_log_record