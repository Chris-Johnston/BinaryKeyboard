# BinaryKeyboard
A two-button backlit mechanical keyboard that types ASCII values, one bit at a time.

## Photos

<img src="Images/binarykeyboard.jpg" alt="Photo of Binary Keyboard" Width="500" />
<img src="Images/screen.jpg" alt="Photo of Screen" Width="300" />
<img src="Images/topdown.jpg" alt="TopDown Photo" Width="300" />

## GIFS
<img src="https://thumbs.gfycat.com/FixedTangibleBluebottle-size_restricted.gif" alt="Boot Up Sequence"/>
<img src="https://thumbs.gfycat.com/FaithfulThoseCrossbill-size_restricted.gif" alt="Normal Use" />
<img src="https://thumbs.gfycat.com/LawfulSafeIndianrockpython-size_restricted.gif" alt="Switching Modes" />
<img src="https://thumbs.gfycat.com/InferiorShortIchidna-size_restricted.gif" alt="Fancy Backlighting" />
<img src="https://thumbs.gfycat.com/AcclaimedAdorableJaguar-size_restricted.gif" alt="Yet another gif" />

# Technical Info

<img src="Schematic_small.png" alt="Image of Schematic" Width="500" />

## Components
- Arduino Pro Micro (knockoff)
- Cherry MX Clear Mechanical Switches
- SSD1306 OLED Display

## Arduino Library Requirements
These libraries are used to drive the SSD1306 OLED display.
- [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)

## Eagle Schematic Details
Because I could not locate a part to represent the SSD1306 OLED Display, I created my own. This can be found in the 
`CSJ Library` file for Eagle. A word of warning, this was my first time using Eagle. The part works, but could be improved.

Uses [c0z3n/cherrymx-eagle](https://github.com/c0z3n/cherrymx-eagle/) library as well.

## Usage
Each bit is typed out from the least significant bit to the most significant bit (left to right). After all 8 bits have
been entered, it will type out the ASCII value equivalent of that binary valiue. The Pro Micro has native USB support,
which means it acts just like any other keyboard.

If both keys are held down, the keyboard will switch to "1/0 mode" / "single button press mode" (I'm bad at names).
In this mode, each key will represent a '1' or a '0' just like on a traditional keyboard, no binary involved.

## Arduino Problems
Initially I was going to use a [DigiStump](http://digistump.com/products/1) board for this, but I found issues with not having enough memory to work with after importing libraries and not enough I/O. I'm using an Arduino Pro Micro knockoff. The reason that there is electrical tape covering it is because I found that shorting the contacts with a finger would cause it to stop working. I'm still not really sure why.

## Contribution and Modifications
Please feel free to fork this project for your own purposes.

## Acknowledgements
Special thanks goes to everyone in the UWB Makerspace that helped make this possible.
