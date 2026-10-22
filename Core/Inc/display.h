#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

typedef enum {
    NO_OK = 0,
    OK = !NO_OK
} RetSt;

typedef struct {
    uint16_t front_min_row;
    uint16_t front_max_row;
    uint16_t rear_min_row;
    uint16_t rear_max_row;
} DisplayEraseBounds;

RetSt SetPixel(uint16_t col, uint16_t row, uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval);
RetSt DrawHorizontalLine(uint16_t init_col, uint16_t end_col, uint16_t row, uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval);
RetSt DrawVerticalLine(uint16_t col, uint16_t init_row, uint16_t end_row, uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval);
RetSt EraseData(uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval);
RetSt OptimizedEraseData(uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval);
RetSt ClearScreen(uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval);

extern volatile uint32_t display_erase_start_us;
extern volatile uint32_t display_erase_end_us;
extern volatile uint32_t display_erase_elapsed_us;
extern volatile uint32_t display_erase_elapsed_ms;
extern volatile DisplayEraseBounds display_prev_erase_bounds;

void DisplayInit(void);
void DisplayUpdate(void);

#endif /* DISPLAY_H */
