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


int main() {
    int grid[ROWS][COLS];
    
    // Initialize memory
    uint8_t** const workingbuffer1 = allocateMemory();
    uint8_t** const workingbuffer2 = allocateMemory();
    uint8_t** foregroundbuffer = workingbuffer1; // display this data
    uint8_t** backgroundbuffer = workingbuffer1; // work on this data

    // Initialize the grid with dead cells (0s) and one cell with a live organism (1) for testing
    initGrid(grid);
    // Initialize the grid with random values

    // Print initial state
    printf("Initial Grid:\n");
    printGrid(grid);

    // Update the grid to the next generation
    updateGrid(grid);

    // Print the updated grid
    printf("\nUpdated Grid:\n");
    printGrid(grid);

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

// Function to initialize the grid with dead cells
void initGrid(int grid[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            grid[i][j] = rand() % 2; // Randomly initialize cells to either 0 or 1
        }
    }
}

// Initialize the grid with random values
void bInitGrid(uint8_t** bgrid) { 
    int bytecols = COLS/8 + (COLS % 8 == 0) ? 0 : 1; // int division truncates (rounds towards 0);
    
    for (int col = 0; col < bytecols; ++col)
        for (int row = 0; row < ROWS; ++row)
            bgrid[col][row] = rand() % 0b11111111; // Randomly initialize BITS (cells) to either 0 or 1
}

// Function to print the current state of the grid
void printGrid(int grid[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            printf("%d ", grid[i][j]);
        }
        printf("\n");
    }
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
        if (val & 0b10000000) // check "first" cell (bit)
            printf("#"); // bit is set, cell is alive
        else
            printf(" "); // bit is not set, cell is dead
        val <<= 1; // shift to left to get the next cell
        --nbbits;
    }
}

// Function to update the grid to the next generation
void updateGrid(int grid[ROWS][COLS]) {
    int newGrid[ROWS][COLS];

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int liveNeighbors = countLiveNeighbors(grid, i, j);

            if (grid[i][j] == 1) {
                // Any live cell with two or three neighbors survives.
                if (liveNeighbors == 2 || liveNeighbors == 3) {
                    newGrid[i][j] = 1;
                } else {
                    newGrid[i][j] = 0;
                }
            } else {
                // Any dead cell with exactly three neighbors becomes a live cell.
                if (liveNeighbors == 3) {
                    newGrid[i][j] = 1;
                } else {
                    newGrid[i][j] = 0;
                }
            }
        }
    }

    // Copy the new grid to the old grid
    memcpy(grid, newGrid, sizeof(int) * ROWS * COLS);
}

// Function to count live neighbors around a cell
int countLiveNeighbors(int grid[ROWS][COLS], int row, int col) {
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue; // Skip the cell itself
            int newRow = row + i;
            int newCol = col + j;
            if (newRow >= 0 && newRow < ROWS && newCol >= 0 && newCol < COLS) {
                count += grid[newRow][newCol];
            }
        }
    }
    return count;
}

void bUpdateGrid(uint8_t** currbgrid, uint8_t** nextbgrid) {
    
}
