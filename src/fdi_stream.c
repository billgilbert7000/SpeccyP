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
