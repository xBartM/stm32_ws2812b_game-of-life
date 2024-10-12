#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constants for the game
#define ROWS 50
#define COLS 50

// Function prototypes
void initGrid(int grid[ROWS][COLS]);
void printGrid(int grid[ROWS][COLS]);
void updateGrid(int grid[ROWS][COLS]);

int main() {
    int grid[ROWS][COLS];

    // Initialize the grid with dead cells (0s) and one cell with a live organism (1) for testing
    initGrid(grid);

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

// Function to initialize the grid with dead cells
void initGrid(int grid[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            grid[i][j] = rand() % 2; // Randomly initialize cells to either 0 or 1
        }
    }
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
