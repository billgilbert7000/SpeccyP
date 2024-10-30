#include "config.h"
#ifdef TFT

#include "tft_driver.h"
#include "inttypes.h"
#include "hardware/gpio.h"
#include "pico/platform.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"



// #define SCREEN_W (320)
// #define SCREEN_H (240)
#define DISP_CS_SELECT gpio_put(TFT_CS_PIN, 0)
#define DISP_CS_UNSELECT gpio_put(TFT_CS_PIN, 1)
#define DISP_RST_RESET gpio_put(TFT_RST_PIN, 0)
#define DISP_RST_WORK gpio_put(TFT_RST_PIN, 1)

// программа конвертации адреса

static uint16_t pio_program1_instructions2[] = {

    // 0x80a0, //  0: pull   block
    // 0xa027, //  1: mov    x, osr
    //         //     .wrap_target
    0x80a0, //  2: pull   block
    // 0x4061, //  3: in     null, 1
    0x40e8, //  4: in     osr, 8
    0x4036, //  5: in     x, 22
    0x8020, //  6: push   block
            //     .wrap

};

static const struct pio_program pio_program_conv = {
    .instructions = pio_program1_instructions2,
    .length = 4,
    .origin = -1,
};

static const uint16_t SPI_program_instructions[] = {
    //     .wrap_target
    0x80a0, //  0: pull   block           side 0
    0xe03f, //  1: set    x, 31           side 0
    0x6001, //  2: out    pins, 1         side 0
    0x1042, //  3: jmp    x--, 1          side 1
            //     .wrap
};

static const struct pio_program SPI_program = {
    .instructions = SPI_program_instructions,
    .length = 4,
    .origin = -1,
};

static const uint16_t SPI_8_program_instructions[] = {
    //     .wrap_target
    0x80a0, //  0: pull   block           side 0
    0xe027, //  1: set    x, 7           side 0
    0x6001, //  2: out    pins, 1         side 0
    0x1042, //  3: jmp    x--, 1          side 1
            //     .wrap
};

static const struct pio_program SPI_8_program = {
    .instructions = SPI_8_program_instructions,
    .length = 4,
    .origin = -1,
};

#define ILI9341_TFTWIDTH 320  ///< ILI9341 max TFT width
#define ILI9341_TFTHEIGHT 240 ///< ILI9341 max TFT height

#define ILI9341_NOP 0x00     ///< No-op register
#define ILI9341_SWRESET 0x01 ///< Software reset register
#define ILI9341_RDDID 0x04   ///< Read display identification information
#define ILI9341_RDDST 0x09   ///< Read Display Status

#define ILI9341_SLPIN 0x10  ///< Enter Sleep Mode
#define ILI9341_SLPOUT 0x11 ///< Sleep Out
#define ILI9341_PTLON 0x12  ///< Partial Mode ON
#define ILI9341_NORON 0x13  ///< Normal Display Mode ON

#define ILI9341_RDMODE 0x0A     ///< Read Display Power Mode
#define ILI9341_RDMADCTL 0x0B   ///< Read Display MADCTL
#define ILI9341_RDPIXFMT 0x0C   ///< Read Display Pixel Format
#define ILI9341_RDIMGFMT 0x0D   ///< Read Display Image Format
#define ILI9341_RDSELFDIAG 0x0F ///< Read Display Self-Diagnostic Result

#define ILI9341_INVOFF 0x20   ///< Display Inversion OFF
#define ILI9341_INVON 0x21    ///< Display Inversion ON
#define ILI9341_GAMMASET 0x26 ///< Gamma Set
#define ILI9341_DISPOFF 0x28  ///< Display OFF
#define ILI9341_DISPON 0x29   ///< Display ON

#define ILI9341_CASET 0x2A ///< Column Address Set
#define ILI9341_PASET 0x2B ///< Page Address Set
#define ILI9341_RAMWR 0x2C ///< Memory Write
#define ILI9341_RAMRD 0x2E ///< Memory Read

#define ILI9341_PTLAR 0x30    ///< Partial Area
#define ILI9341_MADCTL 0x36   ///< Memory Access Control
#define ILI9341_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define ILI9341_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set

#define ILI9341_FRMCTR1 0xB1 ///< Frame Rate Control (In Normal Mode/Full Colors)
#define ILI9341_FRMCTR2 0xB2 ///< Frame Rate Control (In Idle Mode/8 colors)
#define ILI9341_FRMCTR3 0xB3 ///< Frame Rate control (In Partial Mode/Full Colors)
#define ILI9341_INVCTR 0xB4  ///< Display Inversion Control
#define ILI9341_DFUNCTR 0xB6 ///< Display Function Control

#define ILI9341_PWCTR1 0xC0 ///< Power Control 1
#define ILI9341_PWCTR2 0xC1 ///< Power Control 2
#define ILI9341_PWCTR3 0xC2 ///< Power Control 3
#define ILI9341_PWCTR4 0xC3 ///< Power Control 4
#define ILI9341_PWCTR5 0xC4 ///< Power Control 5
#define ILI9341_VMCTR1 0xC5 ///< VCOM Control 1
#define ILI9341_VMCTR2 0xC7 ///< VCOM Control 2

#define ILI9341_RDID1 0xDA ///< Read ID 1
#define ILI9341_RDID2 0xDB ///< Read ID 2
#define ILI9341_RDID3 0xDC ///< Read ID 3
#define ILI9341_RDID4 0xDD ///< Read ID 4

#define ILI9341_GMCTRP1 0xE0  ///< Positive Gamma Correction
#define ILI9341_GMCTRN1 0xE1  ///< Negative Gamma Correction
#define ILI9341_INV_DISP 0x21 ///< 255, 130, 198

static uint32_t conv_arr[512];
static uint32_t *conv_2pix_4_to_16;

// extern uint8_t g_gbuf[];

static uint8_t *v_buf = g_gbuf;
//=============================================
static void Write_Data(uint8_t d)
{
  sleep_ms(1);
  pio_SPI_TFT->txf[sm_SPI_TFT] = d << 24;
  sleep_ms(1);
  return;

  uint8_t bit_n;
  for (bit_n = 0x80; bit_n;)
  {
    if (d & bit_n)
      gpio_put(TFT_DATA_PIN, 1);
    else
      gpio_put(TFT_DATA_PIN, 0);
    gpio_put(TFT_CLK_PIN, 1);
    // sleep_ms(1);
    bit_n >>= 1;
    gpio_put(TFT_CLK_PIN, 0);
    // sleep_ms(1);
  }
}
//==================================================
static void WriteCommand(uint8_t cmd)
{
  gpio_put(TFT_DC_PIN, 0);
  Write_Data(cmd);
  gpio_put(TFT_DC_PIN, 1);
}
//==================================================
static void setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h)
{
  uint16_t x2 = (x1 + w - 1),
           y2 = (y1 + h - 1);
  WriteCommand(ILI9341_CASET); // Column address set
  // sleep_ms(15);
  Write_Data(x1 >> 8);
  Write_Data(x1);
  Write_Data(x2 >> 8);
  Write_Data(x2);
  WriteCommand(ILI9341_PASET); // Row address set
  // sleep_ms(15);
  Write_Data(y1 >> 8);
  Write_Data(y1);
  Write_Data(y2 >> 8);
  Write_Data(y2);
  WriteCommand(ILI9341_RAMWR); // Write to RAM
                               // sleep_ms(15);
}
//=====================================================================================
static void outInit(uint gpio)
{
  gpio_init(gpio);
  gpio_set_dir(gpio, GPIO_OUT);
  gpio_put(gpio, 0);
  // gpio_pull_down(gpio);
}
//============================================================
volatile uint64_t pause_val = 1.1;
void draw_screen_TFT()
{
  uint8_t *v_buf8 = v_buf;
  for (int y = 0; y < ILI9341_TFTHEIGHT; y++)
    for (int x = 0; x < ILI9341_TFTWIDTH / 2; x++)
    {

      uint32_t c32 = conv_2pix_4_to_16[*v_buf8++];
      while (pio_sm_is_tx_fifo_full(pio_SPI_TFT, sm_SPI_TFT));
      pio_SPI_TFT->txf[sm_SPI_TFT] = c32;
    }
}
//===============================================================================================
#ifdef ILI9341
static void TFT_begin(void)
{
   DISP_CS_SELECT;
  DISP_RST_RESET;
  sleep_ms(200);
  DISP_RST_WORK;
  sleep_ms(20);

 
  // SOFTWARE RESET
  WriteCommand(0x01);
  sleep_ms(120); //

  // POWER CONTROL A
  WriteCommand(0xCB); 
  Write_Data(0x39);
  Write_Data(0x2C);
  Write_Data(0x00);
  Write_Data(0x34);
  Write_Data(0x02);

  // POWER CONTROL B
  WriteCommand(0xCF);
  Write_Data(0x00);
  Write_Data(0xC1);
  Write_Data(0x30);

  // DRIVER TIMING CONTROL A
  WriteCommand(0xE8);
  Write_Data(0x85);
  Write_Data(0x00);
  Write_Data(0x78);

  // DRIVER TIMING CONTROL B
  WriteCommand(0xEA);
  Write_Data(0x00);
  Write_Data(0x00);

  // POWER ON SEQUENCE CONTROL
  WriteCommand(0xED);
  Write_Data(0x64);
  Write_Data(0x03);
  Write_Data(0x12);
  Write_Data(0x81);

  // PUMP RATIO CONTROL
  WriteCommand(0xF7);
  Write_Data(0x20);

  // POWER CONTROL,VRH[5:0]
  WriteCommand(0xC0);
  Write_Data(0x23);

  // POWER CONTROL,SAP[2:0];BT[3:0]
  WriteCommand(0xC1);
  Write_Data(0x10);

  // VCM CONTROL
  WriteCommand(0xC5);
  Write_Data(0x3E);
  Write_Data(0x28);

  // VCM CONTROL 2
  WriteCommand(0xC7);
  Write_Data(0x86);
 
  // PIXEL FORMAT
  WriteCommand(0x3A);
  Write_Data(0x55);// Set colour mode to 16 bit

  // FRAME RATIO CONTROL, STANDARD RGB COLOR
  WriteCommand(0xB1);
  Write_Data(0x00);
//  Write_Data(0x18); //79Гц
//Write_Data(0b11011); //70Гц default
Write_Data(0b10011); //100Гц 
//Write_Data(0b11111); //61Гц 

  // DISPLAY FUNCTION CONTROL
  WriteCommand(0xB6);
  Write_Data(0x08);
  Write_Data(0x82);
  Write_Data(0x27);

  // 3GAMMA FUNCTION DISABLE
 // WriteCommand(0xF2);
 // Write_Data(0x00);

  // GAMMA CURVE SELECTED
  WriteCommand(0x26);
  Write_Data(0x01);

  // POSITIVE GAMMA CORRECTION
  WriteCommand(0xE0);
  Write_Data(0x0F);
  Write_Data(0x31);
  Write_Data(0x2B);
  Write_Data(0x0C);
  Write_Data(0x0E);
  Write_Data(0x08);
  Write_Data(0x4E);
  Write_Data(0xF1);
  Write_Data(0x37);
  Write_Data(0x07);
  Write_Data(0x10);
  Write_Data(0x03);
  Write_Data(0x0E);
  Write_Data(0x09);
  Write_Data(0x00);

  // NEGATIVE GAMMA CORRECTION
  WriteCommand(0xE1);
  Write_Data(0x00);
  Write_Data(0x0E);
  Write_Data(0x14);
  Write_Data(0x03);
  Write_Data(0x11);
  Write_Data(0x07);
  Write_Data(0x31);
  Write_Data(0xC1);
  Write_Data(0x48);
  Write_Data(0x08);
  Write_Data(0x0F);
  Write_Data(0x0C);
  Write_Data(0x31);
  Write_Data(0x36);
  Write_Data(0x0F);

  WriteCommand(TFT_INV); // инверсия
  /*
  cmd(0x36); // Memory data access control:
  // MY MX MV ML RGB MH - -
  // data(0x00); // Normal: Top to Bottom; Left to Right; RGB
  // data(0x80); // Y-Mirror: Bottom to top; Left to Right; RGB
  // data(0x40); // X-Mirror: Top to Bottom; Right to Left; RGB
  // data(0xc0); // X-Mirror,Y-Mirror: Bottom to top; Right to left; RGB
  // data(0x20); // X-Y Exchange: X and Y changed positions
  // data(0xA0); // X-Y Exchange,Y-Mirror
  // data(0x60); // X-Y Exchange,X-Mirror
  // data(0xE0); // X-Y Exchange,X-Mirror,Y-Mirror
  */
  // MEMORY ACCESS CONTROL
  WriteCommand(0x36);
  Write_Data(MADCTL); // ST7789 мой 0x40 другие 0x60
 

 // CASET: column addresses
    WriteCommand(0x2a);   
    Write_Data(0x00);
    Write_Data(0x00);
    Write_Data(SCREEN_W >> 8);  
    Write_Data(SCREEN_W & 0xff);  

 // RASET: row addresses
    WriteCommand(0x2b);   
    Write_Data(0x00);
    Write_Data(0x00);
    Write_Data(SCREEN_H  >> 8);  
    Write_Data(SCREEN_H  & 0xff);  
 

  // EXIT SLEEP
  WriteCommand(0x11);
  sleep_ms(120);

// Normal display on, then 10 ms delay
// WriteCommand(0x13);


  // TURN ON DISPLAY
   WriteCommand(0x29);
}





#else  // ST7789
static void TFT_begin(void)
{
  DISP_CS_SELECT;
  DISP_RST_RESET;
  sleep_ms(200);
  DISP_RST_WORK;
  sleep_ms(20);

  // SOFTWARE RESET
  WriteCommand(0x01);
  sleep_ms(120); //

  // PIXEL FORMAT
  WriteCommand(0x3A);
  Write_Data(0x55);// Set colour mode to 16 bit

   // FRAME RATIO CONTROL, STANDARD RGB COLOR
  WriteCommand(0xC6);
 // Write_Data(0x0F); // 60Гц default
 // Write_Data(0x09); // 75Гц
  Write_Data(0x15); // 50Гц
 // Write_Data(0x05); // 90Гц

WriteCommand(TFT_INV); // инверсия
  /*
  cmd(0x36); // Memory data access control:
  // MY MX MV ML RGB MH - -
  // data(0x00); // Normal: Top to Bottom; Left to Right; RGB
  // data(0x80); // Y-Mirror: Bottom to top; Left to Right; RGB
  // data(0x40); // X-Mirror: Top to Bottom; Right to Left; RGB
  // data(0xc0); // X-Mirror,Y-Mirror: Bottom to top; Right to left; RGB
  // data(0x20); // X-Y Exchange: X and Y changed positions
  // data(0xA0); // X-Y Exchange,Y-Mirror
  // data(0x60); // X-Y Exchange,X-Mirror
  // data(0xE0); // X-Y Exchange,X-Mirror,Y-Mirror
  */
  // MEMORY ACCESS CONTROL
  WriteCommand(0x36);
 //Write_Data(MADCTL); // ST7789 мой 0x40 другие 0x60
 Write_Data(0xA0); //60
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
/* // CASET: column addresses
    WriteCommand(0x2a);   
    Write_Data(0x00);
    Write_Data(0x00);
    Write_Data(SCREEN_WIDTH >> 8);  
    Write_Data(SCREEN_WIDTH & 0xff);  

 // RASET: row addresses
    WriteCommand(0x2b);   
    Write_Data(0x00);
    Write_Data(0x00);
    Write_Data(SCREEN_HEIGHT >> 8);  
    Write_Data(SCREEN_HEIGHT & 0xff);  
 */

  // EXIT SLEEP
  WriteCommand(0x11);
  sleep_ms(120);

// Normal display on, then 10 ms delay
// WriteCommand(0x13);


  // TURN ON DISPLAY
   WriteCommand(0x29);
}

#endif
 
//=============================================================================================
const uint16_t pallete16b[] =
    {
        // светло серый
        //  0b0000000000000000,
        //  0b0000000000011100,
        //  0b0000011100100000,
        //  0b0000011100111100,
        //  0b1110000000000000,
        //  0b1110000000011100,
        //  0b1110011100100000,
        //  0b1110011100111100,

        // средне серый
        //  0b0000000000000000,
        //  0b0000000000011000,
        //  0b0000011000100000,
        //  0b0000011000111000,
        //  0b1100000000000000,
        //  0b1100000000011000,
        //  0b1100011000100000,
        //  0b1100011000111000,

        // средне темно серый
        0b0000000000000000,
        0b0000000000010100,
        0b0000010100100000,
        0b0000010100110100,
        0b1010000000000000,
        0b1010000000010100,
        0b1010010100100000,
        0b1010010100110100,

        // тёмно серый
        // 0b0000000000000000,
        // 0b0000000000010000,
        // 0b0000010000100000,
        // 0b0000010000110000,
        // 0b1000000000000000,
        // 0b1000000000010000,
        // 0b1000010000100000,
        // 0b1000010000110000,

        0b0000000000000000,
        0b0000000000011111,
        0b0000011111100000,
        0b0000011111111111,
        0b1111100000000000,
        0b1111100000011111,
        0b1111111111100000,
        0b1111111111111111};

static void pio_set_x(PIO pio, int sm, uint32_t v)
{
  uint instr_shift = pio_encode_in(pio_x, 4);
  uint instr_mov = pio_encode_mov(pio_x, pio_isr);
  for (int i = 0; i < 8; i++)
  {
    const uint32_t nibble = (v >> (i * 4)) & 0xf;
    pio_sm_exec(pio, sm, pio_encode_set(pio_x, nibble));
    pio_sm_exec(pio, sm, instr_shift);
  }
  pio_sm_exec(pio, sm, instr_mov);
}
//=============================================================
static void PWM_init_tft(uint pinN)
{
  gpio_set_function(pinN, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(pinN);

  pwm_config c_pwm = pwm_get_default_config();
 // pwm_config_set_clkdiv(&c_pwm, clock_get_hz(clk_sys) / (4.0 * 500));
 pwm_config_set_clkdiv(&c_pwm, clock_get_hz(clk_sys) / (4.0 * 1000));// частота шима подсветки
  pwm_config_set_wrap(&c_pwm, 100); // MAX PWM value
  pwm_init(slice_num, &c_pwm, true);
}
//=============================================================
void init_TFT()
{
 PWM_init_tft(TFT_LED_PIN);
  pwm_set_gpio_level(TFT_LED_PIN, 0); // 0% яркости
//outInit(TFT_LED_PIN);
//gpio_put(TFT_LED_PIN, 1);
  // настройка SM для конвертации
  uint offset0 = pio_add_program(pio_SPI_TFT_conv, &pio_program_conv);
  uint sm_c = sm_SPI_TFT_conv;
  conv_2pix_4_to_16 = ((uint32_t)conv_arr & 0xfffffc00) + 0x400;
  pio_set_x(pio_SPI_TFT_conv, sm_c, ((uint32_t)conv_2pix_4_to_16 >> 10));

  pio_sm_config c_c = pio_get_default_sm_config();
  sm_config_set_wrap(&c_c, offset0 + 0, offset0 + (pio_program_conv.length - 1));
  sm_config_set_in_shift(&c_c, true, false, 32);

  pio_sm_init(pio_SPI_TFT_conv, sm_c, offset0, &c_c);
  pio_sm_set_enabled(pio_SPI_TFT_conv, sm_c, true);

  for (int i = 0; i < 256; i++)
  {
    uint8_t ci = i;
    uint8_t cl = (ci & 0xf0) >> 4;
    uint8_t ch = ci & 0xf;

    // убрать ярко-чёрный
    if (ch == 0x08)
      ch = 0;
    if (cl == 0x08)
      cl = 0;

    ch = (ch & 0b1001) | ((ch & 0b0100) >> 1) | ((ch & 0b0010) << 1);
    cl = (cl & 0b1001) | ((cl & 0b0100) >> 1) | ((cl & 0b0010) << 1);

    // конвертация в 6 битный формат цвета
    uint16_t ch16 = pallete16b[ch & 0xf];
    uint16_t cl16 = pallete16b[cl & 0xf];
    conv_2pix_4_to_16[i] = ((ch16) << 16) | (cl16);
  }

  outInit(TFT_CLK_PIN);
  outInit(TFT_DATA_PIN);
  outInit(TFT_RST_PIN);
  outInit(TFT_DC_PIN);
  outInit(TFT_CS_PIN);
  // outInit(TFT_LED_PIN);

  DISP_RST_RESET;
  DISP_CS_UNSELECT;

 

  // SM для SPI
  //  DISP_CS_UNSELECT;

  // инициализируем на 8 бит программу
  uint offset = pio_add_program(pio_SPI_TFT, &SPI_8_program);
  pio_sm_config c = pio_get_default_sm_config();
  sm_config_set_wrap(&c, offset, offset + (SPI_8_program.length - 1));
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

  // настройка side set
  sm_config_set_sideset_pins(&c, TFT_CLK_PIN);
  sm_config_set_sideset(&c, 1, false, false);
  for (int i = 0; i < 1; i++)
  {
    pio_gpio_init(pio_SPI_TFT, TFT_CLK_PIN + i);
  }

  pio_sm_set_pins_with_mask(pio_SPI_TFT, sm_SPI_TFT, 1u << TFT_CLK_PIN, 1u << TFT_CLK_PIN);
  pio_sm_set_pindirs_with_mask(pio_SPI_TFT, sm_SPI_TFT, 1u << TFT_CLK_PIN, 1u << TFT_CLK_PIN);

  pio_gpio_init(pio_SPI_TFT, TFT_DATA_PIN); // резервируем под выход PIO

  pio_sm_set_consecutive_pindirs(pio_SPI_TFT, sm_SPI_TFT, TFT_DATA_PIN, 1, true); // конфигурация пинов на выход

  sm_config_set_out_shift(&c, false, false, 32);
  sm_config_set_out_pins(&c, TFT_DATA_PIN, 1);

// Позволяет работать дисплею st7789 с пином CS притянутым к GND
 gpio_set_outover(TFT_CLK_PIN, GPIO_OVERRIDE_INVERT );
 //gpio_set_outover(TFT_CLK_PIN,  GPIO_OVERRIDE_NORMAL );




  pio_sm_init(pio_SPI_TFT, sm_SPI_TFT, offset, &c);
  pio_sm_set_enabled(pio_SPI_TFT, sm_SPI_TFT, true);

  // sleep_ms(3000);
  //  float fdiv=(clock_get_hz(clk_sys)/252000000.0);//частота SPI*2
  //    float fdiv=(clock_get_hz(clk_sys)/ 192000000.0);//частота SPI*2
 //  uint32_t fdiv32=(uint32_t) (fdiv * (1 << 16));
   //  fdiv32=fdiv32&0xfffff000;//округление делителя
   //printf("fdiv32:%d #%x\n",fdiv32,fdiv32);
  //  fdiv32=131072  //fdiv32:131072 #20000
  //  315Mhz fdiv32:155648 #26000
  // uint32_t  fdiv32=0x40000;
  //  pio_SPI_TFT->sm[sm_SPI_TFT].clkdiv=fdiv32; //делитель для конкретной sm
  //fdiv32:163840 #28000  126 MHz
  pio_SPI_TFT->sm[sm_SPI_TFT].clkdiv =SERIAL_CLK_TFT;//0x28000;// 0x40000; // делитель для конкретной sm 

  sleep_ms(20);

  TFT_begin();

  setAddrWindow(0, 0, ILI9341_TFTWIDTH, ILI9341_TFTHEIGHT);
  // setAddrWindow(0,0,ILI9341_TFTHEIGHT,ILI9341_TFTWIDTH);

  // подменяем на программу 32 бита spi
  sleep_ms(1);
  pio_sm_set_enabled(pio_SPI_TFT, sm_SPI_TFT, false);
  pio_SPI_TFT->instr_mem[offset + 1] = SPI_program_instructions[1];
  pio_sm_set_enabled(pio_SPI_TFT, sm_SPI_TFT, true);

  // канал для передачи даных экранного буфера  в конвертор и канал для его управления
  int dma_0 = dma_claim_unused_channel(true);
  int dma_ctrl0 = dma_claim_unused_channel(true);

  dma_channel_config c_conv = dma_channel_get_default_config(dma_0);

  channel_config_set_transfer_data_size(&c_conv, DMA_SIZE_8);

  channel_config_set_read_increment(&c_conv, true);
  channel_config_set_write_increment(&c_conv, false);

  uint dreq0 = DREQ_PIO1_TX0 + sm_SPI_TFT_conv;
  if (pio_SPI_TFT_conv == pio0)
    dreq0 = DREQ_PIO0_TX0 + sm_SPI_TFT_conv;

  channel_config_set_dreq(&c_conv, dreq0);

  channel_config_set_chain_to(&c_conv, dma_ctrl0); // chain to other channel

  dma_channel_configure(
      dma_0,
      &c_conv,
      &pio_SPI_TFT_conv->txf[sm_SPI_TFT_conv], // Write address
      v_buf,                                   // read address
      SCREEN_H * SCREEN_W / 2,                 //
      false                                    // Don't start yet
  );

  static uint32_t addr_v_buf;
  addr_v_buf = (uint32_t)v_buf;

  dma_channel_config c_contrl = dma_channel_get_default_config(dma_ctrl0);

  channel_config_set_transfer_data_size(&c_contrl, DMA_SIZE_32);

  channel_config_set_read_increment(&c_contrl, false);
  channel_config_set_write_increment(&c_contrl, false);

  channel_config_set_chain_to(&c_contrl, dma_0); // chain to other channel

  dma_channel_configure(
      dma_ctrl0,
      &c_contrl,
      &dma_hw->ch[dma_0].read_addr, // Write address
      &addr_v_buf,                  // read address
      1,                            //
      false                         // Don't start yet
  );

  // канал для передачи даных экранного буфера  в конвертор и канал для его управления
  int dma_addr = dma_claim_unused_channel(true);
  int dma_SPI = dma_claim_unused_channel(true);

  dma_channel_config c_addr = dma_channel_get_default_config(dma_addr);

  channel_config_set_transfer_data_size(&c_addr, DMA_SIZE_32);

  channel_config_set_read_increment(&c_addr, false);
  channel_config_set_write_increment(&c_addr, false);

  uint dreq1 = DREQ_PIO1_RX0 + sm_SPI_TFT_conv;
  if (pio_SPI_TFT_conv == pio0)
    dreq1 = DREQ_PIO0_RX0 + sm_SPI_TFT_conv;
  channel_config_set_dreq(&c_addr, dreq1);

  channel_config_set_chain_to(&c_addr, dma_SPI); // chain to other channel

  dma_channel_configure(
      dma_addr,
      &c_addr,
      &dma_hw->ch[dma_SPI].read_addr,          // Write address
      &pio_SPI_TFT_conv->rxf[sm_SPI_TFT_conv], // read address
      1,                                       //
      false                                    // Don't start yet
  );

  dma_channel_config c_SPI = dma_channel_get_default_config(dma_SPI);

  channel_config_set_transfer_data_size(&c_SPI, DMA_SIZE_32);

  channel_config_set_read_increment(&c_SPI, false);
  channel_config_set_write_increment(&c_SPI, false);

  uint dreq2 = DREQ_PIO1_TX0 + sm_SPI_TFT;
  if (pio_SPI_TFT == pio0)
    dreq2 = DREQ_PIO0_TX0 + sm_SPI_TFT;
  channel_config_set_dreq(&c_SPI, dreq2);

  channel_config_set_chain_to(&c_SPI, dma_addr); // chain to other channel

  dma_channel_configure(
      dma_SPI,
      &c_SPI,
      &pio_SPI_TFT->txf[sm_SPI_TFT], // Write address
      conv_2pix_4_to_16,             // read address
      1,                             //
      false                          // Don't start yet
  );

  dma_start_channel_mask((1u << dma_0));
  dma_start_channel_mask((1u << dma_addr));
  pwm_set_gpio_level(TFT_LED_PIN, conf.tft_bright); // 70% яркости
}
#endif