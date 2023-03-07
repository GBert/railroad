#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pulse.pio.h"

enum pulse_types {
    SINGLE_SHORT,
    SINGLE_LONG,
    FOUR_SHORT,
    FOUR_LONG
};

float divider;

void printhelp() {
    puts("\nCommands:");
    puts("i\t: info");
    puts("s\t: send single short pulse (58us)");
    puts("S\t: send single long pulse (100us)");
    puts("d\t: send 4 short pulse");
    puts("D\t: send 4 long pulse");
}

int main(void) {
    stdio_init_all();
    int pulse_type;
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &pulse_program);

    uint sm = pio_claim_unused_sm(pio, true);
    pulse_program_init(pio, sm, offset, 5);

    printf("\n===========================\n");
    printf("RP2040 Pulse Console\n");
    printf("===========================\n");
    printhelp();

    while (1) {
        char c = getchar();
        printf("%c", c);
        switch (c) {
            case 'i':
		printf("\n PIO devider: %f\n", divider);
		break;
            case 's':
		pulse_type = SINGLE_SHORT;
                break;
            case 'S':
		pulse_type = SINGLE_LONG;
                break;
            case 'd':
		pulse_type = FOUR_SHORT;
                break;
            case 'D':
		pulse_type = FOUR_LONG;
                break;
            case '\n':
            case '\r':
                break;
            case 'h':
                printhelp();
                break;
            default:
                printf("\nUnrecognised command: %c\n", c);
                printhelp();
                break;
        }
	switch (pulse_type) {
	    /* delay decrement by 4us  -> 54 (58us) and 112 (116us) */
	    case SINGLE_SHORT:
		pio_sm_put_blocking(pio, sm, 0x00361036);
		break;
	    case SINGLE_LONG:
		pio_sm_put_blocking(pio, sm, 0x00701070);
		break;
	    case FOUR_SHORT:
		pio_sm_put_blocking(pio, sm, 0x00361036);
		pio_sm_put_blocking(pio, sm, 0x00361036);
		pio_sm_put_blocking(pio, sm, 0x00361036);
		pio_sm_put_blocking(pio, sm, 0x00361036);
		break;
	    case FOUR_LONG:
		pio_sm_put_blocking(pio, sm, 0x00701070);
		pio_sm_put_blocking(pio, sm, 0x00701070);
		pio_sm_put_blocking(pio, sm, 0x00701070);
		pio_sm_put_blocking(pio, sm, 0x00701070);
		break;
	    default:
		break;
	}
    }
}
