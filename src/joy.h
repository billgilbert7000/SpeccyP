


#define MAX_JOY_MODE 5


//#define JOY_UP 0x08   //   0000 1000
/* #define JOY_DW 0x04   //   0000 0100
#define JOY_LF 0x02   //   0000 0010
#define JOY_RT 0x01   //   0000 0001
#define JOY_A  0x20   //   0010 0000
#define JOY_B  0x10   //    0001 0000
#define JOY_C 0x30   //    0011 0000
#define JOY_MD 0x40   //0100 0000
#define JOY_ST 0x80   //1000 0000 
 */
//================================================================================
#define JOY_RIGHT (data_joy==0x01)
#define JOY_LEFT  (data_joy==0x02) 
#define JOY_UP    (data_joy==0x08) 
#define JOY_DOWN  (data_joy==0x04) 
#define JOY_A  (data_joy==0x20)
#define JOY_B  (data_joy==0x10)

void d_joy_init();
bool decode_joy_to_keyboard(void);
bool decode_joy();


