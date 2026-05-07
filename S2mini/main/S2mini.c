#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TXD0_PIN (GPIO_NUM_1)
#define RXD0_PIN (GPIO_NUM_3)
#define TXD1_PIN (GPIO_NUM_18)
#define RXD1_PIN (GPIO_NUM_16)

void init_uart0();
void init_uart1();

void blink_led15();
void log_message();

void app_main(void)
{
    init_uart0();
    init_uart1();
    xTaskCreate(blink_led15, "blink_led15", 1024, NULL, 1, NULL);
    xTaskCreate(log_message, "log_message", 1024, NULL, 1, NULL);
}

void init_uart0() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD0_PIN, RXD0_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void init_uart1() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void blink_led15() {
    // gpio_pad_select_gpio(GPIO_NUM_15);
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(GPIO_NUM_15, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_15, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void log_message() {
    const char* message = "Hello from ESP32 UART1!\n";
    while (1) {
        uart_write_bytes(UART_NUM_0, message, strlen(message));
        uart_write_bytes(UART_NUM_1, message, strlen(message));
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}