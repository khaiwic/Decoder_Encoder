#include "control_pannel.h"

#include <ctype.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define CMD_BUF_SIZE 64U
static const struct device *s_uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

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

static control_panel_t parse_command_string(const char *cmd, cp_menu_stage_t stage) {
  if (stage == CP_MENU_IDLE) {
    if (strcmp(cmd, "COLLECT") == 0) {
      return COLLECT;
    }
    if (strcmp(cmd, "ENCODER") == 0) {
      return ENCODER;
    }
    if (strcmp(cmd, "IDLE") == 0) {
      return IDLE;
    }
    return ERROR;
  }

  if (strcmp(cmd, "IDLE") == 0) {
    return IDLE;
  }
  if (strcmp(cmd, "TRANSMIT_UART") == 0) {
    return TRANSMIT_UART;
  }
  return ERROR;
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

void control_pannel_print_menu(cp_menu_stage_t stage) {
  printk("\n+--------------------------------------------------------------+\n");
  printk("|                 CONTROL PANEL - DANH SACH LENH               |\n");
  printk("+--------------------------------------------------------------+\n");
  if (stage == CP_MENU_IDLE) {
    printk("|  COLLECT       : Thu thap 128 mau moi tu SHT30               |\n");
    printk("|  ENCODER       : Ma hoa Huffman + Hamming 7,4 (LTTT06)       |\n");
  } else {
    printk("|  IDLE          : Quay lai trang thai cho                     |\n");
    printk("|  TRANSMIT_UART : Gui goi tin qua UART sang Board 2           |\n");
  }
  printk("+--------------------------------------------------------------+\n");
}

control_panel_t read_serial_line(cp_menu_stage_t stage, char *out_raw_cmd, size_t max_len) {
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

          return parse_command_string(buf, stage);
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
