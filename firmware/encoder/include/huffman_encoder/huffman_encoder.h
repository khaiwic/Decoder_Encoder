#ifndef _HUFFMAN_ENCODER_H_
#define _HUFFMAN_ENCODER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HUFFMAN_MAX_SYMBOLS       256U
#define HUFFMAN_MAX_CODE_LEN      32U

/* Mã trạng thái trả về của Huffman Encoder */
typedef enum {
    HUFFMAN_OK = 0,
    HUFFMAN_ERR_PARAM        = -1, /* Con trỏ tham số NULL hoặc độ dài 0 */
    HUFFMAN_ERR_EMPTY        = -2, /* Dữ liệu đầu vào rỗng */
    HUFFMAN_ERR_BUF_OVERFLOW = -3  /* Bộ đệm đầu ra không đủ dung lượng */
} huffman_status_t;

/* Cặp (symbol, code_length) dùng để tái tạo cây Canonical Huffman tại RX */
typedef struct {
    uint8_t symbol; /* Giá trị byte ký hiệu (0..255) */
    uint8_t len;    /* Độ dài từ mã Huffman nhị phân (bit) */
} huffman_book_entry_t;

/* Bảng mã Codebook gửi kèm trong Header gói tin sang RX */
typedef struct {
    uint16_t used_symbols_count; /* Số lượng ký hiệu phân biệt thực tế có mặt (1..256) */
    huffman_book_entry_t entries[HUFFMAN_MAX_SYMBOLS];
} huffman_codebook_t;

/* Các chỉ số Lý thuyết thông tin và Hiệu suất nén phục vụ Báo cáo */
typedef struct {
    size_t   raw_bytes_count;     /* Số byte đầu vào gốc (ví dụ 4N byte) */
    size_t   coded_valid_bits;    /* Số bit nén thực tế có nghĩa (chưa tính padding) */
    size_t   coded_bytes_count;   /* Số byte sau khi đóng gói bitstream: ceil(valid_bits / 8) */
    size_t   codebook_bytes_size; /* Kích thước Codebook khi tuần tự hóa (byte) */
    float    entropy;             /* Entropy của khối dữ liệu H(X) (bit/symbol) */
    float    avg_code_len;        /* Độ dài mã trung bình L_bar (bit/symbol) */
    float    payload_comp_ratio;  /* Tỷ lệ nén payload: raw_bits / coded_valid_bits */
    float    net_comp_ratio;      /* Tỷ lệ nén toàn phần: raw_bytes / (coded_bytes + codebook) */
} huffman_metrics_t;

/**
 * @brief Mã hóa toàn bộ khối dữ liệu thô (4N byte) bằng thuật toán Canonical Huffman.
 *
 * @param[in]  in_data         Con trỏ trỏ tới mảng dữ liệu thô cần nén (ví dụ 4N byte SHT30).
 * @param[in]  in_len          Số byte dữ liệu thô đầu vào.
 * @param[out] out_bitstream   Bộ đệm nhận dòng bit đã nén (được ghi theo thứ tự MSB-first).
 * @param[in]  max_out_len     Dung lượng tối đa của out_bitstream.
 * @param[out] out_book        Bảng mã Codebook trích xuất để gửi kèm sang RX.
 * @param[out] metrics         Con trỏ nhận các chỉ số entropy, L_bar, compression ratio (có thể NULL nếu không cần).
 *
 * @return HUFFMAN_OK nếu thành công, hoặc mã lỗi âm nếu thất bại.
 */
huffman_status_t huffman_encode(const uint8_t *in_data, size_t in_len,
                               uint8_t *out_bitstream, size_t max_out_len,
                               huffman_codebook_t *out_book,
                               huffman_metrics_t *metrics);

/**
 * @brief Tuần tự hóa Codebook thành mảng byte nhị phân để gắn vào Header gói tin UART.
 * Định dạng:
 *   [2 byte: used_symbols_count (Big-Endian)]
 *   Followed by used_symbols_count * 2 byte: [symbol, code_length]
 *
 * @param[in]  book     Con trỏ tới Codebook cần tuần tự hóa.
 * @param[out] out_buf  Bộ đệm nhận byte nhị phân.
 * @param[in]  max_len  Dung lượng tối đa của out_buf.
 *
 * @return Số byte thực tế đã ghi vào out_buf, hoặc mã lỗi âm nếu bộ đệm quá nhỏ.
 */
int huffman_serialize_codebook(const huffman_codebook_t *book, uint8_t *out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif 
