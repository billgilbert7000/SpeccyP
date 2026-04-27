#ifndef FDI_STREAM_H
#define FDI_STREAM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Константы
#define FDI_MAX_CYLS 256
#define FDI_MAX_SIDES 2
#define FDI_MAX_SEC_PER_TRACK 16
#define FDI_DSC_SIZE 64

// Флаги сектора
#define FDI_FL_DELETED_DATA 0x80
#define FDI_FL_NO_DATA      0x40
#define FDI_FL_GOOD_CRC_4096 0x20
#define FDI_FL_GOOD_CRC_2048 0x10
#define FDI_FL_GOOD_CRC_1024 0x08
#define FDI_FL_GOOD_CRC_512  0x04
#define FDI_FL_GOOD_CRC_256  0x02
#define FDI_FL_GOOD_CRC_128  0x01

// Структуры заголовков (packed)
#pragma pack(push, 1)
typedef struct {
    uint8_t sig[3];        // 'FDI'
    uint8_t rw;            // write protect
    uint16_t c;            // cylinders (LE)
    uint16_t h;            // heads (LE)
    uint16_t text_offset;  // text offset (LE)
    uint16_t data_offset;  // data offset (LE)
    uint16_t add_len;      // additional info length (LE)
} fdi_hdr_t;

typedef struct {
    uint8_t c;
    uint8_t h;
    uint8_t r;
    uint8_t n;
    uint8_t fl;
    uint16_t crc;
    uint16_t data_offset;  // LE
} fdi_sec_hdr_t;

typedef struct {
    uint32_t trk_offset;   // LE
    uint16_t res1;
    uint8_t spt;
    // fdi_sec_hdr_t sec[];
} fdi_trk_hdr_t;

typedef struct {
    uint16_t ver;          // LE
    uint16_t add_info_type;// LE
    uint32_t trk_add_info_offset; // LE
    uint32_t data_offset;  // LE
} fdi_add_info_t;

typedef struct {
    uint8_t flags;
    uint16_t data_offset;  // LE
} fdi_sec_add_info_t;

typedef struct {
    uint32_t trk_offset;   // LE
    // fdi_sec_add_info_t sec[];
} fdi_trk_add_info_t;
#pragma pack(pop)

// Информация о секторе (без данных)
typedef struct {
    uint8_t c;             // cylinder
    uint8_t h;             // head
    uint8_t r;             // sector number
    uint8_t n;             // size code
    uint8_t flags;         // flags
    uint32_t file_offset;  // абсолютное смещение данных в файле
    uint16_t size;         // размер данных
    bool has_data;
    
    // Для сбойных байтов
    bool has_bad_bytes;
    uint32_t bad_file_offset; // смещение битовой маски сбойных байтов
    uint16_t bad_bit_count;
} fdi_sector_info_t;

// Информация о треке
typedef struct {
    uint8_t cylinder;
    uint8_t head;
    uint8_t sector_count;
    uint32_t track_data_offset; // смещение данных трека в файле
    fdi_sector_info_t sectors[FDI_MAX_SEC_PER_TRACK];
    uint8_t sector_map[256];     // mapping R -> index
} fdi_track_info_t;

// Интерфейс файлового ввода-вывода (должен быть реализован пользователем)
typedef struct {
    void *(*open)(const char *filename);
    void (*close)(void *handle);
    bool (*seek)(void *handle, uint32_t pos);
    bool (*read)(void *handle, void *buffer, uint32_t size);
    uint32_t (*tell)(void *handle);
    uint32_t (*size)(void *handle);
} fdi_file_io_t;

// Основной контекст
typedef struct {
    // Геометрия диска
    uint16_t cylinders;
    uint8_t heads;
    bool write_protect;
    uint32_t data_offset;
    
    // Описание диска
    char dsc[FDI_DSC_SIZE];
    
    // Информация о сбойных байтах
    bool has_bad_bytes;
    uint32_t bad_data_offset;
    
    // Позиции в файле
    uint32_t tracks_offset;    // смещение начала заголовков треков
    uint32_t file_size;
    
    // Текущий загруженный трек
    fdi_track_info_t current_track;
    bool track_loaded;
    
    // Файловый ввод-вывод
    fdi_file_io_t io;
    void *file_handle;
} fdi_stream_ctx_t;

// Результаты операций
typedef enum {
    FDI_STREAM_OK = 0,
    FDI_STREAM_ERR_INVALID_SIGNATURE,
    FDI_STREAM_ERR_INVALID_PARAMETERS,
    FDI_STREAM_ERR_FILE_ERROR,
    FDI_STREAM_ERR_CORRUPTED,
    FDI_STREAM_ERR_TRACK_NOT_FOUND,
    FDI_STREAM_ERR_SECTOR_NOT_FOUND,
    FDI_STREAM_ERR_NO_MEMORY
} fdi_stream_result_t;

// Функции
fdi_stream_result_t fdi_stream_open(fdi_stream_ctx_t *ctx, fdi_file_io_t *io, const char *filename);
void fdi_stream_close(fdi_stream_ctx_t *ctx);
fdi_stream_result_t fdi_stream_load_track(fdi_stream_ctx_t *ctx, uint8_t cylinder, uint8_t head);
fdi_sector_info_t* fdi_stream_get_sector(fdi_stream_ctx_t *ctx, uint8_t sector_r);
fdi_stream_result_t fdi_stream_read_sector_data(fdi_stream_ctx_t *ctx, fdi_sector_info_t *sector, uint8_t *buffer, uint16_t buffer_size);

//uint16_t wd93_crc_simple(const uint8_t *data, uint16_t size);
//uint16_t wd93_crc_fast(const uint8_t *data, uint16_t size);
// Вспомогательные функции
static inline uint16_t fdi_sector_size(uint8_t n) {
    return 128 << (n & 3);
}
//---------------------------------------------------------
void* fatfs_open(const char *filename);
void fatfs_close(void *handle);
bool fatfs_seek(void *handle, uint32_t pos);
bool fatfs_read(void *handle, void *buffer, uint32_t size);
uint32_t fatfs_tell(void *handle);
uint32_t fatfs_size(void *handle);  
//---------------------------------------------------------
#endif // FDI_STREAM_H