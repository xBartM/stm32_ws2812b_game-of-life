#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#warning "jak sie wywali to pewnie (bInitGrid) zamienione rows i cols"

// Constants for the game
#define COLS 32
#define ROWS 16

// Check if char is 8 bits long
#if CHAR_BIT != 8 
#error "CHAR_BIT != 8"
#endif

// Bit definitions
#define CELL0  0b10000000
#define CELL1  0b01000000
#define CELL2  0b00100000
#define CELL3  0b00010000
#define CELL4  0b00001000
#define CELL5  0b00000100
#define CELL6  0b00000010
#define CELL7  0b00000001

// Function prototypes
void initGrid(int grid[ROWS][COLS]);
void printGrid(int grid[ROWS][COLS]);
void updateGrid(int grid[ROWS][COLS]);


uint8_t** allocateMemory(); 

void bInitGrid(uint8_t**);
void bPrintGrid(uint8_t**);
void bPrintChar(uint8_t, uint8_t);
void bUpdateGrid(uint8_t**, uint8_t**);
uint8_t bCountNeighbours(uint8_t**, uint8_t, uint8_t);
uint8_t bGetCellStatus(uint8_t**, uint8_t, uint8_t);
void bSetCellStatus(uint8_t**, uint8_t, uint8_t, uint8_t);

int main() {
    int grid[ROWS][COLS];
    
    // Initialize memory
    uint8_t** const workingbuffer1 = allocateMemory();
    uint8_t** const workingbuffer2 = allocateMemory();
    uint8_t** foregroundbuffer = workingbuffer1; // display this data
    uint8_t** backgroundbuffer = workingbuffer1; // work on this data

    // Initialize the grid with random values
    bInitGrid(foregroundbuffer);

    // Print initial state
    bPrintGrid(foregroundbuffer);

    // Update the grid to the next generation
    bUpdateGrid(foregroundbuffer, backgroundbuffer);

    // Print the updated grid
    bPrintGrid(backgroundbuffer);

    return 0;
}

// Allocate memory for World Grid
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
    int bytecols = COLS/8 + (COLS % 8 == 0) ? 0 : 1; // int division truncates (rounds towards 0);
    
    uint8_t** allocmem = malloc(bytecols * sizeof(uint8_t*)); // alloc an array of pointers to specific columns
    for (int col=0; col < bytecols; ++col) // iterate over those columns
        allocmem[col] = malloc(ROWS * sizeof(uint8_t)); // alloc an array of uint8_ts within a column
        
    return allocmem;
}

// Initialize the grid with random values
void bInitGrid(uint8_t** bgrid) { 
    int bytecols = COLS/8 + (COLS % 8 == 0) ? 0 : 1; // int division truncates (rounds towards 0);
    
    for (int col = 0; col < bytecols; ++col)
        for (int row = 0; row < ROWS; ++row)
            bgrid[col][row] = rand() % 0b11111111; // Randomly initialize BITS (cells) to either 0 or 1
}

// Print the current state of the world
void bPrintGrid(uint8_t** bgrid) {
    int fullbytecols = COLS/8; // only whole bytes - the partial bytes will be handeled differentely
    int restbytecols = COLS%8;
    
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < fullbytecols; ++col) {
            bPrintChar(bgrid[col][row]); // print all full byte columns for a given row
        }
        bPrintChar(bgrid[col][row], restbytecols); // print partial byte columns for a given row
        printf("\n");
    }
}

// Print first nbbits from specified byte
void bPrintChar(uint8_t val, uint8_t nbbits=8) {
    while (nbbits > 0) {
        if (val & 0b10000000) // check the "first" cell (bit)
            printf("#"); // bit is set, cell is alive
        else
            printf(" "); // bit is not set, cell is dead
        val <<= 1; // shift to left to get the next cell
        --nbbits;
    }
}

void bUpdateGrid(uint8_t** currbgrid, uint8_t** nextbgrid) {
    uint8_t liveNeighbors = 0;
    for (uint8_t row = 0; row < ROWS; ++row) {
        for (uint8_t col = 0; col < COLS; ++col) {
            liveNeighbors = bCountNeighborus(currbgrid, col, row);

            if (bGetCellStatus(currbgrid, i, j)) { // if alive
                if (liveNeighbors == 2 || liveNeighbors == 3) continue; // do nothing
                else bSetCellStatus(nextbgrid, col, row, 0); // make dead
            } else // if dead
                if (liveNeighbors == 3) bSetCellStatus(nextbgrid, col, row, 1); // make alive
        }
    }
}

uint8_t bCountNeighbours(uint8_t** bgrid, uint8_t _col, uint8_t _row) {
    uint8_t nbrs = 0; // neighbour count
    for (uint8_t i = -1; i <= 1; ++i) {
        for (uint8_t j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue; // Skip the cell itself
            if (bGetCellStatus(bgrid, _col + i, _row + j)) ++nbrs; // out of bounds checked inside the function
        }
    }
    return nbrs;
}

uint8_t bGetCellStatus(uint8_t** bgrid, uint8_t _col, uint8_t _row) {
    if (_col < 0 || _col > COLS - 1 || _row < 0 || _row > ROWS - 1) return 0; // out of bounds
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
    
    if (status) bgrid[_col/8][_row] = bgrid[_col/8][_row] | cell; // make alive
    else bgrid[_col/8][_row] = bgrid[_col/8][_row] & (~cell); // make dead
}
