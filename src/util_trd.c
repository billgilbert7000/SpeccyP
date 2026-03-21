//---------------------------------------------------------------------------------
#include "util_z80.h"
#include "util_sd.h"
#include "ff.h"
#include <fcntl.h>
#include <string.h>
#include "zx_emu/Z80.h"
#include "zx_emu/zx_machine.h"
#include "screen_util.h"

#include "config.h"  

#include "fdi_stream.h"


//-----------------------------------------
/*
+00 (3) Ключевая метка 'FDI'.
+03 (1) Флаг защиты записи (0 - write enabled, 1 - write disabled).
+04 (2) Число цилиндров.
+06 (2) Число поверхностей.
+08 (2) Смещение текста (короткий комментарий к диску).
+0A (2) Смещение данных.
+0C (2) Длина дополнительной информации в заголовке. В этой версии = 0.
+0E "Длина дополнительной информации". Формат еще не определен (резерв для дальнейшей модернизации).
+0E + "длина дополнительной информации".
+0E+extra_len: [ЗАГОЛОВКИ ТРЕКОВ] ← НАЧАЛО ЗДЕСЬ!
*/
//-----------------------------------------
// Глобальный контекст
static fdi_stream_ctx_t fdi_ctx;
static fdi_file_io_t fdi_io = {
    .open = fatfs_open,
    .close = fatfs_close,
    .seek = fatfs_seek,
    .read = fatfs_read,
    .tell = fatfs_tell,
    .size = fatfs_size
};



// Инициализация
bool init_fdi(const char *filename) {
    fdi_stream_result_t res = fdi_stream_open(&fdi_ctx, &fdi_io, filename);
    if (res != FDI_STREAM_OK) {
     //   printf("FDI open error: %d\n", res);
        return false;
    }
    
  //  printf("FDI: %d cyls, %d heads\n", fdi_ctx.cylinders, fdi_ctx.heads);
  //  printf("Desc: %s\n", fdi_ctx.dsc);
    
    return true;
}
/*
// Чтение сектора для WD1793
bool read_sector(uint8_t track, uint8_t side, uint8_t sector_num, uint8_t *buffer) {
    // Загружаем трек (если не загружен или другой)
    static uint8_t last_track = 0xFF;
    static uint8_t last_side = 0xFF;
    
  //  if (last_track != track || last_side != side) 
	{
        if (fdi_stream_load_track(&fdi_ctx, track, side) != FDI_STREAM_OK) {
			
            return false;
        }
        last_track = track;
        last_side = side;
    }
    
    // Получаем информацию о секторе
    fdi_sector_info_t *sec = fdi_stream_get_sector(&fdi_ctx, sector_num);
    if (!sec) {
		
        return false;
    }
    
    // Читаем данные сектора
    if (fdi_stream_read_sector_data(&fdi_ctx, sec, buffer, sec->size) != FDI_STREAM_OK) {
        return false;
    }
    
    // Для эмуляции WD1793 важны флаги
    if (sec->flags & FDI_FL_DELETED_DATA) {
        // Сектор с удаленными данными (метка F8)
        // wd1793_set_deleted_data(true);
    }
    
    uint8_t crc_bit = 1 << (sec->n & 3);
    if (!(sec->flags & crc_bit)) {
        // Ошибка CRC
        // wd1793_set_crc_error(true);
    }
    
    return true;
}

// Получить размер сектора
uint16_t get_sector_size(uint8_t track, uint8_t side, uint8_t sector_num) {
    if (fdi_stream_load_track(&fdi_ctx, track, side) != FDI_STREAM_OK) {
        return 0;
    }
    
    fdi_sector_info_t *sec = fdi_stream_get_sector(&fdi_ctx, sector_num);
    return sec ? sec->size : 0;
}
*/
//##########################################################


// Info FDI
bool Read_Info_FDI(char *file_name, char *disk_name, bool open_file)
{
	CLEAR_INFO; // clear
	#define POS_X 24 + FONT_W * 14
    #define POS_Y 49

	int x = POS_X;
	int y = POS_Y;

// Открыть образ
       if (init_fdi(file_name)) 
	  {
    //  draw_text(24 + FONT_W * 14, 39, "FDI FILES TEST", CL_LT_RED, COLOR_BACKGOUND); y = y + FONT_H;
    //  draw_text(x,y,temp_msg+32,CL_GREEN ,CL_BLACK);y = y + FONT_H;
      snprintf(temp_msg,sizeof temp_msg,"FDI: %d cyls, %d heads", fdi_ctx.cylinders, fdi_ctx.heads);
      draw_text(x,y,temp_msg,CL_LT_BLUE ,CL_BLACK); y = y + FONT_H*2;
	  snprintf(temp_msg, sizeof temp_msg, fdi_ctx.dsc); 
      draw_text_len(x,y,temp_msg,CL_GREEN ,CL_BLACK,32);y = y + FONT_H;
      draw_text_len(x,y,temp_msg+32,CL_GREEN ,CL_BLACK,32);y = y + FONT_H*2;
      draw_text_len(x,y,"[ENTER] Mount to disk A:",CL_GRAY ,CL_BLACK,32);y = y + FONT_H*2;
	  draw_text_len(x,y,"[SPACE] Mount to disk A and run",CL_GRAY ,CL_BLACK,32);
	  fdi_stream_close(&fdi_ctx);
      return true;
	  }
	  fdi_stream_close(&fdi_ctx);
      draw_text(24 + FONT_W * 14, 39, "FDI ERROR", CL_LT_RED, COLOR_BACKGOUND);
      return false;
      // закрыть файл ошибка

	}
//----------
/*
Размер заголовка одного трека = 7 + (sectors_per_track * 7) байт
Где:
7 байт — фиксированный заголовок трека:
4 байта: смещение трека (track_offset)
2 байта: зарезервировано (всегда 0)
1 байт: количество секторов на треке (sectors_per_track)
sectors_per_track * 7 байт — массив описаний секторов:
Каждый сектор описывается 7 байтами (C, H, R, N, flags, data_offset)  

 Значение полей C, H, R, N
Поле	Название	Описание	Типичные значения
C	Cylinder	Номер цилиндра (дорожки)	0-79 (для 40/80 дорожечных дисков)
H	Head	Номер головки (стороны)	0 или 1
R	Record	Номер сектора на дорожке	1-16 (обычно)
N	Number	Код размера сектора	0,1,2,3 (см. таблицу ниже)

 Кодировка размера сектора (N)
N	Размер сектора	Применение
0	128 байт	Ранние 8-дюймовые диски, некоторые защищенные форматы
1	256 байт	Стандарт TR-DOS, большинство 5.25" дисков
2	512 байт	PC-совместимые дискеты, MS-DOS
3	1024 байт	PC-98, некоторые защищенные форматы
4	2048 байт	Редко, спец. форматы
5	4096 байт	Очень редко
// Размер сектора в байтах
uint16_t sector_size = 128 << N;  // 128 * (2^N)

В FDI файле для каждого сектора сохраняются:
sec_info[0] = C  // цилиндр
sec_info[1] = H  // головка
sec_info[2] = R  // номер сектора
sec_info[3] = N  // код размера
sec_info[4] = flags  // флаги (удаленные данные, CRC)
sec_info[5..6] = data_offset  // смещение данных сектора

*/

//------------------------------------------
// Чтение каталога все файлы
bool ReadCatalog(char *file_name, char *disk_name, bool open_file)
{
	FIL f;
	size_t bytesRead;
	int fd = 0;
	//	char fileinfo[30];
//	UINT bytesToRead;

	// размер каталога диcка trd  8*256 + 256 9 сектор 0 дорожки 2304 0x1000
	fd = f_open(&f, file_name, FA_READ); // открыть файл file_name на чтение
											   // printf("f_open=%d\n",fd);
	  //printf("TRD: %s\n",file_name);

	if (fd != FR_OK)
	{
		f_close(&f);
		return false;
	} // закрыть файл ошибка

	fd = f_read(&f, sd_buffer, 0x0900, &bytesRead); // прочитать 9 секторов в bufferIn
	if (fd != FR_OK)
	{
		f_close(&f);
		return false;
	} // ошибка
	//printf("bytesRead=%d\n", bytesRead);
	if (bytesRead != 0x0900)
	{
		f_close(&f);
		return false;
	} // ошибка
	  // memcpy(bufferOut,bufferIn,bytesRead);

	// всего может быть 2048/16= 128 файлов
#define POS_X 24 + FONT_W * 14
#define POS_Y 49

    CLEAR_INFO; // clear
    if (!(sd_buffer[2279] == 0x10))     // TR-DOS ID
{
      draw_text(24 + FONT_W * 14, 39, "NO TR-DOS or ERROR", CL_RED, COLOR_BACKGOUND);


return false;

}
	char symb[10];
	int x = POS_X;
	int y = POS_Y;
	int j = 0;
//	 int i = 44;// количество выводимых названий файлов
	int i =  sd_buffer[0x8e4]; // Количество файлов на диске, включая удаленные.
	if (i > 38)///////////////////////////////////////
		i = 38; // количество выводимых названий файлов

	

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
				y = y + FONT_H;
		}

        else
		
		{
				draw_text_len(x, y, symb, CL_CYAN, COLOR_BACKGOUND, 10);
				y = y + FONT_H;
		}



		}
		if (y >= POS_Y + 20 * FONT_H)//22
		{
			x = POS_X + 88;
			y = POS_Y;
		}

		i--;
		j = j + 16;
	}

	draw_text_len(24 + FONT_W * 14, 39, "DISK: ", CL_GREEN, COLOR_BACKGOUND, 8);
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
	draw_text_len(24 + FONT_W * 20, 39, symb, CL_YELLOW, COLOR_BACKGOUND, 8);

	f_close(&f);
	return true;
}
//------------------------------------------
// Чтение каталога только basic
/*
bool ReadCatalog_b(char *file_name, bool open_file)
{
	
	FIL f;
	size_t bytesRead;
	int fd = 0;
	//	char fileinfo[30];
//	UINT bytesToRead;

	//	memset(bufferOut, 0, 0x0900);

	// размер каталога диcка trd  8*256 + 256 9 сектор 0 дорожки 2304 0x1000
	fd = f_open(&f, file_name, FA_READ); // открыть файл file_name на чтение
											   // printf("f_open=%d\n",fd);
	//  printf("TRD: %s\n",file_name);

	if (fd != FR_OK)
	{
		f_close(&f);
		return false;
	} // закрыть файл ошибка

	fd = f_read(&f, sd_buffer, 0x0900, &bytesRead); // прочитать 9 секторов в bufferIn
	if (fd != FR_OK)
	{
		f_close(&f);
		return false;
	} // ошибка
	//printf("bytesRead=%d\n", bytesRead);
	if (bytesRead != 0x0900)
	{
		f_close(&f);
		return false;
	} // ошибка
	  // memcpy(bufferOut,bufferIn,bytesRead);

	// всего может быть 2048/16= 128 файлов
#define POS_X 24 + FONT_W * 14
#define POS_Y 49

	char symb[8];
	int x = POS_X;
	int y = POS_Y;
	int j = 0;
	// int i // количество выводимых названий файлов
	int i =  sd_buffer[0x8e4]; // Количество файлов на диске, включая удаленные.
	if (i > 40)///////////////////////////////////////
		i = 40; // количество выводимых названий файлов

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
		if (y >= POS_Y + 20 * FONT_H)//22
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

	f_close(&f);
	return true;
}
*/	
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
//uint8_t Drive =
 MenuBox_trd (24,60,34,7,"Drive TR-DOS",4,0,1);

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

   // uint8_t numberOfFiles;

    // Reset track 0 info
    memset(track0,0,0x0b00);
}

//------------------------------------------
// Чтение каталога SCL файлы
bool ReadCatalog_scl(char *file_name, char *disk_name, bool open_file)
{
	
	FIL f;
	size_t bytesRead;
	int fd = 0;
	//	char fileinfo[30];
	//UINT bytesToRead;

	uint8_t numberOfFiles;

	

	// размер каталога диcка trd  8*256 + 256 9 сектор 0 дорожки 2304 0x1000
	fd = f_open(&f, file_name, FA_READ); // открыть файл file_name на чтение
	if (fd != FR_OK)
	{
		f_close(&f);
		return false;
	} // закрыть файл ошибка

//
//unsigned char* track0;
    // Reset track 0 info
    memset(sd_buffer, 0, 0x0900); // обнуление буфера
// SCL to TRD
    f_lseek(&f,8);// смещаемся в файле на 8 байт SINCLAIR
    f_read(&f,&numberOfFiles,1,&bytesRead);// читаем 1 байт это количество файлов


    // Populate FAT.
    int startSector = 0;
    int startTrack = 1; // Since Track 0 is reserved for FAT and Disk Specification.

    uint8_t data;
    for (int i = 0; i < numberOfFiles; i++) {

        int n = i << 4;

        for (int j = 0; j < 13; j++) {
		//	printf("f_read: %d\n",j);
            f_read(&f,&data,1,&bytesRead);// чтение одног байта из файла &f
            sd_buffer[n + j] = data;
		//	printf("SCL file: %c\n",data);
			
        }
        

        f_read(&f,&data,1, &bytesRead); // чтение длины файла 1 байт
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
#define POS_Y 49

	char symb[10];
	int x = POS_X;
	int y = POS_Y;
	int j = 0;
	// int i = 44;// количество выводимых названий файлов
	int i =  sd_buffer[0x8e4]; // Количество файлов на диске, включая удаленные.
	if (i > 38)///////////////////////////////////////
		i = 38; // количество выводимых названий файлов


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
				y = y + FONT_H;
		}

        else
		
		{
				draw_text_len(x, y, symb, CL_CYAN, COLOR_BACKGOUND, 10);
				y = y + FONT_H;
		}



		}
		if (y >= POS_Y + 20 * FONT_H)//22
		{
			x = POS_X + 88;
			y = POS_Y;
		}

		i--;
		j = j + 16;
	}

	draw_text_len(24 + FONT_W * 14, 39, "DISK: ", CL_GREEN, COLOR_BACKGOUND, 8);
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
	draw_text_len(24 + FONT_W * 20, 39, symb, CL_YELLOW, COLOR_BACKGOUND, 8); // имя диска

	f_close(&f);
	return true;
}
// run SCL файлы
bool Run_file_scl(char *file_name,  bool open_file)
{    
	 file_type[0]= SCL;// scl     

    // Reset track 0 info
    memset(track0,0,2304);
  	FIL f;
	size_t  bytesRead;
	int fd = 0;
	//UINT bytesToRead;

	uint8_t numberOfFiles;

	// размер каталога диcка trd  8*256 + 256 9 сектор 0 дорожки 2304 0x1000
	fd = f_open(&f, file_name, FA_READ); // открыть файл file_name на чтение
											   // printf("f_open=%d\n",fd);
	  //printf("SCL: %s\n",file_name);

	if (fd != FR_OK)
	{
		f_close(&f);
		return false;
	} // закрыть файл ошибка


// SCL to TRD
    f_lseek(&f,8);// смещаемся в файле на 8 байт SINCLAIR
    f_read(&f,&numberOfFiles,1,&bytesRead);// читаем 1 байт это количество файлов


    // Populate FAT.
    int startSector = 0;
    int startTrack = 1; // Since Track 0 is reserved for FAT and Disk Specification.

    uint8_t data;
    for (int i = 0; i < numberOfFiles; i++) {

        int n = i << 4;

        for (int j = 0; j < 13; j++) {
		//	printf("f_read: %d\n",j);
            f_read(&f,&data,1,&bytesRead);// чтение одног байта из файла &f
            track0[n + j] = data;
		//	printf("SCL file: %c\n",data);
			
        }
        

        f_read(&f,&data,1,&bytesRead); // чтение длины файла 1 байт
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
    track0[2277] = freeSectors & 0x00ff;
    track0[2278] = freeSectors >> 8;
    track0[2279] = 0x10; // TR-DOS ID

    for (int i = 0; i < 9; i++) track0[0x08f5 + i] = 0x20;

    // Store the image file name in the disk label section of the Disk Specification.

//--------------------------------------------------------------------------
       f_close(&f);
     //    conf.FileAutorunType = 0;
		 OpenTRDFile(file_name, 0);
	     sclDataOffset =  (9 + (numberOfFiles * 14));	

	return true;
}
//==============================================================
