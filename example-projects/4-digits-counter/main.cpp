#include "pico/stdlib.h"
#include <stdio.h>
#include <array>

#include <pico-cpp-lib/FourDigitsDisplay.h>
#include <pico-cpp-lib/PushButton.h>

const uint ALIM_PIN = 28;
const uint BUTTON_PIN = 15;

int main() {
    // Necessary to get IO from USB, and reboot without using the physical boot button
    stdio_init_all();

    gpio_init(ALIM_PIN);
    gpio_set_dir(ALIM_PIN, GPIO_OUT);
    gpio_put(ALIM_PIN, 1);

    uint numberToDisplay = 0;
    FourDigitsDisplay screen({1,2,3,4}, {5,6,7,8,9,10,11});
    screen.init();
    screen.displayNumber(numberToDisplay);

    PushButton button(BUTTON_PIN);
    button.init();

    while (true) {
        screen.refresh();

        ButtonEvent newButtonEvent = button.pollEvent();
        if (newButtonEvent != ButtonEvent::NONE) {
            if (newButtonEvent == ButtonEvent::PUSHED) {
                printf("Button was pushed\n");
                numberToDisplay++;
                screen.displayNumber(numberToDisplay);
            } else {
                printf("Button was released\n");
            }
        }
    }
}

