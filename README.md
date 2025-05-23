# LED Panel controlled with STM32 MCU playing Conway's Game of Life 

## Table of contents
* [General info](#general-info)
* [Glossary of Terms](#glossary-of-terms)
* [Build and Flash](#build-and-flash)
* [Physical Build](#physical-build)
* [Turning It On](#turning-it-on)
* [Code Quirks](#code-quirks)
* [Endnotes](#endnotes)
* [State of the Project](#state-of-the-project)

## General info

This repository enables you to turn a WS2812b LED panel[^1] into a randomly generated mosaic. Patterns change based on the rules of Conway's Game of Life. Developed around STM32F411CE, but being powered by [Zephyr RTOS](https://github.com/zephyrproject-rtos/zephyr), means that adapting to other MCUs should be relatively straightforward.

| <img src="./media/working_panel.gif" /> | 
|:--:| 
| *2xWS2812b LED Panel with light diffusers* |

## Glossary of Terms

| Term | Meaning |
|:-----|:--------|
| [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) | A grid of dead or alive cells that evolves over time based on a specific known ruleset. |
| [Zephyr RTOS](https://github.com/zephyrproject-rtos/zephyr) | From description: "Zephyr is a new generation, scalable, optimized, secure RTOS for multiple hardware architectures."|

## Build and Flash

Assuming you already have the [Zephyr development environment](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) set up. 

1. Clone this repository into zephyrproject workspace. In my case it would be:
```
cd <path_to_zephyr>/usrprojects
git clone https://github.com/xBartM/stm32_ws2812b_game-of-life.git
```
2. To build the app, run (note that it will overwrite whatever you have built previously this way):
```
west build -b blackpill_f411ce usrprojects/stm32_ws2812b_game-of-life/
```
Feel free to change the target board by swapping it's name in the aforementioned command and providing adequate `.overlay` file in `boards` folder.

3. To flash, enter the bootloader (in my case BOOT0+NRST on the board) and simply run:
```
west flash
```
For this particular board, you might need to warm it up a little for it to communicate correctly (see [DFU Mode](https://github.com/WeActStudio/WeActStudio.MiniSTM32F4x1#how-to-enter-isp-mode)).  
Alternatively, install `stm32cubeprogrammer` from the official ST [website](https://www.st.com/en/development-tools/stm32cubeprog.html), connect to the board via ST-LINK/V2, enter the bootloader (as mentioned above), and run:
```
west flash --runner stm32cubeprogrammer
```

## Physical Build

### 3D Printing light diffusion panel

I 3D printed my diffusion panel out of white semi-transparent PETG. Make sure your print bed is clean or you'll have stains on the most visible part of the panel.  
Files necessary to print the panel are located in `model3d/` folder.
1. (Optional) change the code in `model.py` or export the model as a `.stl` or `.step` file and import it into the CAD of your choice to make it a better fit for your use case (take a look at [Endnotes](#endnotes)).
2. Slice the model using your preferred slicer.
    * parts are designed so that everything should be printed (every wall and layer)
    * the model doesn't need supports (max angle is 45 deg; orient it with the flat part facing down)
    * note that the clarity depends on hotend temperature and printing (volumetric) speed
    * ironing (in Cura) might be worth checking out
3. Print according to your filament and 3D printer spec.
4. Press in the panel (tight tolerances should be enough to hold the LED panel within the diffusion panel)

### Connecting the board to the LED Display

The necessary physical connections are straightforward:
1. Ensure you're **not** powering the LED Panel though the board but rather from the same, external power source.
2. The LED Panel and the board need to share the same **GROUND**.
3. Connect both the LED Panel and the board to **5V**.
4. Connect the LED Panel's **DIN** to board's **PA7** pin (this may differ if you choose to use another MCU; in that case you need to write your own `.overlay` file, so you know what you're doing).

## Turning It On

If your board doesn't have a defined user button, then you're done and the LED Panel should light up.

If your board has a defined user button, it will enter a "testing" phase:
1. The first test runs through connected LEDs in order they are physically connected. Click **user button** to break out of this test.
2. The second test runs through connected LEDs left to right, from top to bottom. Reorient panels if needed to achieve continuity. Click **user button** to break out of this test.
3. The LED Panel should light up.
4. At any time you can use **user button** to freeze the animation in place.

## Code Quirks

1. Randomization:
    * needed for generating the starting state for each Game of Life;
    * STM32F411CE doesn't have a built-in Random Number Generator;
    * used the least significant bit of temperature sensor output multiple times to create 16-bit number to pass to `srand()` function;
2. Compressed bytes:
    * the smallest addressible unit is 8 bits long;
    * encoding cell state in Game of Life needs only one bit;
    * eight cells live in a single char that are accessed via bitwise operations;

## Endnotes

* Some parts of the code could be rewritten to enhance readability.
* The program could use threads for each colour, which would clean up the code and provide finer control over fading in or out colours.
* The `translateAddress()` function should be changed to a static array in memory instead of computing desired positions every time they are needed.
* Running games smaller than 32x16 (or 16x16) should be easy; though untested on LED Panel, it worked well in terminal.
* To seamlessly connect panels, one shoud remove half of the thickness of certain walls depending on panel configuration.

## State of the Project

* Game of Life implementation - **done**
* Taming of Zephyr development environment and devicetree secrets - **done**
* Displaying GoL on the panel - **done**
* Displaying all three colours at the same time - **done**
* Randomized seed every time the panel is powered on - **done**
* Design and 3D print proper enclosure with light diffusion - **done**
* Finish up README.md - **done**

[^1]: coded to support one or two 16x16 grid panels
