
#include "config.h"  
#include "hardware/vreg.h"


#include "screen_util.h"
#include "tft_driver.h"

#include "util_sd.h"
#include "util_tap.h"
#include "SpeccyP.h"
#include "usb_key.h"// это добавить
#include "hardware/pwm.h"
#include "tusb.h"
#include "aySoft.h"
//#include <string.h> /* memset */
//#include <unistd.h> /* close */

//#include "math.h"
#define ABS(x) (((x)<0)?(-(x)):(x))
#define SIGN(x) (((x)<0)?(-(1)):(1))

uint8_t* screen_buf;
int scr_W=1;
int scr_H=1;


void init_screen(uint8_t* scr_buf,int scr_width,int scr_height)
{
    screen_buf=scr_buf;
    scr_W=scr_width;
    scr_H=scr_height;
};

bool draw_pixel(int x,int y,color_t color)
{
    if ((x<0)|(x>=scr_W)|(y<0)|(y>=scr_H))  return false;
    uint8_t* pix=&screen_buf[(y*scr_W+x)/2];
    uint8_t tmp=*pix;
    if (x&1) *pix=(tmp&0xf)|((color&0xf)<<4);
    else *pix=((tmp&0xf0))|((color&0xf));
    return true;
}

void draw_symbol(int x,int y,int symb,color_t colorText,color_t colorBg)
{

    for(int line=0;line<FONT_H;line++)
    {
        
        int yt=y+line;
        int xt=x;
          
           uint8_t symb_data=FONT[symb*FONT_H+line];

            for(int i=0;i<FONT_W;i++)
           {
            draw_pixel(xt++, yt, (symb_data & 0x01) ? colorText : colorBg);
                symb_data>>=1;
              //  symb_data<<=1;
           }
           
        }
    
}



void draw_text(int x,int y,char* text,color_t colorText,color_t colorBg)
{
  
   
    for(int line=0;line<FONT_H;line++)
    {
        char* symb=text;

        int yt=y+line;
        int xt=x;

        while(*symb)
        {
           uint8_t symb_data=FONT[*symb*FONT_H+line];
       //      uint8_t wfont = (*symb== 0x2e) ? 2 : FONT_W ;

      //    symb_data<<=1;
            for(int i=0;i<FONT_W;i++)
/* #ifdef FONT6X8 
 symb_data = (symb_data * 0x0202020202ULL & 0x010884422010ULL) % 1023; symb_data>>=1; // зеркалирование шрифта
#endif */
//symb_data = (symb_data * 0x0202020202ULL & 0x010884422010ULL) % 1023; //symb_data>>=1; // зеркалирование шрифта
  // uint8_t s = symb_data; s<<=1; symb_data=s | symb_data ;// bold font
 // uint8_t s = symb_data; s>>=1; symb_data=s | symb_data ;// bold font
       //    for(int i=0;i<FONT_W;i++) 
           {
        //    if (symb_data&(0x01))draw_pixel(xt++,yt,colorText); else draw_pixel(xt++,yt,colorBg);
            draw_pixel(xt++, yt, (symb_data & 0x01) ? colorText : colorBg);
                symb_data>>=1;
              //  symb_data<<=1;
           }
    //   xt++;// увеличение расстояния между буквами
           symb++;
        }
    }
}

void draw_text_len(int x,int y,char* text,color_t colorText,color_t colorBg,int len){
 

    for(int line=0;line<FONT_H;line++){

      char* symb=text;
        int yt=y+line;
        int xt=x;
        int inx_symb=0;




        while(*symb){
            uint8_t symb_data=FONT[*symb*FONT_H+line];
        //   symb_data<<=1;

     //    uint8_t wfont = (*symb== 0x2e) ? 2 : FONT_W ;

   //     symb_data = (symb_data * 0x0202020202ULL & 0x010884422010ULL) % 1023; //symb_data>>=1; // зеркалирование шрифта


            for(int i=0;i<FONT_W ;i++){

            draw_pixel(xt++, yt, (symb_data & 0x01) ? colorText : colorBg);
               symb_data>>=1;
 
           }
           symb++;
           inx_symb++;
           if (inx_symb>=len) break;
        }

        while (inx_symb<len)
        {
           
           for(int i=0;i<FONT_W;i++) 
           {
                 draw_pixel(xt++,yt,colorBg);
              
           }
            inx_symb++;

        }
    }
}
//####################################################################################
// Вывод текста для выбора файлов с курсором
void draw_text_file(int x,int y,char* text,color_t colorText,color_t colorBg,int len)
{
    for(int line=0;line<FONT_H;line++){
      char* symb=text;
        int yt=y+line;
        int xt=x;
        int inx_symb=0;
        while(*symb){
            uint8_t symb_data=FONT[*symb*FONT_H+line];
         //  symb_data<<=1;
            for(int i=0;i<FONT_W ;i++){
            draw_pixel(xt++, yt, (symb_data & 0x01) ? colorText : colorBg);
               symb_data>>=1;
            }
           symb++;
           inx_symb++;
           if (inx_symb>=len) break;
        }

        while (inx_symb<len)
        {
           
           for(int i=0;i<FONT_W;i++) 
           {
                 draw_pixel(xt++,yt,colorBg);
           }
            inx_symb++;

        }
    }
}
//#####################################################################
void draw_rect(int x,int y,int w,int h,color_t color,bool filled)
{
    int xb=x;
    int yb=y;
    int xe=x+w;
    int ye=y+h;

    xb=xb<0?0:xb;
    yb=yb<0?0:yb;
    xe=xe<0?0:xe;
    ye=ye<0?0:ye;

    xb=xb>scr_W?scr_W:xb;
    yb=yb>scr_H?scr_H:yb;
    xe=xe>scr_W?scr_W:xe;
    ye=ye>scr_H?scr_H:ye;     
    for(int y=yb;y<=ye;y++)
    for(int x=xb;x<=xe;x++)
        if (filled)
            draw_pixel(x,y,color);
        else 
            if ((x==xb)|(x==xe)|(y==yb)|(y==ye)) draw_pixel(x,y,color);
    

}

void draw_line(int x0,int y0, int x1, int y1,color_t color)
{
   int dx=ABS(x1-x0);
   int dy=ABS(y1-y0);
   if (dx==0) { if (dy==0) return; for(int y=y0;y!=y1;y+=SIGN(y1-y0)) draw_pixel(x0,y,color) ;return; }
   if (dy==0) { if (dx==0) return; for(int x=x0;x!=x1;x+=SIGN(x1-x0)) draw_pixel(x,y0,color) ;return;  }
   if (dx>dy)
    {
        float k=(float)(x1-x0)/ABS(y1-y0);
        //float kold=0;
        float xf=x0+k;
        int x=x0;
        for(int y=y0;y!=y1;y+=SIGN(y1-y0))
        {
            int i=0;
            for(;i<ABS(xf-x);i++)
            { 
                draw_pixel(x+i*SIGN(x1-x0),y,color) ;
            }
            x+=i*SIGN(x1-x0);
            xf+=k;
        }
    }
   else
    {
        float k=(float)(y1-y0)/ABS(x1-x0);
       

        //float kold=0;
        float yf=y0+k;
        int y=y0;
        for(int x=x0;x!=x1;x+=SIGN(x1-x0))
        {
            int i=0;
            for(;i<ABS(yf-y);i++)
            { 
                draw_pixel(x,y+i*SIGN(y1-y0),color) ;
            }
            y+=i*SIGN(y1-y0);
            yf+=k;
        }
    }

}


void ShowScreenshot(uint8_t* buffer){
    //draw_rect(17+FONT_W*14,16,182,192,0x0,true);
    for(uint8_t segment=0;segment<3;segment++){
		for(uint8_t symbol_row=0;symbol_row<8;symbol_row++){
        	for(uint8_t y=0;y<8;y++){
        		for(uint8_t x=4;x<32;x++){			
	                uint16_t pixel_addr = (segment*2048)+x+(y*256)+(symbol_row*32);
					uint16_t attr_addr = 0x1800+x+(symbol_row*32)+(segment*256);

    	          //  int yt=(y+40+(segment*64)+(symbol_row*8));//16+11
                int yt=(y+ScreenShot_Y+(segment*64)+(symbol_row*8));//16+11

        	        int xt=((x*8)+100-36);//17+FONT_W*14+11
                	uint8_t pixel_data=buffer[pixel_addr];
					uint8_t color_data=buffer[attr_addr];
                	for(int i=0;i<8;i++){
						if((xt<312)&&(yt<230)){
							uint8_t bright_color = (color_data&0b01000000)>>3;
							uint8_t fg_color = color_data&0b00000111;
							uint8_t bg_color = (color_data&0b00111000)>>3;
                    		if (pixel_data&(1<<(8-1)))draw_pixel(xt++,yt,fg_color^bright_color); else draw_pixel(xt++,yt,bg_color^bright_color); 
						}
                    	pixel_data<<=1;
                	}
            	}
			}
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////

/*-------Graphics--------*/

void draw_main_window()
{
    memset(g_gbuf, COLOR_BACKGOUND, sizeof(g_gbuf));
	//draw_rect(9,7,SCREEN_W-16,SCREEN_H-15,COLOR_BORDER,false);//Основная рамка
  draw_rect(9,7,51*FONT_W,SCREEN_H-15,COLOR_BORDER,false);//Основная рамка
    draw_line(10+FONT_W*14,10,10+FONT_W*14,SCREEN_H-8,COLOR_BORDER);  //центральная верт линия 
}
//------------------------------------
void draw_file_window()
{
	draw_rect(17+8*14,210,100,21,COLOR_BACKGOUND,true); //Фон отображения скринов
 
     draw_text_len(4+FONT_W,FONT_H*21-3,"T:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(4+FONT_W*3,FONT_H*21-3,TapeName,COLOR_TR1,COLOR_TR0 ,12);

     draw_text_len(4+FONT_W,FONT_H*22-3,"A:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(4+FONT_W*3,FONT_H*22-3,conf.DiskName[0],COLOR_TR1,COLOR_TR0 ,12);
     draw_text_len(4+FONT_W,FONT_H*23-3,"B:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(4+FONT_W*3,FONT_H*23-3,conf.DiskName[1],COLOR_TR1,COLOR_TR0, 12);
     draw_text_len(4+FONT_W,FONT_H*24-3,"C:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(4+FONT_W*3,FONT_H*24-3,conf.DiskName[2],COLOR_TR1,COLOR_TR0, 12);
     draw_text_len(4+FONT_W,FONT_H*25-3,"D:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(4+FONT_W*3,FONT_H*25-3,conf.DiskName[3],COLOR_TR1,COLOR_TR0, 12);
	
}
//----------------------
 void draw_img(uint8_t xPos,uint8_t yPos)
{ //x:33 y:84
  //uint8_t *img = 0;
 // uint8_t st = 0;
	for(uint16_t y=0;y<220;y++){
		for(uint16_t x=0;x<320;x++){

static uint32_t Cur_Seed = 0xffff;
    Cur_Seed = (Cur_Seed * 2 + 1) ^
               (((Cur_Seed >> 16) ^ (Cur_Seed >> 13)) & 1);




			uint8_t pixel = Cur_Seed;//(g_gbuf[st+1]) &0x1f;//img[x+(y*254)];
   //   st++;
			if (pixel<0xFF)	draw_pixel(xPos+x,yPos+y,pixel);
		}
	}
}  
/*-------Graphics--------*/

//==========================================================================================
uint8_t MenuBox(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,  char *m_text[], uint8_t Pos,uint8_t cPos,uint8_t over_emul)
{
	if(over_emul) zx_machine_enable_vbuf(false);
    uint16_t lFrame = (lPos*FONT_W)+8;
    uint16_t hFrame = ((1+hPos)*(FONT_H+1))+20;
   // draw_rect(xPos+3,yPos+2,lFrame+3,hFrame+3,CL_GRAY,true);// тень
    draw_rect(xPos-2,yPos-2,lFrame+4,hFrame+4,CL_BLACK,false);// рамка 1
    draw_rect(xPos-1,yPos-1,lFrame+2,hFrame+2,CL_GRAY,false);// рамка 2
    draw_rect(xPos,yPos,lFrame,hFrame,CL_BLACK,true);// рамка 3 фон
    draw_rect(xPos,yPos,lFrame,9,CL_GRAY,true); // шапка меню
    draw_text(xPos+10,yPos+0,text,CL_PAPER,CL_INK);// шапка меню
yPos=yPos+10;

  for(uint8_t i=0;i<hPos;i++){
    if (i >= Pos) {draw_text(xPos+1,yPos+8+10*i,m_text[i],CL_BLUE,CL_PAPER); continue;}
    if (i == cPos)  {draw_text(xPos+1,yPos+8+10*i,m_text[i],CL_PAPER,CL_LT_CYAN);continue;} // курсор
    else { draw_text(xPos+1,yPos+8+10*i,m_text[i],CL_INK,CL_PAPER);continue;}
    
  }

wait_enter(); // ожидание отпускания enter
  while (1)
  {

 if (!decode_key_joy()) continue;
  
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
      cPos++;
      if (cPos == Pos)
        cPos = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_BLACK, CL_LT_CYAN);   // курсор
    
   //  sleep_ms(DELAY_KEY);
  
    }

    if (kb_st_ps2.u[2] & KB_U2_UP)

    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
      if (cPos == 0)
        cPos = Pos;
      cPos--;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_BLACK, CL_LT_CYAN);  // курсор
     
   //    sleep_ms(DELAY_KEY);
      
    }

    if (kb_st_ps2.u[1]&KB_U1_ENTER) // enter
    {
     
    wait_enter();

      return cPos;
    }

    if (kb_st_ps2.u[1]&KB_U1_ESC) 
    {
      wait_esc();
    return 0xff; // ESC exit
    }

  }
}

////////////////////////////////////////////////////////////////////////////////////////////
uint8_t MenuBox_help(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, char *m_text[], uint8_t Pos, uint8_t cPos, uint8_t over_emul)
{
  if (over_emul)
    zx_machine_enable_vbuf(false);
  //uint16_t lFrame = (lPos * FONT_W) + 10;
 // uint16_t hFrame = ((1 + hPos) * FONT_H) + 20;

  yPos = yPos + 10;

  for (uint8_t i = 0; i < hPos; i++)
  {

    draw_text(xPos + 1, yPos + 8 + 10 * i, m_text[i], CL_INK, CL_PAPER);
  }
/*  kb_st_ps2.u[0] = 0x0;
 kb_st_ps2.u[1] = 0x0;
 kb_st_ps2.u[2] = 0x0;
 kb_st_ps2.u[3] = 0x0; */


  while (1)
  {
   // if (mode_kbms)  sleep_ms(DELAY_KEY); // задержка если это не ps/2
    if (!decode_key_joy()) continue;

#ifndef MOS2
#ifdef KEY_UPDATE_MODE
           if (KEY_BOOT)
           {
               draw_img(0, 0);
               MessageBox("  switching to firmware update mode ", "", CL_WHITE, CL_RED, 2);
               reset_usb_boot(0, 0);
           }
#endif               
#endif



    if (kb_st_ps2.u[1] & KB_U1_ENTER) // enter
    {  
      wait_enter();
      return 0xff;
    }
    if (kb_st_ps2.u[1] & KB_U1_ESC) // ESC
    {
       wait_esc();
       return 0xff;
    } 


  }
}
////////////////////////////////////////////////////////////////////////////////////////////
uint8_t MenuBox_bw(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos,  char*m_text[], uint8_t Pos, uint8_t cPos, uint8_t over_emul)
{
  if (over_emul)
    zx_machine_enable_vbuf(false);

  yPos = yPos + 10;
  for (uint8_t i = 0; i < hPos; i++)
  {
    if (i >= Pos)
    {
      draw_text(xPos + 1, yPos + 8 + 10 * i, m_text[i], CL_BLUE, CL_INK);
      continue;
    }
    if (i == cPos)
    {
      draw_text(xPos + 1, yPos + 8 + 10 * i, m_text[i], CL_PAPER, CL_INK);
      continue;
    } // курсор
    else
    {
      draw_text(xPos + 1, yPos + 8 + 10 * i, m_text[i], CL_INK, CL_PAPER);
      continue;
    }
  }

  while (1)
  {

if (!decode_key_joy()) continue; // если нажата кнопка на клавиатуре или джой

    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      flag_usb_kb = true;
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
      cPos++;
      if (cPos == Pos)
        cPos = 0;
      //  if (cPos == M_EMPTY_1 )  cPos++;
        if (cPos == M_EMPTY_2 )  cPos++; 
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_BLACK, CL_INK);// рисование курсора
      ; // курсор
    //  sleep_ms(DELAY_KEY);
    }
    if (kb_st_ps2.u[2] & KB_U2_UP)
    {
      flag_usb_kb = true;
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
 
        if (cPos == 0) cPos = Pos;
        cPos--;
        if (cPos == M_EMPTY_2 )  cPos--; 
     //   if (cPos == M_EMPTY_1 )  cPos--;

 
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_BLACK, CL_INK);
      ; // курсор
  //    sleep_ms(DELAY_KEY);
    }

    if (kb_st_ps2.u[1] & KB_U1_ENTER) // enter

    {
      wait_enter();
      return cPos;
    }

    if (kb_st_ps2.u[1] & KB_U1_ESC) // ESC
    {
      
    wait_esc();
      return 0xff;
    }

  }
}
////////////////////////////////////////////////////////////////////////////////////////////
// меню выбора дисковода
// const char*  menu_text[7]={
char *menu_trd[7] = {
    " A:",
    " B:",
    " C:",
    " D:",
    "",
    "ENTER or key A,B,C,D",
    "Z-Eject   ESC-exit"
    "",
};

///
uint8_t MenuBox_trd(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos,  char *text, uint8_t Pos, uint8_t cPos, uint8_t over_emul)
{
#define L_CURSOR 19

  if (over_emul)
    zx_machine_enable_vbuf(false);
  uint16_t lFrame = (lPos * FONT_W)+3;
  uint16_t hFrame = ((4 + hPos) * FONT_H);
  draw_rect(xPos - 2, yPos - 2, lFrame + 4, hFrame + 4, CL_INK, false);   // рамка 1
  draw_rect(xPos - 1, yPos - 1, lFrame + 2, hFrame + 2, CL_BLACK, false); // рамка 2
  draw_rect(xPos, yPos, lFrame, hFrame, CL_PAPER, true);                  // рамка 3 фон
  draw_rect(xPos, yPos, lFrame, 8, CL_INK, true);                         // шапка меню
  draw_text(xPos + 8, yPos + 0, text, CL_PAPER, CL_INK);
  // conf.DiskName[Drive]
  //   draw_logo_header(xPos + lFrame - 42, yPos); // рисование символа спектрума
  xPos = xPos + 1;
  yPos = yPos + 4;
  for (uint8_t i = 0; i < hPos; i++)
  {
    if (i >= Pos)
    {
      draw_text(xPos + 8, yPos + 8 + 10 * i, menu_trd[i], CL_BLUE, CL_PAPER);
      continue;
    } // текст подсказок
    if (i == cPos)
    {
      draw_text(xPos, yPos + 8 + 10 * i, menu_trd[i], CL_PAPER, CL_LT_CYAN); // курсор
      continue;
    }
    else
    {
      draw_text(xPos, yPos + 8 + 10 * i, menu_trd[i], CL_INK, CL_PAPER);
      continue;
    } // текст меню
  }

  // char conf.DisksDisks[4][160];
  for (uint8_t i = 0; i < 4; i++)
  {
    if (i == cPos)
    {
      draw_text_len(xPos + 3 * FONT_W, yPos + 8 + 10 * i, conf.DiskName[i], CL_PAPER, CL_LT_CYAN, L_CURSOR); // курсор
      continue;
    }
    else
    {
      draw_text_len(xPos + 3 * FONT_W, yPos + 8 + 10 * i, conf.DiskName[i], CL_INK, CL_PAPER, L_CURSOR);
      continue;
    }
  }
  wait_enter();

  while (1)
  {
   // if (mode_kbms)  sleep_ms(DELAY_KEY); // задержка если это не ps/2
    if (!decode_key_joy()) continue;

    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      kb_st_ps2.u[2] = 0;
      draw_text_len(xPos, yPos + 8 + 10 * cPos, menu_trd[cPos], CL_INK, CL_PAPER, 3);                     // стирание курсора надпись буквы диска
      draw_text_len(xPos + 3 * FONT_W, yPos + 8 + 10 * cPos, conf.DiskName[cPos], CL_INK, CL_PAPER, L_CURSOR); // стирание курсора надпись файла
      cPos++;
      if (cPos == Pos)
        cPos = 0;
      draw_text_len(xPos, yPos + 8 + 10 * cPos, menu_trd[cPos], CL_PAPER, CL_LT_CYAN, 3);                     // курсор надпись буквы диска
      draw_text_len(xPos + 3 * FONT_W, yPos + 8 + 10 * cPos, conf.DiskName[cPos], CL_PAPER, CL_LT_CYAN, L_CURSOR); // курсор надпись файла
      g_delay_ms(50);
    }

    if (kb_st_ps2.u[2] & KB_U2_UP)

    {
      kb_st_ps2.u[2] = 0;
      draw_text_len(xPos, yPos + 8 + 10 * cPos, menu_trd[cPos], CL_INK, CL_PAPER, 3);                     // стирание курсора
      draw_text_len(xPos + 3 * FONT_W, yPos + 8 + 10 * cPos, conf.DiskName[cPos], CL_INK, CL_PAPER, L_CURSOR); // стирание курсора надпись файла
      if (cPos == 0)
        cPos = Pos;
      cPos--;
      draw_text_len(xPos, yPos + 8 + 10 * cPos, menu_trd[cPos], CL_BLACK, CL_LT_CYAN, 3);
      ;                                                                                                       // курсор надпись буквы диска
      draw_text_len(xPos + 3 * FONT_W, yPos + 8 + 10 * cPos, conf.DiskName[cPos], CL_BLACK, CL_LT_CYAN, L_CURSOR); // курсор надпись файла
      g_delay_ms(50);
    }
    // Копируем строку длиною 3 символов из массива src в массив dst1.
    // strncpy (dst1, src,3);

    if (kb_st_ps2.u[0] & KB_U0_A)
      return 0; // A
    if (kb_st_ps2.u[0] & KB_U0_B)
      return 1; // B
    if (kb_st_ps2.u[0] & KB_U0_C)
      return 2; // C
    if (kb_st_ps2.u[0] & KB_U0_D)
      return 3; // D
    if (kb_st_ps2.u[1] & KB_U1_ENTER)
    {
      wait_enter();
      return cPos; // ENTER
    }
    if (kb_st_ps2.u[1] & KB_U1_ESC)
    {
      wait_esc();
      return 5; // ESC exit
    }
    if (kb_st_ps2.u[0] & KB_U0_Z) // Z-Eject Disk
    {
      //  memset(str,'_',12); // заполнить первые 12 байт символом '_'
      memset(conf.Disks[cPos], 0, DIRS_DEPTH*(LENF));
      memset(conf.DiskName[cPos], 0, LENF);                                                                      // char Disks[4][160];
      draw_text_len(xPos + 3 * FONT_W, yPos + 8 + 10 * cPos, conf.Disks[cPos], CL_PAPER, CL_LT_CYAN, L_CURSOR); // курсор надпись файла
    }
// создание новог диска trd
 /*  if (kb_st_ps2.u[0] & KB_U0_N) // N-New TRD image
    {

    // Populate Disk Specification.
    memset(sd_buffer, 0, 0x0900); // обнуление буфера
    sd_buffer[2273] = 1;
    sd_buffer[2274] = 1;
    sd_buffer[2275] = 22; // Disk Type
    sd_buffer[2276] = 0; // File Count
    sd_buffer[2277] = 0x00;
    sd_buffer[2278] = 0x0A;
    sd_buffer[2279] = 0x10; // TR-DOS ID

    for (int i = 0; i < 9; i++) sd_buffer[2282 + i] = 0x20;

    // Store the image file name in the disk label section of the Disk Specification.


     
	
     //sd_buffer[2293 + i] = disk_name[i];
	

     // sprintf(file_name_image, "0:/save/%d_slot.Z80", num);
      sprintf(temp_msg, "0:/DISK.TRD"); 
    int fd = f_open(&f, temp_msg, FA_CREATE_ALWAYS | FA_WRITE);
    if (fd != FR_OK)
    {
        f_close(&f);
      //  return false;
    }
    UINT bytesWritten;
    fd = f_write(&f, sd_buffer,0x2000, &bytesWritten);
    if (bytesWritten != 0x2000)
    {
        f_close(&f);
      //  return false;
    }
    f_close(&f);
    }

 */
    }
}
    ////////////////////////////////////////////////////////////////////////////////////////////
    void MenuBox_lite(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, char *text, uint8_t over_emul)
    {
      if (over_emul)
        zx_machine_enable_vbuf(false);
      if (over_emul)
        zx_machine_enable_vbuf(false);
      uint16_t lFrame = (lPos * FONT_W) + 10;
      uint16_t hFrame = ((1 + hPos) * FONT_H) + 20;
      // draw_rect(xPos+3,yPos+2,lFrame+3,hFrame+3,CL_GRAY,true);// тень
      draw_rect(xPos - 2, yPos - 2, lFrame + 4, hFrame + 4, CL_BLACK, false); // рамка 1
      draw_rect(xPos - 1, yPos - 1, lFrame + 2, hFrame + 2, CL_GRAY, false);  // рамка 2
      draw_rect(xPos, yPos, lFrame, hFrame, CL_BLACK, true);                  // рамка 3 фон
      draw_rect(xPos, yPos, lFrame, 7, CL_GRAY, true);                        // шапка меню
      draw_text(xPos + 10, yPos + 0, text, CL_PAPER, CL_INK);                 // шапка меню
}
////////////////////////////////////////////////////////////////////////////////////////////

void MessageBox(char *text,char *text1,uint8_t colorFG,uint8_t colorBG,uint8_t over_emul)
{
	uint8_t max_len= strlen(text)>strlen(text1)?strlen(text):strlen(text1);
	uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	uint8_t left_y = strlen(text1)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	left_y -= 24;
	uint8_t height = strlen(text1)>0 ? FONT_H*2+7:FONT_H+7;
	if(over_emul) zx_machine_enable_vbuf(false);
	draw_rect(left_x-2,left_y-2,(max_len*FONT_W)+7,height,colorBG,true);
	draw_rect(left_x-2,left_y-2,(max_len*FONT_W)+7,height,colorFG,false);
	draw_text(left_x,left_y+1,text,colorFG,colorBG);
	if (strlen(text1)>0) draw_text(left_x,left_y+FONT_H+1,text1,colorFG,colorBG);
	switch (over_emul){
	case 1:
		g_delay_ms(3000);
		break;
	case 2:
		g_delay_ms(750);
		break;
	case 3:
		g_delay_ms(250);
		break;
	case 4:
		g_delay_ms(1000);
		break;
	default:
		break;
	}
}
//----------------------------------------------------------------------------------------
void MessageBox_off(char *text,char *text1,uint8_t colorFG,uint8_t colorBG,uint8_t over_emul)
{
	uint8_t max_len= strlen(text)>strlen(text1)?strlen(text):strlen(text1);
	uint8_t left_x =40;
	uint8_t left_y =220;
	
	uint8_t height = strlen(text1)>0 ? FONT_H*2+7:FONT_H+7;
	if(over_emul) zx_machine_enable_vbuf(false);
	draw_rect(left_x,left_y-2,(max_len*FONT_W)+7,height,colorBG,true);
	//draw_rect(left_x-2,left_y-2,(max_len*FONT_W)+7,height,colorFG,false);
	draw_text(left_x,left_y+1,text,colorFG,colorBG);
	if (strlen(text1)>0) draw_text(left_x,left_y+FONT_H+1,text1,colorFG,colorBG);
}

//==========================================================================================
uint8_t MenuBox_tft_setup(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,  char *m_text[], uint8_t Pos,uint8_t cPos,uint8_t over_emul)
{
	if(over_emul) zx_machine_enable_vbuf(false);
    uint16_t lFrame = (lPos*FONT_W)+8;
    uint16_t hFrame = ((1+hPos)*(FONT_H+1))+20;
   // draw_rect(xPos+3,yPos+2,lFrame+3,hFrame+3,CL_GRAY,true);// тень
    draw_rect(xPos-2,yPos-2,lFrame+4,hFrame+4,CL_BLACK,false);// рамка 1
    draw_rect(xPos-1,yPos-1,lFrame+2,hFrame+2,CL_GRAY,false);// рамка 2
    draw_rect(xPos,yPos,lFrame,hFrame,CL_BLACK,true);// рамка 3 фон
    draw_rect(xPos,yPos,lFrame,9,CL_GRAY,true); // шапка меню
    draw_text(xPos+10,yPos+0,text,CL_PAPER,CL_INK);// шапка меню
yPos=yPos+10;

  for(uint8_t i=0;i<hPos;i++){
    if (i >= Pos) {draw_text(xPos+1,yPos+8+10*i,m_text[i],CL_INK,CL_PAPER); continue;}
    if (i == cPos)  {draw_text(xPos+1,yPos+8+10*i,m_text[i],CL_PAPER,CL_LT_CYAN);continue;} // курсор
    else { draw_text(xPos+1,yPos+8+10*i,m_text[i],CL_INK,CL_PAPER);continue;}
    
  }
  if (conf.tft==TFT_9345) draw_text(xPos+100,yPos+8+10*0,"*",CL_INK,CL_PAPER);//0 ili9341  
  if (conf.tft==TFT_9345I) draw_text(xPos+100,yPos+8+10*1,"*",CL_INK,CL_PAPER);//2 ili9341 ips 
  if (conf.tft==TFT_7789) draw_text(xPos+100,yPos+8+10*2,"*",CL_INK,CL_PAPER);//2 st7789 

  if (conf.tft_rotate  == 0) draw_text(xPos+100,yPos+8+10*3,"0",CL_INK,CL_PAPER);
  else  draw_text(xPos+100,yPos+8+10*3,"180",CL_INK,CL_PAPER);
  if (conf.tft_invert  == 0) draw_text(xPos+100,yPos+8+10*4,"OFF",CL_INK,CL_PAPER);
  else  draw_text(xPos+100,yPos+8+10*4,"ON",CL_INK,CL_PAPER);
  if (conf.tft_rgb == 0) draw_text(xPos+100,yPos+8+10*5,"BGR",CL_INK,CL_PAPER);
  else  draw_text(xPos+100,yPos+8+10*5,"RGB",CL_INK,CL_PAPER);
  
  #define CL_PROGRESS CL_LT_GREEN
  char progress[21]= {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,0};
  sprintf(temp_msg, " %d%% ", conf.tft_bright);
  draw_text(xPos + 70, yPos + 90, temp_msg, CL_INK, CL_PAPER);                                      //
  draw_text_len(xPos + 10, yPos + 100,progress, CL_PROGRESS, CL_PAPER, (conf.tft_bright / 5)); //


wait_enter(); // ожидание отпускания enter
  while (1)
  {

 if (!decode_key_joy()) continue;
 //---------------------- 


 if (kb_st_ps2.u[2] & KB_U2_LEFT)
 {
   kb_st_ps2.u[2] = 0;

   conf.tft_bright = conf.tft_bright - 5;
   if (conf.tft_bright < 5)
     conf.tft_bright = 5;
   pwm_set_gpio_level(TFT_LED_PIN, conf.tft_bright); // % яркости

    sprintf(temp_msg, " %d%% ", conf.tft_bright);
 draw_text(xPos + 70, yPos + 90, temp_msg, CL_INK, CL_PAPER);                                          //
 draw_text_len(xPos + 10, yPos + 100, "                    ", CL_INK, CL_PAPER, 20);                   //
 draw_text_len(xPos + 10, yPos + 100, progress, CL_PROGRESS, CL_PAPER, (conf.tft_bright / 5)); //

 }

 if (kb_st_ps2.u[2] & KB_U2_RIGHT)

 {
   kb_st_ps2.u[2] = 0;

   conf.tft_bright = conf.tft_bright + 5;
   if (conf.tft_bright > 100)
     conf.tft_bright = 100;
   pwm_set_gpio_level(TFT_LED_PIN, conf.tft_bright); // % яркости

   sprintf(temp_msg, " %d%% ", conf.tft_bright);
 draw_text(xPos + 70, yPos + 90, temp_msg, CL_INK, CL_PAPER);                                          //
 draw_text_len(xPos + 10, yPos + 100, "                    ", CL_INK, CL_PAPER, 20);                   //
 draw_text_len(xPos + 10, yPos + 100, progress, CL_PROGRESS, CL_PAPER, (conf.tft_bright / 5)); //

 }
//-----------------------------
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
      cPos++;
      if (cPos == Pos)
        cPos = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_BLACK, CL_LT_CYAN);   // курсор
    
   //  sleep_ms(DELAY_KEY);
  
    }

    if (kb_st_ps2.u[2] & KB_U2_UP)

    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
      if (cPos == 0)
        cPos = Pos;
      cPos--;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_BLACK, CL_LT_CYAN);  // курсор
     
   //    sleep_ms(DELAY_KEY);
      
    }

    if (kb_st_ps2.u[1]&KB_U1_ENTER) // enter
    {
     
    wait_enter();

      return cPos;
    }

    if (kb_st_ps2.u[1]&KB_U1_ESC) 
    {
      wait_esc();
    return 0xff; // ESC exit
    }

  }
}
//==========================================================================================
 	// меню tr-dos version
char __in_flash() *menu_trdos[2]={
    " TR-DOS   5.04T  ",
    " TR-DOS   5.05D  ",
    };
 	// меню cpu z80 version
	char __in_flash() *menu_cpu_version [5]={
    " ZILOG  NMOS ",
    " ZILOG  CMOS ",
    " NEC    NMOS ",
    " ST     CMOS ",
    " UNREAL      ",
    };   
//


// Массив для хранения строк меню
char menu_advanced_strings[12][19] = {0};

// Массив указателей на строки
char *menu_advanced[12] = {
    menu_advanced_strings[0],
    menu_advanced_strings[1],
    menu_advanced_strings[2],
    menu_advanced_strings[3],
    menu_advanced_strings[4],
    menu_advanced_strings[5],
    menu_advanced_strings[6],
    menu_advanced_strings[7],
    menu_advanced_strings[8],
    menu_advanced_strings[9],
    menu_advanced_strings[10],
    menu_advanced_strings[11]
};



// Инициализация меню
void init_menu_advanced() {
 
 
     sprintf(menu_advanced_strings[0], " Mouse Speed  *%2d ", conf.mouse_dpi);

         if (conf.vout==VIDEO_AUTO) strcpy(menu_advanced_strings[1], " Video OUT   AUTO ");
         if (conf.vout==VIDEO_VGA)  strcpy(menu_advanced_strings[1], " Video OUT    VGA ");
         if (conf.vout==VIDEO_HDMI) strcpy(menu_advanced_strings[1], " Video OUT   HDMI ");
         if (conf.vout==VIDEO_TFT)  strcpy(menu_advanced_strings[1], " Video OUT    TFT ");

    sprintf(menu_advanced_strings[2], "%s ", menu_trdos[conf.trdos_version]);

    if (conf.cpu_version>4) conf.cpu_version = 0;
    sprintf(menu_advanced_strings[3], " Z80 %s", menu_cpu_version[conf.cpu_version]);

    conf.tape_mode &= 1; // защита от мусора в старом конфиге
    if (conf.tape_mode==0)
        strcpy(menu_advanced_strings[4], " Tape Load   FAST ");
    else
        strcpy(menu_advanced_strings[4], " Tape Load NORMAL ");

   // sprintf(menu_advanced_strings[5], " Voltage  %4d V ",table_voltage[conf.voltage]* 10 );
    sprintf(menu_advanced_strings[5], " Voltage   %.2f V ",table_voltage[conf.voltage]/ 100.0 );
    
    strcpy(menu_advanced_strings[6], " Save config      ");
    strcpy(menu_advanced_strings[7]," Return           ");
}



//    
uint8_t MenuBox_advanced_setup(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,  uint8_t Pos,uint8_t cPos,uint8_t over_emul)
{

  init_menu_advanced();

	if(over_emul) zx_machine_enable_vbuf(false);
    uint16_t lFrame = (lPos*FONT_W)+8;
    uint16_t hFrame = ((1+hPos)*(FONT_H+1))+20;
    draw_rect(xPos-2,yPos-2,lFrame+4,hFrame+4,CL_BLACK,false);// рамка 1
    draw_rect(xPos-1,yPos-1,lFrame+2,hFrame+2,CL_GRAY,false);// рамка 2
    draw_rect(xPos,yPos,lFrame,hFrame,CL_BLACK,true);// рамка 3 фон
    draw_rect(xPos,yPos,lFrame,9,CL_GRAY,true); // шапка меню
    draw_text(xPos+10,yPos+0,text,CL_PAPER,CL_INK);// шапка меню
    yPos=yPos+18;
    xPos++;
  for(uint8_t i=0;i<hPos;i++){
    if (i >= Pos) {draw_text(xPos,yPos+10*i,menu_advanced[i],CL_BLUE,CL_PAPER); continue;}
    if (i == cPos)  {draw_text(xPos,yPos+10*i,menu_advanced[i],CL_PAPER,CL_LT_CYAN);continue;} // курсор
    else { draw_text(xPos,yPos+10*i,menu_advanced[i],CL_INK,CL_PAPER);continue;} 
  }


wait_enter(); // ожидание отпускания enter
  while (1)
  {

 if (!decode_key_joy()) continue;
  
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos, yPos+ 10 * cPos, menu_advanced[cPos], CL_INK, CL_BLACK); // стирание курсора
      cPos++;
      if (cPos == Pos)
        cPos = 0;
      draw_text(xPos, yPos+ 10 * cPos, menu_advanced[cPos], CL_BLACK, CL_LT_CYAN);   // курсор
  
    }

    if (kb_st_ps2.u[2] & KB_U2_UP)

    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos, yPos+ 10 * cPos, menu_advanced[cPos], CL_INK, CL_BLACK); // стирание курсора
      if (cPos == 0)
        cPos = Pos;
      cPos--;
      draw_text(xPos, yPos+ 10 * cPos, menu_advanced[cPos], CL_BLACK, CL_LT_CYAN);  // курсор
      
    }

    if (kb_st_ps2.u[2] & KB_U2_LEFT)
    {
      kb_st_ps2.u[2] = 0;

      switch (cPos)
      {


          case 0:// скорость мыши -
          if (conf.mouse_dpi==1) conf.mouse_dpi=1;
             else conf.mouse_dpi--;
           init_menu_advanced();
           draw_text(xPos,yPos+10*0,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
           break;    

         case 1:// video out -
         if (conf.vout==VIDEO_AUTO) conf.vout=VIDEO_AUTO;
          else conf.vout--;
             init_menu_advanced();
        draw_text(xPos,yPos+10*1,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN);
        break;

          case 4:// tape load mode
          conf.tape_mode = conf.tape_mode ? 0 : 1;
          TAP_SwitchMode();
          init_menu_advanced();
          draw_text(xPos,yPos+10*4,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN);
          break;

          case 5:// voltage
          if (conf.voltage==15) conf.voltage=15;
             else conf.voltage--;
           init_menu_advanced();
           draw_text(xPos,yPos+10*5,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
           break;   


      }
   
    }
   
    if (kb_st_ps2.u[2] & KB_U2_RIGHT)
   
    {
      kb_st_ps2.u[2] = 0;
      switch (cPos)
      {

         case 0:// скорость мыши +
         if (conf.mouse_dpi==9) conf.mouse_dpi=9;
            else conf.mouse_dpi++;
             init_menu_advanced();
        draw_text(xPos,yPos+10*0,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
          break;    

         case 1:// video out +
         if (conf.vout>=VIDEO_TFT) conf.vout=VIDEO_TFT;
         else conf.vout++;
             init_menu_advanced();
        draw_text(xPos,yPos+10*1,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN);
         break;

          case 4:// tape load mode
          conf.tape_mode = conf.tape_mode ? 0 : 1;
          TAP_SwitchMode();
          init_menu_advanced();
          draw_text(xPos,yPos+10*4,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN);
          break;

          case 5:// voltage
          if (conf.voltage==19) conf.voltage=19;
             else conf.voltage++;
           init_menu_advanced();
           draw_text(xPos,yPos+10*5,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
           break;   

      default:
        break;
      }
    }




    if (kb_st_ps2.u[1]&KB_U1_ENTER) // enter
    {
     switch (cPos)
     {
      uint8_t x;

      case 2:// версия TR-DOS
         x = MenuBox(140, 80, 16, 2, "Version", menu_trdos, 2, conf.trdos_version , 1);
         if (x!=0xff) conf.trdos_version  = x;
         break;

      case 3:// версия Z80
         x = MenuBox(155, 90, 12, 5, "Z80 CPU", menu_cpu_version, 5, conf.cpu_version , 1);
       if (x!=0xff)
       {
       conf.cpu_version  = x;
       select_cpu_z80(z1);
       }
       break;

       case 4:// tape load mode
       conf.tape_mode = conf.tape_mode ? 0 : 1;
       TAP_SwitchMode();
       init_menu_advanced();
       draw_text(xPos,yPos+10*4,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN);
       continue;

          case 5:// voltage
           draw_text(xPos,yPos+10*5,menu_advanced_strings[cPos],  CL_WHITE, CL_LT_RED); 
       //    g_delay_ms(100);
           #ifdef PICO_RP2350 
           vreg_set_voltage(conf.voltage);
           #endif
           g_delay_ms(300);
           draw_text(xPos,yPos+10*5,menu_advanced_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
          continue;// break;   





       case 6:// Save config
       save_config();
       break;

      case 7:// Return menu
      return 0xff; // ESC exit
     }


    wait_enter();



      return cPos;
    }

    if (kb_st_ps2.u[1]&KB_U1_ESC) 
    {
      wait_esc();
    return 0xff; // ESC exit
    }

  }
}
//##############################################################
// Массив для хранения строк меню
char menu_sound_setup_strings[12][19] = {0};

// Массив указателей на строки
char *menu_sound_setup[12] = {
    menu_sound_setup_strings[0],
    menu_sound_setup_strings[1],
    menu_sound_setup_strings[2],
    menu_sound_setup_strings[3],
    menu_sound_setup_strings[4],
    menu_sound_setup_strings[5],
    menu_sound_setup_strings[6],
    menu_sound_setup_strings[7],
    menu_sound_setup_strings[8],
    menu_sound_setup_strings[9],
    menu_sound_setup_strings[10],
    menu_sound_setup_strings[11]
};

// Инициализация меню
void init_menu_sound_setup() {
    switch (conf.type_sound)
  {
  case SOFT_AY:
  case SOFT_TS:
    sprintf(menu_sound_setup_strings[0], " Volume       %3d ", conf.vol_ay);
    break;
  case I2S_AY:
  case I2S_TS:
    sprintf(menu_sound_setup_strings[0], " Volume       %3d ", conf.vol_i2s);
    break;
  default:
    sprintf(menu_sound_setup_strings[0], " Volume       N/A ");
    break;
  }

    switch (conf.type_sound) // i2s buster
  {
  case I2S_AY:
  case I2S_TS:
    sprintf(menu_sound_setup_strings[1], " I2S  buster   %d ", conf.audio_buster);
    break;
  default:
    sprintf(menu_sound_setup_strings[1], " I2S  buster  N/A ");
    break;
  }
   if (conf.sound_fdd) strcpy(menu_sound_setup_strings[2], " Noise FDD    OFF ");
  else strcpy(menu_sound_setup_strings[2], " Noise FDD     ON ");

     sprintf(menu_sound_setup_strings[3], " Volume LOAD   %2d ", conf.vol_load );

   

    if (conf.beep_mode==0)
        strcpy(menu_sound_setup_strings[4], " Beep Mode  MIX   ");
    else if (conf.beep_mode==1)
    {
       beep_pin = ZX_BEEP_PIN;
      sprintf(menu_sound_setup_strings[4], " Beep Mode  GP%d  ",ZX_BEEP_PIN);
    }
    else if (conf.beep_mode==2)
    {
      beep_pin = 29;
      sprintf(menu_sound_setup_strings[4], " Beep Mode  GP%d  ", beep_pin);
    }
    strcpy(menu_sound_setup_strings[5], " Save config      ");
    strcpy(menu_sound_setup_strings[6]," Return           ");
}



//    
uint8_t MenuBox_sound_setup(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,  uint8_t Pos,uint8_t cPos,uint8_t over_emul)
{

  init_menu_sound_setup();

	if(over_emul) zx_machine_enable_vbuf(false);
    uint16_t lFrame = (lPos*FONT_W)+8;
    uint16_t hFrame = ((1+hPos)*(FONT_H+1))+20;
    draw_rect(xPos-2,yPos-2,lFrame+4,hFrame+4,CL_BLACK,false);// рамка 1
    draw_rect(xPos-1,yPos-1,lFrame+2,hFrame+2,CL_GRAY,false);// рамка 2
    draw_rect(xPos,yPos,lFrame,hFrame,CL_BLACK,true);// рамка 3 фон
    draw_rect(xPos,yPos,lFrame,9,CL_GRAY,true); // шапка меню
    draw_text(xPos+10,yPos+0,text,CL_PAPER,CL_INK);// шапка меню
    yPos=yPos+18;
    xPos++;
  for(uint8_t i=0;i<hPos;i++){
    if (i >= Pos) {draw_text(xPos,yPos+10*i,menu_sound_setup[i],CL_BLUE,CL_PAPER); continue;}
    if (i == cPos)  {draw_text(xPos,yPos+10*i,menu_sound_setup[i],CL_PAPER,CL_LT_CYAN);continue;} // курсор
    else { draw_text(xPos,yPos+10*i,menu_sound_setup[i],CL_INK,CL_PAPER);continue;} 
  }


wait_enter(); // ожидание отпускания enter
  while (1)
  {

 if (!decode_key_joy()) continue;
  
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos, yPos+ 10 * cPos, menu_sound_setup[cPos], CL_INK, CL_BLACK); // стирание курсора
      cPos++;
      if (cPos == Pos)
        cPos = 0;
      draw_text(xPos, yPos+ 10 * cPos, menu_sound_setup[cPos], CL_BLACK, CL_LT_CYAN);   // курсор
  
    }

    if (kb_st_ps2.u[2] & KB_U2_UP)

    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos, yPos+ 10 * cPos, menu_sound_setup[cPos], CL_INK, CL_BLACK); // стирание курсора
      if (cPos == 0)
        cPos = Pos;
      cPos--;
      draw_text(xPos, yPos+ 10 * cPos, menu_sound_setup[cPos], CL_BLACK, CL_LT_CYAN);  // курсор
      
    }

    if (kb_st_ps2.u[2] & KB_U2_LEFT)
    {
      kb_st_ps2.u[2] = 0;

      switch (cPos)
      {
      case 0:// громкость -
      if (conf.type_sound >I2S_TS) break;
      if ((conf.type_sound==SOFT_AY) |  (conf.type_sound==SOFT_TS))
      {
       if (conf.vol_ay==0) conf.vol_ay=0;
          else conf.vol_ay--;
        vol_ay=conf.vol_ay; 
        init_vol_ay();
      }        
       // I2S AY
       if ((conf.type_sound==I2S_AY) |  (conf.type_sound==I2S_TS))
      {
       if (conf.vol_i2s==0) conf.vol_i2s=0;
          else conf.vol_i2s--;
          vol_ay=conf.vol_i2s; 
         init_vol_ay();
      }

      init_menu_sound_setup();
      draw_text(xPos,yPos+10*0,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
        break;
        
        case 1:// усилитель -
         if ((conf.type_sound == I2S_AY) || (conf.type_sound == I2S_TS))
         {   
         if (conf.vol_ay==0) conf.vol_ay=0;
            else conf.audio_buster--;
            conf.audio_buster &=0x07;
          //  if (conf.audio_buster>7) conf.audio_buster=0;
          set_audio_buster();
             init_menu_sound_setup();
        draw_text(xPos,yPos+10*1,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN); 

         } 
          break;      
          
          case 2:// noise fdd -
          conf.sound_fdd  ^= true;
          init_menu_sound_setup();
          draw_text(xPos,yPos+10*2,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN);  
          break;

          case 3: // громкость load -
            conf.vol_load--;
            conf.vol_load &= 0x0f;
            init_vol_ay();
           init_menu_sound_setup();
           draw_text(xPos,yPos+10*3,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
            break;


          case 4:// beep mode -
          if (conf.beep_mode == 0) conf.beep_mode=0;
            else conf.beep_mode--;
          init_menu_sound_setup();
          draw_text(xPos,yPos+10*4,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN);
          break;



      }
   
    }
   
    if (kb_st_ps2.u[2] & KB_U2_RIGHT)
   
    {
      kb_st_ps2.u[2] = 0;
      switch (cPos)
      {
      case 0:// громкость +
      if (conf.type_sound >I2S_TS) break;
      if ((conf.type_sound==SOFT_AY) |  (conf.type_sound==SOFT_TS))
      {
       if (conf.vol_ay==MAX_VOL_PWM) conf.vol_ay=MAX_VOL_PWM;
          else conf.vol_ay++;
        vol_ay=conf.vol_ay; 
        init_vol_ay();
      }
       // I2S AY
      if ((conf.type_sound==I2S_AY) |  (conf.type_sound==I2S_TS))
      {
       if (conf.vol_i2s==100) conf.vol_i2s=100;
          else conf.vol_i2s++;
        vol_ay=conf.vol_i2s; 
        init_vol_ay();
      }    
             init_menu_sound_setup();
        draw_text(xPos,yPos+10*0,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
        break;

        case 1:// усилитель +
        if ((conf.type_sound == I2S_AY) || (conf.type_sound == I2S_TS))  
        {
        if (conf.vol_ay==0) conf.vol_ay=0;
           else conf.audio_buster++;
           conf.audio_buster &=0x07;
           set_audio_buster();
             init_menu_sound_setup();
        draw_text(xPos,yPos+10*1,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
        } 
         break;   

         case 2:// noise fdd +
         conf.sound_fdd  ^= true;
             init_menu_sound_setup();
        draw_text(xPos,yPos+10*2,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
         break;    

          case 3: // громкость load +
              conf.vol_load++;
            conf.vol_load &= 0x0f;
            init_vol_ay();
             init_menu_sound_setup();
           draw_text(xPos,yPos+10*3,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
            break;

           case 4:// beep mode
          if (conf.beep_mode == 2) conf.beep_mode=2;
            else conf.beep_mode++;
          init_menu_sound_setup();
          draw_text(xPos,yPos+10*4,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN);
          break;



      default:
        break;
      }
    }




    if (kb_st_ps2.u[1]&KB_U1_ENTER) // enter
    {
     switch (cPos)
     {
      uint8_t x;
     case 2:
     conf.sound_fdd  ^= true;
             init_menu_sound_setup();
        draw_text(xPos,yPos+10*2,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN); 
     continue;

           case 4:// beep mode
           draw_text(xPos,yPos+10*4,menu_sound_setup_strings[cPos],  CL_BLACK, CL_PINK);
           g_delay_ms(200);
       //   if (conf.beep_mode =3) conf.beep_mode=3;
       //     else conf.beep_mode++;
       //   init_menu_sound_setup();
          draw_text(xPos,yPos+10*4,menu_sound_setup_strings[cPos],  CL_BLACK, CL_LT_CYAN);
          continue;
         // break;



       case 5:// Save config
       save_config();
       break;

      case 6:// Return menu
      return 0xff; // ESC exit
     }


    wait_enter();



      return cPos;
    }

    if (kb_st_ps2.u[1]&KB_U1_ESC) 
    {
      wait_esc();
    return 0xff; // ESC exit
    }

  }
}
//##############################################################
void draw_logo (int x,int y,color_t colorInc,color_t colorPaper)
{
draw_symbol(x+0, y+4,0,colorInc, colorPaper);
draw_symbol(x+6, y+1,0,colorInc, colorPaper);
draw_symbol(x+12, y+4,0,colorInc, colorPaper);
draw_symbol(x+3, y+11,2,colorInc, colorPaper);
draw_symbol(x+7, y+11,1,colorInc, colorPaper);
}