/*	
	Epson Serial Protocol handler

	Protocol description can be found in:
	Operating System Reference Manual,
	Chapter 15 - I/O and Peripheral Devices, pages 35 - 41	

 FMT:	Identifies the header block type
 	0x00: Indicates transmission from the main unit
 	0x01: Indicates transmission from the FDD
 DID:	Destination device ID
 	0x31: First unit (drive D: or E:)
 	0x32: Second unit (drive F: or G:)
 SID:	Source device ID, 0x22 on PX-10 (MAPLE)
 FNC:	Command for FDD
 SIZ:	Indicates the text block length (0x00 - 0xFF).
 	The actual value in this field is the length of the
 	actual text block minus 1.

 Text block: A block of data necessary for executing the command.
 	This block can contain 1 to 256 data bytes.

 Commands:
 FMT DID SID FNC SIZ Data #	Function and text content
------------------------	Reset terminal floppy
 00  SS  MM  0D  00  00		XX
 01  MM  SS  0D  00  00		Return code 00
------------------------	Format disk
 00  SS  MM  7C  00  00		Drive code (1 or 2)
 01  MM  SS  7C  02  00		High-order byte of the track number
 				of the currently formatting track.
                     01		Low-order byte of the track number
 				of the currently formatting track.
 				(0-39, 0xFFFF = end)
                     02		Return code (BDOS error or 0)
------------------------	Read disk direct
 00  SS  MM  77  02  00		Drive code (1 or 2)
                     01		Track no (0 - 39)
                     02		Sector no (1 - 64)
 01  MM  SS  77  80  00
                     ..		Read in data (128 bytes)
                     7F
                     80		Return code (BDOS error or 0)
-----------------------		Write disk direct
 00  SS  MM  78  83  00		Drive code (1 or 2)
                     01		Track no (0 - 39)
                     02		Sector no (1 - 64)
                     03		Contents of C register (0 - 2, write type)
                     04
                     ..		Write data (128 bytes)
                     83
 01  MM  SS  78  00  00		Return code (BDOS error or 0)
-----------------------		Flush buffer
 00  SS  MM  79  00  00		XX
 01  MM  SS  79  00  00		Return code (BDOS error or 0)
-----------------------		Copy disk
 00  SS  MM  7A  00  00		Drive code (1 or 2)
 01  MM  SS  7A  02  00		High-order byte of the track number of
 				the currently copying track
                     01		Low-order byte of the track number of
 				the currently copying track (0-39, 0xFFFF = end)
                     02		Return code (BDOS error or 0)


	Basic communication goes in these steps:

Master			Drive
	Prepare device
(EOT)		--->		Disk drive waits for this
 P1		--->		Drive select mode (0 = select)
 DID		--->		Slave device ID - disk unit
 SID		--->		Master device ID - computer
 ENQ		--->		ENQ (prepare yourself!)
		<---	 ACK	Slave responds with ACK
	Master sends header
 SOH		--->		Start of header, selects function
 FMT		--->		0 = from computer (1 = from drive)
 DID		--->		Slave device ID - disk unit
 SID		--->		Master device ID - computer
 FNC		--->		Function
 SIZ		--->		Data field length - 1
 HCS		--->		Header checksum, complement of SOH-SIZ
		<---	 ACK (NAK),(WAK)
	Master begins data transfer
 STX		--->		Start text transfer
 DB0		--->		FDC Read/Write: Disk drive
 DB1		--->		FDC Read/Write: Track
 DB2		--->		FDC Read/Write: Sector
 DB3		--->		FDC Read/Write: data byte 0
 		--->
 DBN		--->		FDC Read/Write: data byte 127
 ETX		--->		End text transfer
 CKS		--->		Checksum, STX-ETX
		<---	ACK, (NAK)
(EOT)		--->		Master done sending
		<---	SOH	Start of header
		<---	FMT	1 = from drive (0 = from computer)
		<---	DID	Master device ID - computer
		<---	SID	Slave device ID - disk unit
		<---	FNC	Function
		<---	SIZ	Data block size
		<---	HSC	Header checksum
 ACK, (NAK)	--->
		<---	STX	Start text transfer
		<---	RET	BDOS return code from disk drive
   (optional	<---	DB0-N	FDC Read: data read from device (128 bytes))
		<---	ETX	End text transfer
		<---	CKS	Checksum
 ACK, (NAK)	--->
 		<---	EOT	End transfer
 EOT 		--->		End transfer

*/
#include <stdint.h>
#include "uart.h"
#include "drive.h"
#include "protocol.h"

struct epsp {
	uint8_t format;		/* FMT - receiving or sending */
	uint8_t master_id;	/* DID when receiving, SID when sending */
	uint8_t slave_id;	/* DID when sending, SID when receiving */
	uint8_t function;	/* FNC - selects function */
	uint8_t size;		/* SIZ - message size */
	uint8_t text_block_size; /* Text block size when sending */
	uint8_t checksum;	/* HCS/CKS - checksum of header / text block*/
	uint8_t data[256];	/* Could be only 128 bytes? */
};

uint8_t data_buffer[256];
struct epsp current_message;
extern struct disk_unit disk_units[2];
struct disk_unit *current_unit;
struct disk_drive *current_drive;

void reset_data(void)
{
	int i;
	//clear_uart_buffer();
	for (i = 0; i < 256; i++) {
		data_buffer[i] = 0;
		current_message.data[i] = 0;
	}
	current_message.format = 0;
	current_message.master_id = 0;
	current_message.slave_id = 0;
	current_message.function = 0;
	current_message.size = 0;
	current_message.text_block_size = 0;
	current_message.checksum = 0;
}

uint8_t calculate_checksum(uint8_t *msg, uint8_t msg_size)
{
	int i, checksum;
	for (i = 0, checksum = 0; i < msg_size; i++) {
		checksum += msg[i];
	}
	checksum = 0x100 - checksum;
	checksum &= 0xFF;
	return checksum;
}

uint8_t wait_for_ACK(void)
{
	if (read_byte() == ACK)
		return OK;
	return FAIL;
}


uint8_t wait_for_EOT(void)
{
	if (read_byte() == EOT)
		return OK;
	return FAIL;
}

void send_ACK(void) { write_byte(ACK); }
void send_EOT(void) { write_byte(EOT); }
void send_NAK(void) { write_byte(NAK); }

uint8_t receive_ENQ_block(void)
{
	uint8_t p1, did, sid, enq;
	p1 = read_byte();
	did = read_byte();
	sid = read_byte();
	enq = read_byte();
	if (p1 != 0)
		return FAIL;
	if (did != TF20_DE && did != TF20_FG)
		return FAIL;
	if (!sid)
		return FAIL;
	if (enq != ENQ)
		return FAIL;
	return OK;
}

uint8_t receive_header_block(void)
{
	uint8_t soh;
	read_bytes(data_buffer, HEADER_BLOCK_SIZE);
	soh = data_buffer[0];
	if (soh != SOH)
		return FAIL;	
	current_message.format = data_buffer[1];
	if (current_message.format != 0)
		return FAIL;
	current_message.slave_id = data_buffer[2];
	current_message.master_id = data_buffer[3];
	current_message.function = data_buffer[4];
	current_message.size = data_buffer[5];

	/* Set current device */
	//current_drive.device_id = current_message.slave_id;
	current_unit = &disk_units[current_message.slave_id - TF20_DE];
	return calculate_checksum(data_buffer, HEADER_BLOCK_SIZE);
}

uint8_t fdc_read_function(uint8_t *data_buffer)
{
	/* Buffer[1] holds 1 or 2 for drive */
	current_drive = &current_unit->disk_drives[data_buffer[1] - 1];
	current_drive->track = data_buffer[2];
	current_drive->sector = data_buffer[3];
	return read_sector(current_drive, current_message.data);
}

uint8_t fdc_write_function(uint8_t *data_buffer)
{
	int i;
	current_drive = &current_unit->disk_drives[data_buffer[1] - 1];
	current_drive->track = data_buffer[2];
	current_drive->sector = data_buffer[3];
	for (i = 0; i < 128; i++)
		data_buffer[i] = data_buffer[i+SEND_MSG_SIZE_OFFSET];
	return write_sector(current_drive, data_buffer);
}

uint8_t fdc_not_implemented(void)
{
	current_drive->return_code = OK;
	return OK;
}

uint8_t receive_text_block(void)
{
	/* Size of actual data + header */
	uint8_t size;

	size = current_message.size + RECEIVE_MSG_SIZE_OFFSET;
	read_bytes(data_buffer, size);

	if (data_buffer[0] != STX)
		return FAIL;
	if (data_buffer[size - 2] != ETX)
		return FAIL;
	if (calculate_checksum(data_buffer, size) != OK)
		return FAIL;
	/* Do some black magic */
	switch (current_message.function) {
	case FDC_READ:
		current_message.text_block_size = READ_DATA_SEND_SIZE;
		fdc_read_function(current_message.data);
		break;
	case FDC_WRITE:
		fdc_write_function(data_buffer);
		break;
	case FDC_WRITEHST:
	case FDC_RESET:
	case FDC_RESETM:
	case FDC_COPY:
	case FDC_FORMAT:
		return fdc_not_implemented();
	default:
		return FAIL;
	}
	return OK;
}

uint8_t send_header_block(void)
{
	data_buffer[0] = SOH;
	data_buffer[1] = FMT_FROM_DRIVE;
	data_buffer[2] = current_message.master_id;
	data_buffer[3] = current_message.slave_id;
	data_buffer[4] = current_message.function;
	data_buffer[5] = current_message.text_block_size;
	data_buffer[6] = calculate_checksum(data_buffer, HEADER_BLOCK_SIZE - 1);
	write_bytes(data_buffer, HEADER_BLOCK_SIZE);
	return OK;
}

uint8_t send_text_block(void)
{
	uint8_t data_size, i;
	data_size = current_message.text_block_size + SEND_MSG_SIZE_OFFSET;
	for (i = SEND_MSG_SIZE_OFFSET; i < data_size; i++)
		data_buffer[i] = current_message.data[i-SEND_MSG_SIZE_OFFSET];
	data_buffer[0] = STX;
	data_buffer[data_size - 3] = current_drive->return_code;
	data_buffer[data_size - 2] = ETX;
	data_buffer[data_size - 1] = calculate_checksum(data_buffer, data_size);
	write_bytes(data_buffer, data_size);

	return OK;
}

void communicate(void)
{
	while(1) {
		/* Reset buffers at the start of each session */
		reset_data();
		/* Wait until master calls for it's slaves with EOT */
		if (wait_for_EOT() != OK)
			continue;
		if (receive_ENQ_block() != OK) {
			send_NAK();
			continue;
		}
		send_ACK();
		if (receive_header_block() != OK) {
			send_NAK();
			continue;
		}
		send_ACK();
		if (receive_text_block() != OK) {
			send_NAK();
			continue;
		}
		send_ACK();
		if (wait_for_EOT() != OK) {
			continue;
		}
		send_header_block();
		if (wait_for_ACK() != OK) {
			continue;
		}
		send_text_block();
		if (wait_for_ACK() != OK) {
			continue;
		}
		send_EOT();
		if (wait_for_EOT() == OK);
			/* Celebrations here */
	}
}
