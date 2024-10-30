#include "config.h"

#include "HDMI.h"
#include "hardware/clocks.h"
#include <stdalign.h> 

#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include <string.h> 
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "stdlib.h"



//программы PIO
//программа конвертации адреса для HDMI

uint16_t pio_program_instructions_conv_HDMI[] = {             
                                  
    //         //     .wrap_target
    0x80a0, //  0: pull   block                      
    0x40e8, //  1: in     osr, 8                    
    0x4034, //  2: in     x, 20                      
    0x8020, //  3: push   block                      
            //     .wrap


};


const struct pio_program pio_program_conv_addr_HDMI = {
    .instructions = pio_program_instructions_conv_HDMI,
    .length = 4,
    .origin = -1,
};

//программа конвертации адреса для VGA

uint16_t pio_program_instructions_conv_VGA[] = {             
                                  
    //         //     .wrap_target
    0x80a0, //  0: pull   block                      
    0x40e8, //  1: in     osr, 8                    
    0x4037, //  2: in     x, 23                      
    0x8020, //  3: push   block                      
            //     .wrap


};


const struct pio_program pio_program_conv_addr_VGA= {
    .instructions = pio_program_instructions_conv_VGA,
    .length = 4,
    .origin = -1,
};

//программа видеовывода HDMI
static const uint16_t instructions_PIO_HDMI[] = {
    
    
    0x7006, //  0: out    pins, 6         side 2     
    0x7006, //  1: out    pins, 6         side 2     
    0x7006, //  2: out    pins, 6         side 2     
    0x7006, //  3: out    pins, 6         side 2     
    0x7006, //  4: out    pins, 6         side 2     
    0x6806, //  5: out    pins, 6         side 1     
    0x6806, //  6: out    pins, 6         side 1     
    0x6806, //  7: out    pins, 6         side 1     
    0x6806, //  8: out    pins, 6         side 1     
    0x6806, //  9: out    pins, 6         side 1  

      
};

static const struct pio_program program_PIO_HDMI = {
    .instructions = instructions_PIO_HDMI,
    .length = 10,
    .origin = -1,
};


//программа видеовывода VGA

static uint16_t pio_program_VGA_instructions[] = {

	//	 .wrap_target

	//	 .wrap_target
	0x6008, //  0: out	pins, 8
	//	 .wrap
	//	 .wrap
};

static const struct pio_program program_pio_VGA = {
	.instructions = pio_program_VGA_instructions,
	.length = 1,
	.origin = -1,
};

typedef struct V_MODE{
  int VS_begin;
  int VS_end;
  int V_visible_lines;
  int V_total_lines;

  int H_visible_len;
  int H_visible_begin;
  int HS_len;
  int H_len;

  uint8_t VS_TMPL;
  uint8_t VHS_TMPL;
  uint8_t HS_TMPL;
  uint8_t NO_SYNC_TMPL;
  

} V_MODE;

typedef struct G_BUFFER{
    uint width;
    uint height;
    int shift_x;
    int shift_y;
    uint8_t* data;

}G_BUFFER;
//данные модуля 


//640x480x60
   static V_MODE v_mode=
 {
     .VS_begin=491,
     .VS_end=492,
    .V_total_lines=525,
     .V_visible_lines=480,
    
     .HS_len=96/2,
     .H_len=800/2,
     .H_visible_begin=144/2,
     .H_visible_len=640/2,


     .VS_TMPL=242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
     .VHS_TMPL=243,
     .HS_TMPL=241,
     .NO_SYNC_TMPL=240



 };
 

/* //640x480x60
  static V_MODE v_mode=
 {
     .VS_begin=491,
     .VS_end=492,
    .V_total_lines=525,
     .V_visible_lines=480,
    
     .HS_len=96/2,
     .H_len=800/2,
     .H_visible_begin=144/2,
     .H_visible_len=640/2,


     .VS_TMPL=242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
     .VHS_TMPL=243,
     .HS_TMPL=241,
     .NO_SYNC_TMPL=240



 }; */
/* 
//640x480x75
 static V_MODE v_mode=
 {
     .VS_begin=482,
     .VS_end=484,
     .V_total_lines=500,
     .V_visible_lines=480,
    
     .HS_len=64/2,
     .H_len=840/2,
     .H_visible_begin=184/2,
     .H_visible_len=640/2,


     .VS_TMPL=242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
     .VHS_TMPL=243,
     .HS_TMPL=241,
     .NO_SYNC_TMPL=240



 };
 */
 
//640x480x73
/* static V_MODE v_mode=
{
    .VS_begin=490,
    .VS_end=491,
    .V_total_lines=520,
    .V_visible_lines=480,
    
    .HS_len=40/2,
    .H_len=832/2,
    .H_visible_begin=168/2,
    .H_visible_len=640/2,


    .VS_TMPL=242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
    .VHS_TMPL=243,
    .HS_TMPL=241,
    .NO_SYNC_TMPL=240



}; */
//640x480x85
 /* static V_MODE v_mode=
 {
     .VS_begin=482,
     .VS_end=484,
     .V_total_lines=509,
     .V_visible_lines=480,
    
     .HS_len=56/2,
     .H_len=832/2,
     .H_visible_begin=136/2,
     .H_visible_len=640/2,


     .VS_TMPL=242,   //шаблоны синхронизации заложены в таблицу палитры после 240 индекса
     .VHS_TMPL=243,
     .HS_TMPL=241,
     .NO_SYNC_TMPL=240



 };
 */
static G_BUFFER g_buf=
{
     .data=NULL,
     .shift_x=0,
     .shift_y=0,
     .height=240,
     .width=320
};

//буферы строк
//количество буферов задавать кратно степени двойки
//
#define N_LINE_BUF_log2 (3)


#define N_LINE_BUF_DMA (1<<N_LINE_BUF_log2)
#define N_LINE_BUF (N_LINE_BUF_DMA/2)//2

//максимальный размер строки
#define LINE_SIZE_MAX (420)
//указатели на буферы строк
//выравнивание нужно для кольцевого буфера
static uint32_t rd_addr_DMA_CTRL[N_LINE_BUF*2]__attribute__ ((aligned (4*N_LINE_BUF_DMA))) ;
//непосредственно буферы строк

static uint32_t lines_buf[N_LINE_BUF][LINE_SIZE_MAX/4];



static int SM_video=SM_VIDEO;
static int SM_conv=SM_CONV;


//DMA каналы
//каналы работы с первичным графическим буфером 
static int dma_chan_ctrl=-1;
static int dma_chan=-1;
//каналы работы с конвертацией палитры
static int dma_chan_pal_conv_ctrl=-1;
static int dma_chan_pal_conv=-1;

//ДМА палитра для конвертации
static alignas(4096)uint32_t conv_color[1024];

static g_mode active_mode;
static g_out active_out;
repeating_timer_t hdmi_timer;


//программа установки начального адреса массива-конвертора
static void pio_set_x(PIO pio, int sm, uint32_t v) {
   uint instr_shift = pio_encode_in(pio_x, 4);
   uint instr_mov = pio_encode_mov(pio_x, pio_isr);
  for (int i=0;i<8;i++)
  {
    const uint32_t nibble = (v >> (i * 4)) & 0xf;
    pio_sm_exec(pio, sm, pio_encode_set(pio_x, nibble));
    pio_sm_exec(pio, sm, instr_shift);
  }
  pio_sm_exec(pio, sm, instr_mov);
} 

//подготовка данных HDMI для вывода на пины
static uint64_t get_ser_diff_data(uint16_t dataR,uint16_t dataG,uint16_t dataB)
{
    uint64_t out64=0;
    for(int i=0;i<10;i++)
    {
        out64<<=6;
        if(i==5) out64<<=2;
        uint8_t bR=(dataR>>(9-i))&1;
        uint8_t bG=(dataG>>(9-i))&1;
        uint8_t bB=(dataB>>(9-i))&1;
        
        bR|=(bR^1)<<1;
        bG|=(bG^1)<<1;
        bB|=(bB^1)<<1;
        
        if (HDMI_PIN_invert_diffpairs)
        {
            bR^=0b11;
            bG^=0b11;
            bB^=0b11;
        };
        uint8_t d6;
        if (HDMI_PIN_RGB_notBGR)
        {
            d6=(bR<<4)|(bG<<2)|(bB<<0);
        }
        else
        {
            d6=(bB<<4)|(bG<<2)|(bR<<0);
        };


        out64|=d6;



    }
    return out64;
};

//конвертор TMDS
static uint tmds_encoder(uint8_t d8)
{
    int s1=0;
    for(int i=0;i<8;i++) s1+=(d8&(1<<i))?1:0;
    int is_xnor=0;
    if ((s1>4)||((s1==4)&&((d8&1)==0))) is_xnor=1;
    uint16_t d_out=d8&1;
    uint16_t qi=d_out;
    for(int i=1;i<8;i++)
    {
        d_out|=((qi<<1)^(d8&(1<<i)))^(is_xnor<<i);
        qi=d_out&(1<<i);
    }
    
    if(is_xnor)
    d_out|=1<<9; 
    else 
    d_out|=1<<8; 

    return d_out;

}






void graphics_set_mode(g_mode mode){
    active_mode=mode;
};

//определение палитры
void graphics_set_palette(uint8_t i, uint32_t color888) 
{
    if (i>=240) return;

   
        if (active_out==g_out_HDMI)
            {
            uint64_t* conv_color64=(uint64_t*) conv_color;
            uint8_t R=(color888>>16)&0xff;
            uint8_t G=(color888>>8)&0xff;
            uint8_t B=(color888>>0)&0xff;

            //формирование цветов парами сбаллансированных пикселей
            conv_color64[i*2]=get_ser_diff_data(tmds_encoder(R),tmds_encoder(G),tmds_encoder(B));
          // conv_color64[i*2+1]=conv_color64[i*2]^0xff03ffffffffffc0l;
          conv_color64[i*2+1]=conv_color64[i*2]^0x0003ffffffffffffl;
            };
        if (active_out==g_out_VGA)
            {
            uint8_t conv0[] = { 0b00, 0b00, 0b01, 0b10, 0b10, 0b10, 0b11, 0b11 };
            uint8_t conv1[] = { 0b00, 0b01, 0b01, 0b01, 0b10, 0b11, 0b11, 0b11 };
            uint8_t b = ((color888 & 0xff) / 42);
            uint8_t g = (((color888 >> 8) & 0xff) / 42);
            uint8_t r = (((color888 >> 16) & 0xff) / 42);
            uint8_t c_hi = (conv0[r] << 4) | (conv0[g] << 2) | conv0[b];
            uint8_t c_lo = (conv1[r] << 4) | (conv1[g] << 2) | conv1[b];
            uint16_t palette16_mask=(0xc0<<8)|(0xc0);

            uint16_t* conv_color16=(uint16_t*) conv_color;
            conv_color16[i] = (((c_hi << 8) | c_lo) & 0x3f3f) | palette16_mask;
        
            };

}


//основная функция заполнения буферов видеоданных 
void __not_in_flash_func(main_video_loop)()
{
    static uint dma_inx_out=0;
    static uint lines_buf_inx=0;


    if (dma_chan_ctrl==-1) return;//не определен дма канал

    //получаем индекс выводимой строки
    uint dma_inx=(N_LINE_BUF_DMA-2+((dma_channel_hw_addr(dma_chan_ctrl)->read_addr-(uint32_t)rd_addr_DMA_CTRL)/4))%(N_LINE_BUF_DMA);


    //uint n_loop=(N_LINE_BUF_DMA+dma_inx-dma_inx_out)%N_LINE_BUF_DMA;
    
    static uint32_t line_active=0;
    static uint8_t* vbuf=NULL; 
    static uint32_t frame_i=0;

    //while(n_loop--)
    while(dma_inx_out!=dma_inx)
    {
      //режим VGA
      line_active++;
      if (line_active==v_mode.V_total_lines) {line_active=0;frame_i++;vbuf=g_buf.data;}
      
     
      
      if (line_active<v_mode.V_visible_lines)
      {
        //зона изображения
        //if (false)
        switch (active_mode)
        {
        case g_mode_320x240x4bpp:
        case g_mode_320x240x8bpp:
            //320x240 графика
            if (line_active&1)
            // if (false)
                {
                    //повтор шаблона строки
                }
            else
                {
                     //новая строка
                     lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF; 
                     uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];
                     
                     
                     //зона синхры, затираем все шаблоны
                     if(line_active<(N_LINE_BUF_DMA*2))
                        {
                              memset(out_buf8,v_mode.HS_TMPL,v_mode.HS_len);
                              memset(out_buf8+v_mode.HS_len,v_mode.NO_SYNC_TMPL,v_mode.H_visible_begin-v_mode.HS_len);  
                              memset(out_buf8+v_mode.H_visible_begin+v_mode.H_visible_len,v_mode.NO_SYNC_TMPL,v_mode.H_len-(v_mode.H_visible_begin+v_mode.H_visible_len));

                        }

                    //формирование картинки
                    int line=line_active/2;
                    out_buf8+=v_mode.H_visible_begin;
                    
                    uint8_t* vbuf8;
                    
                    
                    if (active_mode==g_mode_320x240x4bpp)
                    {       //для 4-битного буфера
                            vbuf8=vbuf+(line)*g_buf.width/2; 

                            if (vbuf!=NULL)// ERROR HDMI
                         //  {}
                            // if ((line_active+(frame_i))&1) 
                            // if ((frame_i&1)&((line_active+(frame_i>>1))&1))
                            // if (((frame_i)&1)&((line_active+(frame_i>>2))&1))
                            // if ((line+frame_i*1)&1)
                           
                            if (false)
                                for(int i=v_mode.H_visible_len/2;i--;)
                                    {
                                            // *out_buf8++=16+(*vbuf8&0x0f);                                  
                                            // *out_buf8++=16+(*vbuf8++>>4);    
                                            
                                            *out_buf8++=0;                                  
                                            *out_buf8++=0;  
                                                                     
                                    }   
                            else
                                for(int i=v_mode.H_visible_len/16;i--;)       
                                    {

                                            *out_buf8++=*vbuf8&0x0f;                                  
                                            *out_buf8++=*vbuf8++>>4;  
                                            *out_buf8++=*vbuf8&0x0f;                                  
                                            *out_buf8++=*vbuf8++>>4;  
                                            *out_buf8++=*vbuf8&0x0f;                                  
                                            *out_buf8++=*vbuf8++>>4;  
                                            *out_buf8++=*vbuf8&0x0f;                                  
                                            *out_buf8++=*vbuf8++>>4;  

                                            *out_buf8++=*vbuf8&0x0f;                                  
                                            *out_buf8++=*vbuf8++>>4;  
                                            *out_buf8++=*vbuf8&0x0f;                                  
                                            *out_buf8++=*vbuf8++>>4;  
                                            *out_buf8++=*vbuf8&0x0f;                                  
                                            *out_buf8++=*vbuf8++>>4;  
                                            *out_buf8++=*vbuf8&0x0f;                                  
                                            *out_buf8++=*vbuf8++>>4;  
                                          
                                          
                                                                 
                                    }
                    }
                    else
                    {       //для 8-битного буфера

                            vbuf8=vbuf+(line)*g_buf.width; 
                            if (vbuf!=NULL)
                            for(int i=v_mode.H_visible_len/16;i--;)       
                                    {
                                       // *out_buf8++=100;
                                    //    *out_buf8++=(i<240)?i:0;vbuf8++; 
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  

                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  
                                            *out_buf8++=(*vbuf8<240)?*vbuf8:0;vbuf8++;                                  


                                                                        
                                    }
                    }

                  



                }
            break;
        
        default:
            break;
        }


      }
      else
        {
            if((line_active>=v_mode.VS_begin)&&(line_active<=v_mode.VS_end))
            {//кадровый синхроимпульс
                if (line_active==v_mode.VS_begin)
                    {
                        //новый шаблон
                        lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF; 
                        uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];

                        memset(out_buf8,v_mode.VHS_TMPL,v_mode.HS_len);
                        memset(out_buf8+v_mode.HS_len,v_mode.VS_TMPL,v_mode.H_len-v_mode.HS_len);
                        
                    }
                else
                    {
                        //повтор шаблона
                    }
            }
            else
                {
                    //строчный синхроимпульс
                    if((line_active==(v_mode.VS_end+1))||(line_active==(v_mode.V_visible_lines)))
                        {
                            //новый шаблон
                            lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF; 
                            uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];
                            memset(out_buf8,v_mode.HS_TMPL,v_mode.HS_len);
                            memset(out_buf8+v_mode.HS_len,v_mode.NO_SYNC_TMPL,v_mode.H_len-v_mode.HS_len);
                        }
                    else
                        {
                            //повтор шаблона
                        }
                }
        }
     
       
      rd_addr_DMA_CTRL[dma_inx_out]=(uint32_t)lines_buf[lines_buf_inx];//включаем заполненный буфер в данные для вывода
      dma_inx_out=(dma_inx_out+1)%(N_LINE_BUF_DMA);
      dma_inx=(N_LINE_BUF_DMA-2+((dma_channel_hw_addr(dma_chan_ctrl)->read_addr-(uint32_t)rd_addr_DMA_CTRL)/4))%(N_LINE_BUF_DMA);
    }

}

void graphics_set_buffer(uint8_t *buffer)
{
        g_buf.data=buffer;
       
}


bool __not_in_flash_func (hdmi_timer_callback(repeating_timer_t *rt)) {
    main_video_loop();
    return true;
}


//выделение и настройка общих ресурсов - 4 DMA канала, PIO программ и 2 SM
void graphics_init(g_out g_out)
{
        active_out=g_out;
//настройка PIO
        SM_video = pio_claim_unused_sm(PIO_VIDEO, true);
        SM_conv = pio_claim_unused_sm(PIO_VIDEO_ADDR, true);
//выделение  DMA каналов 
        dma_chan_ctrl=dma_claim_unused_channel(true);
        dma_chan=dma_claim_unused_channel(true);
        dma_chan_pal_conv_ctrl=dma_claim_unused_channel(true);
        dma_chan_pal_conv=dma_claim_unused_channel(true);
       
       //заполнение палитры по умолчанию(ч.б.)
        for(int ci=0;ci<240;ci++) graphics_set_palette(ci,(ci<<16)|(ci<<8)|ci);//

               //---------------

        uint offs_prg0=0;
        uint offs_prg1=0;
        const int base_inx=240;
        switch (active_out)
        {
        case g_out_HDMI:
                offs_prg1= pio_add_program(PIO_VIDEO_ADDR, &pio_program_conv_addr_HDMI);
                offs_prg0 = pio_add_program(PIO_VIDEO, &program_PIO_HDMI);
                pio_set_x(PIO_VIDEO_ADDR,SM_conv,((uint32_t)conv_color>>12));
                //индексы в палитре>240 - служебные    
                //240-243 служебные данные(синхра) напрямую вносим в массив -конвертер 
                uint64_t* conv_color64=(uint64_t*) conv_color;
                /* uint16_t b0=0b1101010100;
                uint16_t b1=0b0010101011;
                uint16_t b2=0b0101010100;
                uint16_t b3=0b1010101011; */
                uint16_t b0=0b1101010100;
                uint16_t b1=0b0010101011;
                uint16_t b2=0b0101010100;
                uint16_t b3=0b1010101011;

                conv_color64[2*base_inx+0]=get_ser_diff_data(b0,b0,b3);
                conv_color64[2*base_inx+1]=get_ser_diff_data(b0,b0,b3);
                    
                conv_color64[2*(base_inx+1)+0]=get_ser_diff_data(b0,b0,b2);
                conv_color64[2*(base_inx+1)+1]=get_ser_diff_data(b0,b0,b2);
                    
                conv_color64[2*(base_inx+2)+0]=get_ser_diff_data(b0,b0,b1);
                conv_color64[2*(base_inx+2)+1]=get_ser_diff_data(b0,b0,b1);
                    
                conv_color64[2*(base_inx+3)+0]=get_ser_diff_data(b0,b0,b0);
                conv_color64[2*(base_inx+3)+1]=get_ser_diff_data(b0,b0,b0);

            break;
         case g_out_VGA:
                offs_prg1= pio_add_program(PIO_VIDEO_ADDR, &pio_program_conv_addr_VGA);
                offs_prg0 = pio_add_program(PIO_VIDEO, &program_pio_VGA);
                pio_set_x(PIO_VIDEO_ADDR,SM_conv,((uint32_t)conv_color>>9));
                uint16_t* conv_color16=(uint16_t*) conv_color;
                
                conv_color16[base_inx]  =0b1100000011000000;//нет синхры
                conv_color16[base_inx+1]=0b1000000010000000;//строчная только
                conv_color16[base_inx+2]=0b0100000001000000;//кадровая только
                conv_color16[base_inx+3]=0b0000000000000000;//кадровая и строчная
          
 
            break;
        
        default:
            return;
            break;
        };
       

        
   
           
      

        //настройка PIO SM для конвертации

        pio_sm_config c_c = pio_get_default_sm_config();
        if (active_out==g_out_HDMI)
            sm_config_set_wrap(&c_c, offs_prg1 , offs_prg1 + (pio_program_conv_addr_HDMI.length-1));
        if (active_out==g_out_VGA)
            sm_config_set_wrap(&c_c, offs_prg1 , offs_prg1 + (pio_program_conv_addr_VGA.length-1));
        sm_config_set_in_shift(&c_c, true, false, 32);

        pio_sm_init(PIO_VIDEO_ADDR, SM_conv, offs_prg1, &c_c);
        pio_sm_set_enabled(PIO_VIDEO_ADDR, SM_conv, true); 

        //настройка PIO SM для вывода данных
        c_c = pio_get_default_sm_config();

        switch (active_out)
        {
        case g_out_HDMI:
            //настройка рабочей SM HDMI
            sm_config_set_wrap(&c_c, offs_prg0 , offs_prg0 + (program_PIO_HDMI.length-1));
         
            //настройка side set(пины CLK дифф пары HDMI)
            sm_config_set_sideset_pins(&c_c,beginHDMI_PIN_clk);
            sm_config_set_sideset(&c_c,2,false,false);
            for(int i=0;i<2;i++)
                {           
                pio_gpio_init(PIO_VIDEO, beginHDMI_PIN_clk+i);
                gpio_set_drive_strength(beginHDMI_PIN_clk+i,GPIO_DRIVE_STRENGTH_12MA);
                gpio_set_slew_rate(beginHDMI_PIN_clk+i,GPIO_SLEW_RATE_FAST);
                }
        
            pio_sm_set_pins_with_mask(PIO_VIDEO, SM_video, 3u<<beginHDMI_PIN_clk, 3u<<beginHDMI_PIN_clk);
            pio_sm_set_pindirs_with_mask(PIO_VIDEO, SM_video,  3u<<beginHDMI_PIN_clk,  3u<<beginHDMI_PIN_clk);
            //пины - диффпары  HDMI 

            for(int i=0;i<6;i++)
                {
                    gpio_set_slew_rate(beginHDMI_PIN_data+i,GPIO_SLEW_RATE_FAST);
                    pio_gpio_init(PIO_VIDEO, beginHDMI_PIN_data+i);
                    gpio_set_drive_strength(beginHDMI_PIN_data+i,GPIO_DRIVE_STRENGTH_12MA);
                    gpio_set_slew_rate(beginHDMI_PIN_data+i,GPIO_SLEW_RATE_FAST);
                }
            pio_sm_set_consecutive_pindirs(PIO_VIDEO, SM_video, beginHDMI_PIN_data, 6, true);//конфигурация пинов на выход            
            sm_config_set_out_pins(&c_c, beginHDMI_PIN_data, 6);


            

                //
            sm_config_set_out_shift(&c_c, true, true, 30);
            sm_config_set_fifo_join(&c_c,PIO_FIFO_JOIN_TX);

            pio_sm_init(PIO_VIDEO, SM_video, offs_prg0, &c_c);
            pio_sm_set_enabled(PIO_VIDEO, SM_video, true); 
        //    float fdiv=clock_get_hz(clk_sys)/(252000000.0);//частота пиксельклока    
            float fdiv=clock_get_hz(clk_sys)/(CPU_KHZ*1000);//частота пиксельклока 315000 HDMI 75 Mhz
            //доступны только целый и полуцелый делитель
            //для разумных частот 1 и 1.5
            fdiv=(fdiv<1)?1:fdiv;
            fdiv=(fdiv>1.4)?1.5:fdiv;
            fdiv=1.5;  //HDMI 1.0 315 75 Hz// 1.0 366 87Hz // 1.5 366 58Hz

            uint32_t div32=(uint32_t) (fdiv * (1 << 16)+0.0);
            PIO_VIDEO->sm[SM_video].clkdiv=div32&0xffff8000; //делитель для конкретной sm , накладываем маску , чтобы делитель был с шагом в 0.5

         break;
        case g_out_VGA:  //настройка рабочей SM VGA
            sm_config_set_wrap(&c_c, offs_prg0 , offs_prg0 + (program_pio_VGA.length-1)); 
        

            for(int i=0;i<8;i++)
                {
                    gpio_set_slew_rate(beginVideo_PIN+i,GPIO_SLEW_RATE_FAST);
                    pio_gpio_init(PIO_VIDEO, beginVideo_PIN+i);
                    gpio_set_drive_strength(beginVideo_PIN+i,GPIO_DRIVE_STRENGTH_12MA);
                    gpio_set_slew_rate(beginVideo_PIN+i,GPIO_SLEW_RATE_FAST);
                }
            pio_sm_set_consecutive_pindirs(PIO_VIDEO, SM_video, beginVideo_PIN, 8, true);//конфигурация пинов на выход            
            sm_config_set_out_pins(&c_c, beginVideo_PIN, 8);
            
            sm_config_set_out_shift(&c_c, true, true, 16);
            sm_config_set_fifo_join(&c_c,PIO_FIFO_JOIN_TX);

            pio_sm_init(PIO_VIDEO, SM_video, offs_prg0, &c_c);
            pio_sm_set_enabled(PIO_VIDEO, SM_video, true); 
                
            float fdiv2=clock_get_hz(clk_sys)/(25200000.0);//частота пиксельклока
      // fdiv2= 2;
            uint32_t div_32=(uint32_t) (fdiv2 * (1 << 16)+0.0);
            PIO_VIDEO->sm[SM_video].clkdiv=div_32&0xffff8000; //делитель для конкретной sm 
         break;
        default:
            return;
            break;
        }
        
      

        //настройки DMA



        //основной рабочий канал   
        dma_channel_config cfg_dma = dma_channel_get_default_config(dma_chan);
        channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_8);
        channel_config_set_chain_to(&cfg_dma, dma_chan_ctrl);// chain to other channel
            
        channel_config_set_read_increment(&cfg_dma, true);
        channel_config_set_write_increment(&cfg_dma, false);

            

        uint dreq=DREQ_PIO1_TX0+SM_conv;
        if (PIO_VIDEO_ADDR==pio0) dreq=DREQ_PIO0_TX0+SM_conv;
        channel_config_set_dreq(&cfg_dma, dreq);
            
        dma_channel_configure(
                dma_chan,
                &cfg_dma,
                &PIO_VIDEO_ADDR->txf[SM_conv], // Write address 
                lines_buf[0],             // read address 
                v_mode.H_len/1, //
                false             // Don't start yet
            );

        //контрольный канал для основного
        cfg_dma = dma_channel_get_default_config(dma_chan_ctrl);
        channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
        channel_config_set_chain_to(&cfg_dma, dma_chan);// chain to other channel
            
        channel_config_set_read_increment(&cfg_dma, true);
        channel_config_set_write_increment(&cfg_dma, false);
        channel_config_set_ring(&cfg_dma,false,2+N_LINE_BUF_log2);
  
    

        dma_channel_configure(
                dma_chan_ctrl,
                &cfg_dma,
                &dma_hw->ch[dma_chan].read_addr, // Write address 
                rd_addr_DMA_CTRL,             // read address 
                1, //
                false             // Don't start yet
            );

        //канал - конвертер палитры

        cfg_dma = dma_channel_get_default_config(dma_chan_pal_conv);
        channel_config_set_chain_to(&cfg_dma, dma_chan_pal_conv_ctrl);// chain to other channel

        int n_trans_data=1;
        switch (active_out)
        {
        case g_out_HDMI:
            channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
            n_trans_data=4;
            channel_config_set_read_increment(&cfg_dma, true);

            break;
        case g_out_VGA:
            channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_16);
            n_trans_data=1;
            channel_config_set_read_increment(&cfg_dma, false);

            break;
         default:
            break;
        }
       
  


            
        channel_config_set_write_increment(&cfg_dma, false);

        dreq=DREQ_PIO1_TX0+SM_video;
        if (PIO_VIDEO==pio0) dreq=DREQ_PIO0_TX0+SM_video;
        channel_config_set_dreq(&cfg_dma, dreq);
            
            dma_channel_configure(
                dma_chan_pal_conv,
                &cfg_dma,
                &PIO_VIDEO->txf[SM_video], // Write address 
                &conv_color[0],             // read address 
                n_trans_data, //
                false             // Don't start yet
            );

        //канал управления конвертером палитры

        cfg_dma = dma_channel_get_default_config(dma_chan_pal_conv_ctrl);
        channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
        channel_config_set_chain_to(&cfg_dma, dma_chan_pal_conv);// chain to other channel
            
        channel_config_set_read_increment(&cfg_dma, false);
        channel_config_set_write_increment(&cfg_dma, false);

        dreq=DREQ_PIO1_RX0+SM_conv;
        if (PIO_VIDEO_ADDR==pio0) dreq=DREQ_PIO0_RX0+SM_conv;

        channel_config_set_dreq(&cfg_dma, dreq);
            
        dma_channel_configure(
                dma_chan_pal_conv_ctrl,
                &cfg_dma,
                &dma_hw->ch[dma_chan_pal_conv].read_addr, // Write address 
                &PIO_VIDEO_ADDR->rxf[SM_conv],             // read address 
                1, //
                true             // start yet
            );
            
      

       
        dma_start_channel_mask((1u << dma_chan_ctrl)) ;

        int hz=50000;
    	if (!add_repeating_timer_us(1000000 / hz, hdmi_timer_callback, NULL, &hdmi_timer)) {
	    	return ;
    	}



};

void startVIDEO(){
 #if defined(TFT)
    if (VIDEO_OUTPUT==2) {init_TFT(); return;}
 #endif
	if (VIDEO_OUTPUT==0) {graphics_init(g_out_VGA);}
    if (VIDEO_OUTPUT==1) {graphics_init(g_out_HDMI);}
   

    graphics_set_buffer(g_gbuf);
    graphics_set_mode(g_mode_320x240x4bpp);
	
    for(int i=0;i<16;i++){
      int I=(i>>3)&1;
      int G=(i>>2)&1;
      int R=(i>>1)&1;
      int B=(i>>0)&1; 


      //uint32_t RGB=((R?I?255:170:000)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
uint32_t RGB=((R?I?255:170:0)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
   //    uint32_t RGB=((R?I?255:213:0)<<16)|((G?I?255:213:0)<<8)|((B?I?255:213:0)<<0);

   
     graphics_set_palette(i,RGB);
   
    }
}
