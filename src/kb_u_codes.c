#include "stddef.h"
#include "stdbool.h"
#include "kb_u_codes.h"
#include "string.h"


//Default accords translator
void convert_kb_accords(kb_u_state* kb_st,uint8_t* zx_kb);

//translate keys outside of standard ZX keyboard by accords
// for example, ';' map to SS+O
static ConvertKbStateToZxHook extKeysConverter = convert_kb_accords;

void setZxExtKeysHook(ConvertKbStateToZxHook customExtKeysConverter) {
    extKeysConverter = customExtKeysConverter;
}

void setZxExtKeysDefault() {
    setZxExtKeysHook(convert_kb_accords);
}

typedef struct {
    const char u, n;
    const char k, schift_k, ctrl_k, alt_k;
} KeyDef;

const KeyDef kbdUS [] = {
    0, 0, 'a','A', 1, 0x81,
    0, 1, 'b','B', 2, 0x82,
    0, 2, 'c','C', 3, 0x83,
    0, 3, 'd','D', 4, 0x84,
    0, 4, 'e','E', 5, 0x85,
    0, 5, 'f','F', 6, 0x86,
    0, 6, 'g','G', 7, 0x87,
    0, 7, 'h','H', 8, 0x88,
    0, 8, 'i','I', 9, 0x89,
    0, 9, 'j','J', 0x0a, 0x8a,
    0,10, 'k','K', 0x0b, 0x8b,
    0,11, 'l','L', 0x0c, 0x8c,
    0,12, 'm','M', 0x0d, 0x8d,
    0,13, 'n','N', 0x0e, 0x8e,
    0,14, 'o','O', 0x0f, 0x8f,
    0,15, 'p','P', 0x10, 0x90,
    0,16, 'q','Q', 0x11, 0x91,
    0,17, 'r','R', 0x12, 0x92,
    0,18, 's','S', 0x13, 0x93,
    0,19, 't','T', 0x14, 0x94,
    0,20, 'u','U', 0x15, 0x95,
    0,21, 'v','V', 0x16, 0x96,
    0,22, 'w','W', 0x17, 0x97,
    0,23, 'x','X', 0x18, 0x98,
    0,24, 'y','Y', 0x1a, 0x99,
    0,25, 'z','Z', 0x1b, 0x9a,

    0,26, ';', ':',  0, 0,
    0,27, '\'','\"', 0, 0,
    0,28, ',','<',   0, 0,
    0,29, '.','>',   0, 0,
    0,30, ',','<',   0, 0,
    0,31, '.','>',   0, 0,

    1, 0, '0',')',      0, 0xa0,
    1, 1, '1','!',      0, 0xa1,
    1, 2, '2','@',      0, 0xa2,
    1, 3, '3','#',      0, 0xa3,
    1, 4, '4','$',      0, 0xa4,
    1, 5, '5','%',      0, 0xa5,
    1, 6, '6','^',      0, 0xa6,
    1, 7, '7','&',      0, 0xa7,
    1, 8, '8','*',      0, 0xa8,
    1, 9, '9','(',      0, 0xa9,

    1,10, '\r','\r',    0, 0,    //ENTER
    1,11, '/','?',      0, 0,    //SLASH
    1,12, '-','_',      0, 0,    //MINUS
    1,13, '=','+',      0, 0,    //EQUALS
    1,14, '\\','|',     0, 0,
    1,15, 0xaa,0xaa,    0, 0,    //CAPS LOCK
    1,16, '\t','\t',    0, 0,    //TAB
    1,17, 0x08,0x08,    0, 0,    //BACKSPACE
    1,18, 0x1b,0x1b,    0, 0,    //ESC
    1,19, '`','~',      0, 0,    //TILDE
    1,20, 0xab,0xab,    0, 0,
    1,29, ' ',' ',      0, 0,    //SPACE
    2, 0, '0','0',      0, 0,    //NUM0
    2, 1, '1','1',      0, 0,    //NUM1
    2, 2, '2','2',      0, 0,    //NUM2
    2, 3, '3','3',      0, 0,    //NUM3
    2, 4, '4','4',      0, 0,    //NUM4
    2, 5, '5','5',      0, 0,    //NUM5
    2, 6, '6','6',      0, 0,    //NUM6
    2, 7, '7','7',      0, 0,    //NUM7
    2, 8, '8','8',      0, 0,    //NUM8
    2, 9, '9','9',      0, 0,    //NUM9
    2,10, '\r','\r',    0, 0,    //NUMENTER
    2,11, '/','/',      0, 0,    //NUMSLASH
    2,12, '-','-',      0, 0,    //NUMMINUS
    2,13, '+','+',      0, 0,    //NUMPLUS
    2,14, '*','*',      0, 0,    //NUMMUL
    2,15, '.','.',      0, 0,    //NUMPERIOD
    2,16, 0,  0,        0, 0,    //NUMLOCK
    2,17, 0x7f,0x7f,    0, 0,    //DEL

    2,18, 0xac,0xac,    0, 0,    //SCRLOCK
    2,19, 0xad,0xad,    0, 0,    //PAUSE
    2,20, 0xB0,0xB0,    0, 0,    //INS
    2,21, 0xB7,0xB7,    0, 0,    //HOME
    2,22, 0xB9,0xB9,    0, 0,    //PGUP
    2,23, 0xB3,0xB3,    0, 0,    //PGDN
    2,24, 0xae,0xae,    0, 0,    //PRTSCR
    2,25, 0xB1,0xB1,    0, 0,    //END
    2,26, 0xB8,0xB8,    0, 0,    //UP
    2,27, 0xB2,0xB2,    0, 0,    //DOWN
    2,28, 0xB4,0xB4,    0, 0,    //LEFT
    2,29, 0xB6,0xB6,    0, 0,    //RIGHT

    3, 1, 0xF1,0xF1,    0, 0,    //F1
    3, 2, 0xF2,0xF2,    0, 0,    //F2
    3, 3, 0xF3,0xF3,    0, 0,    //F3
    3, 4, 0xF4,0xF4,    0, 0,    //F4
    3, 5, 0xF5,0xF5,    0, 0,    //F5
    3, 6, 0xF6,0xF6,    0, 0,    //F6
    3, 7, 0xF7,0xF7,    0, 0,    //F7
    3, 8, 0xF8,0xF8,    0, 0,    //F8
    3, 9, 0xF9,0xF9,    0, 0,    //F9
    3,10, 0xFA,0xFA,    0, 0,    //F10
    3,11, 0xFB,0xFB,    0, 0,    //F11
    3,12, 0xFC,0xFC,    0, 0,    //F12


    0xff,0xff, 0,0, 0,0,        //END OF TABLE
};

void keys_to_str(char* str_buf,char s_char,kb_u_state kb_state)
{
    char s_str[2];
    s_str[0]=s_char;
    s_str[1]='\0';

    str_buf[0]=0;
    strcat(str_buf,"KEY PRESSED: ");
//0 набор
    if (kb_state.u[0]&KB_U0_A) {strcat(str_buf,"A");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_B) {strcat(str_buf,"B");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_C) {strcat(str_buf,"C");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_D) {strcat(str_buf,"D");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_E) {strcat(str_buf,"E");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_F) {strcat(str_buf,"F");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_G) {strcat(str_buf,"G");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_H) {strcat(str_buf,"H");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_I) {strcat(str_buf,"I");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_J) {strcat(str_buf,"J");strcat(str_buf,s_str);};

    if (kb_state.u[0]&KB_U0_K) {strcat(str_buf,"K");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_L) {strcat(str_buf,"L");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_M) {strcat(str_buf,"M");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_N) {strcat(str_buf,"N");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_O) {strcat(str_buf,"O");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_P) {strcat(str_buf,"P");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_Q) {strcat(str_buf,"Q");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_R) {strcat(str_buf,"R");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_S) {strcat(str_buf,"S");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_T) {strcat(str_buf,"T");strcat(str_buf,s_str);};

    if (kb_state.u[0]&KB_U0_U) {strcat(str_buf,"U");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_V) {strcat(str_buf,"V");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_W) {strcat(str_buf,"W");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_X) {strcat(str_buf,"X");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_Y) {strcat(str_buf,"Y");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_Z) {strcat(str_buf,"Z");strcat(str_buf,s_str);};

    if (kb_state.u[0]&KB_U0_SEMICOLON) {strcat(str_buf,";");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_QUOTE) {strcat(str_buf,"\"");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_COMMA) {strcat(str_buf,",");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_PERIOD) {strcat(str_buf,".");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_LEFT_BR) {strcat(str_buf,"[");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_RIGHT_BR) {strcat(str_buf,"]");strcat(str_buf,s_str);};
//1 набор
    if (kb_state.u[1]&KB_U1_0) {strcat(str_buf,"0");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_1) {strcat(str_buf,"1");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_2) {strcat(str_buf,"2");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_3) {strcat(str_buf,"3");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_4) {strcat(str_buf,"4");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_5) {strcat(str_buf,"5");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_6) {strcat(str_buf,"6");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_7) {strcat(str_buf,"7");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_8) {strcat(str_buf,"8");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_9) {strcat(str_buf,"9");strcat(str_buf,s_str);};

    if (kb_state.u[1]&KB_U1_ENTER) {strcat(str_buf,"ENTER");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_SLASH) {strcat(str_buf,"/");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_MINUS) {strcat(str_buf,"MINUS");strcat(str_buf,s_str);};

    if (kb_state.u[1]&KB_U1_EQUALS) {strcat(str_buf,"=");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_BACKSLASH) {strcat(str_buf,"BACKSLASH");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_CAPS_LOCK) {strcat(str_buf,"CAPS_LOCK");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_TAB) {strcat(str_buf,"TAB");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_BACK_SPACE) {strcat(str_buf,"BACK_SPACE");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_ESC) {strcat(str_buf,"ESC");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_TILDE) {strcat(str_buf,"TILDE");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_MENU) {strcat(str_buf,"MENU");strcat(str_buf,s_str);};

    if (kb_state.u[1]&KB_U1_L_SHIFT) {strcat(str_buf,"L_SHIFT");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_L_CTRL) {strcat(str_buf,"L_CTRL");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_L_ALT) {strcat(str_buf,"L_ALT");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_L_WIN) {strcat(str_buf,"L_WIN");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_R_SHIFT) {strcat(str_buf,"R_SHIFT");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_R_CTRL) {strcat(str_buf,"R_CTRL");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_R_ALT) {strcat(str_buf,"R_ALT");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_R_WIN) {strcat(str_buf,"R_WIN");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_SPACE) {strcat(str_buf,"SPACE");strcat(str_buf,s_str);};


//2 набор
    if (kb_state.u[2]&KB_U2_NUM_0) {strcat(str_buf,"NUM_0");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_1) {strcat(str_buf,"NUM_1");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_2) {strcat(str_buf,"NUM_2");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_3) {strcat(str_buf,"NUM_3");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_4) {strcat(str_buf,"NUM_4");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_5) {strcat(str_buf,"NUM_5");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_6) {strcat(str_buf,"NUM_6");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_7) {strcat(str_buf,"NUM_7");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_8) {strcat(str_buf,"NUM_8");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_9) {strcat(str_buf,"NUM_9");strcat(str_buf,s_str);};

    if (kb_state.u[2]&KB_U2_NUM_ENTER) {strcat(str_buf,"ENTER");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_SLASH) {strcat(str_buf,"/");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_MINUS) {strcat(str_buf,"MINUS");strcat(str_buf,s_str);};

    if (kb_state.u[2]&KB_U2_NUM_PLUS) {strcat(str_buf,"NUM_PLUS");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_MULT) {strcat(str_buf,"NUM_MULT");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_PERIOD) {strcat(str_buf,"NUM_PERIOD");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_LOCK) {strcat(str_buf,"NUM_LOCK");strcat(str_buf,s_str);};

    if (kb_state.u[2]&KB_U2_DELETE) {strcat(str_buf,"DEL");strcat(str_buf,s_str);};


    if (kb_state.u[2]&KB_U2_SCROLL_LOCK) {strcat(str_buf,"SCROLL_LOCK");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_PAUSE_BREAK) {strcat(str_buf,"PAUSE_BREAK");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_INSERT) {strcat(str_buf,"INSERT");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_HOME) {strcat(str_buf,"HOME");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_PAGE_UP) {strcat(str_buf,"PG_UP");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_PAGE_DOWN) {strcat(str_buf,"PG_DOWN");strcat(str_buf,s_str);};

    if (kb_state.u[2]&KB_U2_PRT_SCR) {strcat(str_buf,"PRT_SCR");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_END) {strcat(str_buf,"END");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_UP) {strcat(str_buf,"UP");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_DOWN) {strcat(str_buf,"DOWN");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_LEFT) {strcat(str_buf,"LEFT");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_RIGHT) {strcat(str_buf,"RIGHT");strcat(str_buf,s_str);};

//3 набор
    if (kb_state.u[3]&KB_U3_F1) {strcat(str_buf,"F1");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F2) {strcat(str_buf,"F2");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F3) {strcat(str_buf,"F3");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F4) {strcat(str_buf,"F4");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F5) {strcat(str_buf,"F5");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F6) {strcat(str_buf,"F6");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F7) {strcat(str_buf,"F7");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F8) {strcat(str_buf,"F8");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F9) {strcat(str_buf,"F9");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F10) {strcat(str_buf,"F10");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F11) {strcat(str_buf,"F11");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F12) {strcat(str_buf,"F12");strcat(str_buf,s_str);};

    strcat(str_buf,"\n");

};


void convert_kb_u_to_kb_zx(kb_u_state* kb_st,uint8_t* zx_kb)
{
    memset(zx_kb,0,8);

    uint32_t u0 = kb_st->u[0];
    if (u0) {
            if (u0 & KB_U0_A) {zx_kb[1]|=1<<0;};
            if (u0 & KB_U0_B) {zx_kb[7]|=1<<4;};
            if (u0 & KB_U0_C) {zx_kb[0]|=1<<3;};
            if (u0 & KB_U0_D) {zx_kb[1]|=1<<2;};
            if (u0 & KB_U0_E) {zx_kb[2]|=1<<2;};
            if (u0 & KB_U0_F) {zx_kb[1]|=1<<3;};
            if (u0 & KB_U0_G) {zx_kb[1]|=1<<4;};
            if (u0 & KB_U0_H) {zx_kb[6]|=1<<4;};
            if (u0 & KB_U0_I) {zx_kb[5]|=1<<2;};
            if (u0 & KB_U0_J) {zx_kb[6]|=1<<3;};

            if (u0 & KB_U0_K) {zx_kb[6]|=1<<2;};
            if (u0 & KB_U0_L) {zx_kb[6]|=1<<1;};
            if (u0 & KB_U0_M) {zx_kb[7]|=1<<2;};
            if (u0 & KB_U0_N) {zx_kb[7]|=1<<3;};
            if (u0 & KB_U0_O) {zx_kb[5]|=1<<1;};
            if (u0 & KB_U0_P) {zx_kb[5]|=1<<0;};
            if (u0 & KB_U0_Q) {zx_kb[2]|=1<<0;};
            if (u0 & KB_U0_R) {zx_kb[2]|=1<<3;};
            if (u0 & KB_U0_S) {zx_kb[1]|=1<<1;};
            if (u0 & KB_U0_T) {zx_kb[2]|=1<<4;};

            if (u0 & KB_U0_U) {zx_kb[5]|=1<<3;};
            if (u0 & KB_U0_V) {zx_kb[0]|=1<<4;};
            if (u0 & KB_U0_W) {zx_kb[2]|=1<<1;};
            if (u0 & KB_U0_X) {zx_kb[0]|=1<<2;};
            if (u0 & KB_U0_Y) {zx_kb[5]|=1<<4;};
            if (u0 & KB_U0_Z) {zx_kb[0]|=1<<1;};
    }

    uint32_t u1 = kb_st->u[1];
    if (u1) {
            if (u1 & KB_U1_0) {zx_kb[4]|=1<<0;};
            if (u1 & KB_U1_1) {zx_kb[3]|=1<<0;};
            if (u1 & KB_U1_2) {zx_kb[3]|=1<<1;};
            if (u1 & KB_U1_3) {zx_kb[3]|=1<<2;};
            if (u1 & KB_U1_4) {zx_kb[3]|=1<<3;};
            if (u1 & KB_U1_5) {zx_kb[3]|=1<<4;};
            if (u1 & KB_U1_6) {zx_kb[4]|=1<<4;};
            if (u1 & KB_U1_7) {zx_kb[4]|=1<<3;};
            if (u1 & KB_U1_8) {zx_kb[4]|=1<<2;};
            if (u1 & KB_U1_9) {zx_kb[4]|=1<<1;};

            if (u1 & KB_U1_ENTER) {zx_kb[6]|=1<<0;};

            if (u1 & KB_U1_L_SHIFT) {zx_kb[0]|=1<<0;};
            if (u1 & KB_U1_L_CTRL) {zx_kb[7]|=1<<1;};
            if (u1 & KB_U1_L_ALT) {};//ext.mode?
            if (u1 & KB_U1_L_WIN) {};
            if (u1 & KB_U1_R_SHIFT) {zx_kb[0]|=1<<0;};
            if (u1 & KB_U1_R_CTRL) {zx_kb[7]|=1<<1;};
            if (u1 & KB_U1_R_ALT) {};//ext.mode?
            if (u1 & KB_U1_R_WIN) {};
            if (u1 & KB_U1_SPACE) {zx_kb[7]|=1<<0;};

    }

    uint32_t u2 = kb_st->u[2];
    if (u2) {
          //  if (kb_st->u[2]&KB_U2_NUM_PERIOD) {zx_kb[7]|=1<<1;zx_kb[7]|=1<<2;};// отключение кнопки DEL NUM 
          //  if (kb_st->u[2]&KB_U2_NUM_PERIOD) {};// замена кнопки DEL NUM на ALT R


            if (u2 & KB_U2_NUM_LOCK) {};
            if (u2 & KB_U2_DELETE) {};
            if (u2 & KB_U2_SCROLL_LOCK) {};
            if (u2 & KB_U2_PAUSE_BREAK) {};
            if (u2 & KB_U2_INSERT) {};
            if (u2 & KB_U2_HOME) {};
            if (u2 & KB_U2_PAGE_UP) {};
            if (u2 & KB_U2_PAGE_DOWN) {};
            if (u2 & KB_U2_PRT_SCR) {};
            if (u2 & KB_U2_END) {};
    }

    if (extKeysConverter) extKeysConverter(kb_st, zx_kb);

};

void convert_kb_accords(kb_u_state* kb_st,uint8_t* zx_kb) 
{
    uint32_t u0 = kb_st->u[0];
    if (u0) {
        if (u0 & KB_U0_SEMICOLON) {zx_kb[7]|=1<<1;zx_kb[5]|=1<<1;};
        if (u0 & KB_U0_QUOTE) {zx_kb[7]|=1<<1;zx_kb[5]|=1<<0;};
        if (u0 & KB_U0_COMMA) {zx_kb[7]|=1<<1;zx_kb[7]|=1<<3;};
        if (u0 & KB_U0_PERIOD) {zx_kb[7]|=1<<1;zx_kb[7]|=1<<2;};
        if (u0 & KB_U0_LEFT_BR) {zx_kb[7]|=1<<1;zx_kb[4]|=1<<2;};
        if (u0 & KB_U0_RIGHT_BR) {zx_kb[7]|=1<<1;zx_kb[4]|=1<<1;};
    }

    uint32_t u1 = kb_st->u[1];
    if (u1) {
        if (u1 & KB_U1_SLASH) {zx_kb[7]|=1<<1;zx_kb[0]|=1<<3;};
        if (u1 & KB_U1_MINUS) {zx_kb[7]|=1<<1;zx_kb[6]|=1<<3;};

        if (u1 & KB_U1_EQUALS) {zx_kb[7]|=1<<1;zx_kb[6]|=1<<1;};
        if (u1 & KB_U1_BACKSLASH) {zx_kb[7]|=1<<1;zx_kb[0]|=1<<1;};
        if (u1 & KB_U1_CAPS_LOCK) {zx_kb[0]|=1<<0;zx_kb[3]|=1<<1;};
        if (u1 & KB_U1_TAB) {zx_kb[0]|=1<<0;zx_kb[3]|=1<<0;};
        if (u1 & KB_U1_BACK_SPACE) {zx_kb[0]|=1<<0;zx_kb[4]|=1<<0;};
        if (u1 & KB_U1_ESC) {zx_kb[0]|=1<<0;zx_kb[7]|=1<<0;};
        if (u1 & KB_U1_TILDE) {zx_kb[7]|=1<<1;zx_kb[1]|=1<<0;};
        if (u1 & KB_U1_MENU) {};
    }

    uint32_t u2 = kb_st->u[2];
    if (u2) {
        if (u2 & KB_U2_NUM_0) {zx_kb[4]|=1<<0;};
        if (u2 & KB_U2_NUM_1) {zx_kb[3]|=1<<0;};
        if (u2 & KB_U2_NUM_2) {zx_kb[3]|=1<<1;};
        if (u2 & KB_U2_NUM_3) {zx_kb[3]|=1<<2;};
        if (u2 & KB_U2_NUM_4) {zx_kb[3]|=1<<3;};
        if (u2 & KB_U2_NUM_5) {zx_kb[3]|=1<<4;};
        if (u2 & KB_U2_NUM_6) {zx_kb[4]|=1<<4;};
        if (u2 & KB_U2_NUM_7) {zx_kb[4]|=1<<3;};
        if (u2 & KB_U2_NUM_8) {zx_kb[4]|=1<<2;};
        if (u2 & KB_U2_NUM_9) {zx_kb[4]|=1<<1;};
        if (u2 & KB_U2_NUM_ENTER) {zx_kb[6]|=1<<0;};
        if (u2 & KB_U2_NUM_SLASH) {zx_kb[7]|=1<<1;zx_kb[0]|=1<<4;};
        if (u2 & KB_U2_NUM_MINUS) {zx_kb[7]|=1<<1;zx_kb[6]|=1<<3;};
        if (u2 & KB_U2_NUM_PLUS) {zx_kb[7]|=1<<1;zx_kb[6]|=1<<2;};
        if (u2 & KB_U2_NUM_MULT) {zx_kb[7]|=1<<1;zx_kb[7]|=1<<4;};
    }

}