// uart.c - MSP432 UART 串口驱动 (eUSCI_A0)
// 适配: 仅保留 eUSCI_A0 (P1.2/P1.3), 删除了 eUSCI_A2 部分

#include "my_uart.h"
#include <stdarg.h>
#include <string.h>
#include "stdio.h"

// ---- Keil 标准库重定向 (使 printf 可直接输出到串口) ----
#if 1
#pragma import(__use_no_semihosting)
struct __FILE {
    int handle;
};
FILE __stdout;
void _sys_exit(int x) {
    x = x;
}
int fputc(int ch, FILE *f) {
    UART_transmitData(EUSCI_A0_BASE, ch & 0xFF);
    return ch;
}
#endif

// 发送缓冲区
static uint8_t tx_buffer[UART_MAX_SEND_LEN];

// ---- uart_init ----
// 初始化 eUSCI_A0 (P1.2/P1.3), XDS110 虚拟串口
void uart_init(uint32_t baudRate) {
    UART_disableModule(EUSCI_A0_BASE);

    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P1,
        GPIO_PIN2 | GPIO_PIN3,
        GPIO_PRIMARY_MODULE_FUNCTION
    );

    eUSCI_UART_ConfigV1 uartConfig = {
        .selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK,
        .clockPrescalar = 0,
        .firstModReg = 0,
        .secondModReg = 0,
        .parity = EUSCI_A_UART_NO_PARITY,
        .msborLsbFirst = EUSCI_A_UART_LSB_FIRST,
        .numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT,
        .uartMode = EUSCI_A_UART_MODE,
        .overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
    };

    uint32_t clockFrequency = MAP_CS_getSMCLK();
    uint32_t prescalerValue = clockFrequency / baudRate;

    if (prescalerValue > 16) {
        uartConfig.clockPrescalar = prescalerValue / 16;
        uartConfig.firstModReg = (prescalerValue % 16) / 2;
        uartConfig.secondModReg = prescalerValue % 2;
    } else {
        uartConfig.clockPrescalar = prescalerValue;
        uartConfig.overSampling = EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION;
    }

    MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);
    MAP_UART_enableModule(EUSCI_A0_BASE);
}

// ---- uart_send_char ----
void uart_send_char(char c) {
    while (!(UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG)));
    UART_transmitData(EUSCI_A0_BASE, (uint8_t)c);
}

// ---- uart_send_string ----
void uart_send_string(const char *str) {
    while (*str) {
        uart_send_char(*str++);
    }
}

// ---- uart_printf ----
void uart_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf((char *)tx_buffer, UART_MAX_SEND_LEN, format, args);
    va_end(args);

    uint16_t len = strlen((char *)tx_buffer);
    for (uint16_t i = 0; i < len; i++) {
        uart_send_char(tx_buffer[i]);
    }
}

// ---- uart_available ----
bool uart_available(void) {
    return UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);
}

// ---- uart_read_char ----
int uart_read_char(void) {
    if (uart_available()) {
        return UART_receiveData(EUSCI_A0_BASE);
    }
    return -1;
}

// ---- uart_read_line ----
uint16_t uart_read_line(char *buffer, uint16_t buffer_size) {
    uint16_t index = 0;
    int c;
    while (index < buffer_size - 1) {
        c = uart_read_char();
        if (c == -1) continue;
        if (c == '\r' || c == '\n') {
            if (index > 0) break;
            continue;
        }
        buffer[index++] = (char)c;
    }
    buffer[index] = '\0';
    return index;
}
