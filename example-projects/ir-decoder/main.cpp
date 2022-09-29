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


static int chars_rxed = 0;
static uint8_t lastChar = 'a';

const uint ALIM_PIN = 28;
const uint INFRARED_RECEIVER_GPIO_PIN = 26;

const uint STOP_DURATION_US = 100000;

//Start listening to the IR receiver output, and parses the 4 bytes of data
// Address - Inverted address - Command - Inverted command

uint64_t wait_for_1() {
    uint64_t start = time_us_64();
    uint64_t lowDuration;
    bool timeout = false;
    while(gpio_get(INFRARED_RECEIVER_GPIO_PIN) == 0 && !timeout) {
        lowDuration = time_us_64() - start;
        timeout = lowDuration > STOP_DURATION_US; //End of frame, exit and return parsed data
    }
    return lowDuration;
}

uint64_t wait_for_0() {
    uint64_t start = time_us_64();
    uint64_t highDuration;
    bool timeout = false;
    while(gpio_get(INFRARED_RECEIVER_GPIO_PIN) == 1 && !timeout) {
        highDuration = time_us_64() - start;
    }
    return highDuration;
}

uint32_t IR_receive() {
    uint32_t data = 0;
    uint8_t receivedBitsCount = 0;

    while(gpio_get(INFRARED_RECEIVER_GPIO_PIN) == 0); //Skipping the START bursts
    printf("Start burst ended\n");
    wait_for_0(); //Skipping the waiting time after the START bursts
    printf("Wait after start burst ended\n");

    while (true) {
        //A bit will always start with a low burst, skip it
        uint64_t pulseStartTime = time_us_64();
        uint64_t pulseEndTime;
        while(gpio_get(INFRARED_RECEIVER_GPIO_PIN) == 0) {
            pulseEndTime = time_us_64();
        }
        printf("Bit start burst ended\n");

        //Then, the duration of the high signal after the burst will give us the data
        // d(high) = d(low) => 0 
        // d(high) = 3*d(low) => 1
        uint64_t highDuration = wait_for_0();
        //TODO validate with inverted data 

        if (highDuration > STOP_DURATION_US) {
            printf("timeout, returning\n");
            return data;
        }
        printf("Bit wait after start burst ended\n");

        uint64_t pulseDuration = pulseEndTime - pulseStartTime;
        uint8_t receivedBit = 0;
        if(highDuration >= 2 * pulseDuration) { // If it's more than 2x the duration, we reckon we got a 3*d(low)
            receivedBit = 1;
        } 
        printf("New bit: %d\n", receivedBit);

        // We set the bit with an OR mask, with only the current bit set
        printf("Using mask for new bit: %#08x\n", receivedBit << receivedBitsCount);
        data |= (receivedBit << (receivedBitsCount++));
        printf("New data: %#08x, new bit count: %d\n", data, receivedBitsCount);
    }
}

typedef enum class ReceiverState {
    IDLE,
    START_BURST,
    START_SPACE,
    BIT_BURST,
    BIT_SPACE,
    REPEAT_BITS
} ReceiverState;

ReceiverState currentState = ReceiverState::IDLE;
uint64_t lastSignalChangeTime = 0;
uint64_t lastPulseDuration = 0;
uint32_t data = 0;
uint8_t bitsRead = 0;

std::queue<uint32_t> irDataFifo;

void IR_receive_interrupts(uint8_t gpioLevel) {
    //TODO add two controls
    //1) gpioLevel being inconsistent with state should invalidate the data and wait for new frame (ERROR state)
    //2) validate data with inverted bytes, and encapsulate the data in a type to tell if there's an error, and read different fields
    
    //TODO extract this in a IRemote/NEC driver. Another layer on top for my specific remote (TDJL20KEYS)
    //Challenge => how to handle the static/global data (=the fifo) in a clean fashion with libs?
    uint64_t now = time_us_64();
    switch(currentState) {
        case ReceiverState::IDLE:
            if(gpioLevel == 0) currentState = ReceiverState::START_BURST;
            break;
        case ReceiverState::START_BURST:
            if(gpioLevel == 1) currentState = ReceiverState::START_SPACE;
            break;
        case ReceiverState::START_SPACE:
            currentState = ReceiverState::BIT_BURST;
            break;
        case ReceiverState::BIT_BURST:
            lastPulseDuration = now - lastSignalChangeTime;
            currentState = ReceiverState::BIT_SPACE;
            break;
        case ReceiverState::BIT_SPACE: {
            uint8_t receivedBit = 0;
            if( now - lastSignalChangeTime >= 2 * lastPulseDuration) { // If it's more than 2x the duration, we reckon we got a 3*d(low)
                receivedBit = 1;
            } 
            // We set the bit with an OR mask, with only the current bit set
            data |= (receivedBit << (bitsRead++));
            if (bitsRead >= 32) {
                irDataFifo.push(data);
                data = 0;
                bitsRead = 0;
                currentState = ReceiverState::REPEAT_BITS;
            } else {
                currentState = ReceiverState::BIT_BURST;
            }
            break;
        }
        case ReceiverState::REPEAT_BITS: {
            if (now - lastSignalChangeTime > STOP_DURATION_US) {
                //It's the start of a new frame. This last signal was the start burst.
                currentState = ReceiverState::START_BURST;
            }
            break;
        }
    }
    lastSignalChangeTime = now;
}

void handle_incoming_IR_data(uint gpio, uint32_t events) {
    //printf("\nNew interrupt!\n");
    IR_receive_interrupts(events & GPIO_IRQ_EDGE_RISE ? 1 : 0);
    //printf("State: %s, Received data: %#10x, bits: %d\n", stateStr.c_str(), data, bitsRead);
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

