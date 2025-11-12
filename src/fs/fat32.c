#include "fat32.h"
#include "ata.h"
#include "kheap.h"
#include <stdio.h>
#include <string.h>

static uint8_t sector_buffer[512];

static uint32_t cluster_to_lba(fat32_fs_t *fs, uint32_t cluster) {
    return fs->data_start + ((cluster - 2) * fs->boot_sector.sectors_per_cluster);
}

int fat32_init(fat32_fs_t *fs) {
    if (ata_read_sectors(0, 1, sector_buffer) != 0) {
        printf("[FAT32] Failed to read boot sector\n");
        return -1;
    }
    
    memcpy(&fs->boot_sector, sector_buffer, sizeof(fat32_boot_sector_t));
    
    fs->fat_start = fs->boot_sector.reserved_sectors;
    fs->data_start = fs->fat_start + (fs->boot_sector.fat_count * fs->boot_sector.sectors_per_fat_32);
    fs->root_dir_cluster = fs->boot_sector.root_cluster;
    fs->current_cluster = fs->root_dir_cluster;
    fs->current_dir[0] = '/';
    fs->current_dir[1] = '\0';
    
    printf("[FAT32] Initialized\n");
    printf("  Bytes/sector: %d\n", fs->boot_sector.bytes_per_sector);
    printf("  Sectors/cluster: %d\n", fs->boot_sector.sectors_per_cluster);
    printf("  Root cluster: %d\n", fs->root_dir_cluster);
    
    uint32_t lba = cluster_to_lba(fs, fs->root_dir_cluster);
    uint8_t cluster_buf[4096];
    if (ata_read_sectors(lba, fs->boot_sector.sectors_per_cluster, cluster_buf) == 0) {
        fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buf;
        int max_entries = (fs->boot_sector.sectors_per_cluster * 512) / sizeof(fat32_dir_entry_t);
        
        for (int i = 0; i < max_entries; i++) {
            if (entries[i].name[0] == 0x00) break;
            if ((unsigned char)entries[i].name[0] == 0xE5) continue;
            
            uint32_t cluster = ((uint32_t)entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            if (cluster >= 3 && cluster < 1000) {
                uint32_t fat_sector = fs->fat_start + (cluster * 4) / 512;
                uint32_t fat_offset = (cluster * 4) % 512;
                uint8_t fat_buffer[512];
                
                if (ata_read_sectors(fat_sector, 1, fat_buffer) == 0) {
                    uint32_t *fat_entry = (uint32_t *)(fat_buffer + fat_offset);
                    if ((*fat_entry & 0x0FFFFFFF) == 0) {
                        *fat_entry = 0x0FFFFFFF;
                        ata_write_sectors(fat_sector, 1, fat_buffer);
                    }
                }
            }
        }
    }
    
    return 0;
}

static uint32_t fat32_allocate_cluster(fat32_fs_t *fs) {
    static uint32_t next_free = 3;
    uint8_t fat_buffer[512];
    
    for (uint32_t search = 0; search < 1024; search++) {
        uint32_t cluster = next_free + search;
        uint32_t fat_sector = fs->fat_start + (cluster * 4) / 512;
        uint32_t fat_offset = (cluster * 4) % 512;
        
        if (ata_read_sectors(fat_sector, 1, fat_buffer) != 0) {
            continue;
        }
        
        uint32_t *fat_entry = (uint32_t *)(fat_buffer + fat_offset);
        if ((*fat_entry & 0x0FFFFFFF) == 0) {
            *fat_entry = 0x0FFFFFFF;
            ata_write_sectors(fat_sector, 1, fat_buffer);
            next_free = cluster + 1;
            return cluster;
        }
    }
    
    uint32_t allocated = next_free++;
    uint32_t fat_sector = fs->fat_start + (allocated * 4) / 512;
    uint32_t fat_offset = (allocated * 4) % 512;
    if (ata_read_sectors(fat_sector, 1, fat_buffer) == 0) {
        uint32_t *fat_entry = (uint32_t *)(fat_buffer + fat_offset);
        *fat_entry = 0x0FFFFFFF;
        ata_write_sectors(fat_sector, 1, fat_buffer);
    }
    return allocated;
}

static void fat32_format_filename(const char *fat_name, char *output) {
    int j = 0;
    
    for (int i = 0; i < 8 && fat_name[i] != ' '; i++) {
        output[j++] = fat_name[i];
    }
    
    if (fat_name[8] != ' ') {
        output[j++] = '.';
        for (int i = 8; i < 11 && fat_name[i] != ' '; i++) {
            output[j++] = fat_name[i];
        }
    }
    
    output[j] = '\0';
}

static int fat32_match_filename(const char *fat_name, const char *search_name) {
    char formatted[13];
    fat32_format_filename(fat_name, formatted);
    
    int i = 0;
    while (formatted[i] && search_name[i]) {
        char c1 = formatted[i];
        char c2 = search_name[i];
        if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
        if (c1 != c2) return 0;
        i++;
    }
    return formatted[i] == search_name[i];
}

static void fat32_create_fat_name(const char *filename, char *fat_name) {
    int i, j = 0;
    
    memset(fat_name, ' ', 11);
    
    for (i = 0; i < 8 && filename[j] && filename[j] != '.'; i++, j++) {
        fat_name[i] = filename[j];
    }
    
    if (filename[j] == '.') {
        j++;
        for (i = 8; i < 11 && filename[j]; i++, j++) {
            fat_name[i] = filename[j];
        }
    }
}

int fat32_list_root(fat32_fs_t *fs) {
    uint32_t lba = cluster_to_lba(fs, fs->current_cluster);
    
    if (ata_read_sectors(lba, fs->boot_sector.sectors_per_cluster, sector_buffer) != 0) {
        return -1;
    }
    
    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)sector_buffer;
    int count = 0;
    
    printf("Directory contents:\n");
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00) break;
        if ((unsigned char)entries[i].name[0] == 0xE5) continue;
        if (entries[i].attributes & 0x08) continue;
        if (entries[i].attributes & 0x0F) continue;
        
        char filename[256];
        fat32_format_filename(entries[i].name, filename);
        
        if (entries[i].attributes & 0x10) {
            printf("  [DIR]  %s\n", filename);
        } else {
            printf("  %s (%d bytes)\n", filename, entries[i].file_size);
        }
        count++;
    }
    
    return count;
}

int fat32_read_file(fat32_fs_t *fs, const char *filename, uint8_t **data, uint32_t *size) {
    uint32_t lba = cluster_to_lba(fs, fs->current_cluster);
    uint32_t sectors_per_cluster = fs->boot_sector.sectors_per_cluster;
    uint8_t *cluster_buffer = (uint8_t *)kmalloc(sectors_per_cluster * 512);
    if (!cluster_buffer) return -1;
    
    if (ata_read_sectors(lba, sectors_per_cluster, cluster_buffer) != 0) {
        kfree(cluster_buffer);
        return -1;
    }
    
    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buffer;
    int max_entries = (sectors_per_cluster * 512) / sizeof(fat32_dir_entry_t);
    
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] == 0x00) break;
        if ((unsigned char)entries[i].name[0] == 0xE5) continue;
        
        if (fat32_match_filename(entries[i].name, filename)) {
            uint32_t cluster = ((uint32_t)entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            *size = entries[i].file_size;
            
            if (cluster == 0 || *size == 0) {
                *data = NULL;
                kfree(cluster_buffer);
                return 0;
            }
            
            uint32_t file_lba = cluster_to_lba(fs, cluster);
            uint32_t sectors = (*size + 511) / 512;
            if (sectors > fs->boot_sector.sectors_per_cluster) sectors = fs->boot_sector.sectors_per_cluster;
            
            uint8_t *temp_read = (uint8_t *)kmalloc(sectors * 512);
            if (!temp_read) {
                kfree(cluster_buffer);
                return -1;
            }
            
            if (ata_read_sectors(file_lba, sectors, temp_read) != 0) {
                kfree(temp_read);
                kfree(cluster_buffer);
                return -1;
            }
            
            *data = (uint8_t *)kmalloc(*size + 1);
            if (!*data) {
                kfree(temp_read);
                kfree(cluster_buffer);
                return -1;
            }
            
            memcpy(*data, temp_read, *size);
            (*data)[*size] = '\0';
            kfree(temp_read);
            kfree(cluster_buffer);
            return 0;
        }
    }
    
    kfree(cluster_buffer);
    return -1;
}

int fat32_write_file(fat32_fs_t *fs, const char *filename, const uint8_t *data, uint32_t size) {
    uint32_t lba = cluster_to_lba(fs, fs->current_cluster);
    uint32_t sectors_per_cluster = fs->boot_sector.sectors_per_cluster;
    uint8_t *cluster_buffer = (uint8_t *)kmalloc(sectors_per_cluster * 512);
    if (!cluster_buffer) return -1;
    
    if (ata_read_sectors(lba, sectors_per_cluster, cluster_buffer) != 0) {
        kfree(cluster_buffer);
        return -1;
    }
    
    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buffer;
    int max_entries = (sectors_per_cluster * 512) / sizeof(fat32_dir_entry_t);
    int slot = -1;
    uint32_t existing_cluster = 0;
    
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] == 0x00 || (unsigned char)entries[i].name[0] == 0xE5) {
            if (slot == -1) slot = i;
            continue;
        }
        
        if (fat32_match_filename(entries[i].name, filename)) {
            slot = i;
            existing_cluster = ((uint32_t)entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            break;
        }
    }
    
    if (slot == -1) {
        kfree(cluster_buffer);
        return -1;
    }
    
    uint32_t data_cluster = existing_cluster ? existing_cluster : fat32_allocate_cluster(fs);
    
    memset(&entries[slot], 0, sizeof(fat32_dir_entry_t));
    
    fat32_create_fat_name(filename, entries[slot].name);
    
    entries[slot].attributes = 0x20;
    entries[slot].file_size = size;
    entries[slot].first_cluster_low = (uint16_t)(data_cluster & 0xFFFF);
    entries[slot].first_cluster_high = (uint16_t)(data_cluster >> 16);
    
    ata_write_sectors(lba, sectors_per_cluster, cluster_buffer);
    kfree(cluster_buffer);
    
    if (data && size > 0) {
        uint32_t data_lba = cluster_to_lba(fs, data_cluster);
        uint32_t sectors = (size + 511) / 512;
        
        uint8_t *write_buffer = (uint8_t*)kmalloc(sectors * 512);
        memset(write_buffer, 0, sectors * 512);
        memcpy(write_buffer, data, size);
        
        ata_write_sectors(data_lba, sectors, write_buffer);
        kfree(write_buffer);
    }
    
    return 0;
}

int fat32_create_file(fat32_fs_t *fs, const char *filename) {
    return fat32_write_file(fs, filename, NULL, 0);
}

int fat32_create_directory(fat32_fs_t *fs, const char *dirname) {
    uint32_t lba = cluster_to_lba(fs, fs->current_cluster);
    uint32_t sectors_per_cluster = fs->boot_sector.sectors_per_cluster;
    uint8_t *cluster_buffer = (uint8_t *)kmalloc(sectors_per_cluster * 512);
    if (!cluster_buffer) return -1;
    
    if (ata_read_sectors(lba, sectors_per_cluster, cluster_buffer) != 0) {
        kfree(cluster_buffer);
        return -1;
    }
    
    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buffer;
    int max_entries = (sectors_per_cluster * 512) / sizeof(fat32_dir_entry_t);
    int slot = -1;
    
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] == 0x00 || (unsigned char)entries[i].name[0] == 0xE5) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        kfree(cluster_buffer);
        return -1;
    }
    
    uint32_t dir_cluster = fat32_allocate_cluster(fs);
    
    memset(&entries[slot], 0, sizeof(fat32_dir_entry_t));
    
    fat32_create_fat_name(dirname, entries[slot].name);
    
    entries[slot].attributes = 0x10;
    entries[slot].file_size = 0;
    entries[slot].first_cluster_low = (uint16_t)(dir_cluster & 0xFFFF);
    entries[slot].first_cluster_high = (uint16_t)(dir_cluster >> 16);
    
    ata_write_sectors(lba, sectors_per_cluster, cluster_buffer);
    kfree(cluster_buffer);
    
    uint8_t *new_dir_buffer = (uint8_t *)kmalloc(sectors_per_cluster * 512);
    if (new_dir_buffer) {
        memset(new_dir_buffer, 0, sectors_per_cluster * 512);
        uint32_t new_dir_lba = cluster_to_lba(fs, dir_cluster);
        ata_write_sectors(new_dir_lba, sectors_per_cluster, new_dir_buffer);
        kfree(new_dir_buffer);
    }
    
    return 0;
}

int fat32_change_directory(fat32_fs_t *fs, const char *dirname) {
    if (strcmp(dirname, "/") == 0) {
        fs->current_cluster = fs->root_dir_cluster;
        fs->current_dir[0] = '/';
        fs->current_dir[1] = '\0';
        return 0;
    }
    
    if (strcmp(dirname, "..") == 0) {
        if (strcmp(fs->current_dir, "/") != 0) {
            fs->current_cluster = fs->root_dir_cluster;
            fs->current_dir[0] = '/';
            fs->current_dir[1] = '\0';
        }
        return 0;
    }
    
    uint32_t lba = cluster_to_lba(fs, fs->current_cluster);
    uint32_t sectors_per_cluster = fs->boot_sector.sectors_per_cluster;
    uint8_t *cluster_buffer = (uint8_t *)kmalloc(sectors_per_cluster * 512);
    if (!cluster_buffer) return -1;
    
    if (ata_read_sectors(lba, sectors_per_cluster, cluster_buffer) != 0) {
        kfree(cluster_buffer);
        return -1;
    }
    
    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buffer;
    int max_entries = (sectors_per_cluster * 512) / sizeof(fat32_dir_entry_t);
    
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] == 0x00) break;
        if ((unsigned char)entries[i].name[0] == 0xE5) continue;
        if (!(entries[i].attributes & 0x10)) continue;
        
        if (fat32_match_filename(entries[i].name, dirname)) {
            uint32_t cluster = ((uint32_t)entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            fs->current_cluster = cluster;
            
            size_t len = strlen(fs->current_dir);
            if (len < sizeof(fs->current_dir) - 1) {
                fs->current_dir[len] = '/';
                len++;
            }
            for (size_t k = 0; dirname[k] && len < sizeof(fs->current_dir) - 1; k++, len++) {
                fs->current_dir[len] = dirname[k];
            }
            fs->current_dir[len] = '\0';
            
            kfree(cluster_buffer);
            return 0;
        }
    }
    
    kfree(cluster_buffer);
    return -1;
}

int fat32_append_file(fat32_fs_t *fs, const char *filename, const uint8_t *data, uint32_t size) {
    (void)fs;
    (void)filename;
    (void)data;
    (void)size;
    return -1;
}
