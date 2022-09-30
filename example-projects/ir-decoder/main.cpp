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
    uint64_t currentParsingStartTime = 0;
    uint64_t lastSignalChangeTime = 0;
    uint64_t lastPulseDuration = 0;
    uint32_t data = 0;
    uint8_t bitsRead = 0;
} ReceiverState;

#define ADDRESS_BYTE(data) (uint8_t)((data & 0xFF000000) >> 24)
#define INVERTED_ADDRESS_BYTE(data) (uint8_t)((data & 0x00FF0000) >> 16)
#define COMMAND_BYTE(data) (uint8_t)((data & 0x0000FF00) >> 8)
#define INVERTED_COMMAND_BYTE(data) (uint8_t)(data & 0x000000FF)

class IR_Frame {
    public:
        const uint8_t address;
        const uint8_t command;
        const uint64_t startTime; 
        const bool valid;
        IR_Frame(uint32_t data, uint64_t startTime) : 
            startTime(startTime),
            address(data >> 24), 
            command(data << 16 >> 24),
            valid(ADDRESS_BYTE(data) == INVERTED_ADDRESS_BYTE(~data) && COMMAND_BYTE(data) == INVERTED_COMMAND_BYTE(~data)) {
                DEBUG_LOG("Validating IR_FRAME########\nAddress: %#10x, Control: %#10x\nCommand: %#10x, Control: %#10x\n", 
                    ADDRESS_BYTE(data), INVERTED_ADDRESS_BYTE(~data), COMMAND_BYTE(data), INVERTED_COMMAND_BYTE(~data));
            }
};

ReceiverState receiver;

template <typename T> class IRQ_FIFO {
    public: 
        IRQ_FIFO() : m_fifo(), empty(true) {}

        void push(T msg) {
            //TODO add mutex
            m_fifo.push(msg);
            this->empty = false;
        }

        bool hasMessages() const {
            return !empty;
        }
        
        T pop() {
            //TODO add mutex
            T msg = m_fifo.front();
            m_fifo.pop();
            this->empty = m_fifo.empty();
            return msg;
        }

    private:
        std::queue<T> m_fifo;
        volatile bool empty;
};

IRQ_FIFO<IR_Frame> irDataFifo;

void assertGpioLevelAndTransitionToState(uint8_t expectedGpioLevel, ReceiverParsingState state) {
    if (expectedGpioLevel != receiver.currentSignal) {
        DEBUG_LOG("In state %s, got gpio new level %d but expected %d. Switching to ERROR.\n", parsing_state_str(receiver.parsingState), receiver.currentSignal, expectedGpioLevel);
        receiver.parsingState = ReceiverParsingState::ERROR;
        return;
    }

    DEBUG_LOG("%s=>%s\n", parsing_state_str(receiver.parsingState), parsing_state_str(state));
    receiver.parsingState = state;
}

void IR_parse_new_signalLevel(uint8_t gpioLevel) {
    //TODO extract this in a IRemote/NEC driver. Another layer on top for my specific remote (TDJL20KEYS)
    //Challenge => how to handle the static/global data (=the fifo) in a clean fashion with libs?
    uint64_t now = time_us_64();
    receiver.currentSignal = gpioLevel;
    switch(receiver.parsingState) {
        case ReceiverParsingState::IDLE:
            assertGpioLevelAndTransitionToState(0, ReceiverParsingState::START_BURST);
            break;
        case ReceiverParsingState::START_BURST:
            receiver.currentParsingStartTime = receiver.lastSignalChangeTime;
            DEBUG_LOG("Parsing start time: %lld\n", receiver.currentParsingStartTime);
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
                DEBUG_LOG("Parsing start time: %lld\n", receiver.currentParsingStartTime);
                IR_Frame newFrame(receiver.data, receiver.currentParsingStartTime);
                if (newFrame.valid) {
                    irDataFifo.push(newFrame);
                } else {
                    printf("ERROR: Got an invalid IR data frame: %#10x\n", receiver.data);
                }
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
    IR_parse_new_signalLevel(events & GPIO_IRQ_EDGE_RISE ? 1 : 0);
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
        if (irDataFifo.hasMessages()) {
            IR_Frame received = irDataFifo.pop();
            printf("New element in fifo: [Address=%d, Command=%d, Time=%lld ms ago]\n", received.address, received.command, (time_us_64() - received.startTime) / 1000);
        }
    }
}

