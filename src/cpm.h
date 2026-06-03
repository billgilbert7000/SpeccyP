#pragma once
#include <stdint.h>
#include <assert.h>
typedef enum {
    SEC_SIZE_128B   = 0x00,
    SEC_SIZE_256B   = 0x01,
    SEC_SIZE_512B   = 0x02,
    SEC_SIZE_1024B  = 0x03
} CPM_SectorSize;

typedef enum {
    MEDIUM_DENSITY_SD = 0,
    MEDIUM_DENSITY_DD = 1,
    MEDIUM_DENSITY_HD = 2,
} CPM_MediumDensity;

typedef enum {
    TRACKS_DENSITY_48TPI = 0,
    TRACKS_DENSITY_96TPI = 1,
} CPM_TracksDensity;

typedef union {
    struct __attribute ((packed)) {
        uint16_t    boot_baseAddr;  //  Базовый адрес загрузчика ОС в RAM       (0xa880)
        uint16_t    boot_entry;     //  Стартовый адрес выполнения загрузчика   (0xa980)
        uint16_t    os_sectors;     //  Количество секторов, зарезервированных  (0x0014)
                                    //  под ОС CP/M.
        uint8_t     medium_type;    //  0x00 = 5.25"                            (0x00)
        uint8_t     medium_density; //  0x00 = SD, 0x01 = DD                    (0x01)
        uint8_t     tracks_density; //  0x00 = 48tpi (40 дорожек), 0x01 = 80tpi (0x01)
        uint8_t     sec_translating;//  Перенумеровывание секторов для ускорения(0x01)      ???
                                    //  доступа.
                                    //  0x01 = no translation; 
                                    //  0x00 = translation vector used
        uint8_t     sec_size;       //  Размер сектора                          (0x03)
                                    //  0x00 = 128B, 0x01 = 256B, 0x02 = 512B, 0x03 = 1024B
        uint8_t     medium_sides;   //  Число сторон диска 0x00 = 1, 0x01 = 2   (0x01)                     
        uint16_t    sec_per_track;  //  Число секторов на дорожке               (0x05)
        uint16_t    medium_tracks;  //  Число дорожек на диске                  (0x0050 = 80)
        uint16_t    sec128_per_trk; //  Число логических (128-байтных) секторов (0x0028 = 40)
                                    //  на дорожке  
        uint8_t     block_shift;    //  Фактор сдвига блока                     (0x04)      ???
        uint8_t     block_mask;     //  Маска блока данных                      (0x0f)      ???
        uint8_t     exm_mask;       //  маска размера блока                     (0x00)      ???
        uint16_t    datablocks;     //  Количество блоков данных -1             (0x185)
        uint16_t    dir_entries;    //  Количество элементов оглавления -1      (0x7f)
        uint16_t    dirblk_mask;    //  маска блоков оглавления                 (0x00c0)
        uint16_t    dir_entry_sz;   //  размер элемента оглавления, байт        (0x20) 
        uint16_t    os_tracks;      //  Количество зарезервированных под ОС
                                    //  дорожек (цилиндров*секторов)            (0x04)
                                    //  После зарезервированной области на диске 
                                    //  располагается оглавление CP/M
                                    //  Начало оглавления выровнено по началу дорожки, первая головка
        uint8_t     checksum;        //  Контрольная сумма                       (0x4b)
    };
    uint8_t bytes[0x20];
} CPM_DiskHeader;

#define CPM_DISKHEADER_SZ (sizeof(CPM_DiskHeader))
#define QUORUM_CPM_BOOT_BASE (0xa880)
static_assert(CPM_DISKHEADER_SZ == 0x20, "Wrong structure packing");

typedef struct __attribute ((packed)) {
    uint8_t user;
    char name[8];
    char type[3];
    uint8_t ex;
    uint8_t s1;
    uint8_t s2;
    uint8_t recordsCount;
    uint8_t allocation[16];
} CPM_DirEntry;
#define CPM_DIRENTRY_SZ (sizeof(CPM_DirEntry))
static_assert(CPM_DIRENTRY_SZ == 0x20, "Wrong structure packing");



typedef struct {
    FIL file;
    bool headerOk;

    CPM_DiskHeader diskHeader;
    uint32_t cpm_dirOffset;
    uint32_t cpm_dirSize;
    uint32_t cpm_sectorsz;
} CPM_Image;


