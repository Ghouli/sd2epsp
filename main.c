#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "uart.h"
#include "fat.h"
#include "fat_config.h"
#include "partition.h"
#include "sd_raw.h"
#include "sd_raw_config.h"
#include "drive.h"
#include "protocol.h"

int main(void)
{
	/* Initialize uart for communication */
	init_uart();
	/* Enable interrupts */
	sei();
	/* Initialize virtual drives */
	init_floppy_system();
	/* Start communication */
	communicate();
}
