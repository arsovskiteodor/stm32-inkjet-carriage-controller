#include "display.h"
#include "stm32f429i_discovery_lcd.h"

extern float FrontSensorProc_ORG[300];
extern float RearSensorProc_ORG[300];
extern SPI_HandleTypeDef hspi5;

extern LTDC_HandleTypeDef LtdcHandler;

volatile uint32_t display_erase_start_us = 0;
volatile uint32_t display_erase_end_us = 0;
volatile uint32_t display_erase_elapsed_us = 0;
volatile uint32_t display_erase_elapsed_ms = 0;

#define SCREEN_COLS 320
#define SCREEN_ROWS 240
#define BYTES_PP 2

#define FB_LAYER1 0xD0000000U
#define FB_LAYER2 (0xD0000000U + SCREEN_ROWS * SCREEN_COLS * BYTES_PP)

#define WIN_BORDER_L 9
#define WIN_BORDER_R 310
#define WIN_USEFUL_L 10
#define WIN_USEFUL_R 309

#define WIN1_TOP 5
#define WIN1_BOT 107
#define WIN1_DAT_TOP 6
#define WIN1_DAT_BOT 106

#define WIN2_TOP 120
#define WIN2_BOT 222
#define WIN2_DAT_TOP 121
#define WIN2_DAT_BOT 221

#define COL_ALPHA_OPAQUE 0xFF
#define COL_ALPHA_TRANSP 0x00

#define BLK_R 0x00
#define BLK_G 0x00
#define BLK_B 0x00
#define WHT_R 0xFF
#define WHT_G 0xFF
#define WHT_B 0xFF
#define FRN_R 0xFF
#define FRN_G 0x00
#define FRN_B 0x00
#define REA_R 0x00
#define REA_G 0x00
#define REA_B 0xFF

volatile DisplayEraseBounds display_prev_erase_bounds = {
    WIN1_DAT_TOP,
    WIN1_DAT_BOT,
    WIN2_DAT_TOP,
    WIN2_DAT_BOT
};

static uint32_t s_fb_base = FB_LAYER2;

static uint16_t *pixel_addr(uint16_t col, uint16_t row)
{
    return (uint16_t *)(s_fb_base + (col * SCREEN_ROWS + row) * BYTES_PP);
}

static uint16_t pack_argb4444(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((a & 0xF0) << 8 | (r & 0xF0) << 4 | (g & 0xF0) | b >> 4);
}

static uint16_t mm_to_pix(float v_mm)
{
    int pix = (int)(v_mm * 50.0f + 0.5f);  
    if (pix < 0) {
        pix = 0;
    }
    if (pix > 100) {
        pix = 100;
    }
    return (uint16_t)(100 - pix);          
}

void BSP_SDRAM_MspInit(SDRAM_HandleTypeDef *hsdram, void *Params)
{
    (void)hsdram;
    (void)Params;
}

void LCD_IO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin = LCD_NCS_PIN;          
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_NCS_GPIO_PORT, &gpio);

    __HAL_RCC_GPIOD_CLK_ENABLE();
    gpio.Pin = LCD_WRX_PIN;            
    HAL_GPIO_Init(LCD_WRX_GPIO_PORT, &gpio);

    LCD_CS_HIGH();
    LCD_WRX_HIGH();
}

void LCD_IO_WriteReg(uint8_t Reg)
{
    LCD_WRX_LOW();    
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi5, &Reg, 1, SPIx_TIMEOUT_MAX);
    while (HAL_SPI_GetState(&hspi5) != HAL_SPI_STATE_READY);
    LCD_CS_HIGH();
}

void LCD_IO_WriteData(uint16_t RegValue)
{
    uint8_t data = (uint8_t)RegValue;
    LCD_WRX_HIGH();  
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi5, &data, 1, SPIx_TIMEOUT_MAX);
    while (HAL_SPI_GetState(&hspi5) != HAL_SPI_STATE_READY);
    LCD_CS_HIGH();
}

uint32_t LCD_IO_ReadData(uint16_t RegValue, uint8_t ReadSize)
{
    (void)RegValue; (void)ReadSize;
    return 0;
}

void LCD_Delay(uint32_t delay)
{
    HAL_Delay(delay);
}

RetSt SetPixel(uint16_t col, uint16_t row, uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval)
{
    if (col >= SCREEN_COLS || row >= SCREEN_ROWS) {
        return NO_OK;
    }
    *pixel_addr(col, row) = pack_argb4444(alpha, Rval, Gval, Bval);
    return OK;
}

RetSt DrawHorizontalLine(uint16_t init_col, uint16_t end_col, uint16_t row, uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval)
{
    if (row >= SCREEN_ROWS) {
        return NO_OK;
    }
    if (init_col > end_col) { 
        uint16_t t = init_col; init_col = end_col; end_col = t; 
    }
    RetSt st = OK;
    for (uint16_t c = init_col; c <= end_col; c++) {
        if (SetPixel(c, row, alpha, Rval, Gval, Bval) != OK) {
            st = NO_OK;
        }
    }
    return st;
}

RetSt DrawVerticalLine(uint16_t col, uint16_t init_row, uint16_t end_row, uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval)
{
    if (col >= SCREEN_COLS) {
        return NO_OK;
    }
    if (init_row > end_row) { 
        uint16_t t = init_row; init_row = end_row; end_row = t; 
    }
    RetSt st = OK;
    for (uint16_t r = init_row; r <= end_row; r++) {
        if (SetPixel(col, r, alpha, Rval, Gval, Bval) != OK) {
            st = NO_OK;
        }
    }
    return st;
}

RetSt EraseData(uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval)
{
    for (uint16_t col = WIN_USEFUL_L; col <= WIN_USEFUL_R; col++) {
        for (uint16_t row = WIN1_DAT_TOP; row <= WIN1_DAT_BOT; row++) {
            SetPixel(col, row, alpha, Rval, Gval, Bval);
        }
        for (uint16_t row = WIN2_DAT_TOP; row <= WIN2_DAT_BOT; row++) {
            SetPixel(col, row, alpha, Rval, Gval, Bval);
        }
    }
    return OK;
}

RetSt OptimizedEraseData(uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval)
{
    for (uint16_t col = WIN_USEFUL_L; col <= WIN_USEFUL_R; col++) {
        for (uint16_t row = display_prev_erase_bounds.front_min_row; row <= display_prev_erase_bounds.front_max_row; row++) {
            SetPixel(col, row, alpha, Rval, Gval, Bval);
        }
        for (uint16_t row = display_prev_erase_bounds.rear_min_row; row <= display_prev_erase_bounds.rear_max_row; row++) {
            SetPixel(col, row, alpha, Rval, Gval, Bval);
        }
    }
    return OK;
}

RetSt ClearScreen(uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval)
{
    for (uint16_t col = 0; col < SCREEN_COLS; col++) {
        for (uint16_t row = 0; row < SCREEN_ROWS; row++) {
            SetPixel(col, row, alpha, Rval, Gval, Bval);
        }
    }
    return OK;
}

void DisplayInit(void)
{
    BSP_LCD_Init();

    LCD_LayerCfgTypeDef cfg = {0};
    cfg.WindowX0 = 0;
    cfg.WindowX1 = SCREEN_ROWS;
    cfg.WindowY0 = 0;
    cfg.WindowY1 = SCREEN_COLS;
    cfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB4444;
    cfg.Alpha = 255;
    cfg.Alpha0 = 0;
    cfg.Backcolor.Red = 0;
    cfg.Backcolor.Green = 0;
    cfg.Backcolor.Blue = 0;
    cfg.ImageWidth = SCREEN_ROWS;
    cfg.ImageHeight = SCREEN_COLS;

    cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
    cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
    cfg.FBStartAdress = FB_LAYER1;
    HAL_LTDC_ConfigLayer(&LtdcHandler, &cfg, 0);

    cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    cfg.FBStartAdress = FB_LAYER2;
    HAL_LTDC_ConfigLayer(&LtdcHandler, &cfg, 1);

    s_fb_base = FB_LAYER2;
    ClearScreen(COL_ALPHA_TRANSP, 0, 0, 0);

    s_fb_base = FB_LAYER1;

    ClearScreen(COL_ALPHA_OPAQUE, WHT_R, WHT_G, WHT_B);

    DrawHorizontalLine(WIN_BORDER_L, WIN_BORDER_R, WIN1_TOP, COL_ALPHA_OPAQUE, BLK_R, BLK_G, BLK_B);
    DrawHorizontalLine(WIN_BORDER_L, WIN_BORDER_R, WIN1_BOT, COL_ALPHA_OPAQUE, BLK_R, BLK_G, BLK_B);
    DrawVerticalLine(WIN_BORDER_L, WIN1_TOP, WIN1_BOT, COL_ALPHA_OPAQUE, BLK_R, BLK_G, BLK_B);
    DrawVerticalLine(WIN_BORDER_R, WIN1_TOP, WIN1_BOT, COL_ALPHA_OPAQUE, BLK_R, BLK_G, BLK_B);

    DrawHorizontalLine(WIN_BORDER_L, WIN_BORDER_R, WIN2_TOP, COL_ALPHA_OPAQUE, BLK_R, BLK_G, BLK_B);
    DrawHorizontalLine(WIN_BORDER_L, WIN_BORDER_R, WIN2_BOT, COL_ALPHA_OPAQUE, BLK_R, BLK_G, BLK_B);
    DrawVerticalLine(WIN_BORDER_L, WIN2_TOP, WIN2_BOT, COL_ALPHA_OPAQUE, BLK_R, BLK_G, BLK_B);
    DrawVerticalLine(WIN_BORDER_R, WIN2_TOP, WIN2_BOT, COL_ALPHA_OPAQUE, BLK_R, BLK_G, BLK_B);

    for (uint16_t r = 226; r <= 236; r++) {
        DrawHorizontalLine(80, 149, r, COL_ALPHA_OPAQUE, REA_R, REA_G, REA_B);
        DrawHorizontalLine(170, 239, r, COL_ALPHA_OPAQUE, FRN_R, FRN_G, FRN_B);
    }

    s_fb_base = FB_LAYER2;      

    BSP_LCD_SetLayerVisible(0, ENABLE);
    BSP_LCD_SetLayerVisible(1, ENABLE);
    BSP_LCD_DisplayOn();
}

void DisplayUpdate(void)
{
    display_erase_start_us = TIM2->CNT;
    OptimizedEraseData(COL_ALPHA_OPAQUE, WHT_R, WHT_G, WHT_B);
    display_erase_end_us = TIM2->CNT;
    display_erase_elapsed_us = display_erase_end_us - display_erase_start_us;
    display_erase_elapsed_ms = display_erase_elapsed_us / 1000;

    uint16_t front_min_row = WIN1_DAT_BOT;
    uint16_t front_max_row = WIN1_DAT_TOP;
    for (uint16_t i = 0; i < 300; i++) {
        uint16_t pix_row = mm_to_pix(FrontSensorProc_ORG[i]);
        uint16_t screen_row = WIN1_DAT_TOP + pix_row;  
        SetPixel(WIN_USEFUL_R - i, screen_row, COL_ALPHA_OPAQUE, FRN_R, FRN_G, FRN_B);
        if (screen_row < front_min_row) {
            front_min_row = screen_row;
        }
        if (screen_row > front_max_row) {
            front_max_row = screen_row;
        }
    }

    uint16_t rear_min_row = WIN2_DAT_BOT;
    uint16_t rear_max_row = WIN2_DAT_TOP;
    for (uint16_t i = 0; i < 300; i++) {
        uint16_t pix_row = mm_to_pix(RearSensorProc_ORG[i]);
        uint16_t screen_row = WIN2_DAT_TOP + pix_row;  
        SetPixel(WIN_USEFUL_R - i, screen_row, COL_ALPHA_OPAQUE, REA_R, REA_G, REA_B);
        if (screen_row < rear_min_row) {
            rear_min_row = screen_row;
        }
        if (screen_row > rear_max_row) {
            rear_max_row = screen_row;
        }
    }

    display_prev_erase_bounds.front_min_row = front_min_row;
    display_prev_erase_bounds.front_max_row = front_max_row;
    display_prev_erase_bounds.rear_min_row = rear_min_row;
    display_prev_erase_bounds.rear_max_row = rear_max_row;
}
