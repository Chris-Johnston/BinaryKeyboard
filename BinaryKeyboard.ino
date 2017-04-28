/*
	Binary Keyboard
	For full source and contributors, please visit: https://github.com/Chris-Johnston/BinaryKeyboard
*/

// Included Libraries for Arduino Micro 
#include <Keyboard.h>
#include <Wire.h>
#include <SPI.h>

/* 
	Prerequisite Libraries 
	These must be in your libraries directory before building!
	See https://www.arduino.cc/en/Guide/Libraries for more details.
*/

// Adafruit GFX - https://github.com/adafruit/Adafruit-GFX-Library
// Provides methods used for drawing.
#include <Adafruit_GFX.h>


// Adafruit SSD1306 - https://github.com/adafruit/Adafruit_SSD1306 
// Enables support for the SSD1306 OLED display.
#include <Adafruit_SSD1306.h>

// Pixel Art Frame Definition File 
#include "PixelArt.h"

/*
	Software Configuration Definitions
	Adjust these to your preference before compiling and uploading.
*/

// flag for reading/entering right to left or left to right
// true = right to left (least significant bit to most)
// false = left to right (most significant bit to least)
#define USE_RIGHT_TO_LEFT false

// flag for outputting chars in HID keyboard mode or pure ctrl char mode (pure ASCII)
// true = HID keyboard - outputs keyboard codes for Backspace (8), Tab (9) and Enter (10), otherwise ctrl chars as below
// false = Ctrl char ASCII - outputs ctrl char corresponding to byte value entered (BS == Ctrl-H, Tab == Ctrl-I, Enter == Ctrl-J, etc.)
#define HID_MODE true

// Output characters for "Single button press mode"
//todo allow picking which side is which character, currently is kinda flip flopped
#define CHAR_ZERO '0' // right
#define CHAR_ONE '1' // left

// time in ms to hold both buttons to switch modes
#define DELAY_SWITCH_MODES 2000
// time in ms to show the last printed character on screen
#define DELAY_SHOW_LAST_PRINTED 2000
// toggle to disable changing modes
#define CAN_SWITCH_MODES true

// LED PWM value decay rate
#define LED_DECAY_RATE 10

// which mode to start up in. False = Binary Mode, True = Single button press mode
#define MODE_ON_START false

/*
	Configuration Definitions
	Adjust these to match your circuit and hardware before compiling and uploading.
*/

// Button Pins - Connected as input 
#define BUTTON_ZERO 8
#define BUTTON_ONE 9

// Led Pins - Connected as PWM output 
#define LED_ZERO 5
#define LED_ONE 6

// time in ms for button debouncing
// cherry reports <= 5 ms debounce : http://cherryamericas.com/product/mx-series-2/#84b4bc7a7a0396678
#define DELAY_DEBOUNCE 5


// Rotation of the SSD_1306 OLED Display
// In testing, 2 rotated the screen so that the up direction faced away from the pins.
// 0 rotated the screen so that the up direction faced toward the pins.
// YMMV

#define SCREEN_ROTATION 2

/* Other Definitions */

// These are the indices into the _asciimap array defined in Arduino Keyboard.cpp
#define HID_BS 0x08
#define HID_TAB 0x09
#define HID_ENTER 0x0A

// Strings to display when in HID mode
#define HID_BS_STRING "BS"
#define HID_TAB_STRING "Tab"
#define HID_ENTER_STRING "Enter"

// set up display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// resolution of the SSD_1306 display
#define X_RESOLUTION 128
#define Y_RESOLUTION 32

// various timers for debouncing, animations, mode switching

// indicates the time of the last button press for each button
unsigned long debounceTimerZero = 0;
unsigned long debounceTimerOne = 0;

// the timer used to track how long both buttons have been pressed down
unsigned long switchModeTimerStartPress = 0; 

// time when last character was printed out
unsigned long lastPrintTime = 0;

// the timer used to track when the last time modes were switched
unsigned long switchTime = 0;

// used for one/zero mode animation timing
unsigned long timerOne = 0, timerZero = 0;

// flag for the current mode
bool mode = MODE_ON_START; // false = binary mode, true = single button press mode

// value of the last printed character
char lastPrinted = ' ';

// keystroke value being entered
byte keyStroke = 0;
// the index of the byte being entered
int index = 0;

// values for the state of each of the buttons
bool buttonStateZero, buttonStateOne;
// PWM values for each led
int ledZero, ledOne;


void setup() {

	//todo deal with ASCII values > 128. it actually looks like Keyboard.h does not support these.
	//not sure if I can get around it or if I should restrict to only using normal ASCII table

	if (USE_RIGHT_TO_LEFT)
	{
		index = 0;
	}
	else
	{
		index = 7;
	}

	//Serial.begin(9600);
	Keyboard.begin();

	// leds
	pinMode(LED_ZERO, OUTPUT);
	pinMode(LED_ONE, OUTPUT);

	// buttons
	pinMode(BUTTON_ZERO, INPUT);
	pinMode(BUTTON_ONE, INPUT);

	// display
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	// set screen rotation
	display.setRotation(SCREEN_ROTATION);

	// do some fancy screen stuff on boot
	display.clearDisplay();
	display.setCursor(0, 0);
	display.setTextColor(WHITE);
	display.print("Binary Keyboard!\ngithub.com/\n     Chris-Johnston");
	display.display();
	delay(1000);
	// wipes the screen away with an effect
	display.setCursor(0, 0);
	int width = 15;
	for (int i = 0; i < X_RESOLUTION / 6 + width + Y_RESOLUTION / 8; i++)
	{
		analogWrite(LED_ZERO, millis() / 2 % 255);
		analogWrite(LED_ONE, millis() / 2 % 255);

		// iterate horizontally
		for (int j = 0; j < Y_RESOLUTION / 8; j++)
		{
			//iterate vertically
			char c = ((i + j) % 2) ? '1' : '0';
			if (i == 2 && j == 2)
			{
				c = '2'; // wonder if anyone will notice? EDIT: Yes, people did. LOL
				/*
				* Bender:	Whoa, what an awful dream. Ones and zeroes everywhere. And I thought I saw a two.
				* Fry:		It was just a dream Bender. There's no such thing as two.
				*/
			}
			display.drawChar(i * 6, j * 8, c, WHITE, BLACK, 1);
			display.drawChar((i - width) * 6, j * 8, ' ', WHITE, BLACK, 1);
		}
		delay(10);
		display.display();
	}

	analogWrite(LED_ZERO, 0);
	analogWrite(LED_ONE, 0);
}

/*
	Method called whenever a key is pressed down in binary mode
	val is assumed to be either a 1 or a 0
*/
void keypress(int val)
{
	if (USE_RIGHT_TO_LEFT)
	{
		// clear the keystroke only when starting to type once again
		if (index == 0) { keyStroke = 0; }

		keyStroke += val << index;
		// index increments from 0 -> 7 when RTL
		index++;

		if (index > 7)
		{
			sendVal((char)keyStroke);
			index = 0;
		}
	}
	else
	{
		// clear the keystroke only when starting to type once again
		if (index == 7) { keyStroke = 0; }

		keyStroke += (val << index);
		// index decrements from 0->7 when LTR
		index--;

		if (index < 0)
		{
			sendVal((char)keyStroke);
			index = 7;
		}
	}
}

/*
	Handles writing a value in binary mode
*/
void sendVal(char val)
{
    if (val >= 32 || (HID_MODE && (val == HID_BS || val == HID_TAB || val == HID_ENTER))) {
        Keyboard.write(val);
    } else {
		// for ASCII < 0x20, use control characters in uppercase
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press(val + 96);
        Keyboard.releaseAll();
    }
	// update timers and last printed value to be used for drawing output
    lastPrintTime = millis();
    lastPrinted = val;
}

/*
	Handles the displaying of control characters
*/
void dispCtrlChar(char val)
{
    display.print('^');
    display.print((char)(val + 64)); // Use uppercase representation (e.g. ^A instead of ^a)
}

void loop() {
	bool previousZero = buttonStateZero;
	bool previousOne = buttonStateOne;

	buttonStateOne = digitalRead(BUTTON_ONE);
	buttonStateZero = digitalRead(BUTTON_ZERO);

	// write LED values
	analogWrite(LED_ZERO, ledZero);
	analogWrite(LED_ONE, ledOne);

	ledZero = max(0, ledZero - LED_DECAY_RATE);
	ledOne = max(0, ledOne - LED_DECAY_RATE);

	display.clearDisplay();

	// display the last printed character in binary keyboard mode for a given amount of time
	if (!mode)
	{
		if ((millis() - lastPrintTime) < DELAY_SHOW_LAST_PRINTED)
		{
			display.setCursor(0, 0);
			display.setTextSize(1);
			display.print("Last: ");
			// handle printing of control characters or their equivalents
            if (lastPrinted > 0 && lastPrinted < 32)
            {
                if (HID_MODE)
                {
                    switch (lastPrinted)
                    {
                        case HID_BS:
                            display.print(HID_BS_STRING);
                            break;
                        case HID_TAB:
                            display.print(HID_TAB_STRING);
                            break;
                        case HID_ENTER:
                            display.print(HID_ENTER_STRING);
                            break;
                        default:
                            dispCtrlChar(lastPrinted);
                            break;
                    }
                }
                else
                {
                    dispCtrlChar(lastPrinted);
                }
            }
            else
            {
                display.print(lastPrinted);
            }
        }

		// print out all 8 bits in (roughly) the center of the screen
		display.setTextSize(2);
		display.setTextColor(WHITE, BLACK);
		display.setCursor(16, 8);
		for (int i = 7; i >= 0; i--)
		{
			display.print(bitRead(keyStroke, i));
		}
		display.drawLine((8 - index - 1) * 12 + 16, 24, (8 - index) * 12 + 16, 24, WHITE);
	}

	// do backlighting stuff
	if (buttonStateOne)
	{
		ledOne = 255;
	}
	if (buttonStateZero)
	{
		ledZero = 255;
	}

	// handle switching between modes
	if (buttonStateZero && buttonStateOne && CAN_SWITCH_MODES)
	{
		if (switchModeTimerStartPress == 0)
		{
			// track when both buttons were first pressed
			switchModeTimerStartPress = millis();
		}
		// when both buttons were pressed DELAY_SWITCH_MODES ms ago, switch the mode
		// probably don't have to test that previousOne and previousZero are true
		else if (((millis() - switchModeTimerStartPress) > DELAY_SWITCH_MODES) && previousOne && previousZero)
		{
			switchModeTimerStartPress = millis();
			mode = !mode;
			switchTime = millis();
			lastPrinted = '0';
		}

		// draw a 'progress bar' indicating when this will switch over
		int width = map(millis() - switchModeTimerStartPress, 0, DELAY_SWITCH_MODES, 0, X_RESOLUTION);
		display.drawRect(0, 30, width, 2, WHITE);
	}
	else
	{
		// reset the timer when both buttons are not being pressed
		switchModeTimerStartPress = 0;
	}

	// if single button press mode
	if (mode)
	{
		// if one is debounced
		if ((millis() - debounceTimerOne) > DELAY_DEBOUNCE)
		{
			// check if it has been pressed
			if (buttonStateOne && !previousOne)
			{
				// the press function will press this button down and it will not be released
				// unless explicitly told to
				Keyboard.press(CHAR_ONE);
				debounceTimerOne = millis();
				//timerOne = millis();
			}
		}
		if(!buttonStateOne)
		{
			// time on release
			timerOne = millis();
			Keyboard.release(CHAR_ONE);
		}

		// do animation for button one (left)
		if (!buttonStateOne)
		{
			display.drawChar(TEXT_X_1, TEXT_Y_0, CHAR_ONE, WHITE, BLACK, 2);
			display.drawBitmap(KEY_X_0, KEY_Y_0, FRAME_0, ART_WIDTH, ART_HEIGHT, WHITE);
		}
		else
		{
			int v = (millis() - timerOne);// / FRAME_DELAY_MS;
			if (v <= FRAME_DELAY_MS * 1)
			{
				display.drawChar(TEXT_X_1, TEXT_Y_1, CHAR_ONE, WHITE, BLACK, 2);
				display.drawBitmap(KEY_X_0, KEY_Y_0, FRAME_1, ART_WIDTH, ART_HEIGHT, WHITE);
			}
			else if (v <= FRAME_DELAY_MS * 2)
			{
				display.drawChar(TEXT_X_1, TEXT_Y_2, CHAR_ONE, WHITE, BLACK, 2);
				display.drawBitmap(KEY_X_0, KEY_Y_0, FRAME_2, ART_WIDTH, ART_HEIGHT, WHITE);
			}
			else if (v <= FRAME_DELAY_MS * 3)
			{
				display.drawChar(TEXT_X_1, TEXT_Y_3, CHAR_ONE, WHITE, BLACK, 2);
				display.drawBitmap(KEY_X_0, KEY_Y_0, FRAME_3, ART_WIDTH, ART_HEIGHT, WHITE);
			}
			else
			{
				display.drawChar(TEXT_X_1, TEXT_Y_4, CHAR_ONE, WHITE, BLACK, 2);
				display.drawBitmap(KEY_X_0, KEY_Y_0, FRAME_4, ART_WIDTH, ART_HEIGHT, WHITE);
			}
		}

		// handle stuff for button zero

		// if zero is debounced
		if ((millis() - debounceTimerZero) > DELAY_DEBOUNCE)
		{
			// check if it has been pressed
			if (buttonStateZero && !previousZero)
			{
				Keyboard.press(CHAR_ZERO);
				debounceTimerZero = millis();
			}
		}
		if (!buttonStateZero)
		{
			// time on release
			timerZero = millis();
			Keyboard.release(CHAR_ZERO);
		}

		// do animation for button zero (right)
		if (!buttonStateZero)
		{
			display.drawChar(TEXT_X_2, TEXT_Y_0, CHAR_ZERO, WHITE, BLACK, 2);
			display.drawBitmap(KEY_X_1, KEY_Y_0, FRAME_0, ART_WIDTH, ART_HEIGHT, WHITE);
		}
		else
		{
			int v = (millis() - timerZero);// / FRAME_DELAY_MS;
			if (v <= FRAME_DELAY_MS * 1)
			{
				display.drawChar(TEXT_X_2, TEXT_Y_1, CHAR_ZERO, WHITE, BLACK, 2);
				display.drawBitmap(KEY_X_1, KEY_Y_1, FRAME_1, ART_WIDTH, ART_HEIGHT, WHITE);
			}
			else if (v <= FRAME_DELAY_MS * 2)
			{
				display.drawChar(TEXT_X_2, TEXT_Y_2, CHAR_ZERO, WHITE, BLACK, 2);
				display.drawBitmap(KEY_X_1, KEY_Y_1, FRAME_2, ART_WIDTH, ART_HEIGHT, WHITE);
			}
			else if (v <= FRAME_DELAY_MS * 3)
			{
				display.drawChar(TEXT_X_2, TEXT_Y_3, CHAR_ZERO, WHITE, BLACK, 2);
				display.drawBitmap(KEY_X_1, KEY_Y_1, FRAME_3, ART_WIDTH, ART_HEIGHT, WHITE);
			}
			else
			{
				display.drawChar(TEXT_X_2, TEXT_Y_4, CHAR_ZERO, WHITE, BLACK, 2);
				display.drawBitmap(KEY_X_1, KEY_Y_1, FRAME_4, ART_WIDTH, ART_HEIGHT, WHITE);
			}
		}
	}
	else // in normal binary keyboard mode
	{
		// debouncing for each button
		if ((millis() - debounceTimerZero) > DELAY_DEBOUNCE)
		{
			// zero pressed, and one is not pressed (otherwise this would cause issues with holding down both)
			if (buttonStateZero && !buttonStateOne && !previousZero)
			{
				keypress(0);
				debounceTimerZero = millis();
			}
		}

		if ((millis() - debounceTimerOne) > DELAY_DEBOUNCE)
		{
			// one pressed, and zero is not pressed
			if (!buttonStateZero && buttonStateOne && !previousOne)
			{
				keypress(1);
				debounceTimerOne = millis();
			}
		}
	}

	// actually update the screen with these graphics
	display.display();
}
