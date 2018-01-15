#ifndef _UART_H_
#define _UART_H_ 1

void init_uart(void);
void uart_write(uint8_t data);
void uart_write_str(char *s);
void clear_buffer(void);
/* Non blocking read */
uint8_t uart_get_byte(void);
/* Blocking read */
uint8_t read_byte(void);
void read_bytes(uint8_t *buffer, int count);

void write_byte(uint8_t byte);
void write_bytes(uint8_t *buffer, int count);

uint8_t uart_rx_buffer[256];
uint8_t uart_rx_writer;
uint8_t uart_rx_reader;
uint8_t uart_rx_bytes_in_buffer;

#endif
