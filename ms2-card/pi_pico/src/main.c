/* 
* ---------------------------------------
* Copyright (c) Sebastian Günther 2021  |
*                                       |    
* devcon@admantium.com                  |    
*                                       | 
* SPDX-License-Identifier: BSD-3-Clause | 
* ---------------------------------------
*/
#include <stdio.h>
#include <stdbool.h>

int LED_BUILTIN = 25;
int LED_GREEN = 15;

void setup() {
    stdio_init_all();
    gpio_init(LED_BUILTIN);
    gpio_set_dir(LED_BUILTIN, GPIO_OUT);
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
}

void blink() {
    gpio_put(LED_BUILTIN, 1);
    gpio_put(LED_GREEN, 1);
    sleep_ms(750);
    gpio_put(LED_BUILTIN, 0);
    gpio_put(LED_GREEN, 0);
    sleep_ms(1050);
}

int main() {
    setup();
    printf("Hello World\n");
    while (true) {    
        printf(".");    
        blink();
    }
    return 0;
}
