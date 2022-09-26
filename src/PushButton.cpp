#include "PushButton.h"
#include "pico/stdlib.h"

const uint DEBOUNCE_TIME_MS = 50;

PushButton::PushButton(uint pin) : pin(pin), lastPinState(false), debounceEndTime(0) {

}

void PushButton::init() {
    gpio_init(this->pin);
    gpio_set_dir(this->pin, GPIO_IN);
}

ButtonEvent PushButton::pollEvent() {
    if (!time_reached(this->debounceEndTime)) return ButtonEvent::NONE;

    bool newPinState = gpio_get(this->pin);

    if (newPinState != this->lastPinState) {
        this->lastPinState = newPinState;
        this->debounceEndTime = time_us_64() + DEBOUNCE_TIME_MS * 1000;
        return newPinState ? ButtonEvent::PUSHED : ButtonEvent::RELEASED;
    }

    return ButtonEvent::NONE;
}
