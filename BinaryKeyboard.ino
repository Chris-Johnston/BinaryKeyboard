/**
   Binary Keyboard for Arduino Pro Micro
   because Digispark/digistump didn't have any free io and I didn't want to burn a bootloader
*/

// libraries
#include <Keyboard.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// set up display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// timer
unsigned long debounceTimer = 0;
unsigned int debounceDelay = 175;
// switch mode delay
unsigned long switchModeTimer = 0;
unsigned int switchModeDelay = 250;

// flag for what mode it will run in
bool mode = false; // false = binary mode, true = single button press mode

char lastPrinted = ' ';

// button pins
#define BUTTON_ZERO 4
#define BUTTON_ONE 5

#define X_RESOLUTION 128
#define Y_RESOLUTION 32

// constant characters for zero and one (for single button press mode)
#define CHAR_ZERO '0'
#define CHAR_ONE '1'

// keystroke value
byte keyStroke = 0; //B01010101;
byte index = 0;

int stateZero, stateOne;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Keyboard.begin();

  // display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.print("Binary Keyboard!\n by Chris Johnston");
  display.display();
  delay(500);
  // wipes the screen away with an effect
  display.setCursor(0,0);
  int width = 15;
  for(int i = 0; i < X_RESOLUTION / 6 + width + Y_RESOLUTION / 8; i++)
  {
    // iterate horizontally
    for(int j = 0; j < Y_RESOLUTION / 8; j++)
    {
      //iterate vertically
      char c = ((i + j)%2) ? '1' : '0';
      if(i == 2 && j == 2)
      {
        c = '2'; 
        /*
         * B: Whoa, what an awful dream. Ones and zeroes everywhere. And I thought I saw a two.
         * F: It was just a dream Bender. There's no such thing as two.
         */
      }
      display.drawChar(i * 6, j * 8, c, WHITE, BLACK, 1);
      display.drawChar((i-width) * 6, j * 8, ' ', WHITE, BLACK, 1);
    }
    delay(10);
    display.display();
  }
  delay(300);
  

  // set up pins
  pinMode(BUTTON_ZERO, INPUT);
  pinMode(BUTTON_ONE, INPUT);
}

// deals with a keypress
void keypress(int val)
{
  // keep showing last value until next keypress
  if(index == 0) { keyStroke = 0; }
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
  lastPrinted = val;
}

void loop() {

  // testing
  display.clearDisplay();
  /*display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(millis());
  */
      // draw an expanding box depending on the timer
  if ((millis() - debounceTimer) < debounceDelay)
    {
      int x0,y0,w,h;
      int a = (int)(millis() - debounceTimer);
      x0 = map(a, 0, debounceDelay, 61, 0);
      w = map(a, 0, debounceDelay, 6, X_RESOLUTION);
      y0 = map(a, 0, debounceDelay, 8, 0);
      h = map(a, 0, debounceDelay, 8, Y_RESOLUTION);
      display.drawRoundRect(x0,y0,w,h, 2, WHITE);
    }
    
  if(mode)
  {
    // single button press mode
    // draw black rectangle behind text    
    display.setTextSize(2);
    display.setTextColor(WHITE, BLACK);
    display.setCursor(61, 8); // roughly centered text
    display.print(lastPrinted);
  }
  else
  {
    // minimalist style
    display.setTextSize(2);
    display.setTextColor(WHITE, BLACK);
    display.setCursor(16, 8);
    for(int i = 7; i >=0; i--)
    {
      display.print(bitRead(keyStroke, i));
    }
    display.drawLine((8-index - 1) * 12 + 16, 24, (8-index) * 12 + 16, 24, WHITE);
    display.display();
    /*
    // old style
    display.setTextSize(1);
    display.setCursor(8, 8);
    display.print("v: ");
    display.print(keyStroke); 
    display.print(" ");
    display.print((char)keyStroke); // todo just have an array of ascii to plain text comparisons, ex, instead of \n just new line etc.
    display.print("\n");
    for(int i = 7; i >=0; i--)
    {
      display.print(bitRead(keyStroke, i));
    }
    display.drawLine((8-index - 1) * 6, 24, (8-index) * 6, 24, WHITE);
    */
  }
  /*for(int i=0; i < index; i++)
  {
    Serial.print(bitRead(keyStroke, index));
    display.print(bitRead(keyStroke, index));
  }
  Serial.println();*/
  /*for(int i=1; i < 7; i++)
  {
    if(keyStroke < pow(2, i))
    {
      display.print('0');
      Serial.print('0');
    }
  }
  display.println(keyStroke, BIN);*/
  
  //Serial.println(keyStroke, BIN);
    //display.print(keyStroke, BIN);

  //Serial.println(millis());

  // draw bottom line indicating index
  
  
  display.display();

  // get button states
  stateZero = digitalRead(BUTTON_ZERO);
  stateOne = digitalRead(BUTTON_ONE);

  // debounce
  if ((millis() - debounceTimer) > debounceDelay)
  {
    // zero pressed
    if (stateZero == 1 && stateOne == 0)
    {
      // if mode true, single button press
      if (mode)
      {
        sendVal(CHAR_ZERO);
      }
      else
      {
        /*
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0,0);
        display.print("send nudes");
        display.display();
        delay(2000); */
        keypress(0);
      }
      debounceTimer = millis();
    }
    // one pressed
    else if (stateZero == 0 && stateOne == 1)
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
    // both pressed
    else if (stateZero == 1 && stateOne == 1)
    {
      // toggle state
      mode = !mode;
      lastPrinted = '0';
      debounceTimer = millis();
    }
  }
}
