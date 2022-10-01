#include "ir-remotes/TDJL20KEYS.h"
#define BUTTON_STR_CASE(enumVal) case Button::#enumVal: return "#enum";

ir::Tdjl20Keys::Tdjl20Keys(uint gpio) {
    ir::nec::initOnGpio(gpio);
}

bool ir::Tdjl20Keys::hasEvent() const {
    return ir::nec::outFifo().hasMessages();
}


ir::Tdjl20Keys::ButtonEvent ir::Tdjl20Keys::nextEvent() const {
    nec::DataFrame data = ir::nec::outFifo().pop();
    return { .button = static_cast<Button>(data.address), .time = data.startTime };
}

std::string ir::Tdjl20Keys::buttonStr(Button button) {
    switch(button) {
        case Button::on: return "on/off";
        case Button::menu: return "menu";
        case Button::test: return "test";
        case Button::back: return "back";
        case Button::minusSign: return "-";
        case Button::plusSign: return "+";
        case Button::previous: return "previous";
        case Button::next: return "next";
        case Button::play: return "play";
        case Button::c: return "c";
        case Button::num0: return "0";
        case Button::num1: return "1";
        case Button::num2: return "2";
        case Button::num3: return "3";
        case Button::num4: return "4";
        case Button::num5: return "5";
        case Button::num6: return "6";
        case Button::num7: return "7";
        case Button::num8: return "8";
        case Button::num9: return "9";
        default: return "UNKOWN";
    }
}
