#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "uart.h"

#ifndef UART_BAUDRATE
#define UART_BAUDRATE 38400
#endif

#define BAUD_PRESCALE (((F_CPU / ( UART_BAUDRATE * 16UL))) - 1 )

void init_uart(void)
{
	
#ifndef USE_FAST_SERIAL
	UBRR0H = (BAUD_PRESCALE >> 8);
	UBRR0L = BAUD_PRESCALE;
	UCSR0A = 0;
#else
	UBRR0H = 0x00;
	UBRR0L = 0x0A;
	/* Set double USART transmission speed on */
	UCSR0A = (1 << U2X0);
#endif
	/* Enable RX, TX, enable RX interrupt */
	UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
	/* 8 databits */
	UCSR0C = (3 << UCSZ00);
	uart_rx_bytes_in_buffer = 0;
	uart_rx_writer = 0;
	uart_rx_reader = 0;
}

void uart_write(uint8_t data)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

void uart_write_str(char *s)
{
	while (*s)
		uart_write(*s++);
}

uint8_t uart_get_byte(void)
{
	if (uart_rx_bytes_in_buffer) {
		cli();
		--uart_rx_bytes_in_buffer;
		sei();
		return uart_rx_buffer[uart_rx_reader++];
	}
	return 0;
}

void clear_buffer(void)
{
	cli();
	uart_rx_bytes_in_buffer = uart_rx_reader = uart_rx_writer = 0;
	sei();
}

/* Blocking reads */
uint8_t read_byte(void)
{
	while (!uart_rx_bytes_in_buffer);
	return uart_get_byte();
}

void read_bytes(uint8_t *buffer, int count)
{
	int i;
	for (i = 0; i < count; i++)
		buffer[i] = read_byte();
}

void write_byte(uint8_t byte)
{
	uart_write(byte);
}

void write_bytes(uint8_t *buffer, int count)
{
	int i;
	for (i = 0; i < count; i++)
		uart_write(buffer[i]);
}
