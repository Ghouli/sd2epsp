#ifndef _DRIVE_H_
 #define _DRIVE_H_
#include <stdint.h>

#define TRACKS		40
#define SECTORS		64
#define RECORD		128

struct disk_drive {
	char *image_name;
	uint8_t drive_number;
	uint8_t track;
	uint8_t sector;
	uint8_t write_protect;
	uint8_t return_code;
};
struct disk_unit {
	uint8_t unit_id;
	struct disk_drive disk_drives[2];
};

struct disk_unit disk_units[2];

void init_floppy_system(void);
void init_disk_drives(void);
uint8_t read_sector(struct disk_drive *drive, uint8_t *data);
uint8_t write_sector(struct disk_drive *drive, uint8_t *data);

#endif
