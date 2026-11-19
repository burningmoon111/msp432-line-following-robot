#ifndef MY_UART_H_
#define MY_UART_H_

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdint.h>
#include <stdbool.h>

// 最大发送缓冲区长度
#define UART_MAX_SEND_LEN 128

// 初始化串口 (eUSCI_A0, P1.2/P1.3, XDS110 虚拟串口)
void uart_init(uint32_t baudRate);

// 发送格式化字符串 (可在ISR中使用)
void uart_printf(const char *format, ...);

// 发送字符串
void uart_send_string(const char *str);

// 发送单个字符
void uart_send_char(char c);

// 检查是否有数据可读
bool uart_available(void);

// 读取一个字节 (非阻塞)
int uart_read_char(void);

// 读取一行数据
uint16_t uart_read_line(char *buffer, uint16_t buffer_size);

#endif /* MY_UART_H_ */
