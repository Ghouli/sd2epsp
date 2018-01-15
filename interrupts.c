#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "interrupts.h"

ISR(USART_RX_vect)
{
	uart_rx_buffer[uart_rx_writer++] = UDR0;
	++uart_rx_bytes_in_buffer;
}
