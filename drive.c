#include <string.h>
#include "protocol.h"
#include "partition.h"
#include "fat.h"
#include "fat_config.h"
#include "sd_raw.h"
#include "sd_raw_config.h"
#include "drive.h"

struct partition_struct* partition;
struct fat_fs_struct* fs;
struct fat_dir_entry_struct directory;
struct fat_dir_struct* dd;

static char disk_d[] = "DISKD.IMG";
static char disk_e[] = "DISKE.IMG";
static char disk_f[] = "DISKF.IMG";
static char disk_g[] = "DISKG.IMG";

static struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name);

uint8_t find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry)
{
	while(fat_read_dir(dd, dir_entry))
	{
		if(strcmp(dir_entry->long_name, name) == 0)
		{
			fat_reset_dir(dd);
			return 1;
		}
	}
	return 0;
}

struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name)
{
	struct fat_dir_entry_struct file_entry;
	if(!find_file_in_dir(fs, dd, name, &file_entry))
		return 0;

	return fat_open_file(fs, &file_entry);
}

void init_floppy_system(void)
{
	/* initialize sd card slot */
	sd_raw_init();
	/* open partition */
	partition = partition_open(sd_raw_read, sd_raw_read_interval,
		sd_raw_write, sd_raw_write_interval, -1);
	/* open filesystem */
	fs = fat_open(partition);
	/* open root directory */
	fat_get_dir_entry_of_path(fs, "/", &directory);
	dd = fat_open_dir(fs, &directory);
	/* init vdrives */
	init_disk_drives();
}

/* Two units with two drives each */
void init_disk_drives(void)
{
	int unit, drive;
	for (unit = 0; unit < 2; unit++) {
		disk_units[unit].unit_id = 0x31 + unit;
		for (drive = 0; drive < 2; drive++) {
			disk_units[unit].disk_drives[drive].drive_number = drive;
			disk_units[unit].disk_drives[drive].track = 0;
			disk_units[unit].disk_drives[drive].sector = 0;
			disk_units[unit].disk_drives[drive].write_protect = 0;
			disk_units[unit].disk_drives[drive].return_code = 0;
		}
	}
	disk_units[0].disk_drives[0].image_name = disk_d;
	disk_units[0].disk_drives[1].image_name = disk_e;
	disk_units[1].disk_drives[0].image_name = disk_f;
	disk_units[1].disk_drives[1].image_name = disk_g;
}

uint8_t write_sector(struct disk_drive *drive, uint8_t *data)
{
	int32_t start_location;
	start_location = drive->track * drive->sector * RECORD;
	/* Open file-image */
	struct fat_file_struct* fd = open_file_in_dir(fs, dd, drive->image_name);
	if (!fd) {
		drive->return_code = BDOS_WR_ERR;
		return FAIL;
	}
	/* Seek correct position (start_location is updated with new offset) */
	if (!fat_seek_file(fd, &start_location, FAT_SEEK_SET)) {
		drive->return_code = BDOS_WR_ERR;
		return FAIL;
	}
	/* Write 128 bytes */
	if(fat_write_file(fd, data, RECORD) != RECORD) {
		drive->return_code = BDOS_WR_ERR;
		return FAIL;
	}
	/* Close file */
	 fat_close_file(fd);
	 drive->return_code = OK;
	return OK;
}

uint8_t read_sector(struct disk_drive *drive, uint8_t *data)
{
	int32_t start_location;
	start_location = drive->track * drive->sector * RECORD;
	struct fat_file_struct* fd = open_file_in_dir(fs, dd, drive->image_name);
	if (!fd) {
		drive->return_code = BDOS_RD_ERR;
		return FAIL;
	}
	/* Seek correct position (start_location is updated with new offset) */
	if (!fat_seek_file(fd, &start_location, FAT_SEEK_SET)) {
		drive->return_code = BDOS_RD_ERR;
		return FAIL;	
	}
	/* Read 128 bytes to buffer */
	if(fat_read_file(fd, data, RECORD) != RECORD){
		drive->return_code = BDOS_RD_ERR;
		return FAIL;	
	}
	/* Close file */
	 fat_close_file(fd);
	drive->return_code = OK;
	return OK;
}
