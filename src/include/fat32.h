#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

#define FAT32_MAX_FILES 128
#define FAT32_MAX_FILENAME 256

typedef struct {
    uint8_t jump[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed)) fat32_boot_sector_t;

typedef struct {
    char name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;

typedef struct {
    fat32_boot_sector_t boot_sector;
    uint32_t fat_start;
    uint32_t data_start;
    uint32_t root_dir_cluster;
    uint32_t current_cluster;
    char current_dir[256];
} fat32_fs_t;

int fat32_init(fat32_fs_t *fs);
int fat32_list_root(fat32_fs_t *fs);
int fat32_read_file(fat32_fs_t *fs, const char *filename, uint8_t **data, uint32_t *size);
int fat32_write_file(fat32_fs_t *fs, const char *filename, const uint8_t *data, uint32_t size);
int fat32_create_file(fat32_fs_t *fs, const char *filename);
int fat32_create_directory(fat32_fs_t *fs, const char *dirname);
int fat32_change_directory(fat32_fs_t *fs, const char *dirname);
int fat32_append_file(fat32_fs_t *fs, const char *filename, const uint8_t *data, uint32_t size);

#endif
