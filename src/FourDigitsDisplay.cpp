#include "FourDigitsDisplay.h"
#include "pico/stdlib.h"

FourDigitsDisplay::FourDigitsDisplay(const std::array<uint, 4>& digitSelectPins, const std::array<uint, 7>& segmentPins)
	: digitSelectPins(digitSelectPins), segmentPins(segmentPins), currentNumber({0,0,0,0}), digitCurrentlyDisplayed(1), nextDigitDisplayTime(0) {
}

void FourDigitsDisplay::init() {
    for(int i=0; i<4; i++) {
        gpio_init(this->digitSelectPins[i]);
        gpio_set_dir(this->digitSelectPins[i], 1);
    }

    for(int i=0; i<7; i++) {
        gpio_init(this->segmentPins[i]);
        gpio_set_dir(this->segmentPins[i], 1);
    }
}

void FourDigitsDisplay::clear() {
    for (int i=0; i<4; i++) {
        gpio_put(this->digitSelectPins[i], 1);
        for (int j=0; j<7; j++) {
            gpio_put(this->segmentPins[j], 1);
        }
    }
}

void FourDigitsDisplay::displayNumber(uint number) { 
    if (number > 9999) number %= 10000;

    this->clear();

    uint digit1 = number/1000;
    uint remainder = number - digit1*1000;
    uint digit2 =  remainder / 100;
    remainder -= digit2*100;
    uint digit3 =  remainder / 10;
    uint digit4 = remainder - digit3*10;
    this->currentNumber = { digit1, digit2, digit3, digit4 };

    this->digitCurrentlyDisplayed = 0;
    this->digitDisplayed = true;
    this->nextDigitDisplayTime = time_us_64() + DIGIT_DISPLAY_TIME_MS * 1000;
}

void FourDigitsDisplay::refresh() {
    if (time_reached(this->nextDigitDisplayTime)) {
        this->digitCurrentlyDisplayed++;
        if (this->digitCurrentlyDisplayed > 4) this->digitCurrentlyDisplayed = 1;
        this->displayDigit(this->digitCurrentlyDisplayed, this->currentNumber[this->digitCurrentlyDisplayed-1]);
        this->nextDigitDisplayTime = time_us_64() + DIGIT_DISPLAY_TIME_MS * 1000;
    }
}

void FourDigitsDisplay::displayDigit(uint digitSelect, uint digitToDisplay) {
    if (digitToDisplay > 9) digitToDisplay %= 10;
    for (int i=0; i<4; i++) {
        gpio_put(this->digitSelectPins[i], i == digitSelect - 1);
    }
    for(int i=0; i<7; i++) {
        gpio_put(this->segmentPins[i], FourDigitsDisplay::SEGMENTS_CHARS[digitToDisplay][i]);
    }
}

