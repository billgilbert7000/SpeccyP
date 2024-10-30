


#define MAX_JOY_MODE 5
#define D_JOY_DATA_PIN (16)
#define D_JOY_CLK_PIN (14)
#define D_JOY_LATCH_PIN (15)

#define JOY_UP 0x08   //   0000 1000
#define JOY_DW 0x04   //   0000 0100
#define JOY_LF 0x02   //   0000 0010
#define JOY_RT 0x01   //   0000 0001
#define JOY_A  0x20   //   0010 0000
#define JOY_B  0x10   //    0001 0000
#define JOY_C 0x30   //    0011 0000
#define JOY_MD 0x40   //0100 0000
#define JOY_ST 0x80   //1000 0000 


bool joy_connected;

uint8_t data_joy;
uint8_t old_data_joy;

void d_joy_init();
bool decode_joy_to_keyboard(void);
bool decode_joy();


