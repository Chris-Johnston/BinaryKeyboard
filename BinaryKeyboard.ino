/*
Chris Johnston Apr 13 2017
Binary Keyboard for Arduino Pro Micro
*/

// libraries
#include <Keyboard.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// pixel art for 1/0 mode w/ animations
#include "PixelArt.h"

// flag for reading/entering right to left or left to right
// true = right to left (least significant bit to most)
// false = left to right (most significant bit to least)
#define USE_RIGHT_TO_LEFT false

// flag for outputting chars in HID keyboard mode or pure ctrl char mode (pure ASCII)
// true = HID keyboard - outputs keyboard codes for Backspace (8), Tab (9) and Enter (10), otherwise ctrl chars as below
// false = Ctrl char ASCII - outputs ctrl char corresponding to byte value entered (BS == Ctrl-H, Tab == Ctrl-I, Enter == Ctrl-J, etc.)
#define HID_MODE true

// These are the indices into the _asciimap array defined in Arduino Keyboard.cpp
#define HID_BS 0x08
#define HID_TAB 0x09
#define HID_ENTER 0x0A

// Strings to display when in HID mode
#define HID_BS_STRING "BS"
#define HID_TAB_STRING "Tab"
#define HID_ENTER_STRING "Enter"

// button pins
#define BUTTON_ZERO 8
#define BUTTON_ONE 9

#define LED_ZERO 5
#define LED_ONE 6

#define SCREEN_ROTATION 2

#define X_RESOLUTION 128
#define Y_RESOLUTION 32

// constant characters for zero and one (for single button press mode)
// hey osu players, you should look at these, do X for ZERO, Z for ONE
#define CHAR_ZERO '0' // right
#define CHAR_ONE '1' // left

// set up display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// timer
unsigned long debounceTimer = 0;
unsigned int debounceDelay = 5;
// switch mode delay
unsigned long switchModeTimerStartPress = 0;
// increased switch mode delay
unsigned int switchModeDelay = 2000;
// display last timer
unsigned long lastPrintTime = 0;
// this should be long enough so that people can tell what it says

unsigned int showDelay = 1500;

// last switch time
unsigned long switchTime = 0;

// used for one/zero mode timing
unsigned long timerOne = 0, timerZero = 0;

// flag for what mode it will start in
bool mode = false; // false = binary mode, true = single button press mode

char lastPrinted = ' ';

// keystroke value
byte keyStroke = 0; //B01010101;
int index = 0;

bool buttonStateZero, buttonStateOne;
// PWM values for each led
int ledZero, ledOne;
int ledDecayRate = 10;

void setup() {

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

// deals with a keypress
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

void sendVal(char val)
{
    if (val >= 32 || (HID_MODE && (val == HID_BS || val == HID_TAB || val == HID_ENTER))) {
        Keyboard.write(val);
    } else {
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press(val + 96);
        Keyboard.releaseAll();
    }
    lastPrintTime = millis();
    lastPrinted = val;
}

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

	ledZero = max(0, ledZero - ledDecayRate);
	ledOne = max(0, ledOne - ledDecayRate);

	display.clearDisplay();

	// do drawing stuff
	if (mode)
	{
		// single button press mode
		// draw black rectangle behind text
		//display.setTextSize(2);
		//display.setTextColor(WHITE, BLACK);
		//display.setCursor(61, 8); // roughly centered text
		//display.print(lastPrinted);


		// removing this for now, it gets in the way of the bitmaps
		/*if (((millis() - switchTime) < switchModeDelay))
		{
			display.setCursor(0, 0);
			display.setTextSize(1);
			display.print("Press & Hold Both To Return");
		}*/
	}
	else
	{
		if ((millis() - lastPrintTime) < showDelay)
		{
			display.setCursor(0, 0);
			display.setTextSize(1);
			display.print("Last: ");
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

		// minimalist style
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
	if (buttonStateZero && buttonStateOne)
	{
		if (switchModeTimerStartPress == 0)
		{
			switchModeTimerStartPress = millis();
		}
		else if (((millis() - switchModeTimerStartPress) > switchModeDelay) && previousOne && previousZero)
		{
			switchModeTimerStartPress = millis();
			mode = !mode;
			switchTime = millis();
			lastPrinted = '0';
		}

		int width = map(millis() - switchModeTimerStartPress, 0, switchModeDelay, 0, 128);

		// draw a 'progress bar' indicating when this will switch over
		display.drawRect(0, 30, width, 2, WHITE);
	}
	else
	{
		switchModeTimerStartPress = 0;
	}
	// in single button press mode, ignore the debounce timers
	if (mode)
	{
		if (buttonStateOne)
		{
			//display.drawRect(1, 1, 62, 30, WHITE);
			Keyboard.press(CHAR_ONE);
		}
		else
		{
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
		if (buttonStateZero)
		{
			Keyboard.press(CHAR_ZERO);
		}
		else
		{
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
		// debounce
		if ((millis() - debounceTimer) > debounceDelay)
		{
			// zero pressed
			if (buttonStateZero && !buttonStateOne && !previousZero)
			{
				keypress(0);
				debounceTimer = millis();
			}
			// one pressed
			else if (!buttonStateZero && buttonStateOne && !previousOne)
			{
				keypress(1);
				debounceTimer = millis();
			}
		}
	}
	display.display();
}
