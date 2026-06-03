bool save_config(void);
 void pico_reset(void);

 typedef struct {
    const char name[24];
    uint8_t NeedPSRAM;
    int id;
} ZxMachineVariant;