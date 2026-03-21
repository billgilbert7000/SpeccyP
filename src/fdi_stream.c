#include "fdi_stream.h"
#include "ff.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "config.h"  

// Чтение little-endian значений из буфера
static inline uint16_t read_le16(const uint8_t *p) {
    return p[0] | (p[1] << 8);
}

static inline uint32_t read_le32(const uint8_t *p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

// Открыть FDI файл (читает только заголовок)
fdi_stream_result_t fdi_stream_open(fdi_stream_ctx_t *ctx, fdi_file_io_t *io, const char *filename) {
    if (!ctx || !io || !filename) {
        return FDI_STREAM_ERR_INVALID_PARAMETERS;
    }
    
    memset(ctx, 0, sizeof(fdi_stream_ctx_t));
    ctx->io = *io;
    
    // Открываем файл
    ctx->file_handle = ctx->io.open(filename);
    if (!ctx->file_handle) {
        return FDI_STREAM_ERR_FILE_ERROR;
    }
    
    ctx->file_size = ctx->io.size(ctx->file_handle);
    
    // Читаем заголовок (14 байт)
    uint8_t hdr_buf[14];
    if (!ctx->io.seek(ctx->file_handle, 0) || 
        !ctx->io.read(ctx->file_handle, hdr_buf, 14)) {
        ctx->io.close(ctx->file_handle);
        return FDI_STREAM_ERR_FILE_ERROR;
    }
    
    const fdi_hdr_t *hdr = (const fdi_hdr_t*)hdr_buf;
    
    // Проверка сигнатуры
    if (hdr->sig[0] != 'F' || hdr->sig[1] != 'D' || hdr->sig[2] != 'I') {
        ctx->io.close(ctx->file_handle);
        return FDI_STREAM_ERR_INVALID_SIGNATURE;
    }
    
    // Чтение параметров
    ctx->cylinders = read_le16((const uint8_t*)&hdr->c);
    ctx->heads = read_le16((const uint8_t*)&hdr->h);
    ctx->write_protect = (hdr->rw != 0);
    
    uint16_t text_offset = read_le16((const uint8_t*)&hdr->text_offset);
    ctx->data_offset = read_le16((const uint8_t*)&hdr->data_offset);
    uint16_t add_len = read_le16((const uint8_t*)&hdr->add_len);
    
    // Проверка геометрии
    if (ctx->cylinders == 0 || ctx->cylinders > FDI_MAX_CYLS ||
        ctx->heads == 0 || ctx->heads > FDI_MAX_SIDES) {
        ctx->io.close(ctx->file_handle);
        return FDI_STREAM_ERR_INVALID_PARAMETERS;
    }
    
    // Позиция заголовков треков
    ctx->tracks_offset = 14 + add_len;
    
    // Чтение комментария
    if (text_offset > 0 && text_offset < ctx->file_size) {
        ctx->io.seek(ctx->file_handle, text_offset);
        ctx->io.read(ctx->file_handle, ctx->dsc, FDI_DSC_SIZE - 1);
        ctx->dsc[FDI_DSC_SIZE - 1] = '\0';
    }
    
    // Чтение дополнительной информации (FDI 2 с bad bytes)
    if (add_len >= 16) { // sizeof(fdi_add_info_t)
        uint8_t add_buf[16];
        ctx->io.seek(ctx->file_handle, 14);
        ctx->io.read(ctx->file_handle, add_buf, 16);
        
        const fdi_add_info_t *add_info = (const fdi_add_info_t*)add_buf;
        
        uint16_t ver = read_le16((const uint8_t*)&add_info->ver);
        uint16_t type = read_le16((const uint8_t*)&add_info->add_info_type);
        
        if (ver >= 2 && type == 1) {
            ctx->has_bad_bytes = true;
            // Пропускаем TrkAddInfoOffset и DataOffset (8 байт)
            uint32_t trk_add_offset = read_le32((const uint8_t*)&add_info->trk_add_info_offset);
            ctx->bad_data_offset = read_le32((const uint8_t*)&add_info->data_offset);
        }
    }
    
    return FDI_STREAM_OK;
}
//###################################################
// Таблица CRC для быстрого вычисления (pre-calculated)
static const uint16_t crc_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

// Быстрое вычисление CRC с таблицей
uint16_t wd93_crc_fast(const uint8_t *data, uint16_t size) {
    uint16_t crc = 0xCDB4;  // начальное значение для WD1793
    
    for (uint16_t i = 0; i < size; i++) {
        crc = (crc << 8) ^ crc_table[((crc >> 8) ^ data[i]) & 0xFF];
    }
    
    // В оригинале используется byteswap, но для CRC-16 это не обязательно
    // return crc;  // прямой порядок
    return (crc >> 8) | (crc << 8);  // byteswap как в оригинале
}
// Упрощенная версия для маленьких данных
uint16_t wd93_crc_simple(const uint8_t *data, uint16_t size) {
    uint16_t crc = 0xCDB4;
    
    for (uint16_t i = 0; i < size; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return (crc >> 8) | (crc << 8);  // byteswap
}
//###################################################
// Закрыть файл
void fdi_stream_close(fdi_stream_ctx_t *ctx) {
    if (ctx && ctx->file_handle) {
        ctx->io.close(ctx->file_handle);
        ctx->file_handle = NULL;
    }
    ctx->track_loaded = false;
}

// Загрузить информацию о треке (без данных секторов)
fdi_stream_result_t fdi_stream_load_track(fdi_stream_ctx_t *ctx, uint8_t cylinder, uint8_t head) {
    if (!ctx || !ctx->file_handle) {
        return FDI_STREAM_ERR_INVALID_PARAMETERS;
    }
    
    if (cylinder >= ctx->cylinders || head >= ctx->heads) {
        return FDI_STREAM_ERR_TRACK_NOT_FOUND;
    }
    
    uint32_t track_index = cylinder * ctx->heads + head;
    uint32_t current_pos = ctx->tracks_offset;
    
    // Сброс маппинга секторов
    memset(ctx->current_track.sector_map, 0xFF, sizeof(ctx->current_track.sector_map));
    
    // Поиск нужного трека
    for (uint32_t i = 0; i <= track_index; i++) {
        if (current_pos + 7 > ctx->file_size) { // минимум 7 байт для заголовка трека
            return FDI_STREAM_ERR_CORRUPTED;
        }
        
        // Читаем заголовок трека (первые 7 байт)
        uint8_t trk_hdr_buf[7];
        ctx->io.seek(ctx->file_handle, current_pos);
        ctx->io.read(ctx->file_handle, trk_hdr_buf, 7);
        
        uint32_t trk_offset = read_le32(trk_hdr_buf);
        uint8_t spt = trk_hdr_buf[6]; // sectors per track
        sectors_per_track = spt;
        
        if (i == track_index) {
            // Это наш трек - загружаем информацию о секторах
            ctx->current_track.cylinder = cylinder;
            ctx->current_track.head = head;
            ctx->current_track.sector_count = spt;
            ctx->current_track.track_data_offset = ctx->data_offset + trk_offset;
            
            // Читаем заголовки секторов
            uint32_t sec_info_pos = current_pos + 7;
            uint8_t sec_buf[7 * FDI_MAX_SEC_PER_TRACK];
            
            if (sec_info_pos + (spt * 7) > ctx->file_size) {
                return FDI_STREAM_ERR_CORRUPTED;
            }
            
            ctx->io.seek(ctx->file_handle, sec_info_pos);
            ctx->io.read(ctx->file_handle, sec_buf, spt * 7);
            
            // Парсим секторы
            for (uint8_t s = 0; s < spt; s++) {
                uint8_t *sec_ptr = sec_buf + (s * 7);
                fdi_sector_info_t *sec = &ctx->current_track.sectors[s];
                
                sec->c = sec_ptr[0];
                sec->h = sec_ptr[1];
                sec->r = sec_ptr[2];
                sec->n = sec_ptr[3];
                sec->flags = sec_ptr[4];
                sec->size = fdi_sector_size(sec->n);
                sec->has_data = !(sec->flags & FDI_FL_NO_DATA);
                
                uint16_t sec_data_offset = read_le16(sec_ptr + 5);
                sec->file_offset = ctx->current_track.track_data_offset + sec_data_offset;
                
                // Маппинг номера сектора -> индекс
                if (sec->r < 256) {
                    ctx->current_track.sector_map[sec->r] = s;
                }
                
                // По умолчанию нет сбойных байтов
                sec->has_bad_bytes = false;
            }
            
            ctx->track_loaded = true;
            return FDI_STREAM_OK;
        }
        
        // Переход к следующему треку
        current_pos += 7 + (spt * 7);
    }
    
    return FDI_STREAM_ERR_TRACK_NOT_FOUND;
}

// Получить информацию о секторе по номеру R (из текущего трека)
fdi_sector_info_t* fdi_stream_get_sector(fdi_stream_ctx_t *ctx, uint8_t sector_r) {
     if (!ctx || !ctx->track_loaded || sector_r >= 256) {
        
        return NULL;
    }  
    
    uint8_t idx = ctx->current_track.sector_map[sector_r];
    
    if (idx == 0xFF || idx >= ctx->current_track.sector_count) {
      //  gpio_put(LED_BOARD, 1);
        return &ctx->current_track.sectors[1];
       // return NULL;
    }  
    
    return &ctx->current_track.sectors[idx];
}

// Прочитать данные сектора (только когда нужны)
fdi_stream_result_t fdi_stream_read_sector_data(
    fdi_stream_ctx_t *ctx,
    fdi_sector_info_t *sector,
    uint8_t *buffer,
    uint16_t buffer_size
) {
    if (!ctx || !ctx->file_handle || !sector || !buffer) {
        return FDI_STREAM_ERR_INVALID_PARAMETERS;
    }
    
    if (!sector->has_data) {
        return FDI_STREAM_ERR_SECTOR_NOT_FOUND;
    }
    
    if (buffer_size < sector->size) {
        return FDI_STREAM_ERR_NO_MEMORY;
    }
    
    if (sector->file_offset + sector->size > ctx->file_size) {
        return FDI_STREAM_ERR_CORRUPTED;
    }
    
    // Читаем данные сектора
    ctx->io.seek(ctx->file_handle, sector->file_offset);
    if (!ctx->io.read(ctx->file_handle, buffer, sector->size)) {
        return FDI_STREAM_ERR_FILE_ERROR;
    }
    
    

    return FDI_STREAM_OK;
}

// Функция для чтения сбойных байтов (если нужны)
fdi_stream_result_t fdi_stream_read_bad_bytes(
    fdi_stream_ctx_t *ctx,
    fdi_sector_info_t *sector,
    uint8_t *bad_bitmap,
    uint16_t bitmap_size
) {
    if (!ctx || !sector || !sector->has_bad_bytes) {
        return FDI_STREAM_ERR_SECTOR_NOT_FOUND;
    }
    
    uint16_t bitmap_len = (sector->size + 7) / 8;
    if (bitmap_size < bitmap_len) {
        return FDI_STREAM_ERR_NO_MEMORY;
    }
    
    // Здесь нужно добавить поддержку чтения bad bytes из дополнительной секции
    // Для этого нужно сохранить bad_data_offset и прочитать битовую маску
    
    return FDI_STREAM_ERR_INVALID_PARAMETERS;
}

//#################################################################################
// Реализация файлового ввода-вывода для FatFS
void* fatfs_open(const char *filename) {
    FIL *fil = malloc(sizeof(FIL));
    if (!fil) return NULL;
    
    if (f_open(fil, filename, FA_READ) != FR_OK) {
        free(fil);
        return NULL;
    }
    return fil;
}

void fatfs_close(void *handle) {
    if (handle) {
        f_close((FIL*)handle);
        free(handle);
    }
}

bool fatfs_seek(void *handle, uint32_t pos) {
    return f_lseek((FIL*)handle, pos) == FR_OK;
}

bool fatfs_read(void *handle, void *buffer, uint32_t size) {
    UINT br;
    return (f_read((FIL*)handle, buffer, size, &br) == FR_OK && br == size);
}

uint32_t fatfs_tell(void *handle) {
    return f_tell((FIL*)handle);
}

uint32_t fatfs_size(void *handle) {
    return f_size((FIL*)handle);
}
//#################################################################################
