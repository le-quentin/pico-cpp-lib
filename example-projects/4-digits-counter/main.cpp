#include "pico/stdlib.h"
#include <stdio.h>
#include <array>

#include <pico-cpp-lib/FourDigitsDisplay.h>

const uint ALIM_PIN = 28;
const uint BUTTON_PIN = 15;

const uint DEBOUNCE_TIME_MS = 50;

int main() {
    // Necessary to get IO from USB, and reboot without using the physicall boot button
    stdio_init_all();

    gpio_init(ALIM_PIN);
    gpio_set_dir(ALIM_PIN, GPIO_OUT);
    gpio_put(ALIM_PIN, 1);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);

    uint numberToDisplay = 0;
    FourDigitsDisplay screen({1,2,3,4}, {5,6,7,8,9,10,11});
    screen.init();
    screen.displayNumber(numberToDisplay);

    uint lastButtonState = gpio_get(BUTTON_PIN);
    uint64_t lastButtonChangeTime = 0;

    while (true) {
        screen.refresh();

        if (!time_reached(lastButtonChangeTime + DEBOUNCE_TIME_MS * 1000)) continue;

        uint newButtonState = gpio_get(BUTTON_PIN);
        if (newButtonState != lastButtonState) {
            printf("Button state changed: %d\n", newButtonState);
            lastButtonState = newButtonState;
            lastButtonChangeTime = time_us_64();

            if (newButtonState == 1) {

                numberToDisplay++;
                screen.displayNumber(numberToDisplay);
            }
        }
    }
}

