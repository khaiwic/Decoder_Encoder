#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define COMMAND_SIZE 128

int main(void)
{
    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}