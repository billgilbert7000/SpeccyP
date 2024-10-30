#pragma once
#include <VFS.h>
#include "config.h"


//uint8_t dirs[DIRS_DEPTH][14];
const char dirs[DIRS_DEPTH][LENF];

//uint8_t files[MAX_FILES][14];
char files[MAX_FILES][LENF];

char dir_patch[DIRS_DEPTH*(LENF+2)];

char afilename[LENF];
char tape_file_name[DIRS_DEPTH*LENF]; // 400 // 5*16 96
//char sd_buffer[SD_BUFFER_SIZE]; //буфер для работы с файлами
uint8_t sd_buffer[SD_BUFFER_SIZE]; //буфер для работы с файлами
uint8_t error[0x0100]; 
bool init_fs;

bool init_filesystem(void);
const char* get_file_extension(const char *filename);
int get_files_from_dir(char *dir_name,char* nf_buf, int MAX_N_FILES);
int read_select_dir(int dir_index);
char* get_current_altname(char *dir_name,char *part_name);

int sd_open_file(FIL *file, char* file_name, BYTE mode);
// Returns a negative error code on failure.
//int lfs_file_open(lfs_t *lfs, lfs_file_t *file,const char *path, int flags);
        

int sd_read_file(FIL *file, void* buffer, unsigned int bytestoread, unsigned int* bytesreaded);
int sd_write_file(FIL *file,void* buffer, UINT bytestowrite, UINT* byteswrited);
int sd_seek_file(FIL *file, FSIZE_t offset);
int sd_close_file(FIL *file);
DWORD sd_file_pos(FIL *file);
DWORD sd_file_size(FIL *file);
int sd_mkdir(const TCHAR* path);

char* get_lfn_from_dir(char *dir_name,char *part_name);


