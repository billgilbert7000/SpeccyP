/*
данные передаваеиые через I2C

ibuff[0] какие устройства подключены по USB 
ibuff[1] мышь кнопки 
ibuff[2] мышь X
ibuff[3] мышь Y
ibuff[4] джойстик кемпстон пока зарезервированно данные с него можно наверное уже куда то пихать отдает 0xff если не подключен 
ibuff[5 6 7 8 9] резерв
ibuff[10]   по ibuff[25] клавиатура
ibuff[26] ibuff[31] пока оставил для ровного счета можно их не считывать

*/

#include "usb_key.h"
#include "kb_u_codes.h"
#include "ps2.h"
#include "I2C_rp2040.h"
#include "g_config.h"
#include "config.h"
#include "tusb.h"

#include "tusb_config.h"

#include "xinput_host.h"

// usb устройства
uint8_t usb_device;// 1 клавиатура 2 мышь 3 клавиатура+мышь

kb_u_state kb_st_ps2;

///////////////////////////////////////////////////////////////////////////////////////////
/* ============================================================================================= */
/* ============                        XBOX controllers                          =============== */
/* ============================================================================================= */



//================================================================================================
//Since https://github.com/hathach/tinyusb/pull/2222, we can add in custom vendor drivers easily
usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count){
    *driver_count = 1;
    return &usbh_xinput_driver;
}
//===============================================
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* xid_itf, uint16_t len)
//void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) 
{
  //  auto xid_itf = (xinputh_interface_t *)report;
    const xinput_gamepad_t* p = &xid_itf->pad;
          joy_k = 0x00;//#1F - кемпстон джойстик 0001 1111
  //  if (p->wButtons & XINPUT_GAMEPAD_A)   joy_k |=  0x20  ;//0x1000;// A; 0001.0000 00x0.0000
  //  if (p->wButtons & XINPUT_GAMEPAD_B)  joy_k |= 0x10; //(1 << 5);//0x2000");// B 0x0010 0000 000x 0000

//gamepad1_bits.a = p->wButtons & XINPUT_GAMEPAD_A;

    const uint16_t dpad = p->wButtons ;//& 0x0f;
     if (!dpad) {
      //  gamepad1_bits.up = p->sThumbLY > 0 || p->sThumbRY > 0;
      //  gamepad1_bits.down = p->sThumbLY < 0 || p->sThumbRY < 0;

      //  gamepad1_bits.right = p->sThumbLX > 0 || p->sThumbRX > 0;
      //  gamepad1_bits.left = p->sThumbLX < 0 || p->sThumbRX < 0;
    }
    else {
     //   if (dpad == XINPUT_GAMEPAD_DPAD_UP ) joy_k |=(1 << 3); ;//0x0001
     //   if (dpad == XINPUT_GAMEPAD_DPAD_DOWN ) joy_k |=(1 << 2); ;//0x0002
    //    if (dpad == XINPUT_GAMEPAD_DPAD_LEFT ) joy_k |=(1 << 1); ;//0x0004
    //    if (dpad == XINPUT_GAMEPAD_DPAD_RIGHT ) joy_k |=(1 << 0); ;//0x0008
//if (dpad == 9) joy_k |=(1 << 3)|(1 << 0) ;
//if (dpad == 9) joy_k |=(1 << 3)|(1 << 0) ;

joy_k |= ((dpad&1)<<3) | ((dpad&2)<<1) | ((dpad&4)>>1) | ((dpad&8)>>3) | ((dpad& 0x1000)>>7) | ((dpad& 0x2000)>>9);






      //  gamepad1_biXINPUT_GAMEPAD_DPAD_RIGHTts.up = dpad & XINPUT_GAMEPAD_DPAD_UP;
      //  gamepad1_bits.left = dpad & XINPUT_GAMEPAD_DPAD_LEFT;
      //  gamepad1_bits.right = dpad & XINPUT_GAMEPAD_DPAD_RIGHT;
    }
/*
    if (p->wButtons & XINPUT_GAMEPAD_GUIDE) {
        gamepad1_bits.start = true;
        gamepad1_bits.select = true;
    }
    else {
        gamepad1_bits.start = p->wButtons & XINPUT_GAMEPAD_START;
        gamepad1_bits.select = p->wButtons & XINPUT_GAMEPAD_BACK;
    }
    const uint8_t dpad = p->wButtons & 0xf;

    if (!dpad) {
        gamepad1_bits.up = p->sThumbLY > 0 || p->sThumbRY > 0;
        gamepad1_bits.down = p->sThumbLY < 0 || p->sThumbRY < 0;

        gamepad1_bits.right = p->sThumbLX > 0 || p->sThumbRX > 0;
        gamepad1_bits.left = p->sThumbLX < 0 || p->sThumbRX < 0;
    }
    else {
        gamepad1_bits.down = dpad & XINPUT_GAMEPAD_DPAD_DOWN;
        gamepad1_bits.up = dpad & XINPUT_GAMEPAD_DPAD_UP;
        gamepad1_bits.left = dpad & XINPUT_GAMEPAD_DPAD_LEFT;
        gamepad1_bits.right = dpad & XINPUT_GAMEPAD_DPAD_RIGHT;
    }
*/
    /*char tmp[128];
    sprintf(tmp, "[%02x, %02x], Type: %s, Buttons %04x, LT: %02x RT: %02x, LX: %d, LY: %d, RX: %d, RY: %d\n",
                 dev_addr, instance, type_str, p->wButtons, p->bLeftTrigger, p->bRightTrigger, p->sThumbLX, p->sThumbLY, p->sThumbRX, p->sThumbRY);
    draw_text(tmp, 0,0, 15,0);*/

    tuh_xinput_receive_report(dev_addr, instance);
}
//==============================
/*
case 0:  joy_k |= (1 << 3); break; //3 бит  up
      case 1:  joy_k |= (1 << 3); joy_k |= (1 << 0); break; //3 бит up  0 бит  right  up right
      case 2:  joy_k |= (1 << 0); break; // 0 бит  right
      case 3:  joy_k |= (1 << 2); joy_k |= (1 << 0); break; //2 бит down  0 бит  right  down right
      case 4:  joy_k |= (1 << 2); break; //2 бит down  
      case 5:  joy_k |= (1 << 2); joy_k |= (1 << 1); break; //2 бит down  1 бит  left  down left
      case 6:  joy_k |= (1 << 1); break; // 1 бит  left
      case 7:  joy_k |= (1 << 3); joy_k |= (1 << 1); break; //3 бит up  1 бит  left  up left


*/


//===============================================
/* 
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* xid_itf, uint16_t len)
{
    if (len==0) return;
    const xinput_gamepad_t *p = &xid_itf->pad;
    const char* type_str;

    if (xid_itf->last_xfer_result == XFER_RESULT_SUCCESS)
    {
        switch (xid_itf->type)
        {
            case 1: type_str = "Xbox One";          break;
            case 2: type_str = "Xbox 360 Wireless"; break;
            case 3: type_str = "Xbox 360 Wired";    break;
            case 4: type_str = "Xbox OG";           break;
            default: type_str = "Unknown";
        }

        if (xid_itf->connected && xid_itf->new_pad_data)
        {
      //      debug_print("[%02x, %02x], Type: %s, Buttons %04x, LT: %02x RT: %02x, LX: %d, LY: %d, RX: %d, RY: %d\n",
      //          dev_addr, instance, type_str, p->wButtons, p->bLeftTrigger, p->bRightTrigger, p->sThumbLX, p->sThumbLY, p->sThumbRX, p->sThumbRY);

            //How to check specific buttons
        //    if (p->wButtons & XINPUT_GAMEPAD_A) debug_print("You are pressing A\n");
        }
    }
    tuh_xinput_receive_report(dev_addr, instance);  
}*/

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
 //   debug_print("XINPUT MOUNTED %02x %d\n", dev_addr, instance);
    // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
    // on the in pipe before setting LEDs etc. So just start getting data until a controller is connected.
    if (xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false)
    {
        tuh_xinput_receive_report(dev_addr, instance);
        return;
    }

      msg_bar=8;
      wait_msg = 5000; 

    tuh_xinput_set_led(dev_addr, instance, 0, true);
    tuh_xinput_set_led(dev_addr, instance, 1, true);
    tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
 //   debug_print("XINPUT UNMOUNTED %02x %d\n", dev_addr, instance);
}

///////////////////////////////////////////////////////////////////////////////////////////



//----------------------------------------------
//   
//----------------------------------------------
/* 
 
bool key_ps2(void){return decode_PS2(); }
bool key_ps2_ms(void){ I2C_Read_nbytes_stop(i2c0,0x77, ibuff, 32);    return decode_PS2();  };// мышь
bool key_usb(void){ I2C_Read_nbytes_stop(i2c0,0x77, ibuff, 32);    return decode_USB();  };// клавиатура
bool key_usb_ms(void) { I2C_Read_nbytes_stop(i2c0,0x77, ibuff, 32);    return decode_USB();  }// клавиатура + мышь
bool key_usb_hid(void){return (decode_USB_HID() || decode_PS2() ); }

bool (*decode_key)(); // определение указателя на функцию
void init_decode_key(uint8_t kms)
{



   switch (kms)
   {
   case 0:
      decode_key = key_ps2;
      break; // 0 клавиатура ps/2
   case 1:
      decode_key = key_ps2_ms;
      break; // 1  клавиатура ps/2 + мышь
   case 2:
      decode_key = key_usb;
      break; // 2  клавиатура usb
   case 3:
      decode_key = key_usb_ms;
      break; // 3 клавиатура usb + мышь


   case 8:
      
      decode_key = key_usb_hid;
    //  tuh_init(0);






      break; // 0 клавиатура ps/2



   default:
      decode_key = key_ps2;
      break;
   }
} */
//================================================
bool decode_key (bool menu_mode)
{
 
  tuh_task(); // tinyusb host task

 // if (decode_PS2()) return true;

if (menu_mode) // если это меню
{
if (flag_usb_kb) 
{
 // sleep_ms(120);
 //flag_usb_kb=false;
if ((kb_st_ps2.u[2] & KB_U2_DOWN) | (kb_st_ps2.u[2] & KB_U2_UP)) 
{
  sleep_ms(DELAY_KEY);
flag_usb_kb = true;
}

   else flag_usb_kb = false;   // клавиша клавиатуры usb была нажата / есть событие usb клавиатуры

return true;
}
//flag_usb_kb=false;
return false; //decode_PS2();
}

else
{
if (flag_usb_kb) 
{
  flag_usb_kb=false;
return true;
}
return false; //decode_PS2();
}



   if (flag_usb_kb) {
       keyboard (); 
  if (menu_mode) 
   { flag_usb_kb = true; 
     sleep_ms(DELAY_KEY);
   }
    else flag_usb_kb = false; 
     return true;}

//if (decode_PS2()) return true;
   return false;

}

//================================================

//==============================================================
bool decode_key_joy()
{
 // sleep_ms(delay_key);
//tuh_task(); /* tinyusb host task*/
 if (decode_joy_to_keyboard())
    return true;
 if ((decode_PS2()) | (decode_key(true))) return true;
    return true;

  return false;
}
//==============================================================
/******************************************************************************************/
void task_usb(void)
{
tuh_task(); // tinyusb host task
}
//-------------------------------------------
uint8_t keyboard_addr = 0;
uint8_t keyboard_instance = 0;
//bool keyboard_mounted = false;
bool auto_toggle_start = false;
static uint32_t successful_led_toggles = 0;
static uint8_t leds = 7;
static bool is_on = false;
static bool set_report_failed = false;
 //----------------------------------------------------------
void scancode_usb_s(uint8_t code){

  if (code & 0x02) kb_st_ps2.u[1]|=KB_U1_L_SHIFT;// left shift  
 //  if (code & 0x02) kb_st_ps2.u[1]|=KB_U1_R_SHIFT;// left shift  переназначение на правый
   if (code & 0x20) kb_st_ps2.u[1]|=KB_U1_R_SHIFT;// righr shift 

   
   if (code & 0x04) kb_st_ps2.u[1]|=KB_U1_L_ALT;  // left alt     0000 0100
   if (code & 0x40) kb_st_ps2.u[1]|=KB_U1_R_ALT;; // right alt   0100 0000
   
   if (code & 0x01) kb_st_ps2.u[1]|=KB_U1_L_CTRL; // right ctrl   
   if (code & 0x10) kb_st_ps2.u[1]|=KB_U1_R_CTRL;
   
   if (code & 0x08) kb_st_ps2.u[1]|=KB_U1_L_WIN;; // win  l
   if (code & 0x80) kb_st_ps2.u[1]|=KB_U1_L_WIN;; // win r
   }
//----------------------------------------------------------
 void scancode_usb(uint8_t code){
   
   switch (code)
   {
    case 0x00:   break;
    case 0x04:  kb_st_ps2.u[0]|=KB_U0_A; break;
    case 0x05:  kb_st_ps2.u[0]|=KB_U0_B; break;
    case 0x06:  kb_st_ps2.u[0]|=KB_U0_C; break;
    case 0x07:  kb_st_ps2.u[0]|=KB_U0_D; break;
    case 0x08:  kb_st_ps2.u[0]|=KB_U0_E; break;
    case 0x09:  kb_st_ps2.u[0]|=KB_U0_F; break;
    case 0x0a:  kb_st_ps2.u[0]|=KB_U0_G; break;
    case 0x0b:  kb_st_ps2.u[0]|=KB_U0_H; break;
    case 0x0c:  kb_st_ps2.u[0]|=KB_U0_I; break;
    case 0x0d:  kb_st_ps2.u[0]|=KB_U0_J; break;
    
    case 0x0e:  kb_st_ps2.u[0]|=KB_U0_K; break;
    case 0x0f:  kb_st_ps2.u[0]|=KB_U0_L; break;
    case 0x10:  kb_st_ps2.u[0]|=KB_U0_M; break;
    case 0x11:  kb_st_ps2.u[0]|=KB_U0_N; break;
    case 0x12:  kb_st_ps2.u[0]|=KB_U0_O; break;
    case 0x13:  kb_st_ps2.u[0]|=KB_U0_P; break;
    case 0x14:  kb_st_ps2.u[0]|=KB_U0_Q; break;
    case 0x15:  kb_st_ps2.u[0]|=KB_U0_R; break;
    case 0x16:  kb_st_ps2.u[0]|=KB_U0_S; break;
    case 0x17:  kb_st_ps2.u[0]|=KB_U0_T; break;
    
    case 0x18:  kb_st_ps2.u[0]|=KB_U0_U; break;
    case 0x19:  kb_st_ps2.u[0]|=KB_U0_V; break;
    case 0x1a:  kb_st_ps2.u[0]|=KB_U0_W; break;
    case 0x1b:  kb_st_ps2.u[0]|=KB_U0_X; break;
    case 0x1c:  kb_st_ps2.u[0]|=KB_U0_Y; break;
    case 0x1d:  kb_st_ps2.u[0]|=KB_U0_Z; break;
    
    case 0x2f:  kb_st_ps2.u[0]|=KB_U0_LEFT_BR; break;
    case 0x30:  kb_st_ps2.u[0]|=KB_U0_RIGHT_BR; break;
    case 0x33:  kb_st_ps2.u[0]|=KB_U0_SEMICOLON; break;
    case 0x34:  kb_st_ps2.u[0]|=KB_U0_QUOTE; break;
    case 0x36:  kb_st_ps2.u[0]|=KB_U0_COMMA; break;
    case 0x37:  kb_st_ps2.u[0]|=KB_U0_PERIOD; break;
    
    //1 -----------
    case 0x27:  kb_st_ps2.u[1]|=KB_U1_0; break;
    case 0x1e:  kb_st_ps2.u[1]|=KB_U1_1; break;
    case 0x1f:  kb_st_ps2.u[1]|=KB_U1_2; break;
    case 0x20:  kb_st_ps2.u[1]|=KB_U1_3; break;
    case 0x21:  kb_st_ps2.u[1]|=KB_U1_4; break;
    case 0x22:  kb_st_ps2.u[1]|=KB_U1_5; break;
    case 0x23:  kb_st_ps2.u[1]|=KB_U1_6; break;
    case 0x24:  kb_st_ps2.u[1]|=KB_U1_7; break;
    case 0x25:  kb_st_ps2.u[1]|=KB_U1_8; break;
    case 0x26:  kb_st_ps2.u[1]|=KB_U1_9; break;
    
    case 0x2d:  kb_st_ps2.u[1]|=KB_U1_MINUS; break;
    case 0x2e:  kb_st_ps2.u[1]|=KB_U1_EQUALS; break;
    case 0x31:  kb_st_ps2.u[1]|=KB_U1_BACKSLASH;break;
    case 0x2a:  kb_st_ps2.u[1]|=KB_U1_BACK_SPACE; break;
    case 0x28:  kb_st_ps2.u[1]|=KB_U1_ENTER; break;
    case 0x38:  kb_st_ps2.u[1]|=KB_U1_SLASH; break;
    case 0x32:  kb_st_ps2.u[1]|=KB_U1_TILDE; break;
    case 0x2b:  kb_st_ps2.u[1]|=KB_U1_TAB; break;
    case 0x39:  kb_st_ps2.u[1]|=KB_U1_CAPS_LOCK; break;
    case 0x29:  kb_st_ps2.u[1]|=KB_U1_ESC;break;
    case 0x65:  kb_st_ps2.u[1]|=KB_U1_MENU;  break;
    case 0x2c:  kb_st_ps2.u[1]|=KB_U1_SPACE; break;
    
    //2 -----------
    case 0x48:  kb_st_ps2.u[2]|=KB_U2_PAUSE_BREAK; break;
    
    case 0x53:  kb_st_ps2.u[2]|=KB_U2_NUM_LOCK; break;
    case 0x59:  kb_st_ps2.u[2]|=KB_U2_NUM_1; break;
    case 0x5a:  kb_st_ps2.u[2]|=KB_U2_NUM_2; break;
    case 0x5b:  kb_st_ps2.u[2]|=KB_U2_NUM_3; break;
    case 0x5c:  kb_st_ps2.u[2]|=KB_U2_NUM_4; break;
    case 0x5d:  kb_st_ps2.u[2]|=KB_U2_NUM_5; break;
    case 0x5e:  kb_st_ps2.u[2]|=KB_U2_NUM_6; break;
    case 0x5f:  kb_st_ps2.u[2]|=KB_U2_NUM_7; break;
    case 0x60:  kb_st_ps2.u[2]|=KB_U2_NUM_8; break;
    case 0x61:  kb_st_ps2.u[2]|=KB_U2_NUM_9; break;
    case 0x62:  kb_st_ps2.u[2]|=KB_U2_NUM_0; break;
    case 0x63:  kb_st_ps2.u[2]|=KB_U2_NUM_PERIOD; break;
    
    case 0x55:  kb_st_ps2.u[2]|=KB_U2_NUM_MULT; break;
    case 0x56:  kb_st_ps2.u[2]|=KB_U2_NUM_MINUS; break;
    case 0x57:  kb_st_ps2.u[2]|=KB_U2_NUM_PLUS; break;
    
    case 0x47:  kb_st_ps2.u[2]|=KB_U2_SCROLL_LOCK;break;
    case 0x46:  kb_st_ps2.u[2]|=KB_U2_PRT_SCR; break;

    case 0x54:  kb_st_ps2.u[2]|=KB_U2_NUM_SLASH;break;
    case 0x58:  kb_st_ps2.u[2]|=KB_U2_NUM_ENTER;  break;
    case 0x52:  kb_st_ps2.u[2]|=KB_U2_UP;  break;
    case 0x51:  kb_st_ps2.u[2]|=KB_U2_DOWN;  break;
    case 0x4f:  kb_st_ps2.u[2]|=KB_U2_RIGHT; break;
    case 0x50:  kb_st_ps2.u[2]|=KB_U2_LEFT;  break;
    case 0x4c:  kb_st_ps2.u[2]|=KB_U2_DELETE; break;
    case 0x4d:  kb_st_ps2.u[2]|=KB_U2_END;  break;
    case 0x4e:  kb_st_ps2.u[2]|=KB_U2_PAGE_DOWN;  break;
    case 0x4b:  kb_st_ps2.u[2]|=KB_U2_PAGE_UP;  break; 
    case 0x4a:  kb_st_ps2.u[2]|=KB_U2_HOME; break;
    case 0x49:  kb_st_ps2.u[2]|=KB_U2_INSERT;  break;

    
    //3 -----------
    case 0x3a:  kb_st_ps2.u[3]|=KB_U3_F1; break;
    case 0x3b:  kb_st_ps2.u[3]|=KB_U3_F2; break;
    case 0x3c:  kb_st_ps2.u[3]|=KB_U3_F3; break;
    case 0x3d:  kb_st_ps2.u[3]|=KB_U3_F4; break;
    case 0x3e:  kb_st_ps2.u[3]|=KB_U3_F5; break;
    case 0x3f:  kb_st_ps2.u[3]|=KB_U3_F6; break;
    case 0x40:  kb_st_ps2.u[3]|=KB_U3_F7; break;
    case 0x41:  kb_st_ps2.u[3]|=KB_U3_F8; break;
    case 0x42:  kb_st_ps2.u[3]|=KB_U3_F9; break;
    case 0x43:  kb_st_ps2.u[3]|=KB_U3_F10;break;
    case 0x44:  kb_st_ps2.u[3]|=KB_U3_F11; break;
    case 0x45:  kb_st_ps2.u[3]|=KB_U3_F12; break;
    default : break;
   }
 }
//----------------------------------------------------------------------------
void keyboard (void)
{
    return;
        /* kb_st_ps2.u[0] = 0;
    kb_st_ps2.u[1] = 0;
    kb_st_ps2.u[2] = 0;
    kb_st_ps2.u[3] = 0;


    scancode_usb_s(key_report[0]); // Ctrl, Shift, Alt, Win   

    scancode_usb(key_report[2]);
    
    scancode_usb(key_report[3]);
   
    scancode_usb(key_report[4]);
    
    scancode_usb(key_report[5]);

    scancode_usb(key_report[6]);
 
    scancode_usb(key_report[7]);
    

    */

   
}
//--------------------------------------------------------------------
void keyboard_report (uint8_t const *report, uint16_t len)
{


  flag_usb_kb = true;   // клавиша клавиатуры usb была нажата / есть событие usb клавиатуры
 //if (report[0]&&report[2]&&report[3]&&report[4]&&report[5]&&report[6]&&report[7]==0)   flag_usb_kb = false;
 
    kb_st_ps2.u[0] = 0;
    kb_st_ps2.u[1] = 0;
    kb_st_ps2.u[2] = 0;
    kb_st_ps2.u[3] = 0;

    scancode_usb_s(report[0]); // Ctrl, Shift, Alt, Win   /**< Keyboard modifier (KEYBOARD_MODIFIER_* masks). */
   
  //  scancode_usb(key_report[1]);/**< Reserved for OEM use, always set to 0. */

    /**< Key codes of the currently pressed keys. */

    scancode_usb(report[2]);
    
    scancode_usb(report[3]);
   
    scancode_usb(report[4]);
    
    scancode_usb(report[5]);

    scancode_usb(report[6]);
 
    scancode_usb(report[7]);
    
//if ((kb_st_ps2.u[0]=0)&(kb_st_ps2.u[1]=0)&(kb_st_ps2.u[2]=0)&(kb_st_ps2.u[3]=0)) flag_usb_kb = true;
 //  else flag_usb_kb = false;   // клавиша клавиатуры usb была нажата / есть событие usb клавиатуры
   
}

//-------------------------------------------------------------
// USB vid pid 
//-------------------------------------------------------------
void print_lsusb(void) {
  bool no_device = true;
//   gamepad_hid=0;
  for ( uint8_t daddr = 1; daddr < CFG_TUH_DEVICE_MAX+1; daddr++ ) {

    if ( tuh_mounted(daddr-1)) {
  
tuh_vid_pid_get(daddr, &vid, &pid);
// debug_print("Device %u: VID %04X PID %04X\r\n", daddr,vid, pid);




  //  if (vid==0x20bc && pid==0x5500) 
    
  //  gamepad_hid=1;

    //  msg_bar=6;
    //  wait_msg = 5000; 
    //  no_device = false;
    }
  }

  if (no_device) {
   //  debug_print("No device connected (except hub)");
   return;
    }


}
//--------------------------------------------------------------------
// Вызывается, когда устройство с интерфейсом hid отключено
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
 // led_blink(1);
 // LED_OFF();
 // keyboard_mounted = false;
  usb_device = 0;
   
  print_lsusb(); // print device summary
}


//--------------------------------------------------------------------
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  
  (void)desc_report;
  (void)desc_len;

  //const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD )
  {
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      // Ошибка: не удается запросить отчет
     // keyboard_mounted = false;
    }
    else
    {
       // led_blink(2);
      //  LED_ON();
        keyboard_addr = dev_addr;
        keyboard_instance = instance;
       // keyboard_mounted = true;
        usb_device = usb_device | 1;

    }
  }
  
 if (itf_protocol == HID_ITF_PROTOCOL_MOUSE )
  {
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      // Ошибка: не удается запросить отчет
    //  keyboard_mounted = false;
    }
    else
    {

/*         keyboard_addr = dev_addr;
        keyboard_instance = instance;
        keyboard_mounted = true; */
        usb_device = usb_device | 2;

    }
  }


   //   debug_print("HID Interface Protocol = other\n");
     tuh_hid_receive_report(dev_addr, instance);// !!! 
//print_lsusb(); // print device summary

tuh_vid_pid_get(dev_addr, &vid, &pid);

    if (vid==0x20bc && pid==0x5500) 
    {
    //gamepad_hid=1;

    gamepad_addr = dev_addr;
    gamepad_instance = instance;
    gamepad_hid = dev_addr+instance;
    }

      msg_bar=6;
      wait_msg = 5000; 
    

}
//-----------------------------------------------------------------
 // send mouse report to usb device CDC
static void mouse_report(uint8_t const *report, uint16_t len) {
       u_int8_t mouse_b = 0xff; ;//#FADF - поpт  кнопок
       static uint8_t ms_x =0;
       static uint8_t ms_y =0;
       static uint8_t ms_w =0;
       static uint8_t mouse_x =0;
       static uint8_t mouse_y =0;
       static uint8_t mouse_w =0;

switch (len)
{
case 6: /*мышь тачпад */
  
  if (report[1] & (1<<0) ) mouse_b &= ~(1 << 1); //1 бит - состояние левой кнопки;MOUSE_BUTTON_LEFT
  if (report[1] & (1<<1) ) mouse_b &= ~(1 << 0); //0 бит - состояние правой кнопки; MOUSE_BUTTON_RIGHT
  if (report[1] & (1<<2) ) mouse_b &= ~(1 << 2); //2 бит - состояние сpедней кнопки; MOUSE_BUTTON_MIDDLE)
 
  // X
    if (ms_x != report[2]){ 
    if (report[2]&(1<<7)) mouse_x = mouse_x-1;//mouse_x--;//left
    else mouse_x = mouse_x+1;//mouse_x++;//right 
    ms_x = report[2];
    }

  // мышь 8 байт 
  //  if (len==8) report++;
  // Y   
    if (ms_y != report[3]){ 
    if (report[3]&(1<<7)) mouse_y = mouse_y+1;//mouse_y++;//up
    else mouse_y = mouse_y-1;//mouse_y--; //down
    ms_y = report[3];
    }
  
  // колесо
    if (report[5] !=0){ 
    if (report[5]&(1<<7)) mouse_w++;//up
    else mouse_w--; //down
   }
   ibuff[1] =mouse_b; ;//#FADF - поpт  кнопок
   ibuff[2]=mouse_x ; // #FBDF
   ibuff[3]=(mouse_y); // #FFDF
 // int count = sprintf(tempbuf, "[%u] %c%c%c %d %d %d\r\n", dev_addr, l, m, r, report->x, report->y, report->wheel);
break;


case 3:
  
  if (report[0] & (1<<0) ) mouse_b &= ~(1 << 1); //1 бит - состояние левой кнопки;MOUSE_BUTTON_LEFT
  if (report[0] & (1<<1) ) mouse_b &= ~(1 << 0); //0 бит - состояние правой кнопки; MOUSE_BUTTON_RIGHT
  if (report[0] & (1<<2) ) mouse_b &= ~(1 << 2); //2 бит - состояние сpедней кнопки; MOUSE_BUTTON_MIDDLE)
 
  // X
    if (ms_x != report[1]){ 
    if (report[1]&(1<<7)) mouse_x = mouse_x-1;//mouse_x--;//left
    else mouse_x = mouse_x+1;//mouse_x++;//right 
    ms_x = report[1];
    }

  // мышь 8 байт 
  //  if (len==8) report++;
  // Y   
    if (ms_y != report[2]){ 
    if (report[2]&(1<<7)) mouse_y = mouse_y+1;//mouse_y++;//up
    else mouse_y = mouse_y-1;//mouse_y--; //down
    ms_y = report[2];
    }
  
  // колесо
    if (report[3] !=0){ 
    if (report[3]&(1<<7)) mouse_w++;//up
    else mouse_w--; //down
   }
   ibuff[1] =mouse_b; ;//#FADF - поpт  кнопок
   ibuff[2]=mouse_x ; // #FBDF
   ibuff[3]=(mouse_y); // #FFDF

break;

 
}
  
}

// send mouse report to usb device CDC
static void mouse_report1( hid_mouse_report_t const* report) 
{
       u_int8_t mouse_b = 0xff; ;//#FADF - поpт  кнопок
       static uint8_t ms_x =0;
       static uint8_t ms_y =0;
       static uint8_t ms_w =0;
       static uint8_t mouse_x =0;
       static uint8_t mouse_y =0;
       static uint8_t mouse_w =0;

  if (report->buttons & MOUSE_BUTTON_LEFT ) mouse_b &= ~(1 << 1); //1 бит - состояние левой кнопки;MOUSE_BUTTON_LEFT
  if (report->buttons & MOUSE_BUTTON_RIGHT  ) mouse_b &= ~(1 << 0); //0 бит - состояние правой кнопки; MOUSE_BUTTON_RIGHT
  if (report->buttons & MOUSE_BUTTON_MIDDLE ) mouse_b &= ~(1 << 2); //2 бит - состояние сpедней кнопки; MOUSE_BUTTON_MIDDLE)

 
  // X
    if (ms_x != report->x){ 
    if (report->x&(1<<7)) mouse_x = mouse_x-1;//mouse_x--;//left
    else mouse_x = mouse_x+1;//mouse_x++;//right 
    ms_x = report->x;
    }


  // Y   
    if (ms_y != report->y){ 
    if (report->y&(1<<7)) mouse_y = mouse_y+1;//mouse_y++;//up
    else mouse_y = mouse_y-1;//mouse_y--; //down
    ms_y = report->y;
    }
  
  // колесо
    if (report->wheel !=0){ 
    if (report->wheel&(1<<7)) mouse_w++;//up
    else mouse_w--; //down
   }
   ibuff[1] =mouse_b; ;//#FADF - поpт  кнопок
   ibuff[2]=mouse_x ; // #FBDF
   ibuff[3]=(mouse_y); // #FFDF
 // int count = sprintf(tempbuf, "[%u] %c%c%c %d %d %d\r\n", dev_addr, l, m, r, report->x, report->y, report->wheel); 
}
//----------------------------------------------------------
 //кемпстон джойстик
//void __not_in_flash_func(gamepad_1)(uint8_t const *report,uint8_t instance)
void gamepad_10(uint8_t const *report,uint8_t instance)
{

//uint8_t joy_k = 0x00 ;//#1F - кемпстон джойстик 0001 1111
joy_k = 0x00;//#1F - кемпстон джойстик 0001 1111

//if (report[0] !=0)
{
  uint8_t b = report[0];
//if (report[0] & 0b00000001)  joy_k |= (1 << 4);//debug_print("PRESSING A\n");// A
joy_k |= (b & 0b00000001)<<4;//4 A
joy_k |= (b & 0b00000010)<<4;//5 B
joy_k |= (b & 0b00001000)<<2;//6 X
joy_k |= (b & 0b00010000)<<3;//7 Y

/* if (report[0] & 0b00000010)  joy_k |= (1 << 5);//debug_print("PRESSING B\n");// B
if (report[0] & 0b00001000)  joy_k |= (1 << 6);//debug_print("PRESSING X\n");// X
if (report[0] & 0b00010000)  joy_k |= (1 << 7);// debug_print("PRESSING Y\n");// Y
 */
}

if (report[1] !=0)
{
//if (report[1] & 0b00000001)  debug_print("PRESSING L2\r\n");// L2
//if (report[1] & 0b00000010)  debug_print("PRESSING R2\r\n");// R2
//if (report[1] & 0b00000100)  debug_print("PRESSING SELECT\r\n");// SELECT
//if (report[1] & 0b00001000)  debug_print("PRESSING START\r\n");// START
//if (report[1] & 0b00010000)  debug_print("PRESSING MODE\r\n");// MODE
//debug_print("1: 0x%02X 0x%02X 0x%02X 0x%02X  0x%02X 0x%02X 0x%02X ", report[0] ,report[1], report[2], report[3], report[4], report[5], report[6], report[7]);
//debug_print("2: 0x%02X 0x%02X 0x%02X 0x%02X  0x%02X 0x%02X 0x%02X\r\n", report[8] , report[9], report[10], report[11], report[12], report[13], report[14]);



}
if (report[2] !=0x0f)
{     
     uint8_t p = report[2] & 0x0f; // 0000XXXX 0-8
     switch (p){
      case 0:  joy_k |= (1 << 3); break; //3 бит  up
      case 1:  joy_k |= (1 << 3); joy_k |= (1 << 0); break; //3 бит up  0 бит  right  up right
      case 2:  joy_k |= (1 << 0); break; // 0 бит  right
      case 3:  joy_k |= (1 << 2); joy_k |= (1 << 0); break; //2 бит down  0 бит  right  down right
      case 4:  joy_k |= (1 << 2); break; //2 бит down  
      case 5:  joy_k |= (1 << 2); joy_k |= (1 << 1); break; //2 бит down  1 бит  left  down left
      case 6:  joy_k |= (1 << 1); break; // 1 бит  left
      case 7:  joy_k |= (1 << 3); joy_k |= (1 << 1); break; //3 бит up  1 бит  left  up left
      default: break;
      }


}

  //  msg_bar=7;
  //    wait_msg = 1000;  
  
// data = data| joy_k;

//  joy_key_ext = joy_key_ext | joy_k;



}
 //----------------------------------------------------------


 //----------------------------------------------------------
 //кемпстон джойстик
//void __not_in_flash_func(gamepad_1)(uint8_t const *report,uint8_t instance)
void gamepad_1(uint8_t const *report,uint8_t instance)
{

//uint8_t joy_k = 0x00 ;//#1F - кемпстон джойстик 0001 1111
joy_k = 0x00;//#1F - кемпстон джойстик 0001 1111

if (report[0] !=0)
{
if (report[0] & 0b00000001)  joy_k |=  0x20  ;//(1 << 4);//debug_print("PRESSING A\n");// A
if (report[0] & 0b00000010)  joy_k |= 0x10; //(1 << 5);//debug_print("PRESSING B\n");// B
if (report[0] & 0b00001000)  joy_k |= 0x40;//(1 << 6);//debug_print("PRESSING X\n");// X
if (report[0] & 0b00010000)  joy_k |= 0x80;//(1 << 7);// debug_print("PRESSING Y\n");// Y

//if (report[0] & 0b00100000)  joy_k |=0x10;
if (report[0] & 0b10000000)  joy_k |=0x20;// дубль A
}

if (report[1] !=0)
{
//if (report[1] & 0b00000001)  //debug_print("PRESSING L2\r\n");// L2
//joy_k |=  0x20  ;
if (report[1] & 0b00000010) // debug_print("PRESSING R2\r\n");// R2
joy_k |= 0x10;// fire дубль B

if (report[1] & 0b00000100)  //debug_print("PRESSING SELECT\r\n");// SELECT
 //{ joy_key_ext = SELECT_J;joy_k=0; return;}
 joy_k |=  SELECT_J ;
if (report[1] & 0b00001000) // debug_print("PRESSING START\r\n");// START
// { joy_key_ext = START_J;joy_k=0; return;}
joy_k |=START_J; 
//if (report[1] & 0b00010000)  debug_print("PRESSING MODE\r\n");// MODE
//debug_print("1: 0x%02X 0x%02X 0x%02X 0x%02X  0x%02X 0x%02X 0x%02X ", report[0] ,report[1], report[2], report[3], report[4], report[5], report[6], report[7]);
//debug_print("2: 0x%02X 0x%02X 0x%02X 0x%02X  0x%02X 0x%02X 0x%02X\r\n", report[8] , report[9], report[10], report[11], report[12], report[13], report[14]);



}
if (report[2] !=0x0f)
{     
     uint8_t p = report[2] & 0x0f; // 0000XXXX 0-8
     switch (p){
      case 0:  joy_k |= (1 << 3); break; //3 бит  up
      case 1:  joy_k |= (1 << 3); joy_k |= (1 << 0); break; //3 бит up  0 бит  right  up right
      case 2:  joy_k |= (1 << 0); break; // 0 бит  right
      case 3:  joy_k |= (1 << 2); joy_k |= (1 << 0); break; //2 бит down  0 бит  right  down right
      case 4:  joy_k |= (1 << 2); break; //2 бит down  
      case 5:  joy_k |= (1 << 2); joy_k |= (1 << 1); break; //2 бит down  1 бит  left  down left
      case 6:  joy_k |= (1 << 1); break; // 1 бит  left
      case 7:  joy_k |= (1 << 3); joy_k |= (1 << 1); break; //3 бит up  1 бит  left  up left
      default: break;
      }


}

  //  msg_bar=7;
  //    wait_msg = 1000;  
  
// data = data| joy_k;

 //joy_key_ext = joy_k;

//data_joy =  joy_k;

}
 //----------------------------------------------------------







// Вызывается при получении отчета от устройства через конечную точку прерывания
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)

{
  (void) len;
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

 // joy_k =itf_protocol;
 // msg_bar=7;
  //    wait_msg = 5000; 

  switch(itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:

    keyboard_report(report, len);
   // process_kbd_report(dev_addr, instance, (hid_keyboard_report_t const*) report ); 
  
    break;

    case HID_ITF_PROTOCOL_MOUSE:
    mouse_report(report, len);
      break;

 

    default: 
       //  if (gamepad_hid==1)
        {
          uint8_t k =  dev_addr+instance;

    //   if (gamepad_hid == k)
if (gamepad_addr == dev_addr)
    if (instance==0)
           gamepad_1(report,instance);// kempston
        }  
    break;
  }

  // продолжайте запрашивать получение отчета
 if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    // Не удается запросить отчет
  }
}
//-------------------------------------------------------
bool init_usb_hid(void)
{
  return tuh_init(0);
}
//--------------------------------------------------------







void wait_enter(void)
     {
      
      while(1)
      {
       decode_key_joy();
      if (!(kb_st_ps2.u[1]&KB_U1_ENTER) ) return;
   //    sleep_ms(DELAY_KEY);
      }
    } 

void wait_esc(void)
     {
    //  return;
      while(1)
      {
       decode_key_joy();
      if (!(kb_st_ps2.u[1]&KB_U1_ESC) ) return;
   //    sleep_ms(DELAY_KEY);
      }
    } 