#include "protocols/NEC.h"
#include <string>

#define BTN_ON 186
#define BTN_MENU 184
#define BTN_TEST 187
#define BTN_BACK 188
#define BTN_MINUS_SIGN 230
#define BTN_PLUS_SIGN 191
#define BTN_PREVIOUS 248
#define BTN_NEXT 246
#define BTN_PLAY 234
#define BTN_C 242
#define BTN_NUM_0 233
#define BTN_NUM_1 243
#define BTN_NUM_2 231
#define BTN_NUM_3 161
#define BTN_NUM_4 247
#define BTN_NUM_5 227
#define BTN_NUM_6 165
#define BTN_NUM_7 189
#define BTN_NUM_8 173
#define BTN_NUM_9 181

namespace ir {

    class Tdjl20Keys {
        public:
            typedef enum Button {
                on=BTN_ON, menu=BTN_MENU, test=BTN_TEST, back=BTN_BACK, minusSign=BTN_MINUS_SIGN,
                plusSign=BTN_PLUS_SIGN, previous=BTN_PREVIOUS, next=BTN_NEXT, play=BTN_PLAY, c=BTN_C, 
                num0=BTN_NUM_0, num1=BTN_NUM_1, num2=BTN_NUM_2, num3=BTN_NUM_3, num4=BTN_NUM_4, 
                num5=BTN_NUM_5, num6=BTN_NUM_6, num7=BTN_NUM_7, num8=BTN_NUM_8, num9=BTN_NUM_9
            } Button;

            typedef struct ButtonEvent {
                const Button button;
                const uint64_t time;
            } ButtonEvent;

            Tdjl20Keys(uint gpio);
            bool hasEvent() const;
            ButtonEvent nextEvent() const;

            static std::string buttonStr(Button button);
    };


}
