#include "util_z80.h"
#include "util_sd.h"
#include "ff.h"
#include <fcntl.h>
#include <string.h>
#include "zx_emu/Z80.h"
#include "zx_emu/zx_machine.h"
#include "screen_util.h"

#include "config.h"  

#include "util_cpm.h"
#include "cpm.h"

#define POS_X 24 + FONT_W * 14
#define POS_Y 49

CPM_Image image;

bool cpm_read_diskheader(CPM_Image *image) {
    CPM_DiskHeader *diskheader = &(image->diskHeader);

    if (f_lseek(&(image->file), 0) != FR_OK ) return false;  //some error happened

    UINT br;
    int rd = f_read(&(image->file), diskheader, CPM_DISKHEADER_SZ, &br);
    if (rd != FR_OK) return false;        // some error happened

    uint8_t checksum=0x66;
    for (int i=0; i<CPM_DISKHEADER_SZ-1; i++) {
        checksum += diskheader->bytes[i];
    }

    if (checksum != diskheader->checksum) {
        //fprintf(stderr,"Checksum = %02x, claimed = %02x\n", checksum, diskheader->checksum);
        return false;
    }

    if (diskheader->boot_baseAddr != QUORUM_CPM_BOOT_BASE) {
        //fprintf(stderr, "Unknown CP/M Bootloader base address! %04x\n", diskheader->boot_baseAddr);
        return false;
    }

    image->cpm_sectorsz = 128 << diskheader->sec_size;
    //cpm_dirOffset = diskheader->os_sectors * cpm_sectorsz;
    image->cpm_dirOffset = diskheader->os_tracks * diskheader->sec_per_track * image->cpm_sectorsz;
    image->cpm_dirSize = diskheader->dir_entries * diskheader->dir_entry_sz;

    image->headerOk = true;

    return true;
}

bool cpm_readDirEntry(uint16_t entryIndex, CPM_DirEntry *entry, CPM_Image *image) {
    if (entryIndex > image->diskHeader.dir_entries) return false;
    uint32_t entryOffset = image->diskHeader.dir_entry_sz * entryIndex + image->cpm_dirOffset;

    if (f_lseek(&(image->file), entryOffset) != FR_OK) return false;  //some error happened

    UINT br;
    int rd = f_read(&(image->file), entry, CPM_DISKHEADER_SZ, &br);
    if (rd != FR_OK) return false;

    return true;
} 

bool cpm_openImage(char *filename, CPM_Image *image) {
    image->headerOk = false;

    int fd = f_open(&(image->file), filename, FA_READ);
    if (fd != FR_OK) { 
        //fprintf(stderr, "File not found: %s\n", filename); 
        return false; 
    }

    cpm_read_diskheader(image);

    return true;
}

// void cpm_print_entry(CPM_DirEntry *entry) {
//     char u = entry->user;
//     if (u<10) u+='0'; else u+='A'-10;

//     putchar('(');
//     putchar(u);
//     putchar(')');

//     for (int i=0; i<8; i++) {
//         uint8_t c = entry->name[i];
//         if (c < 0x20) c = '~';
//         putchar(c);
//     }
//     putchar('.');

//     for (int i=0; i<3; i++) {
//         uint8_t c = entry->type[i];
//         if (c < 0x20) c = '~';
//         putchar(c & 0x7f);
//     }

//     putchar(' ');
//     putchar(entry->type[0] & 0x80 ? 'R':'-');
//     putchar(entry->type[1] & 0x80 ? 'S':'-');
//     putchar(entry->type[2] & 0x80 ? 'X':'-');

//     printf(" EX=%d S1=%d S2=%d RC=%d", entry->ex, entry->s1, entry->s2, entry->recordsCount);

//     printf("\n");
// }

bool ReadCPMDir(char *file_name,char *disk_name, bool open_file)
{
    CPM_Image image;
    CPM_DirEntry entry;

    CLEAR_INFO;
    
    if (!cpm_openImage(file_name, &image)) {
        draw_text(24 + FONT_W * 14, 39, "NO CP/M or ERROR", CL_RED, COLOR_BACKGOUND);

        return false;
    }

	draw_text_len(24 + FONT_W * 14, 39, "DISK: ", CL_GREEN, COLOR_BACKGOUND, 8);

    int x = POS_X;
	int y = POS_Y;

    int e = 0;
    
    while (1) {
        bool result = cpm_readDirEntry(e++, &entry, &image);
        if (!result) break;

        if (entry.ex) continue;
        if (entry.user == 0xe5) continue; //deleted file

        char u = entry.user;
        if (u<10) u+='0'; else u+='A'-10;

          temp_msg[0] = '/';
          temp_msg[1] = u;
          temp_msg[2] = '/';

        int p=3;

        // Имя файла (8 символов)
        for (int i = 0; i < 8; i++) {
            temp_msg[p++] = entry.name[i] & 0x7f;
        }    
        temp_msg[p++] = '.'; 
        // Расширение (3 символа)
        for (int i = 0; i < 3; i++) {
            temp_msg[p++] = entry.type[i] & 0x7f;
        }
        temp_msg[p] = 0;

       draw_text_len(x, y, temp_msg, CL_GRAY, COLOR_BACKGOUND, 16);
        y += FONT_H;

   		if (y >= POS_Y + 20 * FONT_H)//22
		{
			x += FONT_W*18;
			y = POS_Y;
		}

    }

    f_close(&image.file);
    return true;
}
