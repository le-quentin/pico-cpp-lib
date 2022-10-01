#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

#include <pico-cpp-lib/PushButton.h>

const uint ALIM_PIN = 28;
const uint BUTTON_PIN = 15;
const uint BUZZER_PIN = 16;
const uint INFRARED_RECEIVER_GPIO_PIN = 26;

const uint SQUARE_WIDTH_PERCENT = 25;
const uint PITCH_FREQUENCY_HERTZ = 440;

int main() {
    // Necessary to get IO from USB, and reboot without using the physical boot button
    stdio_init_all();

    gpio_init(ALIM_PIN);
    gpio_set_dir(ALIM_PIN, GPIO_OUT);
    gpio_put(ALIM_PIN, 1);

    PushButton button(BUTTON_PIN);
    button.init();

    bool buzzerOn = false;
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    const uint period_µs = 1 * 1000 * 1000 / PITCH_FREQUENCY_HERTZ;
    uint64_t nextPulseBegin = time_us_64() + period_µs;
    uint64_t nextPulseEnd = nextPulseBegin + (period_µs * SQUARE_WIDTH_PERCENT)/100;

    while (true) {

        if (buzzerOn) {
            if (time_reached(nextPulseEnd)) {
                gpio_put(BUZZER_PIN, 0);
                nextPulseBegin += period_µs;
                nextPulseEnd = nextPulseBegin + (period_µs * SQUARE_WIDTH_PERCENT)/100;
            }
            else if (time_reached(nextPulseBegin)) {
                gpio_put(BUZZER_PIN, 1);
            }
        }

        ButtonEvent newButtonEvent = button.pollEvent();
        if (newButtonEvent != ButtonEvent::NONE) {
            if (newButtonEvent == ButtonEvent::PUSHED) {
                buzzerOn = !buzzerOn;
            } 
        }
    }
}

