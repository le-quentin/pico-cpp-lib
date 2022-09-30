#include "pico/stdlib.h"
#include <stdio.h>

#include "ir-remotes/protocols/NEC.h"

#ifdef DEBUG
    #define DEBUG_LOG(args...) printf(args);
#else
    #define DEBUG_LOG(args...)
#endif

const uint ALIM_PIN = 28;
const uint INFRARED_RECEIVER_GPIO_PIN = 26;

int main() {
    // Necessary to get IO from USB, and reboot without using the physical boot button
    stdio_init_all();

    gpio_init(ALIM_PIN);
    gpio_set_dir(ALIM_PIN, GPIO_OUT);
    gpio_put(ALIM_PIN, 1);

    ir::nec::initOnGpio(INFRARED_RECEIVER_GPIO_PIN);

    while(true) {
        if (ir::nec::outFifo().hasMessages()) {
            ir::nec::DataFrame received = ir::nec::outFifo().pop();
            printf("New element in fifo: [Address=%d, Command=%d, Time=%lld ms ago]\n", received.address, received.command, (time_us_64() - received.startTime) / 1000);
        }
    }
}

