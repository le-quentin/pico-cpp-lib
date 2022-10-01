#include "pico/stdlib.h"
#include <iostream>

#include "ir-remotes/TDJL20KEYS.h"

const uint ALIM_PIN = 28;
const uint INFRARED_RECEIVER_GPIO_PIN = 26;

int main() {
    // Necessary to get IO from USB, and reboot without using the physical boot button
    stdio_init_all();

    gpio_init(ALIM_PIN);
    gpio_set_dir(ALIM_PIN, GPIO_OUT);
    gpio_put(ALIM_PIN, 1);

    ir::Tdjl20Keys remote(INFRARED_RECEIVER_GPIO_PIN);

    while(true) {
        if (remote.hasEvent()) {
            ir::Tdjl20Keys::ButtonEvent event = remote.nextEvent();
            std::cout << "Button [" << ir::Tdjl20Keys::buttonStr(event.button) << "] was pressed ";
            std::cout << (time_us_64() - event.time) / 1000 << " ms ago" << std::endl;
        }
    }
}

