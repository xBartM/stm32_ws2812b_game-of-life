#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
//#include <unistd.h>
#include <string.h> // memcpy

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
//#include <zephyr/drivers/spi.h>
//#include <zephyr/sys/util.h>

// Zephyr specific
#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define DELAY_TIME K_MSEC(100*5)
#define FAST_DELAY K_MSEC(100*1)
#define NONE_DELAY K_MSEC(1)

// Check if char is 8 bits long
#if CHAR_BIT != 8 
#error "CHAR_BIT != 8"
#endif

// Constants for the game
#define COLS 32
#define ROWS 16

#if COLS % 16 != 0
#error "bUpdatePixels() not sufficient for partially compressed bytes"
#endif

#if COLS*ROWS > 512
#error "translateAddress() not sufficient for more than two panels; easily expandable to an arbitrary number of panels"
#endif

#if STRIP_NUM_PIXELS < COLS*ROWS
#error "Game size bigger than  led chain length"
#endif

#warning "TODO: bInitGrid() doesn't use srand() so it's always the same"

// Bit definitions
#define CELL0 0x80 // 0b10000000
#define CELL1 0x40 // 0b01000000
#define CELL2 0x20 // 0b00100000
#define CELL3 0x10 // 0b00010000
#define CELL4 0x08 // 0b00001000
#define CELL5 0x04 // 0b00000100
#define CELL6 0x02 // 0b00000010
#define CELL7 0x01 // 0b00000001

// led_rgb struct helper
#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

// Function prototypes
void bInitGrid(uint8_t (*)[ROWS]);
void bUpdatePixels(uint8_t (*)[ROWS]);
void bUpdateGrid(uint8_t (*)[ROWS], uint8_t (*)[ROWS]);
uint8_t bCountNeighbours(uint8_t (*)[ROWS], uint16_t, uint8_t);
uint8_t bGetCellStatus(uint8_t (*)[ROWS], uint16_t, uint8_t);
void bSetCellStatus(uint8_t (*)[ROWS], uint16_t, uint8_t, uint8_t);
uint16_t translateAddress(uint16_t, uint8_t);

// Testing
void testPanel();
void testTranslateAddress();

// Global variables
static const struct led_rgb colors[] = {
	RGB(0x08, 0x00, 0x00), /* red */
	RGB(0x00, 0x08, 0x00), /* green */
	RGB(0x00, 0x00, 0x08), /* blue */
	RGB(0x00, 0x00, 0x00)  /* off */
};

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[STRIP_NUM_PIXELS]; // initialized as 0s
static uint8_t thebuffer1[COLS][ROWS];
static uint8_t thebuffer2[COLS][ROWS];

int main() {

    if (!(device_is_ready(strip)))
        return 0;
    
    // Initialize memory
    uint8_t (*workingbuffer1)[ROWS] = thebuffer1;
    uint8_t (*workingbuffer2)[ROWS] = thebuffer2;
    
    // Testing
    //testPanel();
    //testTranslateAddress();

    // Initialize the grid with random values
    bInitGrid(workingbuffer1);

    while (1) { // main loop
        // Print initial state
        k_sleep(DELAY_TIME);
        bUpdatePixels(workingbuffer1);
        led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
        //bPrintGrid(workingbuffer1);

        // Update the grid to the next generation
        bUpdateGrid(workingbuffer1, workingbuffer2);

        // Print the updated grid
        k_sleep(DELAY_TIME);
        bUpdatePixels(workingbuffer2);
        led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
        //bPrintGrid(workingbuffer2);
        
        // Update the grid to the next generation
        bUpdateGrid(workingbuffer2, workingbuffer1);

    }
    
    return 0;
}

// Initialize the grid with random values
void bInitGrid(uint8_t (*bgrid1)[ROWS]) { 
    int bytecols = COLS/8; // int division truncates (rounds towards 0)
    if (COLS % 8 != 0) ++bytecols;
    
    for (int col = 0; col < bytecols; ++col)
        for (int row = 0; row < ROWS; ++row)
            bgrid1[col][row] = rand() % 0xFF; // 0b11111111; // Randomly initialize BITS (cells) to either 0 or 1
}

void bUpdatePixels(uint8_t (*bgrid)[ROWS]) {
    const uint8_t fullbytecols = COLS/8;
    
    for (uint8_t row = 0; row < ROWS; ++row) {
        for (uint8_t col = 0; col < fullbytecols; ++col) {
            for (uint8_t i = 0; i < 8; ++i) {
                if ((bgrid[col][row] << i) & CELL0) // move to the next bit and check the highest 
                    memcpy(&pixels[translateAddress(col*8+i, row)], &colors[0], sizeof(struct led_rgb)); // if alive - set alive
                else // if dead
                    memcpy(&pixels[translateAddress(col*8+i, row)], &colors[3], sizeof(struct led_rgb)); // set dead
            }
        }
    }
}

void bUpdateGrid(uint8_t (*currbgrid)[ROWS], uint8_t (*nextbgrid)[ROWS]) {
    uint8_t liveNeighbors = 0;
    for (uint8_t row = 0; row < ROWS; ++row) {
        for (uint16_t col = 0; col < COLS; ++col) {
            liveNeighbors = bCountNeighbours(currbgrid, col, row);

            if (bGetCellStatus(currbgrid, col, row)) { // if alive
                if (liveNeighbors == 2 || liveNeighbors == 3) bSetCellStatus(nextbgrid, col, row, 1); // make alive
                else bSetCellStatus(nextbgrid, col, row, 0); // make dead
            } else // if dead
                if (liveNeighbors == 3) bSetCellStatus(nextbgrid, col, row, 1); // make alive
                else bSetCellStatus(nextbgrid, col, row, 0); // make dead
        }
    }
}

uint8_t bCountNeighbours(uint8_t (*bgrid)[ROWS], uint16_t _col, uint8_t _row) {
    uint8_t nbrs = 0; // neighbour count
    for (int8_t i = -1; i <= 1; ++i) {
        for (int8_t j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue; // Skip the cell itself
            if (bGetCellStatus(bgrid, _col + i, _row + j)) ++nbrs; // out of bounds checked inside the function
        }
    }
    return nbrs;
}

uint8_t bGetCellStatus(uint8_t (*bgrid)[ROWS], uint16_t _col, uint8_t _row) {
    if (_col > COLS - 1 || _row > ROWS - 1) return 0; // out of bounds (_col/_row == -1 -> 255)
    switch (_col % 8) {
        case 0: return bgrid[_col/8][_row] & CELL0;
        case 1: return bgrid[_col/8][_row] & CELL1;
        case 2: return bgrid[_col/8][_row] & CELL2;
        case 3: return bgrid[_col/8][_row] & CELL3;
        case 4: return bgrid[_col/8][_row] & CELL4;
        case 5: return bgrid[_col/8][_row] & CELL5;
        case 6: return bgrid[_col/8][_row] & CELL6;
        case 7: return bgrid[_col/8][_row] & CELL7;
    }
    return 0; // this shouldn't be reached
}

void bSetCellStatus(uint8_t (*bgrid)[ROWS], uint16_t _col, uint8_t _row, uint8_t status) {
    uint8_t cell = 0;
    
    switch (_col % 8) {
        case 0: cell = CELL0; break;
        case 1: cell = CELL1; break;
        case 2: cell = CELL2; break;
        case 3: cell = CELL3; break;
        case 4: cell = CELL4; break;
        case 5: cell = CELL5; break;
        case 6: cell = CELL6; break;
        case 7: cell = CELL7; break;
    }
    
    if (status) bgrid[_col/8][_row] |= cell; // make alive
    else bgrid[_col/8][_row] &= (~cell); // make dead
}

uint16_t translateAddress(uint16_t col, uint8_t row) {
    /* 32x16 grid example:					*
     *      00   01 ...   0e   0f |   10   11 ...   1e   1f	*
     * 00 000f 000e ... 0001 0000 | 01ff 01fe ... 01f1 01f0	*
     * 01 0010 0011 ... 001e 001f | 01e0 01e1 ... 01ee 01ef	*
     * 02 002f 002e ... 0021 0020 | 01df 01de ... 01d1 01d0	*
     * ...          ...           |           ...		*
     * 0e 00ef 00ee ... 00e1 00e0 | 011f 011e ... 0111 0110	*
     * 0f 00f0 00f1 ... 00fe 00ff | 0100 0101 ... 010e 010f */
    uint16_t addr;
        
    if (col < 0x0010) { // leftmost panel
        addr = 0x0000;
        addr |= (uint16_t)row << 4; // 0x00#* |= 0x000# << 4 
        if (row & 0x01) addr |= col; // if row number is odd: 0x00*# |= 0x000#
        else addr |= (0x000f - col); // if row number is even: 0x00*# |= (0x000f - 0x000#)
    } else { // rightmost panel
        addr = 0x0100;
        addr |= (uint16_t)(0x0f - row) << 4; // 0x01(f-#)* = (0x0f - 0x0#) << 4
        if (row & 0x01) addr |= (col & 0x000f); // if row number is odd: 0x01*# |= 0x000#
        else addr |= (0x000f - (col & 0x000f)); // if row number is even: 0x01*# |= (0x000f - 0x000#)
    }
    return addr;

}

void testPanel() {
    /* expected behaviour:				*
     * go through all of LEDs in order			*
     * start on leftmost panel in top right corner	*
     * finish on bottom right corner			*
     * step onto rightmost panel in bottom left corner	*
     * finish on top left corner			*/
    while (1) {
        for (uint16_t addr = 0; addr < STRIP_NUM_PIXELS; ++addr) {
            pixels[addr].r = 0x02;
            led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
            k_sleep(NONE_DELAY);
            
            pixels[addr].r = 0x00;
        }
    }
}

void testTranslateAddress() {
    /* expected behaviour:		*
     * 1. start left top corner		*
     * 2. go to right top corner	*
     * 3. start left top-1 corner	*
     * 4. go to right top-1 corner	*
     * ...				*/
    while (1) {
        for (uint8_t row = 0; row < ROWS; ++row)
            for (uint16_t col = 0; col < COLS; ++col) {
                pixels[translateAddress(col, row)].r = 0x02;
                led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
                k_sleep(NONE_DELAY);
            
                pixels[translateAddress(col, row)].r = 0x00;
            }
    }
}

