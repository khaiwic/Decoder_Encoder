#ifndef _CONTROL_PANNEL_H_
#define _CONTROL_PANNEL_H_

#include <stdbool.h>
#include <stddef.h>

//   ENCODER_C,
typedef enum {
  IDLE = 0,
  START,
  COLLECT,
  ENCODER_A,
  ENCODER_B,

  //   TRANSMIT_UART,
  ERROR
} control_panel_t;

typedef enum { CP_TRUE = 0, CP_FALSE = 1, CP_LAG = 2 } control_pannel_status;

control_pannel_status control_pannel_init(void);

void control_pannel_print_menu(void);

control_panel_t read_serial_line(char *out_raw_cmd, size_t max_len);

#endif