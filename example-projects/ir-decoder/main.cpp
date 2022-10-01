/*
 * Get the button codes from your NEC protocol remote.
 *
 */
#include "pico/stdlib.h"
#include <iostream>

#include "pico-cpp-lib/ir-remotes/protocols/NEC.h"

const uint ALIM_PIN = 28;
const uint INFRARED_RECEIVER_GPIO_PIN = 26;

// TODO extract in a front lib header containing this and the debug utils...
// Or put it in a cpp built and linked, no need for header? 
std::ostream& operator<<(std::ostream& s, uint8_t n) { return s << (uint) n; }
std::ostream& operator<<(std::ostream& s, uint16_t n) { return s << (uint) n; }

int main() {
    // Necessary to get IO from USB, and reboot without using the physical boot button
    stdio_init_all();

    gpio_init(ALIM_PIN);
    gpio_set_dir(ALIM_PIN, GPIO_OUT);
    gpio_put(ALIM_PIN, 1);

    ir::nec::initOnGpio(INFRARED_RECEIVER_GPIO_PIN);

    IrqFifo<ir::nec::DataFrame>& remoteFifo = ir::nec::outFifo();

    while(true) {
        if (remoteFifo.hasMessages()) {
            ir::nec::DataFrame data = remoteFifo.pop();
            std::cout << "NEC data frame received [Address: " << data.address << ", Command: " << data.command << "] ";
            std::cout << (time_us_64() - data.startTime) / 1000 << " ms ago" << std::endl;
        }
    }
}

