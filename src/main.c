#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
//#include <unistd.h>
//#include <string.h> // memcpy

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util_macro.h> // for LISTIFY
//#include <zephyr/drivers/spi.h>
//#include <zephyr/sys/util.h>

// Zephyr specific - setup led strip
#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif
// Zephyr specific - setup button
#define SW0_NODE DT_ALIAS(sw0)
#if DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#define BUTTON_TESTING_BREAK
#endif
// Zephyr specific - temperature sensor
#define DIE_TEMP_NODE DT_ALIAS(die_temp0)
#if !DT_NODE_HAS_STATUS_OKAY(DIE_TEMP_NODE)
#error "DIE_TEMP status not okay"
#endif

// Zephyr specific - ksleep() time
#define DELAY_TIME K_MSEC(100*5)
#define DELAY_FAST K_MSEC(100*1)
#define DELAY_NONE K_MSEC(1)

// Check if char is 8 bits long
#if CHAR_BIT != 8 
#error "CHAR_BIT != 8"
#endif

// Constants for the game
#define COLS 32
#define ROWS 16

#if COLS % 16 != 0
#error "bShowPixels() not sufficient for partially compressed bytes"
#endif

#if COLS*ROWS > 512
#error "translateAddress() not sufficient for more than two panels; easily expandable to an arbitrary number of panels"
#endif

#if STRIP_NUM_PIXELS < COLS*ROWS
#error "Game size bigger than  led chain length"
#endif

// not needed - performance is good enough
//#warning "TODO: translateAddress() to preprocessor magic and global static const"

// Bit definitions
#define CELL0 0x80 // 0b10000000
#define CELL1 0x40 // 0b01000000
#define CELL2 0x20 // 0b00100000
#define CELL3 0x10 // 0b00010000
#define CELL4 0x08 // 0b00001000
#define CELL5 0x04 // 0b00000100
#define CELL6 0x02 // 0b00000010
#define CELL7 0x01 // 0b00000001

// Panel drawing settings
#define MAX_LUMENS 0x08 // max brightness at 0xff
#define LUMEN_STEPS 8 // number of steps to fade out/fade in in
#define LUMEN_STEPS_PLUS_BLACK 9 // change this too -.-; it's LUMEN_STEPS + 1 but LISTIFY doesn't allow that
#define RGB_DIM(bin, _) { .r = (((MAX_LUMENS) / (LUMEN_STEPS))*(bin)), .g = (((MAX_LUMENS) / (LUMEN_STEPS))*(bin)), .b = (((MAX_LUMENS) / (LUMEN_STEPS))*(bin)) }

void setupLumcolours();

// Function prototypes
uint16_t getRandomSeed(); 
void bInitGrid(uint8_t (*)[ROWS]);
void bShowPixels(uint8_t (*)[ROWS], uint8_t (*)[ROWS], uint8_t (*)[ROWS], uint8_t (*)[ROWS], uint8_t (*)[ROWS], uint8_t (*)[ROWS]);
void bUpdateGrid(uint8_t (*)[ROWS], uint8_t (*)[ROWS]);
uint8_t bCountNeighbours(uint8_t (*)[ROWS], uint16_t, uint8_t);
uint8_t bGetCellStatus(uint8_t (*)[ROWS], uint16_t, uint8_t);
void bSetCellStatus(uint8_t (*)[ROWS], uint16_t, uint8_t, uint8_t);
uint16_t translateAddress(uint16_t, uint8_t);
#ifdef BUTTON_TESTING_BREAK
int8_t configure_button();
void button_pressed_callback(const struct device*, struct gpio_callback*, uint32_t);
#endif

// Testing
uint8_t testPanel();
uint8_t testTranslateAddress();

// Global variables
// led strip specific
static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb lumcolours[] = { LISTIFY(LUMEN_STEPS_PLUS_BLACK, RGB_DIM, (,)) }; // for fade out/fade in; black + rest of the palette
//static struct led_rgb lumcolours[1+LUMEN_STEPS]; // for fade out/fade in; black + rest of the palette
static struct led_rgb pixels[STRIP_NUM_PIXELS]; // to display on led strip; initialized as 0s
//buffers for game of life
static uint8_t gbufferred0[COLS][ROWS];
static uint8_t gbufferred1[COLS][ROWS];
static uint8_t gbuffergreen0[COLS][ROWS];
static uint8_t gbuffergreen1[COLS][ROWS];
static uint8_t gbufferblue0[COLS][ROWS];
static uint8_t gbufferblue1[COLS][ROWS];
// button specific
#ifdef BUTTON_TESTING_BREAK
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback button_cb_data;
#endif
static uint8_t button_state = 0;
// die_temp specific
static const struct device *const sensor = DEVICE_DT_GET(DIE_TEMP_NODE);

int main() {

    if (!device_is_ready(strip))
        return 0;
#ifdef BUTTON_TESTING_BREAK
    if (configure_button())
        return 0;
#endif
    if (!device_is_ready(sensor))
        return 0;
    
    // Initialize memory
    uint8_t (*redgameprev)[ROWS] = gbufferred1;
    uint8_t (*redgamecurr)[ROWS] = gbufferred0;
    uint8_t (*redgamenext)[ROWS] = gbufferred1;
    uint8_t (*greengameprev)[ROWS] = gbuffergreen1;
    uint8_t (*greengamecurr)[ROWS] = gbuffergreen0;
    uint8_t (*greengamenext)[ROWS] = gbuffergreen1;
    uint8_t (*bluegameprev)[ROWS] = gbufferblue1;
    uint8_t (*bluegamecurr)[ROWS] = gbufferblue0;
    uint8_t (*bluegamenext)[ROWS] = gbufferblue1;
    uint8_t (*swapbuffer)[ROWS];
    //setupLumcolours();
    
    // Testing
#ifdef BUTTON_TESTING_BREAK
    button_state = testPanel();
    button_state = testTranslateAddress();
#else // uncomment and reflash to test if not using button
    //testPanel();
    //testTranslateAddress();
#endif

    // Initialize the grid with random values
    srand(getRandomSeed());
    bInitGrid(redgamecurr);
    bInitGrid(greengamecurr);
    bInitGrid(bluegamecurr);

    while (1) { // main loop
        // Print state
        bShowPixels(redgamecurr, redgameprev, greengamecurr, greengameprev, bluegamecurr, bluegameprev);

        // Update the grid to the next generation
        bUpdateGrid(redgamecurr, redgamenext);
        bUpdateGrid(greengamecurr, greengamenext);
        bUpdateGrid(bluegamecurr, bluegamenext);
        
        // Swap buffers
	swapbuffer = redgamecurr;
	redgamecurr = redgamenext;
	redgameprev = swapbuffer;
	redgamenext = swapbuffer;
	swapbuffer = greengamecurr;
	greengamecurr = greengamenext;
	greengameprev = swapbuffer;
	greengamenext = swapbuffer;
	swapbuffer = bluegamecurr;
	bluegamecurr = bluegamenext;
	bluegameprev = swapbuffer;
	bluegamenext = swapbuffer;
	
	// Sleep
#ifdef BUTTON_TESTING_BREAK
        while (button_state) k_sleep(DELAY_TIME);
#endif
        k_sleep(DELAY_NONE);

    }
    
    return 0;
}



void setupLumcolours() {
    /* example for: 			*
     * MAX_LUMENS == 0x08,		*
     * LUMEN_STEPS == 4			*
     * so stepsize == 0x02		*
     *   (red)0    1    2    3    4	*
     * r   0x00 0x02 0x04 0x06 0x08	*
     * g   0x00 0x02 0x04 0x06 0x08	*
     * b   0x00 0x02 0x04 0x06 0x08	*/
    const uint8_t stepsize = MAX_LUMENS / LUMEN_STEPS;
    for (uint8_t thebin = 1; thebin < LUMEN_STEPS+1; ++thebin) {
        lumcolours[thebin].r = stepsize * thebin;
        lumcolours[thebin].g = stepsize * thebin;
        lumcolours[thebin].b = stepsize * thebin;
        
    }
}

uint16_t getRandomSeed() {
    /* get LSB from each temp reading		* 
     * sleep may not be needed, but won't hurt	*/
    struct sensor_value val;
    uint16_t ret = 0;

    for (uint8_t i = 0; i < 16; ++i) {
        // fetch sensor samples
        if (sensor_sample_fetch(sensor)) continue;
        if (sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &val)) continue;
        ret |= (uint16_t)(val.val2 & (0x00000001)); // least significant bit of the fractional part of the value 
        ret <<= 1;
        k_sleep(DELAY_FAST);
    }
    return ret;
    
}

// Initialize the grid with random values
void bInitGrid(uint8_t (*bgrid1)[ROWS]) { 
    int bytecols = COLS/8; // int division truncates (rounds towards 0)
    if (COLS % 8 != 0) ++bytecols;
    
    for (int col = 0; col < bytecols; ++col)
        for (int row = 0; row < ROWS; ++row)
            bgrid1[col][row] = rand() % 0xFF; // 0b11111111; // Randomly initialize BITS (cells) to either 0 or 1

}

void bShowPixels(uint8_t (*currredgrid)[ROWS], uint8_t (*prevredgrid)[ROWS], uint8_t (*currgreengrid)[ROWS], uint8_t (*prevgreengrid)[ROWS], uint8_t (*currbluegrid)[ROWS], uint8_t (*prevbluegrid)[ROWS]) {
    const uint8_t fullbytecols = COLS/8;
    
    for (uint8_t lumcolour = 0; lumcolour < LUMEN_STEPS + 1; ++lumcolour) {
        for (uint8_t row = 0; row < ROWS; ++row) {
            for (uint8_t col = 0; col < fullbytecols; ++col) {
                for (uint8_t i = 0; i < 8; ++i) {
                // RED
                    if ((currredgrid[col][row] << i) & CELL0) // move to the next bit and check the highest 
                        if ((prevredgrid[col][row] << i) & CELL0) // staying alive
                            pixels[translateAddress(col*8+i, row)].r = lumcolours[LUMEN_STEPS].r;
                        else pixels[translateAddress(col*8+i, row)].r = lumcolours[lumcolour].r; // arriving
                    else if ((prevredgrid[col][row] << i) & CELL0) // dying
                        pixels[translateAddress(col*8+i, row)].r = lumcolours[LUMEN_STEPS - lumcolour].r;
                    else // if dead
                        pixels[translateAddress(col*8+i, row)].r = 0x00;
                // GREEN
                    if ((currgreengrid[col][row] << i) & CELL0) // move to the next bit and check the highest 
                        if ((prevgreengrid[col][row] << i) & CELL0) // staying alive
                            pixels[translateAddress(col*8+i, row)].g = lumcolours[LUMEN_STEPS].g;
                        else pixels[translateAddress(col*8+i, row)].g = lumcolours[lumcolour].g; // arriving
                    else if ((prevgreengrid[col][row] << i) & CELL0) // dying
                        pixels[translateAddress(col*8+i, row)].g = lumcolours[LUMEN_STEPS - lumcolour].g;
                    else // if dead
                        pixels[translateAddress(col*8+i, row)].g = 0x00;
                // BLUE
                    if ((currbluegrid[col][row] << i) & CELL0) // move to the next bit and check the highest 
                        if ((prevbluegrid[col][row] << i) & CELL0) // staying alive
                            pixels[translateAddress(col*8+i, row)].b = lumcolours[LUMEN_STEPS].b;
                        else pixels[translateAddress(col*8+i, row)].b = lumcolours[lumcolour].b; // arriving
                    else if ((prevbluegrid[col][row] << i) & CELL0) // dying
                        pixels[translateAddress(col*8+i, row)].b = lumcolours[LUMEN_STEPS - lumcolour].b;
                    else // if dead
                        pixels[translateAddress(col*8+i, row)].b = 0x00;
                }
            }
        }
        led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
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
    if (_col > COLS - 1 || _row > ROWS - 1) 
        return 0; // out of bounds (_col/_row == -1 -> 65535/255)
    else 
        return bgrid[_col/8][_row] & (CELL0 >> _col % 8);
    
}

void bSetCellStatus(uint8_t (*bgrid)[ROWS], uint16_t _col, uint8_t _row, uint8_t status) {
    if (status) 
        bgrid[_col/8][_row] |= (CELL0 >> _col % 8); // make alive
    else 
        bgrid[_col/8][_row] &= (~(CELL0 >> _col % 8)); // make dead
        
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

uint8_t testPanel() {
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
            k_sleep(DELAY_NONE);
            pixels[addr].r = 0x00;
            
            if (button_state) return 0;

        }
    }
}

uint8_t testTranslateAddress() {
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
                k_sleep(DELAY_NONE);        
                pixels[translateAddress(col, row)].r = 0x00;
                
                if (button_state) return 0;
            }
    }
}

#ifdef BUTTON_TESTING_BREAK
int8_t configure_button() {
    if (!gpio_is_ready_dt(&button)) 
        return -1;
    
    if (gpio_pin_configure_dt(&button, GPIO_INPUT) != 0)
        return -1;

    if (gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE) != 0)
        return -1;

    gpio_init_callback(&button_cb_data, button_pressed_callback, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);
    return 0;
}

void button_pressed_callback(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	button_state ^= 1;
}
#endif
