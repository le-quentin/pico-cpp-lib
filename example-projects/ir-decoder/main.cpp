#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <sstream>
#include <stdio.h>

#include <pico-cpp-lib/PushButton.h>
#include <sys/_stdint.h>

#include <string>
#include <queue>

#ifdef DEBUG
    #define DEBUG_LOG(args...) printf(args);
#else
    #define DEBUG_LOG(args...)
#endif

static int chars_rxed = 0;
static uint8_t lastChar = 'a';

const uint ALIM_PIN = 28;
const uint INFRARED_RECEIVER_GPIO_PIN = 26;

const uint STOP_DURATION_US = 100000;

//Start listening to the IR receiver output, and parses the 4 bytes of data
// Address - Inverted address - Command - Inverted command

typedef enum class ReceiverParsingState {
    IDLE,
    START_BURST,
    START_SPACE,
    BIT_BURST,
    BIT_SPACE,
    REPEAT_BITS,
    ERROR
} ReceiverParsingState;

constexpr const char* parsing_state_str(ReceiverParsingState state) {
    switch (state) {
        case ReceiverParsingState::IDLE: return "Idle";
        case ReceiverParsingState::START_BURST: return "Start Burst";
        case ReceiverParsingState::START_SPACE: return "Start Space";
        case ReceiverParsingState::BIT_BURST: return "Bit Burst";
        case ReceiverParsingState::BIT_SPACE: return "Bit Space";
        case ReceiverParsingState::REPEAT_BITS: return "Repeat Bits";
        case ReceiverParsingState::ERROR: return "Error";
    }
    return "ERROR:UNKOWN";
}

typedef struct ReceiverState {
    ReceiverParsingState parsingState = ReceiverParsingState::IDLE;
    uint8_t currentSignal = 1;
    uint64_t lastSignalChangeTime = 0;
    uint64_t lastPulseDuration = 0;
    uint32_t data = 0;
    uint8_t bitsRead = 0;
} ReceiverState;

ReceiverState receiver;
std::queue<uint32_t> irDataFifo;

void assertGpioLevelAndTransitionToState(uint8_t expectedGpioLevel, ReceiverParsingState state) {
    if (expectedGpioLevel != receiver.currentSignal) {
        DEBUG_LOG("In state %s, got gpio new level %d but expected %d. Switching to ERROR.\n", parsing_state_str(receiver.parsingState), receiver.currentSignal, expectedGpioLevel);
        receiver.parsingState = ReceiverParsingState::ERROR;
        return;
    }

    DEBUG_LOG("%s=>%s\n", parsing_state_str(receiver.parsingState), parsing_state_str(state));
    receiver.parsingState = state;
}

void IR_receive_interrupts(uint8_t gpioLevel) {
    //TODO add two controls
    //1) gpioLevel being inconsistent with state should invalidate the data and wait for new frame (ERROR state)
    //2) validate data with inverted bytes, and encapsulate the data in a type to tell if there's an error, and read different fields
    
    //TODO extract this in a IRemote/NEC driver. Another layer on top for my specific remote (TDJL20KEYS)
    //Challenge => how to handle the static/global data (=the fifo) in a clean fashion with libs?
    uint64_t now = time_us_64();
    receiver.currentSignal = gpioLevel;
    switch(receiver.parsingState) {
        case ReceiverParsingState::IDLE:
            assertGpioLevelAndTransitionToState(0, ReceiverParsingState::START_BURST);
            break;
        case ReceiverParsingState::START_BURST:
            assertGpioLevelAndTransitionToState(1, ReceiverParsingState::START_SPACE);
            break;
        case ReceiverParsingState::START_SPACE:
            assertGpioLevelAndTransitionToState(0, ReceiverParsingState::BIT_BURST);
            break;
        case ReceiverParsingState::BIT_BURST:
            receiver.lastPulseDuration = now - receiver.lastSignalChangeTime;
            assertGpioLevelAndTransitionToState(1, ReceiverParsingState::BIT_SPACE);
            break;
        case ReceiverParsingState::BIT_SPACE: {
            assertGpioLevelAndTransitionToState(0, ReceiverParsingState::BIT_BURST);
            uint8_t receivedBit = 0;
            if( now - receiver.lastSignalChangeTime >= 2 * receiver.lastPulseDuration) { // If it's more than 2x the duration, we reckon we got a 3*d(low)
                receivedBit = 1;
            } 
            // We set the bit with an OR mask, with only the current bit set
            receiver.data |= (receivedBit << (receiver.bitsRead++));
            DEBUG_LOG("Received data: %#10x, bits: %d\n", receiver.data, receiver.bitsRead);
            if (receiver.bitsRead >= 32) {
                irDataFifo.push(receiver.data);
                receiver.data = 0;
                receiver.bitsRead = 0;
                receiver.parsingState = ReceiverParsingState::REPEAT_BITS;
            }
            break;
        }
        case ReceiverParsingState::REPEAT_BITS:
        case ReceiverParsingState::ERROR: {
            if (gpioLevel == 0 && now - receiver.lastSignalChangeTime > STOP_DURATION_US) {
                //It's the start of a new frame. This last signal was the start burst.
                DEBUG_LOG("%s=>%s\n", parsing_state_str(receiver.parsingState), parsing_state_str(ReceiverParsingState::START_BURST));
                receiver.parsingState = ReceiverParsingState::START_BURST;
            }
            break;
        }
    }
    receiver.lastSignalChangeTime = now;
}

void handle_incoming_IR_data(uint gpio, uint32_t events) {
    IR_receive_interrupts(events & GPIO_IRQ_EDGE_RISE ? 1 : 0);
    if (!irDataFifo.empty()) {
        printf("New element in fifo: %#10x\n", irDataFifo.front());
        irDataFifo.pop();
    }
}

int main() {
    // Necessary to get IO from USB, and reboot without using the physical boot button
    stdio_init_all();

    gpio_init(ALIM_PIN);
    gpio_set_dir(ALIM_PIN, GPIO_OUT);
    gpio_put(ALIM_PIN, 1);

    gpio_init(INFRARED_RECEIVER_GPIO_PIN);
    gpio_set_dir(INFRARED_RECEIVER_GPIO_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(INFRARED_RECEIVER_GPIO_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &handle_incoming_IR_data);

    while(true) {
        //nothing
    }
}

