#ifndef _PROTOCOL_H_
 #define _PROTOCOL_H_

/* Protocol marks */
#define	NUL		0x00
#define SOH		0x01
#define STX		0x02
#define ETX		0x03
#define EOT		0x04
#define ENQ		0x05
#define	ACK		0x06
#define NAK		0x15
#define US		0x31

/* Selection mode */
#define PS		0x31

#define FMT_FROM_COMP	0x00
#define FMT_FROM_DRIVE	0x01
/* Device ID's */
#define	HX20		0x20
#define	PX8		0x22
#define PX4		0x23
/* Both TF-20 units contain two disk drives
 * these are mapped as drives D & E and F & G
 */
#define TF20_DE		0x31
#define TF20_FG		0x32
/* Disk station select command */
#define DS_SEL		0x05

/* Epson PX-10 CP/M BIOS commands */
/* Operating System Referance manual chapter 15, pages 35 - 41 */
/* Real device uses 1K write buffer */
/* Return codes:
   0x00	Normal termination
   0xFA	BDOS error	Read error
   0xFB	BDOS error	Write error
   0xFC	BDOS error	Drive select error
   0xFD	BDOS error	Write protect
   0xFE	BDOS error	General failure?

   The third byte of the data block for FNC = 0x78 indicates the write mode:
   0x00	Standard write	FDD blocks data before write
   0x01	Flush buffer	FDD immediately writes data on the FDD without blocking
   0x02	Sequential write

   0x00 is used when writing ordinary files.
   0x01 is used only when writing directories.
 */
#define FDC_RESETM	0x0D	/* Terminal floppy reset on PX-10 */
#define FDC_RESET	0x0E	/* Terminal floppy reset on HX-20 */
#define FDC_READ	0x77	/* Read disk direct */
#define FDC_WRITE	0x78	/* "Direct" write - actually just inserts to buffer */
#define FDC_WRITEHST	0x79	/* Flush buffer - should be used for accurate implementation? */
/* Commands used by user programs */
/* COPYDISK uses these? */
/* "Causes the FDD to copy the entire diskette on the specified drive
    onto another diskette. This command is not available if the system
    has only one drive."
 */
#define FDC_COPY	0x7A
/* "Format causes FDD to format two tracks and return  the corresponding
   track number (logical numbers) and a return code to system. The FDD
   continues formatting in two track units and sets the track number in
   the return message to 0FFFFH when it completes formatting."
 */
#define FDC_FORMAT	0x7C

/* Return codes */
#define OK		0x00	/* Normal termination		*/
#define	BDOS_RD_ERR	0xFA	/* BDOS read error		*/
#define BDOS_WR_ERR	0xFB	/* BDOS write error		*/
#define BDOS_DS_ERR	0xFC	/* BDOS drive select error 	*/
#define BDOS_WP_ERR	0xFD	/* BDOS write protect error	*/

#define FAIL		0xFF

/* Start of transfer block sizes */
#define ENQ_BLOCK_SIZE			0x04
#define HEADER_BLOCK_SIZE		0x07

#define RECEIVE_MSG_SIZE_OFFSET		0x04
/* Command block sizes */
#define SEND_MSG_SIZE_OFFSET		0x04
#define NORMAL_DATA_SEND_SIZE		0x00
#define READ_DATA_SEND_SIZE		0x80
/* Generic block sizes*/
#define RET_CODE_TXT_BLOCK_SIZE		0x00
#define RET_CODE_TXT_BLOCK_SEND_SIZE	0x04

#define SEND_BUFFER_SIZE		0x100

void communicate(void);

#endif
