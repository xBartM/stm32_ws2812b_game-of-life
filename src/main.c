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

#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define DELAY_TIME K_MSEC(100*5)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

// Constants for the game
#define COLS 16
#define ROWS 16

#if COLS != 16
#error "bUpdatePixels() not sufficient for partially compressed bytes"
#endif

#if COLS*ROWS > 256
#error "translateAddress() not sufficient for two panels"
#endif

#if STRIP_NUM_PIXELS != COLS*ROWS
#error "Game size not equal to led chain length"
#endif

// Check if char is 8 bits long
#if CHAR_BIT != 8 
#error "CHAR_BIT != 8"
#endif

// Bit definitions
#define CELL0 0x80 // 0b10000000
#define CELL1 0x40 // 0b01000000
#define CELL2 0x20 // 0b00100000
#define CELL3 0x10 // 0b00010000
#define CELL4 0x08 // 0b00001000
#define CELL5 0x04 // 0b00000100
#define CELL6 0x02 // 0b00000010
#define CELL7 0x01 // 0b00000001

// Function prototypes
uint8_t** allocateMemory();
void freeMemory(uint8_t**);

void bInitGrid(uint8_t**);
void bPrintGrid(uint8_t**);
void bPrintChar(uint8_t, uint8_t);
void bUpdatePixels(uint8_t**, struct led_rgb*);
void bUpdateGrid(uint8_t**, uint8_t**);
uint8_t bCountNeighbours(uint8_t**, uint8_t, uint8_t);
uint8_t bGetCellStatus(uint8_t**, uint8_t, uint8_t);
void bSetCellStatus(uint8_t**, uint8_t, uint8_t, uint8_t);
uint8_t translateAddress(uint8_t, uint8_t);

// Global variables
static const struct led_rgb colors[] = {
	RGB(0xff, 0x00, 0x00), /* red */
	RGB(0x00, 0x08, 0x00), /* green */
	RGB(0x00, 0x00, 0x08), /* blue */
	RGB(0x00, 0x00, 0x00)  /* off */
};

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

int main() {

    if (!(device_is_ready(strip)))
        return 0;
    
    // Initialize memory
    uint8_t** const workingbuffer1 = allocateMemory();
    uint8_t** const workingbuffer2 = allocateMemory();
    struct led_rgb pixels[STRIP_NUM_PIXELS];
    memset(&pixels, 0x00, sizeof(pixels));
    
    while (1) {
    //break;
        for (uint8_t row = 0; row < ROWS; ++row)
            for (uint8_t col = 0; col < COLS; ++col) {
                memcpy(&pixels[translateAddress(col, row)], &colors[0], sizeof(struct led_rgb));
                led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
                k_sleep(DELAY_TIME);
            }
    }

    // Initialize the grid with random values
    bInitGrid(workingbuffer1);

    while (1) { // main loop
        // Print initial state
        k_sleep(DELAY_TIME);
        bUpdatePixels(workingbuffer1, pixels);
        led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
        //bPrintGrid(workingbuffer1);

        // Update the grid to the next generation
        bUpdateGrid(workingbuffer1, workingbuffer2);

        // Print the updated grid
        k_sleep(DELAY_TIME);
        bUpdatePixels(workingbuffer2, pixels);
        led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
        //bPrintGrid(workingbuffer2);
        
        // Update the grid to the next generation
        bUpdateGrid(workingbuffer2, workingbuffer1);

    }
    
    // Free the working memory
    freeMemory(workingbuffer1);
    freeMemory(workingbuffer2);
    
    return 0;
}

// Allocate the memory for World Grid
uint8_t** allocateMemory() {
    /****************************************
     * sample representation of 12x4 grid:  *
     *      COLS                            *
     * ROWS 0-7(byte0)  8-11(byte1)         *
     *    0 0b00000000  0b0000xxxx          *
     *    1 0b00000000  0b0000xxxx          *
     *    2 0b00000000  0b0000xxxx          *
     *    3 0b00000000  0b0000xxxx          *
     ****************************************/
    int bytecols = COLS/8; // int division truncates (rounds towards 0)
    if (COLS % 8 != 0) ++bytecols;

    uint8_t** allocmem = malloc(bytecols * sizeof(uint8_t*)); // alloc an array of pointers to specific columns
    for (int col=0; col < bytecols; ++col) // iterate over those columns
        allocmem[col] = malloc(ROWS * sizeof(uint8_t)); // alloc an array of uint8_ts within a column
        
    return allocmem;
}

// Free the memory
void freeMemory(uint8_t** allocdmem) {
    int bytecols = COLS/8; // int division truncates (rounds towards 0)
    if (COLS % 8 != 0) ++bytecols;

    for (int col=0; col < bytecols; ++col) // iterate over columns
        free(allocdmem[col]); // free allocd memory within a column
    free(allocdmem); // free an array of pointers to specific columns
}

// Initialize the grid with random values
void bInitGrid(uint8_t** bgrid1) { 
    int bytecols = COLS/8; // int division truncates (rounds towards 0)
    if (COLS % 8 != 0) ++bytecols;
    
    for (int col = 0; col < bytecols; ++col)
        for (int row = 0; row < ROWS; ++row)
            bgrid1[col][row] = rand() % 0xFF; // 0b11111111; // Randomly initialize BITS (cells) to either 0 or 1
}

// Print the current state of the world
void bPrintGrid(uint8_t** bgrid) {
    int fullbytecols = COLS/8; // only whole bytes - the partial bytes will be handeled differentely
    int restbytecols = COLS%8;
    
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < fullbytecols; ++col) {
            bPrintChar(bgrid[col][row], 8); // print all full byte columns for a given row
        }
        if (restbytecols != 0) bPrintChar(bgrid[fullbytecols][row], restbytecols); // print partial byte columns for a given row
        printf("\n");
    }
}

// Print first nbbits from specified byte
void bPrintChar(uint8_t val, uint8_t nbbits) {
    while (nbbits > 0) {
        if (val & CELL0) // check the "first" cell (bit)
            printf("#"); // bit is set, cell is alive
        else
            printf(" "); // bit is not set, cell is dead
        val <<= 1; // shift to left to get the next cell
        --nbbits;
    }
}

void bUpdatePixels(uint8_t** bgrid, struct led_rgb* pixels) {
    const uint8_t fullbytecols = COLS/8;
    
    for (uint8_t row = 0; row < ROWS; ++row) {
        for (uint8_t col = 0; col < fullbytecols; ++col) {
            if (bgrid[col][row] & CELL0) memcpy(&pixels[translateAddress(col*8+0, row)], &colors[0], sizeof(struct led_rgb)); // set alive
            else 			 memcpy(&pixels[translateAddress(col*8+0, row)], &colors[3], sizeof(struct led_rgb)); // set dead
            if (bgrid[col][row] & CELL1) memcpy(&pixels[translateAddress(col*8+1, row)], &colors[0], sizeof(struct led_rgb)); // set alive
            else 			 memcpy(&pixels[translateAddress(col*8+1, row)], &colors[3], sizeof(struct led_rgb)); // set dead
            if (bgrid[col][row] & CELL2) memcpy(&pixels[translateAddress(col*8+2, row)], &colors[0], sizeof(struct led_rgb)); // set alive
            else 			 memcpy(&pixels[translateAddress(col*8+2, row)], &colors[3], sizeof(struct led_rgb)); // set dead
            if (bgrid[col][row] & CELL3) memcpy(&pixels[translateAddress(col*8+3, row)], &colors[0], sizeof(struct led_rgb)); // set alive
            else 			 memcpy(&pixels[translateAddress(col*8+3, row)], &colors[3], sizeof(struct led_rgb)); // set dead
            if (bgrid[col][row] & CELL4) memcpy(&pixels[translateAddress(col*8+4, row)], &colors[0], sizeof(struct led_rgb)); // set alive
            else 			 memcpy(&pixels[translateAddress(col*8+4, row)], &colors[3], sizeof(struct led_rgb)); // set dead
            if (bgrid[col][row] & CELL5) memcpy(&pixels[translateAddress(col*8+5, row)], &colors[0], sizeof(struct led_rgb)); // set alive
            else 			 memcpy(&pixels[translateAddress(col*8+5, row)], &colors[3], sizeof(struct led_rgb)); // set dead
            if (bgrid[col][row] & CELL6) memcpy(&pixels[translateAddress(col*8+6, row)], &colors[0], sizeof(struct led_rgb)); // set alive
            else 			 memcpy(&pixels[translateAddress(col*8+6, row)], &colors[3], sizeof(struct led_rgb)); // set dead
            if (bgrid[col][row] & CELL7) memcpy(&pixels[translateAddress(col*8+7, row)], &colors[0], sizeof(struct led_rgb)); // set alive
            else 			 memcpy(&pixels[translateAddress(col*8+7, row)], &colors[3], sizeof(struct led_rgb)); // set dead
        }
        
    }
}

void bUpdateGrid(uint8_t** currbgrid, uint8_t** nextbgrid) {
    uint8_t liveNeighbors = 0;
    for (uint8_t row = 0; row < ROWS; ++row) {
        for (uint8_t col = 0; col < COLS; ++col) {
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

uint8_t bCountNeighbours(uint8_t** bgrid, uint8_t _col, uint8_t _row) {
    uint8_t nbrs = 0; // neighbour count
    for (int8_t i = -1; i <= 1; ++i) {
        for (int8_t j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue; // Skip the cell itself
            if (bGetCellStatus(bgrid, _col + i, _row + j)) ++nbrs; // out of bounds checked inside the function
        }
    }
    return nbrs;
}

uint8_t bGetCellStatus(uint8_t** bgrid, uint8_t _col, uint8_t _row) {
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

void bSetCellStatus(uint8_t** bgrid, uint8_t _col, uint8_t _row, uint8_t status) {
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

uint8_t translateAddress(uint8_t col, uint8_t row) {
    /*  16x16 grid example:					*
     *     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f	*
     *  0 0f 0e 0d 0c 0b 0a 09 08 07 06 05 04 03 02 01 00	*
     *  1 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f	*
     *  2 2f 2e 2d 2c 2b 2a 29 28 27 26 25 24 23 22 21 20	*
     *  ...							*
     *  e ef ee ed ec eb ea e9 e8 e7 e6 e5 e4 e3 e2 e1 e0	*
     *  f f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb fc fd fe ff	*/
    uint8_t addr = row << 4; // the first hex of translation is always equal to the row number 
    
    if (row & 0x01) addr |= col; // if row number is odd then the second hex is equal to column number
    else addr |= (0x0f - col); // if row number is even reverse second hex
    
    return addr;
}
