#include "control_pannel.h"

#include <ctype.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define CMD_BUF_SIZE 64U
static const struct device *s_uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

/* Hàm phụ trợ nội bộ: cắt khoảng trắng đầu/cuối và chuyển thành chữ IN HOA */
static void normalize_cmd(char *str) {
  char *start = str;
  while (*start && isspace((unsigned char)*start)) {
    start++;
  }

  size_t len = strlen(start);
  if (start != str) {
    memmove(str, start, len + 1U);
  }

  while (len > 0U && isspace((unsigned char)str[len - 1U])) {
    str[--len] = '\0';
  }

  for (size_t i = 0; i < len; i++) {
    str[i] = (char)toupper((unsigned char)str[i]);
  }
}

/* Hàm phụ trợ nội bộ: ánh xạ chuỗi sang enum control_panel_t */
static control_panel_t parse_command_string(const char *cmd) {
  if (strcmp(cmd, "IDLE") == 0) {
    return IDLE;
  } else if (strcmp(cmd, "COLLECT") == 0) {
    return COLLECT;
  } else if (strcmp(cmd, "ENCODER_A") == 0) {
    return ENCODER_A;
  } else if (strcmp(cmd, "ENCODER_B") == 0) {
    return ENCODER_B;
    //   } else if (strcmp(cmd, "ENCODER_C") == 0) {
    //     return ENCODER_C;
    //   } else if (strcmp(cmd, "TRANSMIT_UART") == 0 || strcmp(cmd, "TRANS_UART") == 0) {
    //     return TRANSMIT_UART;
  } else {
    return ERROR;
  }
}

control_pannel_status control_pannel_init(void) {
  if (!device_is_ready(s_uart_dev)) {
    return CP_FALSE;
  }

  // Clear FIFO
  uint8_t dummy;
  while (uart_poll_in(s_uart_dev, &dummy) == 0) {
  }

  return CP_TRUE;
}

void control_pannel_print_menu(void) {
  printk("\n+--------------------------------------------------------------+\n");
  printk("|                 CONTROL PANEL - DANH SACH LENH               |\n");
  printk("+--------------------------------------------------------------+\n");
  printk("|  IDLE          : Dua he thong ve trang thai nghi             |\n");
  printk("|  COLLECT       : Thu thap 128 mau tu cam bien SHT30          |\n");
  printk("|  ENCODER_A     : Ma hoa Che do A (RAW - Khong nen)           |\n");
  printk("|  ENCODER_B     : Ma hoa Che do B (Canonical Huffman)         |\n");
  // printk("|  ENCODER_C     : Ma hoa Che do C (Huffman + Hamming 7,4)     |\n");
  // printk("|  TRANSMIT_UART : Gui goi tin qua UART sang Board 2           |\n");
  printk("+--------------------------------------------------------------+\n");
}

control_panel_t read_serial_line(char *out_raw_cmd, size_t max_len) {
  char buf[CMD_BUF_SIZE];
  size_t idx = 0;
  uint8_t c;

  while (1) {
    while (uart_poll_in(s_uart_dev, &c) == 0) {
      if (c == '\r' || c == '\n') {
        if (idx > 0U) {
          buf[idx] = '\0';
          printk("\n");

          normalize_cmd(buf);

          if (out_raw_cmd != NULL && max_len > 0U) {
            strncpy(out_raw_cmd, buf, max_len - 1U);
            out_raw_cmd[max_len - 1U] = '\0';
          }

          return parse_command_string(buf);
        }
      } else if (c == 0x08 || c == 0x7F) {
        if (idx > 0U) {
          idx--;
          printk("\b \b");
        }
      } else if (idx < (CMD_BUF_SIZE - 1U)) {
        buf[idx++] = (char)c;
        printk("%c", c);
      }
    }
    k_msleep(10);
  }
}
