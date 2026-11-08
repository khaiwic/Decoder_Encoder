#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "sht30_driver.h"
#include "hamming74_encoder.h"
#include "huffman_encoder.h"

#define COMMAND_SIZE 128

int main(void)
{
    //Khoi dong ham 
    sht30_init();

    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}