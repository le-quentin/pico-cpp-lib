#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

#include <pico-cpp-lib/PushButton.h>

#define UART_ID uart0
#define BAUD_RATE 3330
#define DATA_BITS 17
#define STOP_BITS 1
#define PARITY    UART_PARITY_EVEN


// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

static int chars_rxed = 0;
static uint8_t lastChar = 'a';

// RX interrupt handler
void on_uart_rx() {
    printf("Received UART interrupt: ");
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        lastChar = ch;
        chars_rxed++;
        printf("%hhx", ch);
    }
    printf("\n");
}

const uint ALIM_PIN = 28;
const uint BUTTON_PIN = 15;
const uint BUZZER_PIN = 16;
const uint INFRARED_RECEIVER_GPIO_PIN = 1;

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

    uart_init(uart0, BAUD_RATE);
    gpio_set_function(INFRARED_RECEIVER_GPIO_PIN, GPIO_FUNC_UART);

    const uint period_µs = 1 * 1000 * 1000 / PITCH_FREQUENCY_HERTZ;
    uint64_t nextPulseBegin = time_us_64() + period_µs;
    uint64_t nextPulseEnd = nextPulseBegin + (period_µs * SQUARE_WIDTH_PERCENT)/100;


    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    //uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);

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
                //printf("Button was pushed\n");
                //buzzerOn = !buzzerOn;

                // Can we send it back?
                //if (uart_is_writable(UART_ID)) {
                    // Change it slightly first!
                    //lastChar++;
                    //const char str[3] = { lastChar, lastChar, '\0' };
                    //uart_puts(UART_ID, str);
                //}
            } else {
                //printf("Button was released\n");
            }
        }
    }
}

