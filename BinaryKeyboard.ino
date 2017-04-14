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

// button pins
#define BUTTON_ZERO 8
#define BUTTON_ONE 9

#define LED_ZERO 5
#define LED_ONE 6

#define SCREEN_ROTATION 2

#define X_RESOLUTION 128
#define Y_RESOLUTION 32

// constant characters for zero and one (for single button press mode)
#define CHAR_ZERO '0'
#define CHAR_ONE '1'

// set up display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// timer
unsigned long debounceTimer = 0;
unsigned int debounceDelay = 5;
// switch mode delay
unsigned long switchModeTimerStartPress = 0;
unsigned int switchModeDelay = 1000;
// display last timer
unsigned long lastPrintTime = 0;
unsigned int showDelay = 500;
// last switch time
unsigned long switchTime = 0;

// flag for what mode it will run in
bool mode = false; // false = binary mode, true = single button press mode

char lastPrinted = ' ';

// keystroke value
byte keyStroke = 0; //B01010101;
byte index = 0;

bool buttonStateZero, buttonStateOne;
// PWM values for each led
int ledZero, ledOne;
int ledDecayRate = 10;

void setup() {
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
	display.print("Binary Keyboard!\n by Chris Johnston");
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
				c = '2'; // wonder if anyone will notice?
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
	// keep showing last value until next keypress
	if (index == 0) { keyStroke = 0; }
	keyStroke += val << index;
	index++;
	if (index > 7)
	{
		//Keyboard.write((char)keyStroke);
		sendVal((char)keyStroke);
		index = 0;
	}
}

void sendVal(char val)
{
	Keyboard.write(val);
	lastPrintTime = millis();
	lastPrinted = val;
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
		display.setTextSize(2);
		display.setTextColor(WHITE, BLACK);
		display.setCursor(61, 8); // roughly centered text
		display.print(lastPrinted);

		if (((millis() - switchTime) < switchModeDelay))
		{
			display.setCursor(0, 0);
			display.setTextSize(1);
			display.print("Press & Hold Both To Return");
		}
	}
	else
	{
		if ((millis() - lastPrintTime) < showDelay)
		{
			display.setCursor(0, 0);
			display.setTextSize(1);
			display.print("Last: ");
			display.print(lastPrinted);
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
		else if( ((millis() - switchModeTimerStartPress) > switchModeDelay) && previousOne && previousZero)
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
	display.display();
	// debounce
	
	if ((millis() - debounceTimer) > debounceDelay)
	{
		// zero pressed
		if (buttonStateZero && !buttonStateOne && !previousZero )
		{
			// if mode true, single button press
			if (mode)
			{
				sendVal(CHAR_ZERO);
			}
			else
			{
				keypress(0);
			}
			debounceTimer = millis();
		}
		// one pressed
		else if (!buttonStateZero && buttonStateOne && !previousOne )
		{
			// if mode true, single button press
			if (mode)
			{
				sendVal(CHAR_ONE);
			}
			else
			{
				keypress(1);
			}
			debounceTimer = millis();
		}
		//// both pressed
		//else if (buttonStateZero && buttonStateOne)
		//{	
		//	// toggle state
		//	mode = !mode;
		//	lastPrinted = '0';
		//	debounceTimer = millis();
		//}
	}
}