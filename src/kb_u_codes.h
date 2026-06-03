#pragma once
#include "inttypes.h"
typedef struct kb_u_state
{
   uint32_t u[4];

}kb_u_state;


#define KB_U0_A_POS (0)
#define KB_U0_B_POS (1)
#define KB_U0_C_POS (2)
#define KB_U0_D_POS (3)
#define KB_U0_E_POS (4)
#define KB_U0_F_POS (5)
#define KB_U0_G_POS (6)
#define KB_U0_H_POS (7)
#define KB_U0_I_POS (8)
#define KB_U0_J_POS (9)
#define KB_U0_K_POS (10)
#define KB_U0_L_POS (11)
#define KB_U0_M_POS (12)
#define KB_U0_N_POS (13)
#define KB_U0_O_POS (14)
#define KB_U0_P_POS (15)
#define KB_U0_Q_POS (16)
#define KB_U0_R_POS (17)
#define KB_U0_S_POS (18)
#define KB_U0_T_POS (19)
#define KB_U0_U_POS (20)
#define KB_U0_V_POS (21)
#define KB_U0_W_POS (22)
#define KB_U0_X_POS (23)
#define KB_U0_Y_POS (24)
#define KB_U0_Z_POS (25)

#define KB_U0_SEMICOLON_POS (26)
#define KB_U0_QUOTE_POS (27)
#define KB_U0_COMMA_POS (28)
#define KB_U0_PERIOD_POS (29)
#define KB_U0_LEFT_BR_POS (30)
#define KB_U0_RIGHT_BR_POS (31)

// блок 1 цифры и контролы


#define KB_U1_0_POS (0)
#define KB_U1_1_POS (1)
#define KB_U1_2_POS (2)
#define KB_U1_3_POS (3)
#define KB_U1_4_POS (4)
#define KB_U1_5_POS (5)
#define KB_U1_6_POS (6)
#define KB_U1_7_POS (7)
#define KB_U1_8_POS (8)
#define KB_U1_9_POS (9)

#define KB_U1_ENTER_POS (10)
#define KB_U1_SLASH_POS (11)
#define KB_U1_MINUS_POS (12)

#define KB_U1_EQUALS_POS    (13)
#define KB_U1_BACKSLASH_POS (14)
#define KB_U1_CAPS_LOCK_POS (15)
#define KB_U1_TAB_POS       (16)
#define KB_U1_BACK_SPACE_POS (17)
#define KB_U1_ESC_POS       (18)
#define KB_U1_TILDE_POS     (19)
#define KB_U1_MENU_POS      (20)

#define KB_U1_L_SHIFT_POS   (21)
#define KB_U1_L_CTRL_POS    (22)
#define KB_U1_L_ALT_POS     (23)
#define KB_U1_L_WIN_POS     (24)
#define KB_U1_R_SHIFT_POS   (25)
#define KB_U1_R_CTRL_POS    (26)
#define KB_U1_R_ALT_POS     (27)
#define KB_U1_R_WIN_POS     (28)
#define KB_U1_SPACE_POS     (29)

#define KB_U2_NUM_0_POS (0)
#define KB_U2_NUM_1_POS (1)
#define KB_U2_NUM_2_POS (2)
#define KB_U2_NUM_3_POS (3)
#define KB_U2_NUM_4_POS (4)
#define KB_U2_NUM_5_POS (5)
#define KB_U2_NUM_6_POS (6)
#define KB_U2_NUM_7_POS (7)
#define KB_U2_NUM_8_POS (8)
#define KB_U2_NUM_9_POS (9)

#define KB_U2_NUM_ENTER_POS (10)
#define KB_U2_NUM_SLASH_POS (11)
#define KB_U2_NUM_MINUS_POS (12)

#define KB_U2_NUM_PLUS_POS (13)
#define KB_U2_NUM_MULT_POS (14)
#define KB_U2_NUM_PERIOD_POS  (15)
#define KB_U2_NUM_LOCK_POS (16)


#define KB_U2_DELETE_POS (17)


#define KB_U2_SCROLL_LOCK_POS   (18)
#define KB_U2_PAUSE_BREAK_POS   (19)
#define KB_U2_INSERT_POS        (20)
#define KB_U2_HOME_POS          (21)
#define KB_U2_PAGE_UP_POS       (22)
#define KB_U2_PAGE_DOWN_POS     (23)

#define KB_U2_PRT_SCR_POS       (24)
#define KB_U2_END_POS           (25)
#define KB_U2_UP_POS            (26)
#define KB_U2_DOWN_POS          (27)
#define KB_U2_LEFT_POS          (28)
#define KB_U2_RIGHT_POS         (29)

// блок 3 F клавиши и прочие допы


#define KB_U3__POS   (0)
#define KB_U3_F1_POS (1)
#define KB_U3_F2_POS (2)
#define KB_U3_F3_POS (3)
#define KB_U3_F4_POS (4)
#define KB_U3_F5_POS (5)
#define KB_U3_F6_POS (6)
#define KB_U3_F7_POS (7)
#define KB_U3_F8_POS (8)
#define KB_U3_F9_POS (9)
#define KB_U3_F10_POS (10)
#define KB_U3_F11_POS (11)
#define KB_U3_F12_POS (12)



///////////////////////////////////////

// блок 0 буквы
#define KB_U0_A             (1<<KB_U0_A_POS)
#define KB_U0_B             (1<<KB_U0_B_POS)
#define KB_U0_C             (1<<KB_U0_C_POS)
#define KB_U0_D             (1<<KB_U0_D_POS)
#define KB_U0_E             (1<<KB_U0_E_POS)
#define KB_U0_F             (1<<KB_U0_F_POS)
#define KB_U0_G             (1<<KB_U0_G_POS)
#define KB_U0_H             (1<<KB_U0_H_POS)
#define KB_U0_I             (1<<KB_U0_I_POS)
#define KB_U0_J             (1<<KB_U0_J_POS)
#define KB_U0_K             (1<<KB_U0_K_POS)
#define KB_U0_L             (1<<KB_U0_L_POS)
#define KB_U0_M             (1<<KB_U0_M_POS)
#define KB_U0_N             (1<<KB_U0_N_POS)
#define KB_U0_O             (1<<KB_U0_O_POS)
#define KB_U0_P             (1<<KB_U0_P_POS)
#define KB_U0_Q             (1<<KB_U0_Q_POS)
#define KB_U0_R             (1<<KB_U0_R_POS)
#define KB_U0_S             (1<<KB_U0_S_POS)
#define KB_U0_T             (1<<KB_U0_T_POS)
#define KB_U0_U             (1<<KB_U0_U_POS)
#define KB_U0_V             (1<<KB_U0_V_POS)
#define KB_U0_W             (1<<KB_U0_W_POS)
#define KB_U0_X             (1<<KB_U0_X_POS)
#define KB_U0_Y             (1<<KB_U0_Y_POS)
#define KB_U0_Z             (1<<KB_U0_Z_POS)

#define KB_U0_SEMICOLON     (1<<KB_U0_SEMICOLON_POS)
#define KB_U0_QUOTE         (1<<KB_U0_QUOTE_POS)
#define KB_U0_COMMA         (1<<KB_U0_COMMA_POS)
#define KB_U0_PERIOD        (1<<KB_U0_PERIOD_POS)
#define KB_U0_LEFT_BR       (1<<KB_U0_LEFT_BR_POS)
#define KB_U0_RIGHT_BR      (1<<KB_U0_RIGHT_BR_POS)

// блок 1 цифры и контролы


#define KB_U1_0             (1<<KB_U1_0_POS)
#define KB_U1_1             (1<<KB_U1_1_POS)
#define KB_U1_2             (1<<KB_U1_2_POS)
#define KB_U1_3             (1<<KB_U1_3_POS)
#define KB_U1_4             (1<<KB_U1_4_POS)
#define KB_U1_5             (1<<KB_U1_5_POS)
#define KB_U1_6             (1<<KB_U1_6_POS)
#define KB_U1_7             (1<<KB_U1_7_POS)
#define KB_U1_8             (1<<KB_U1_8_POS)
#define KB_U1_9             (1<<KB_U1_9_POS)

#define KB_U1_ENTER         (1<<KB_U1_ENTER_POS)
#define KB_U1_SLASH         (1<<KB_U1_SLASH_POS)
#define KB_U1_MINUS         (1<<KB_U1_MINUS_POS)

#define KB_U1_EQUALS        (1<<KB_U1_EQUALS_POS)
#define KB_U1_BACKSLASH     (1<<KB_U1_BACKSLASH_POS)
#define KB_U1_CAPS_LOCK     (1<<KB_U1_CAPS_LOCK_POS)
#define KB_U1_TAB           (1<<KB_U1_TAB_POS)
#define KB_U1_BACK_SPACE    (1<<KB_U1_BACK_SPACE_POS)
#define KB_U1_ESC           (1<<KB_U1_ESC_POS)
#define KB_U1_TILDE         (1<<KB_U1_TILDE_POS)
#define KB_U1_MENU          (1<<KB_U1_MENU_POS)

#define KB_U1_L_SHIFT       (1<<KB_U1_L_SHIFT_POS)
#define KB_U1_L_CTRL        (1<<KB_U1_L_CTRL_POS)
#define KB_U1_L_ALT         (1<<KB_U1_L_ALT_POS)
#define KB_U1_L_WIN         (1<<KB_U1_L_WIN_POS)
#define KB_U1_R_SHIFT       (1<<KB_U1_R_SHIFT_POS)
#define KB_U1_R_CTRL        (1<<KB_U1_R_CTRL_POS)
#define KB_U1_R_ALT         (1<<KB_U1_R_ALT_POS)
#define KB_U1_R_WIN         (1<<KB_U1_R_WIN_POS)

#define KB_U1_SPACE         (1<<KB_U1_SPACE_POS)




// блок 2 нум клавиши и доп клавиши



#define KB_U2_NUM_0         (1<<KB_U2_NUM_0_POS)
#define KB_U2_NUM_1         (1<<KB_U2_NUM_1_POS)
#define KB_U2_NUM_2         (1<<KB_U2_NUM_2_POS)
#define KB_U2_NUM_3         (1<<KB_U2_NUM_3_POS)
#define KB_U2_NUM_4         (1<<KB_U2_NUM_4_POS)
#define KB_U2_NUM_5         (1<<KB_U2_NUM_5_POS)
#define KB_U2_NUM_6         (1<<KB_U2_NUM_6_POS)
#define KB_U2_NUM_7         (1<<KB_U2_NUM_7_POS)
#define KB_U2_NUM_8         (1<<KB_U2_NUM_8_POS)
#define KB_U2_NUM_9         (1<<KB_U2_NUM_9_POS)

#define KB_U2_NUM_ENTER     (1<<KB_U2_NUM_ENTER_POS)
#define KB_U2_NUM_SLASH     (1<<KB_U2_NUM_SLASH_POS)
#define KB_U2_NUM_MINUS     (1<<KB_U2_NUM_MINUS_POS)

#define KB_U2_NUM_PLUS      (1<<KB_U2_NUM_PLUS_POS)
#define KB_U2_NUM_MULT      (1<<KB_U2_NUM_MULT_POS)
#define KB_U2_NUM_PERIOD    (1<<KB_U2_NUM_PERIOD_POS)
#define KB_U2_NUM_LOCK      (1<<KB_U2_NUM_LOCK_POS)


#define KB_U2_DELETE        (1<<KB_U2_DELETE_POS)


#define KB_U2_SCROLL_LOCK   (1<<KB_U2_SCROLL_LOCK_POS)
#define KB_U2_PAUSE_BREAK   (1<<KB_U2_PAUSE_BREAK_POS)
#define KB_U2_INSERT        (1<<KB_U2_INSERT_POS)
#define KB_U2_HOME          (1<<KB_U2_HOME_POS)
#define KB_U2_PAGE_UP       (1<<KB_U2_PAGE_UP_POS)
#define KB_U2_PAGE_DOWN     (1<<KB_U2_PAGE_DOWN_POS)

#define KB_U2_PRT_SCR       (1<<KB_U2_PRT_SCR_POS)
#define KB_U2_END           (1<<KB_U2_END_POS)
#define KB_U2_UP            (1<<KB_U2_UP_POS)
#define KB_U2_DOWN          (1<<KB_U2_DOWN_POS)
#define KB_U2_LEFT          (1<<KB_U2_LEFT_POS)
#define KB_U2_RIGHT         (1<<KB_U2_RIGHT_POS)

// блок 3 F клавиши и прочие допы


#define KB_U3_              (1<<KB_U3__POS)
#define KB_U3_F1            (1<<KB_U3_F1_POS)
#define KB_U3_F2            (1<<KB_U3_F2_POS)
#define KB_U3_F3            (1<<KB_U3_F3_POS)
#define KB_U3_F4            (1<<KB_U3_F4_POS)
#define KB_U3_F5            (1<<KB_U3_F5_POS)
#define KB_U3_F6            (1<<KB_U3_F6_POS)
#define KB_U3_F7            (1<<KB_U3_F7_POS)
#define KB_U3_F8            (1<<KB_U3_F8_POS)
#define KB_U3_F9            (1<<KB_U3_F9_POS)
#define KB_U3_F10           (1<<KB_U3_F10_POS)
#define KB_U3_F11           (1<<KB_U3_F11_POS)
#define KB_U3_F12           (1<<KB_U3_F12_POS)

void convert_kb_u_to_kb_zx(kb_u_state* kb_st,uint8_t* zx_kb);
//для тестирования
void keys_to_str(char* str_buf,char s_char,kb_u_state kb_state);

//
typedef void (*ConvertKbStateToZxHook)(kb_u_state* kbState, uint8_t* zxKb);
void setZxExtKeysHook(ConvertKbStateToZxHook customExtKeysConverter);
void setZxExtKeysDefault();
