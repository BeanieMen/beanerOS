#include "ata.h"
#include "io.h"
#include <stdio.h>

static uint16_t ata_base = ATA_PRIMARY_IO;

static void ata_wait_bsy(void) {
    while (inb(ata_base + ATA_REG_STATUS) & ATA_SR_BSY);
}

static void ata_wait_drq(void) {
    while (!(inb(ata_base + ATA_REG_STATUS) & ATA_SR_DRQ));
}

void ata_init(void) {
    ata_base = ATA_PRIMARY_IO;
    printf("[ATA] Primary bus initialized at 0x%x\n", ata_base);
}

int ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    ata_wait_bsy();
    
    outb(ata_base + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ata_base + ATA_REG_SECCOUNT, sector_count);
    outb(ata_base + ATA_REG_LBA_LO, (uint8_t)lba);
    outb(ata_base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(ata_base + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(ata_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    for (uint8_t i = 0; i < sector_count; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        
        for (int j = 0; j < 256; j++) {
            uint16_t data = inw(ata_base + ATA_REG_DATA);
            buffer[i * 512 + j * 2] = (uint8_t)data;
            buffer[i * 512 + j * 2 + 1] = (uint8_t)(data >> 8);
        }
    }
    
    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t sector_count, const uint8_t *buffer) {
    ata_wait_bsy();
    
    outb(ata_base + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ata_base + ATA_REG_SECCOUNT, sector_count);
    outb(ata_base + ATA_REG_LBA_LO, (uint8_t)lba);
    outb(ata_base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(ata_base + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(ata_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    for (uint8_t i = 0; i < sector_count; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        
        for (int j = 0; j < 256; j++) {
            uint16_t data = buffer[i * 512 + j * 2] | 
                           (buffer[i * 512 + j * 2 + 1] << 8);
            outw(ata_base + ATA_REG_DATA, data);
        }
    }
    
    return 0;
}
