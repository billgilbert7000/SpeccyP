//---------------------------------------------------------------------------------
#include "util_z80.h"
#include "util_sd.h"
#include <VFS.h>
#include <fcntl.h>
#include <string.h>
#include <zx_emu/z80.h>
//#include "zx_emu/aySoundSoft.h"
#include "zx_emu/zx_machine.h"
#include "screen_util.h"

#include "config.h"




//------------------------------------------
// Чтение каталога все файлы
bool ReadCatalog(char *file_name, char *disk_name, bool open_file)
{
	FIL f;
	size_t bytesRead;
	int fd = 0;
	//	char fileinfo[30];
	UINT bytesToRead;

	// размер каталога диcка trd  8*256 + 256 9 сектор 0 дорожки 2304 0x1000
	fd = sd_open_file(&f, file_name, FA_READ); // открыть файл file_name на чтение
											   // printf("sd_open_file=%d\n",fd);
	  //printf("TRD: %s\n",file_name);

	if (fd != FR_OK)
	{
		sd_close_file(&f);
		return false;
	} // закрыть файл ошибка

	fd = sd_read_file(&f, sd_buffer, 0x0900, &bytesRead); // прочитать 9 секторов в bufferIn
	if (fd != FR_OK)
	{
		sd_close_file(&f);
		return false;
	} // ошибка
	//printf("bytesRead=%d\n", bytesRead);
	if (bytesRead != 0x0900)
	{
		sd_close_file(&f);
		return false;
	} // ошибка
	  // memcpy(bufferOut,bufferIn,bytesRead);

	// всего может быть 2048/16= 128 файлов
#define POS_X 24 + FONT_W * 14
#define POS_Y 52

    CLEAR_INFO; // clear
    if (!(sd_buffer[2279] == 0x10))     // TR-DOS ID
{
      draw_text(24 + FONT_W * 14, 52-16, "NO TR-DOS or ERROR", CL_RED, COLOR_BACKGOUND);


return false;

}
	char symb[10];
	int x = POS_X;
	int y = POS_Y;
	int j = 0;
//	 int i = 44;// количество выводимых названий файлов
	int i =  sd_buffer[0x8e4]; // Количество файлов на диске, включая удаленные.
	if (i > 44)///////////////////////////////////////
		i = 44; // количество выводимых названий файлов

	

	while (i != 0)
	{

		//  if (bufferOut[j] ==0) {i--; j = j+16; continue;}
       if (sd_buffer[j + 0] != 0x01){
		symb[0] = sd_buffer[j];
		symb[1] = sd_buffer[j + 1];
		symb[2] = sd_buffer[j + 2];
		symb[3] = sd_buffer[j + 3];
		symb[4] = sd_buffer[j + 4];
		symb[5] = sd_buffer[j + 5];
		symb[6] = sd_buffer[j + 6];
		symb[7] = sd_buffer[j + 7];
        symb[8] = '.';
        symb[9] = sd_buffer[j + 8];

     
		if (sd_buffer[j + 8] == 'B')
		{
				draw_text_len(x, y, symb, CL_GRAY, COLOR_BACKGOUND, 10);
				y = y + 8;
		}

        else
		
		{
				draw_text_len(x, y, symb, CL_CYAN, COLOR_BACKGOUND, 10);
				y = y + 8;
		}



		}
		if (y >= POS_Y + 22 * 8)
		{
			x = POS_X + 88;
			y = POS_Y;
		}

		i--;
		j = j + 16;
	}

	draw_text_len(24 + FONT_W * 14, 52-16, "DISK: ", CL_GREEN, COLOR_BACKGOUND, 8);
	// sprintf(fileinfo,"Type:.TRD - TR-DOS");
	//  системная информация 0x800 начало
	//  имя диска +245 0x8f5
	symb[0] = sd_buffer[0x8f5];
	symb[1] = sd_buffer[0x8f5 + 1];
	symb[2] = sd_buffer[0x8f5 + 2];
	symb[3] = sd_buffer[0x8f5 + 3];
	symb[4] = sd_buffer[0x8f5 + 4];
	symb[5] = sd_buffer[0x8f5 + 5];
	symb[6] = sd_buffer[0x8f5 + 6];
	symb[7] = sd_buffer[0x8f5 + 7];
	draw_text_len(24 + FONT_W * 20, 52-16, symb, CL_YELLOW, COLOR_BACKGOUND, 8);

	sd_close_file(&f);
	return true;
}
//------------------------------------------
// Чтение каталога только basic
bool ReadCatalog_b(char *file_name, bool open_file)
{
	
	FIL f;
	size_t bytesRead;
	int fd = 0;
	//	char fileinfo[30];
	UINT bytesToRead;

	//	memset(bufferOut, 0, 0x0900);

	// размер каталога диcка trd  8*256 + 256 9 сектор 0 дорожки 2304 0x1000
	fd = sd_open_file(&f, file_name, FA_READ); // открыть файл file_name на чтение
											   // printf("sd_open_file=%d\n",fd);
	//  printf("TRD: %s\n",file_name);

	if (fd != FR_OK)
	{
		sd_close_file(&f);
		return false;
	} // закрыть файл ошибка

	fd = sd_read_file(&f, sd_buffer, 0x0900, &bytesRead); // прочитать 9 секторов в bufferIn
	if (fd != FR_OK)
	{
		sd_close_file(&f);
		return false;
	} // ошибка
	//printf("bytesRead=%d\n", bytesRead);
	if (bytesRead != 0x0900)
	{
		sd_close_file(&f);
		return false;
	} // ошибка
	  // memcpy(bufferOut,bufferIn,bytesRead);

	// всего может быть 2048/16= 128 файлов
#define POS_X 24 + FONT_W * 14
#define POS_Y 52

	char symb[8];
	int x = POS_X;
	int y = POS_Y;
	int j = 0;
	// int i = 44;// количество выводимых названий файлов
	int i =  sd_buffer[0x8e4]; // Количество файлов на диске, включая удаленные.
	if (i > 44)///////////////////////////////////////
		i = 44; // количество выводимых названий файлов

	draw_rect(129, 16, 182, 24 * 8, COLOR_BACKGOUND, true); // clear




	while (i != 0)
	{

		//  if (bufferOut[j] ==0) {i--; j = j+16; continue;}
       if (sd_buffer[j + 0] != 0x01){
		symb[0] = sd_buffer[j];
		symb[1] = sd_buffer[j + 1];
		symb[2] = sd_buffer[j + 2];
		symb[3] = sd_buffer[j + 3];
		symb[4] = sd_buffer[j + 4];
		symb[5] = sd_buffer[j + 5];
		symb[6] = sd_buffer[j + 6];
		symb[7] = sd_buffer[j + 7];

     
		if (sd_buffer[j + 8] == 'B')
		{
				draw_text_len(x, y, symb, CL_GRAY, COLOR_BACKGOUND, 8);
				y = y + 8;
		}
		}
		if (y >= POS_Y + 22 * 8)
		{
			x = POS_X + 88;
			y = POS_Y;
		}

		i--;
		j = j + 16;
	}
	// sprintf(fileinfo,"Type:.TRD - TR-DOS");
	//  системная информация 0x800 начало
	//  имя диска +245 0x8f5
	symb[0] = sd_buffer[0x8f5];
	symb[1] = sd_buffer[0x8f5 + 1];
	symb[2] = sd_buffer[0x8f5 + 2];
	symb[3] = sd_buffer[0x8f5 + 3];
	symb[4] = sd_buffer[0x8f5 + 4];
	symb[5] = sd_buffer[0x8f5 + 5];
	symb[6] = sd_buffer[0x8f5 + 6];
	symb[7] = sd_buffer[0x8f5 + 7];
	draw_text_len(24 + FONT_W * 14, 16, symb, CL_CYAN, COLOR_BACKGOUND, 8);

	sd_close_file(&f);
	return true;
}
//-------------------------------------------------------------------------------
/* for (int x = 0; x < 2; x++)
	{
		for (int i = 0; i < 23; i++)
		{
			//  if (bufferOut[j-8] !='B') {i--; j = j+16; continue;}
			//  if (bufferOut[j] ==0) {i--; j = j+16; continue;}

			symb[0] = sd_buffer[j];
			symb[1] = sd_buffer[j + 1];
			symb[2] = sd_buffer[j + 2];
			symb[3] = sd_buffer[j + 3];
			symb[4] = sd_buffer[j + 4];
			symb[5] = sd_buffer[j + 5];
			symb[6] = sd_buffer[j + 6];
			symb[7] = sd_buffer[j + 7];

			j = j + 16;

			draw_text_len(24 + x * 88 + FONT_W * 14, 32 + i * 8, symb, CL_GRAY, COLOR_BACKGOUND, 8);
		}
	} */

void MenuTRDOS ()
{
uint8_t Drive = MenuBox_trd (24,60,34,7,"Drive TR-DOS",4,0,1);

   // Копируем строку длиною не более 10 символов из массива src в массив dst1. 
  // strncpy (dst1, src,3);
//strncpy(menu_trd[Drive], "la-la-la", 8);
//strcpy(menu_trd[1] + 2, "la-la-la"); 
//menu_trd[Drive]   conf.activefilename

/* if (Drive==5) return ;


// Копируем строку длиною не более 10 символов из массива src в массив dst1. 
// strncpy (dst1, src,3);

strncpy(conf.DiskName[Drive], current_lfn,16);

else OpenTRDFile(conf.activefilename,Drive);
return ;
 */
}
//----------------------------------------------------------------------------------
void SCLtoTRD(unsigned char* track0, unsigned char UnitNum) {

    uint8_t numberOfFiles;

    // Reset track 0 info
    memset(track0,0,0x0b00);
}

//------------------------------------------
// Чтение каталога SCL файлы
bool ReadCatalog_scl(char *file_name, char *disk_name, bool open_file)
//bool ReadCatalog_scl(char *file_name,  bool open_file)
{
	
	FIL f;
	size_t bytesRead;
	int fd = 0;
	//	char fileinfo[30];
	UINT bytesToRead;



	uint8_t numberOfFiles;

	

	// размер каталога диcка trd  8*256 + 256 9 сектор 0 дорожки 2304 0x1000
	fd = sd_open_file(&f, file_name, FA_READ); // открыть файл file_name на чтение
											   // printf("sd_open_file=%d\n",fd);
	  //printf("SCL: %s\n",file_name);

	if (fd != FR_OK)
	{
		sd_close_file(&f);
		return false;
	} // закрыть файл ошибка

//
//unsigned char* track0;
    // Reset track 0 info
    memset(sd_buffer, 0, 0x0900); // обнуление буфера
// SCL to TRD
    sd_seek_file(&f,8);// смещаемся в файле на 8 байт SINCLAIR
    sd_read_file(&f,&numberOfFiles,1,&bytesRead);// читаем 1 байт это количество файлов


    // Populate FAT.
    int startSector = 0;
    int startTrack = 1; // Since Track 0 is reserved for FAT and Disk Specification.

    uint8_t data;
    for (int i = 0; i < numberOfFiles; i++) {

        int n = i << 4;

        for (int j = 0; j < 13; j++) {
		//	printf("sd_read_file: %d\n",j);
            sd_read_file(&f,&data,1,&bytesRead);// чтение одног байта из файла &f
            sd_buffer[n + j] = data;
		//	printf("SCL file: %c\n",data);
			
        }
        

        sd_read_file(&f,&data,1, &bytesRead); // чтение длины файла 1 байт
        sd_buffer[n + 13] = data;

        // printf("File #: %d, Filelenght: %d\n",i + 1,(int)data);

        sd_buffer[n + 14] = (uint8_t)startSector;
        sd_buffer[n + 15] = (uint8_t)startTrack;

        int newStartTrack = (startTrack * 16 + startSector + data) / 16;
        startSector = (startTrack * 16 + startSector + data) - 16 * newStartTrack;
        startTrack = newStartTrack;
    
    }

       


    // Populate Disk Specification.
    sd_buffer[2273] = (uint8_t)startSector;
    sd_buffer[2274] = (uint8_t)startTrack;
    sd_buffer[2275] = 22; // Disk Type
    sd_buffer[2276] = (uint8_t)numberOfFiles; // File Count
    uint16_t freeSectors = 2560 - (startTrack << 4) + startSector;
    // printf("Free Sectors: %d\n",freeSectors);
    sd_buffer[2277] = freeSectors & 0x00ff;
    sd_buffer[2278] = freeSectors >> 8;
    sd_buffer[2279] = 0x10; // TR-DOS ID

    for (int i = 0; i < 9; i++) sd_buffer[2282 + i] = 0x20;

    // Store the image file name in the disk label section of the Disk Specification.


    for (int i = 0; i < 8; i++)
	{ if (disk_name[i] =='.') break;
     sd_buffer[2293 + i] = disk_name[i];
	}


	// всего может быть 2048/16= 128 файлов
#define POS_X 24 + FONT_W * 14
#define POS_Y 52

	char symb[10];
	int x = POS_X;
	int y = POS_Y;
	int j = 0;
	// int i = 44;// количество выводимых названий файлов
	int i =  sd_buffer[0x8e4]; // Количество файлов на диске, включая удаленные.
	if (i > 44)///////////////////////////////////////
		i = 44; // количество выводимых названий файлов


CLEAR_INFO;
	while (i != 0)
	{

		//  if (bufferOut[j] ==0) {i--; j = j+16; continue;}
       if (sd_buffer[j + 0] != 0x01){
		symb[0] = sd_buffer[j];
		symb[1] = sd_buffer[j + 1];
		symb[2] = sd_buffer[j + 2];
		symb[3] = sd_buffer[j + 3];
		symb[4] = sd_buffer[j + 4];
		symb[5] = sd_buffer[j + 5];
		symb[6] = sd_buffer[j + 6];
		symb[7] = sd_buffer[j + 7];
        symb[8] = '.';
        symb[9] = sd_buffer[j + 8];

     
		if (sd_buffer[j + 8] == 'B')
		{
				draw_text_len(x, y, symb, CL_GRAY, COLOR_BACKGOUND, 10);
				y = y + 8;
		}

        else
		
		{
				draw_text_len(x, y, symb, CL_CYAN, COLOR_BACKGOUND, 10);
				y = y + 8;
		}



		}
		if (y >= POS_Y + 22 * 8)
		{
			x = POS_X + 88;
			y = POS_Y;
		}

		i--;
		j = j + 16;
	}

	draw_text_len(24 + FONT_W * 14, 52-16, "DISK: ", CL_GREEN, COLOR_BACKGOUND, 8);
	// sprintf(fileinfo,"Type:.TRD - TR-DOS");
	//  системная информация 0x800 начало
	//  имя диска +245 0x8f5
	symb[0] = sd_buffer[0x8f5];
	symb[1] = sd_buffer[0x8f5 + 1];
	symb[2] = sd_buffer[0x8f5 + 2];
	symb[3] = sd_buffer[0x8f5 + 3];
	symb[4] = sd_buffer[0x8f5 + 4];
	symb[5] = sd_buffer[0x8f5 + 5];
	symb[6] = sd_buffer[0x8f5 + 6];
	symb[7] = sd_buffer[0x8f5 + 7];
	draw_text_len(24 + FONT_W * 20, 52-16, symb, CL_YELLOW, COLOR_BACKGOUND, 8); // имя диска

	sd_close_file(&f);
	return true;
}
// run SCL файлы
bool Run_file_scl(char *file_name,  bool open_file)
{    
	  
	 printf(" Run_file_scl\n");
	//unsigned char* track0;

    // Reset track 0 info
    memset(track0,0,2304);
  	FIL f;
	size_t  bytesRead;
	int fd = 0;
	UINT bytesToRead;

	uint8_t numberOfFiles;

	// размер каталога диcка trd  8*256 + 256 9 сектор 0 дорожки 2304 0x1000
	fd = sd_open_file(&f, file_name, FA_READ); // открыть файл file_name на чтение
											   // printf("sd_open_file=%d\n",fd);
	  //printf("SCL: %s\n",file_name);

	if (fd != FR_OK)
	{
		sd_close_file(&f);
		return false;
	} // закрыть файл ошибка


// SCL to TRD
    sd_seek_file(&f,8);// смещаемся в файле на 8 байт SINCLAIR
    sd_read_file(&f,&numberOfFiles,1,&bytesRead);// читаем 1 байт это количество файлов


    // Populate FAT.
    int startSector = 0;
    int startTrack = 1; // Since Track 0 is reserved for FAT and Disk Specification.

    uint8_t data;
    for (int i = 0; i < numberOfFiles; i++) {

        int n = i << 4;

        for (int j = 0; j < 13; j++) {
		//	printf("sd_read_file: %d\n",j);
            sd_read_file(&f,&data,1,&bytesRead);// чтение одног байта из файла &f
            track0[n + j] = data;
		//	printf("SCL file: %c\n",data);
			
        }
        

        sd_read_file(&f,&data,1,&bytesRead); // чтение длины файла 1 байт
        track0[n + 13] = data;

        // printf("File #: %d, Filelenght: %d\n",i + 1,(int)data);

        track0[n + 14] = (uint8_t)startSector;
        track0[n + 15] = (uint8_t)startTrack;

        int newStartTrack = (startTrack * 16 + startSector + data) / 16;
        startSector = (startTrack * 16 + startSector + data) - 16 * newStartTrack;
        startTrack = newStartTrack;
    
    }

  uint8_t boot_name[16] = {0x62, 0x6F, 0x6F, 0x74 ,0x20 ,0x20 ,0x20, 0x20, 0x42, 0xFC, 0x01 ,0xFC, 0x01, 0x02, 0x09,0x00};
  uint16_t s = (numberOfFiles << 4) ;
for (int i = 0; i < 16; i++) {
     track0[s + i] = boot_name[i];
}

    // Populate Disk Specification.
    track0[2273] = (uint8_t)startSector;
    track0[2274] = (uint8_t)startTrack;
    track0[2275] = 22; // Disk Type
    track0[2276] = (uint8_t)numberOfFiles+1; // File Count
    uint16_t freeSectors = 2560 - (startTrack << 4) + startSector;
    // printf("Free Sectors: %d\n",freeSectors);
    track0[2277] = freeSectors & 0x00ff;
    track0[2278] = freeSectors >> 8;
    track0[2279] = 0x10; // TR-DOS ID

    for (int i = 0; i < 9; i++) track0[0x08f5 + i] = 0x20;

    // Store the image file name in the disk label section of the Disk Specification.

//--------------------------------------------------------------------------
       sd_close_file(&f);
       conf.sclConverted = 0;
		//strncpy(conf.DiskName[Drive], current_lfn, 16);
		 OpenTRDFile(file_name, 0);

/* 
#if defined(TRDOS_0) 
 OpenTRDFile(file_name, 0);
#else
 load_image_TRDOS(file_name,0);
#endif
 */





	sclDataOffset =  (9 + (numberOfFiles * 14));	
    
 //printf("OpenTRDFile\n");
 //printf("track0[] = %d\n", track0[2275]);


	return true;
}
//==============================================================
