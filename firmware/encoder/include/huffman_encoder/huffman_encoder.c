#include "huffman_encoder.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define LN2 0.6931471805599453f

typedef struct {
  uint32_t weight;
  int16_t left;
  int16_t right;
  uint8_t symbol;
  bool is_leaf;
} huffman_node_t;

typedef struct {
  int16_t data[HUFFMAN_MAX_SYMBOLS];
  size_t size;
} min_heap_t;

static huffman_node_t s_tree_nodes[HUFFMAN_MAX_SYMBOLS * 2];
static size_t s_nodes_count = 0;
static min_heap_t s_heap;
static uint32_t s_freq[HUFFMAN_MAX_SYMBOLS];
static uint8_t s_raw_lengths[HUFFMAN_MAX_SYMBOLS];
static uint32_t s_canonical_codes[HUFFMAN_MAX_SYMBOLS];
static uint8_t s_canonical_lens[HUFFMAN_MAX_SYMBOLS];

static void heap_init(min_heap_t *heap) { heap->size = 0; }

static void heap_push(min_heap_t *heap, int16_t node_idx) {
  size_t i = heap->size++;
  while (i > 0) {
    size_t parent = (i - 1) / 2;
    if (s_tree_nodes[heap->data[parent]].weight <= s_tree_nodes[node_idx].weight) {
      break;
    }
    heap->data[i] = heap->data[parent];
    i = parent;
  }
  heap->data[i] = node_idx;
}

static int16_t heap_pop(min_heap_t *heap) {
  if (heap->size == 0) {
    return -1;
  }

  int16_t root = heap->data[0];
  heap->size--;
  if (heap->size > 0) {
    int16_t last = heap->data[heap->size];
    size_t i = 0;

    while (i * 2 + 1 < heap->size) {
      size_t left = i * 2 + 1;
      size_t right = left + 1;
      size_t smallest = left;

      if (right < heap->size && s_tree_nodes[heap->data[right]].weight < s_tree_nodes[heap->data[left]].weight) {
        smallest = right;
      }
      if (s_tree_nodes[last].weight <= s_tree_nodes[heap->data[smallest]].weight) {
        break;
      }

      heap->data[i] = heap->data[smallest];
      i = smallest;
    }
    heap->data[i] = last;
  }

  return root;
}

static void compute_tree_depths(int16_t node_idx, uint8_t depth, uint8_t lengths[HUFFMAN_MAX_SYMBOLS]) {
  if (node_idx < 0) {
    return;
  }
  if (s_tree_nodes[node_idx].is_leaf) {
    lengths[s_tree_nodes[node_idx].symbol] = (depth == 0) ? 1U : depth;
    return;
  }
  compute_tree_depths(s_tree_nodes[node_idx].left, (uint8_t)(depth + 1), lengths);
  compute_tree_depths(s_tree_nodes[node_idx].right, (uint8_t)(depth + 1), lengths);
}

static int compare_canonical_entries(const void *a, const void *b) {
  const huffman_book_entry_t *ea = (const huffman_book_entry_t *)a;
  const huffman_book_entry_t *eb = (const huffman_book_entry_t *)b;

  if (ea->len != eb->len) {
    return (int)ea->len - (int)eb->len;
  }
  return (int)ea->symbol - (int)eb->symbol;
}

huffman_status_t huffman_encode(const uint8_t *in_data, size_t in_len, uint8_t *out_bitstream, size_t max_out_len, huffman_codebook_t *out_book, huffman_metrics_t *metrics) {
  if (in_data == NULL || out_bitstream == NULL || out_book == NULL) {
    return HUFFMAN_ERR_PARAM;
  }
  if (in_len == 0U) {
    return HUFFMAN_ERR_EMPTY;
  }

  memset(s_freq, 0, sizeof(s_freq));
  for (size_t i = 0; i < in_len; i++) {
    s_freq[in_data[i]]++;
  }

  float entropy = 0.0f;
  for (int i = 0; i < (int)HUFFMAN_MAX_SYMBOLS; i++) {
    if (s_freq[i] > 0U) {
      float p = (float)s_freq[i] / (float)in_len;
      entropy -= p * (logf(p) / LN2);
    }
  }

  s_nodes_count = 0;
  heap_init(&s_heap);

  for (int i = 0; i < (int)HUFFMAN_MAX_SYMBOLS; i++) {
    if (s_freq[i] > 0U) {
      int16_t idx = (int16_t)s_nodes_count++;
      s_tree_nodes[idx].weight = s_freq[i];
      s_tree_nodes[idx].left = -1;
      s_tree_nodes[idx].right = -1;
      s_tree_nodes[idx].symbol = (uint8_t)i;
      s_tree_nodes[idx].is_leaf = true;
      heap_push(&s_heap, idx);
    }
  }

  uint16_t used_symbols = (uint16_t)s_heap.size;
  if (used_symbols == 0U) {
    return HUFFMAN_ERR_EMPTY;
  }

  int16_t root_idx = -1;
  if (used_symbols == 1U) {
    root_idx = s_heap.data[0];
  } else {
    while (s_heap.size > 1U) {
      int16_t left = heap_pop(&s_heap);
      int16_t right = heap_pop(&s_heap);

      int16_t parent = (int16_t)s_nodes_count++;
      s_tree_nodes[parent].weight = s_tree_nodes[left].weight + s_tree_nodes[right].weight;
      s_tree_nodes[parent].left = left;
      s_tree_nodes[parent].right = right;
      s_tree_nodes[parent].symbol = 0;
      s_tree_nodes[parent].is_leaf = false;

      heap_push(&s_heap, parent);
    }
    root_idx = heap_pop(&s_heap);
  }

  memset(s_raw_lengths, 0, sizeof(s_raw_lengths));
  compute_tree_depths(root_idx, 0, s_raw_lengths);

  out_book->used_symbols_count = used_symbols;
  uint16_t entry_idx = 0U;
  for (int i = 0; i < (int)HUFFMAN_MAX_SYMBOLS; i++) {
    if (s_freq[i] > 0U) {
      out_book->entries[entry_idx].symbol = (uint8_t)i;
      out_book->entries[entry_idx].len = s_raw_lengths[i];
      entry_idx++;
    }
  }

  qsort(out_book->entries, used_symbols, sizeof(huffman_book_entry_t), compare_canonical_entries);

  memset(s_canonical_codes, 0, sizeof(s_canonical_codes));
  memset(s_canonical_lens, 0, sizeof(s_canonical_lens));

  uint32_t cur_code = 0;
  uint8_t prev_len = out_book->entries[0].len;
  s_canonical_codes[out_book->entries[0].symbol] = 0;
  s_canonical_lens[out_book->entries[0].symbol] = prev_len;

  for (uint16_t i = 1U; i < used_symbols; i++) {
    uint8_t cur_len = out_book->entries[i].len;
    cur_code = (cur_code + 1U) << (cur_len - prev_len);

    s_canonical_codes[out_book->entries[i].symbol] = cur_code;
    s_canonical_lens[out_book->entries[i].symbol] = cur_len;
    prev_len = cur_len;
  }

  memset(out_bitstream, 0, max_out_len);
  size_t bit_pos = 0;

  for (size_t i = 0; i < in_len; i++) {
    uint8_t sym = in_data[i];
    uint32_t code = s_canonical_codes[sym];
    uint8_t len = s_canonical_lens[sym];

    if (((bit_pos + len + 7U) / 8U) > max_out_len) {
      return HUFFMAN_ERR_BUF_OVERFLOW;
    }

    for (int b = (int)len - 1; b >= 0; b--) {
      if (((code >> (uint32_t)b) & 1U) != 0U) {
        out_bitstream[bit_pos >> 3U] |= (uint8_t)(1U << (7U - (bit_pos & 7U)));
      }
      bit_pos++;
    }
  }

  if (metrics != NULL) {
    metrics->raw_bytes_count = in_len;
    metrics->coded_valid_bits = bit_pos;
    metrics->coded_bytes_count = (bit_pos + 7U) / 8U;
    metrics->codebook_bytes_size = 2U + ((size_t)used_symbols * 2U);
    metrics->entropy = entropy;
    metrics->avg_code_len = (float)bit_pos / (float)in_len;
    metrics->coding_efficiency = (metrics->avg_code_len > 0.0f) ? (entropy / metrics->avg_code_len) : 0.0f;
    metrics->payload_comp_ratio = ((float)in_len * 8.0f) / (float)bit_pos;

    size_t total_frame_payload = metrics->coded_bytes_count + metrics->codebook_bytes_size;
    metrics->net_comp_ratio = (float)in_len / (float)total_frame_payload;
  }

  return HUFFMAN_OK;
}

int huffman_serialize_codebook(const huffman_codebook_t *book, uint8_t *out_buf, size_t max_len) {
  if (book == NULL || out_buf == NULL) {
    return HUFFMAN_ERR_PARAM;
  }

  size_t needed = 2U + ((size_t)book->used_symbols_count * 2U);
  if (max_len < needed) {
    return HUFFMAN_ERR_BUF_OVERFLOW;
  }

  out_buf[0] = (uint8_t)(book->used_symbols_count >> 8U);
  out_buf[1] = (uint8_t)(book->used_symbols_count & 0xFFU);
  for (uint16_t i = 0U; i < book->used_symbols_count; i++) {
    out_buf[2U + (i * 2U)] = book->entries[i].symbol;
    out_buf[2U + (i * 2U) + 1U] = book->entries[i].len;
  }

  return (int)needed;
}
