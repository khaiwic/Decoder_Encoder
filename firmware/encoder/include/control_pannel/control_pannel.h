#ifndef _CONTROL_PANNEL_H_
#define _CONTROL_PANNEL_H_

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  IDLE = 0,
  START,
  COLLECT,
  ENCODER,
  TRANSMIT_UART,
  ERROR
} control_panel_t;

typedef enum {
  CP_TRUE = 0,
  CP_FALSE = 1,
  CP_LAG = 2
} control_pannel_status;

typedef enum {
  CP_MENU_IDLE = 0,
  CP_MENU_POST_ENCODER
} cp_menu_stage_t;

control_pannel_status control_pannel_init(void);

void control_pannel_print_menu(cp_menu_stage_t stage);

control_panel_t read_serial_line(cp_menu_stage_t stage, char *out_raw_cmd, size_t max_len);

#endif