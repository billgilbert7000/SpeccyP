#include "config.h"
#include "screen_util.h"

#include "usb_key.h"// это добавить
#include "hardware/pwm.h"
//#include "kbd_img.h"
#include "tusb.h"
#include <string.h> /* memset */
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

void draw_text(int x,int y,char* text,color_t colorText,color_t colorBg)
{
    for(int line=0;line<FONT_H;line++)
    {
       // uint8_t* symb=text;
       char* symb=text;

        int yt=y+line;
        int xt=x;
        while(*symb)
        {
           uint8_t symb_data=FONT[*symb*FONT_H+line];
#ifdef FONT6X8 
 symb_data = (symb_data * 0x0202020202ULL & 0x010884422010ULL) % 1023; symb_data>>=1; // зеркалирование шрифта
#endif
  // uint8_t s = symb_data; s<<=1; symb_data=s | symb_data ;// bold font
 // uint8_t s = symb_data; s>>=1; symb_data=s | symb_data ;// bold font
           for(int i=0;i<FONT_W;i++) 
           {
             
                if (symb_data&(1<<(FONT_W-1)))draw_pixel(xt++,yt,colorText); else draw_pixel(xt++,yt,colorBg);
            
                symb_data<<=1;
           }
    //   xt++;// увеличение расстояния между буквами
           symb++;
        }
    }
}

void draw_text_len(int x,int y,char* text,color_t colorText,color_t colorBg,int len){

    //printf("text[%s]\t",text);

    for(int line=0;line<FONT_H;line++){
       // uint8_t* symb=text;

        char* symb=text;
        //printf("\nch[%2X]\t",*symb);
        //printf("addr[%4X]\t",*symb*FONT_H);
        int yt=y+line;
        int xt=x;
        int inx_symb=0;
        while(*symb){
            uint8_t symb_data=FONT[*symb*FONT_H+line];
   #ifdef FONT6X8          
   symb_data = (symb_data * 0x0202020202ULL & 0x010884422010ULL) % 1023; symb_data>>=1; // зеркалирование шрифта      
  //    uint8_t s = symb_data; s<<=1; symb_data=s | symb_data ;// bold font
  #endif
            for(int i=0;i<FONT_W;i++){

                if (symb_data&(1<<(FONT_W-1)))draw_pixel(xt++,yt,colorText); else draw_pixel(xt++,yt,colorBg);
                symb_data<<=1;
              
           }
           symb++;
           inx_symb++;
           if (inx_symb>=len) break;
        }
        while (inx_symb<len)
        {
           uint8_t symb_data=0;
           for(int i=0;i<FONT_W;i++) 
           {
                if (symb_data&(1<<(FONT_W-1)))draw_pixel(xt++,yt,colorText); else draw_pixel(xt++,yt,colorBg);
                symb_data<<=1;
           }
            inx_symb++;

        }
    }
    //printf("\n");
}

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
    	            int yt=(y+52+(segment*64)+(symbol_row*8));//16+11
        	        int xt=((x*8)+17+FONT_W*14-(FONT_W*4));//17+FONT_W*14+11
					//printf("x:[%d]\ty:[%d]\txt:[%d]\tyt:[%d]\tseg:[%d]\tsymb:[%d]\taddr[%d]\n",x,y,xt,yt,segment,symb,pixel_addr);
					//printf("seg:[%d]\tsymbol_row:[%d]\taddr[%d]\tattr[%d]\n",segment,symbol_row,pixel_addr,attr_addr);
                	uint8_t pixel_data=buffer[pixel_addr];
					uint8_t color_data=buffer[attr_addr];
                	for(int i=0;i<8;i++){
						if((xt<312)&&(yt<230)){
							uint8_t bright_color = (color_data&0b01000000)>>3;
							uint8_t fg_color = color_data&0b00000111;
							uint8_t bg_color = (color_data&0b00111000)>>3;
							//printf("bright_color:[%02X]\tfg_color:[%02X]\tbg_color[%02X]\n",bright_color,fg_color,bg_color);
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
	draw_rect(9,7,SCREEN_W-16,SCREEN_H-15,COLOR_BORDER,false);//Основная рамка
    draw_line(10+FONT_W*14,10,10+FONT_W*14,SCREEN_H-8,COLOR_BORDER);  //центральная верт линия 
}
//------------------------------------
void draw_file_window()
{
	draw_rect(17+FONT_W*14,208,100,22,COLOR_BACKGOUND,true); //Фон отображения скринов
 
     draw_text_len(2+FONT_W,FONT_H*24,"T:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(2+FONT_W*3,FONT_H*24,TapeName,COLOR_TR1,COLOR_TR0 ,12);

     draw_text_len(2+FONT_W,FONT_H*25,"A:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(2+FONT_W*3,FONT_H*25,conf.DiskName[0],COLOR_TR1,COLOR_TR0 ,12);
     draw_text_len(2+FONT_W,FONT_H*26,"B:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(2+FONT_W*3,FONT_H*26,conf.DiskName[1],COLOR_TR1,COLOR_TR0, 12);
     draw_text_len(2+FONT_W,FONT_H*27,"C:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(2+FONT_W*3,FONT_H*27,conf.DiskName[2],COLOR_TR1,COLOR_TR0, 12);
     draw_text_len(2+FONT_W,FONT_H*28,"D:",COLOR_TR1,COLOR_TR0, 2);
	 draw_text_len(2+FONT_W*3,FONT_H*28,conf.DiskName[3],COLOR_TR1,COLOR_TR0, 12);
	
}
//----------------------


/* void draw_keyboard(uint8_t xPos,uint8_t yPos)
{ //x:33 y:84
	for(uint8_t y=0;y<156;y++){
		for(uint8_t x=0;x<254;x++){
			uint8_t pixel = kbd_img[x+(y*254)];
			if (pixel<0xFF)	draw_pixel(xPos+x,yPos+y,pixel);
			//printf("X:%d Y:%d C:[%02X]\n",x,y,pixel);
		}
	}
} */

/*-------Graphics--------*/

//==========================================================================================const char
//uint8_t MenuBox(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text, char  *m_text[], uint8_t Pos,uint8_t cPos,uint8_t over_emul)
uint8_t MenuBox(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text, const char *m_text[], uint8_t Pos,uint8_t cPos,uint8_t over_emul)
{
	//uint8_t max_len= strlen(text)>strlen(text1)?strlen(text):strlen(text1);
	//uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	//uint8_t left_y = strlen(text1)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	//uint8_t height = strlen(text1)>0 ? FONT_H*2+5:FONT_H+5;
	if(over_emul) zx_machine_enable_vbuf(false);
    uint16_t lFrame = (lPos*FONT_W)+10;
    uint16_t hFrame = ((1+hPos)*FONT_H)+20;
   // draw_rect(xPos+3,yPos+2,lFrame+3,hFrame+3,CL_GRAY,true);// тень
    draw_rect(xPos-2,yPos-2,lFrame+4,hFrame+4,CL_BLACK,false);// рамка 1
    draw_rect(xPos-1,yPos-1,lFrame+2,hFrame+2,CL_GRAY,false);// рамка 2
    draw_rect(xPos,yPos,lFrame,hFrame,CL_BLACK,true);// рамка 3 фон
    draw_rect(xPos,yPos,lFrame,7,CL_GRAY,true); // шапка меню
    draw_text(xPos+10,yPos+0,text,CL_PAPER,CL_INK);// шапка меню

   // draw_logo_header(xPos+lFrame-42,yPos );// рисование символа спектрума

   // draw_text(xPos+6,yPos+9,"No cursor",CL_BLACK,CL_WHITE);
yPos=yPos+10;
//xPos=xPos+10;
//draw_rect(xPos,yPos+cPos*8,lFrame,8,CL_WHITE,true); // курсор
  //  draw_rect(xPos,yPos+16,lFrame,8,CL_LT_CYAN,true); // курсор 
  for(uint8_t i=0;i<hPos;i++){
    if (i >= Pos) {draw_text(xPos+1,yPos+8+8*i,m_text[i],CL_BLUE,CL_PAPER); continue;}
    if (i == cPos)  {draw_text(xPos+1,yPos+8+8*i,m_text[i],CL_PAPER,CL_LT_CYAN);continue;} // курсор
    else { draw_text(xPos+1,yPos+8+8*i,m_text[i],CL_INK,CL_PAPER);continue;}
    
  }


	//draw_rect(left_x-2,left_y-2,(max_len*FONT_W)+3,height,CL_BLUE,true);
	//draw_rect(left_x-2,left_y-2,(max_len*FONT_W)+3,height,CL_GREEN,false);

	
	//if (strlen(text1)>0) draw_text(left_x,left_y+FONT_H+1,text1,colorFG,colorBG);
	//printf("X:%d,Y:%d\n",left_x,left_y);
/*      kb_st_ps2.u[0]=0x0;
     kb_st_ps2.u[1]=0x0;
     kb_st_ps2.u[2]=0x0;
     kb_st_ps2.u[3]=0x0; */

//sleep_ms(200);

wait_enter(); // ожидание отпускания enter
  while (1)
  {

 if (!decode_key_joy()) continue;
  
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 8 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
      cPos++;
      if (cPos == Pos)
        cPos = 0;
      draw_text(xPos + 1, yPos + 8 + 8 * cPos, m_text[cPos], CL_BLACK, CL_LT_CYAN);   // курсор
    
   //  sleep_ms(DELAY_KEY);
  
    }

    if (kb_st_ps2.u[2] & KB_U2_UP)

    {
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 8 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
      if (cPos == 0)
        cPos = Pos;
      cPos--;
      draw_text(xPos + 1, yPos + 8 + 8 * cPos, m_text[cPos], CL_BLACK, CL_LT_CYAN);  // курсор
     
   //    sleep_ms(DELAY_KEY);
      
    }

    if (kb_st_ps2.u[1]&KB_U1_ENTER) // enter
    {
   //  draw_rect(xPos-1,yPos-11,lFrame+2,hFrame+2,CL_PAPER,true);// рамка 1

    
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
uint8_t MenuBox_help(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, const char *m_text[], uint8_t Pos, uint8_t cPos, uint8_t over_emul)
{
  if (over_emul)
    zx_machine_enable_vbuf(false);
  uint16_t lFrame = (lPos * FONT_W) + 10;
  uint16_t hFrame = ((1 + hPos) * FONT_H) + 20;

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
 /*    if (F1) // F1
    {
       kb_st_ps2.u[3] = 0x00;
      return 0xff;
    } */
  }
}
////////////////////////////////////////////////////////////////////////////////////////////
uint8_t MenuBox_bw(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, const char*m_text[], uint8_t Pos, uint8_t cPos, uint8_t over_emul)
{
  if (over_emul)
    zx_machine_enable_vbuf(false);
  uint16_t lFrame = (lPos * FONT_W) + 10;
  uint16_t hFrame = ((1 + hPos) * FONT_H) + 20;
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
  // decode_joy_to_keyboard();
  // if (!decode_key())  continue; // если нажата кнопка на клавиатуре или джой
  // if ( (!decode_key()) && (!decode_joy_to_keyboard())) continue; // если нажата кнопка на клавиатуре или джой
if (!decode_key_joy()) continue; // если нажата кнопка на клавиатуре или джой

    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      flag_usb_kb = true;
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
      cPos++;
      if (cPos == Pos)
        cPos = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_BLACK, CL_INK);
      ; // курсор
    //  sleep_ms(DELAY_KEY);
    }
    if (kb_st_ps2.u[2] & KB_U2_UP)
    {
      flag_usb_kb = true;
      kb_st_ps2.u[2] = 0;
      draw_text(xPos + 1, yPos + 8 + 10 * cPos, m_text[cPos], CL_INK, CL_BLACK); // стирание курсора
      if (cPos == 0)
        cPos = Pos;
      cPos--;
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
/*     if (kb_st_ps2.u[2] & KB_U2_HOME) //HOME
    {
      kb_st_ps2.u[2] = 0x00;
     // sleep_ms(DELAY_KEY);
      return 0xff;
    } */
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
  uint16_t lFrame = (lPos * FONT_W);
  uint16_t hFrame = ((4 + hPos) * FONT_H);
  draw_rect(xPos - 2, yPos - 2, lFrame + 4, hFrame + 4, CL_INK, false);   // рамка 1
  draw_rect(xPos - 1, yPos - 1, lFrame + 2, hFrame + 2, CL_BLACK, false); // рамка 2
  draw_rect(xPos, yPos, lFrame, hFrame, CL_PAPER, true);                  // рамка 3 фон
  draw_rect(xPos, yPos, lFrame, 7, CL_INK, true);                         // шапка меню
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
  }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////
    void MenuBox_lite(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, char *text, uint8_t over_emul)
    {
      if (over_emul)
        zx_machine_enable_vbuf(false);
      // uint8_t max_len= strlen(text)>strlen(text1)?strlen(text):strlen(text1);
      // uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
      // uint8_t left_y = strlen(text1)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
      // uint8_t height = strlen(text1)>0 ? FONT_H*2+5:FONT_H+5;
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
	//printf("X:%d,Y:%d\n",left_x,left_y);
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

//==========================================================================================
uint8_t MenuBox_tft(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, char *text, uint8_t over_emul)
{
  if (over_emul)
    zx_machine_enable_vbuf(false);
  uint16_t lFrame = (lPos * FONT_W) + 10;
  uint16_t hFrame = ((1 + hPos) * FONT_H) + 20;
  draw_rect(xPos - 2, yPos - 2, lFrame + 4, hFrame + 4, CL_BLACK, false); // рамка 1
  draw_rect(xPos - 1, yPos - 1, lFrame + 2, hFrame + 2, CL_GRAY, false);  // рамка 2
  draw_rect(xPos, yPos, lFrame, hFrame, CL_BLACK, true);                  // рамка 3 фон
  draw_rect(xPos, yPos, lFrame, 7, CL_GRAY, true);                        // шапка меню
  draw_text(xPos + 10, yPos + 0, text, CL_PAPER, CL_INK);                 // шапка меню

  yPos = yPos + 20;

  kb_st_ps2.u[0] = 0x0;
  kb_st_ps2.u[1] = 0x0;
  kb_st_ps2.u[2] = 0x0;
  kb_st_ps2.u[3] = 0x0;
  char progress[21]= {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,0};

  //icon[0] = 0x1F;
  sprintf(temp_msg, " %d%% ", conf.tft_bright);
  draw_text(xPos + 64, yPos + 0, temp_msg, CL_INK, CL_PAPER);                                      //
  draw_text_len(xPos + 10, yPos + 15,progress, CL_LT_CYAN, CL_PAPER, (conf.tft_bright / 5)); //


wait_enter(); // ожидание отпускания enter
  while (1)
  {

    // if (mode_kbms)  sleep_ms(DELAY_KEY); // задержка если это не ps/2
    if (!decode_key_joy())
      continue;

    if ((kb_st_ps2.u[2] & KB_U2_DOWN) | (kb_st_ps2.u[2] & KB_U2_LEFT))
    {
      kb_st_ps2.u[2] = 0;

      conf.tft_bright = conf.tft_bright - 5;
      if (conf.tft_bright < 5)
        conf.tft_bright = 5;
      pwm_set_gpio_level(TFT_LED_PIN, conf.tft_bright); // % яркости

   //   sleep_ms(DELAY_KEY);
    }

    if ((kb_st_ps2.u[2] & KB_U2_UP) | (kb_st_ps2.u[2] & KB_U2_RIGHT))

    {
      kb_st_ps2.u[2] = 0;

      conf.tft_bright = conf.tft_bright + 5;
      if (conf.tft_bright > 100)
        conf.tft_bright = 100;
      pwm_set_gpio_level(TFT_LED_PIN, conf.tft_bright); // % яркости

  //    sleep_ms(DELAY_KEY);
    }

    sprintf(temp_msg, " %d%% ", conf.tft_bright);
    draw_text(xPos + 64, yPos + 0, temp_msg, CL_INK, CL_PAPER);                                          //
    draw_text_len(xPos + 10, yPos + 15, "                    ", CL_INK, CL_PAPER, 20);                   //
    draw_text_len(xPos + 10, yPos + 15, progress, CL_LT_CYAN, CL_PAPER, (conf.tft_bright / 5)); //

    if (kb_st_ps2.u[1] & KB_U1_ENTER) // enter
    {

  wait_enter();

      return 0;
    }

    if (kb_st_ps2.u[1] & KB_U1_ESC)
    {
      wait_esc();
      return 0xff; // ESC exit
    }
  }
}
