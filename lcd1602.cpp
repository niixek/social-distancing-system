#include "lcd1602.h"
#include "mbed.h"

CSE321_LCD::CSE321_LCD(unsigned char lcd_cols, unsigned char lcd_rows,
                       unsigned char charsize, PinName sda, PinName scl)
    : i2c(sda, scl) {

  _addr = LCD_ADDRESS_1602; //address of the device
  _cols = lcd_cols;
  _rows = lcd_rows;
  _charsize = charsize;
  _backlightval = LCD_BACKLIGHT;
}

void CSE321_LCD::begin() {
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

  if (_rows > 1) {
    _displayfunction |= LCD_2LINE;
  }

  // for some 1 line displays you can select a 10 pixel high font
  if ((_charsize != 0) && (_rows == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  // According to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands.
  thread_sleep_for(50);

  // Now we pull both RS and R/W low to begin commands
  expanderWrite(
      _backlightval); // reset expanderand turn backlight off (Bit 8 =1)
  thread_sleep_for(1000);

  // put the LCD into 4 bit mode
  // this is according to the hitachi HD44780 datasheet
  // figure 24, pg 46

  // we start in 8bit mode, try to set 4 bit mode
  write4bits(0x03 << 4);
  wait_us(4500); // wait min 4.1ms

  // second try
  write4bits(0x03 << 4);
  wait_us(4500); // wait min 4.1ms

  // third go!
  write4bits(0x03 << 4);
  wait_us(150);

  // finally, set to 4-bit interface
  write4bits(0x02 << 4);

  // set # lines, font size, etc.
  command(LCD_FUNCTIONSET | _displayfunction);

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  display();

  // clear it off
  clear();

  // Initialize to default text direction (for roman languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);

  home();
  backlight();
}

//------------------Core Functions-----------------------------------------

void CSE321_LCD::clear() {
  command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
  wait_us(2000);             // this command takes a long time!
}

void CSE321_LCD::home() {
  command(LCD_RETURNHOME); // set cursor position to zero
  wait_us(2000);           // this command takes a long time!
}

void CSE321_LCD::setCursor(unsigned char col, unsigned char row) {
  int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  if (row > _rows) {
    row = _rows - 1; // we count rows starting w/0
  }
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void CSE321_LCD::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void CSE321_LCD::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

//------------Cursor Function---------------------------
// Turns the underline cursor on/off
void CSE321_LCD::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void CSE321_LCD::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void CSE321_LCD::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void CSE321_LCD::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
//----------------------Text Configuration functions-----------------------
//not addressing, explore if you wish
// These commands scroll the display without changing the RAM
void CSE321_LCD::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void CSE321_LCD::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void CSE321_LCD::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void CSE321_LCD::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void CSE321_LCD::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void CSE321_LCD::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void CSE321_LCD::createChar(unsigned char location, unsigned char charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i = 0; i < 8; i++) {
    write(charmap[i]);
  }
}

// Turn the (optional) backlight off/on
void CSE321_LCD::noBacklight(void) {
  _backlightval = LCD_NOBACKLIGHT;
  expanderWrite(0);
}

void CSE321_LCD::backlight(void) {
  _backlightval = LCD_BACKLIGHT;
  expanderWrite(0);
}
bool CSE321_LCD::getBacklight() { return _backlightval == LCD_BACKLIGHT; }

//-----------functions to output to LCD---------------------------------------
inline void CSE321_LCD::command(unsigned char value) { send(value, 0); }

inline int CSE321_LCD::write(unsigned char value) {
  send(value, Rs);
  return 1;
}


// write either command or data
void CSE321_LCD::send(unsigned char value, unsigned char mode) {
  unsigned char highnib = value & 0xf0;
  unsigned char lownib = (value << 4) & 0xf0;
  write4bits((highnib) | mode);
  write4bits((lownib) | mode);
}

void CSE321_LCD::write4bits(unsigned char value) {
  expanderWrite(value);
  pulseEnable(value);
}

void CSE321_LCD::expanderWrite(unsigned char _data) {
  char data_write[2];
  data_write[0] = _data | _backlightval;
  // Wire.beginTransmission(_addr);
  // Wire.write((int)(_data) | _backlightval);
  // Wire.endTransmission();
  i2c.write(_addr, data_write, 1, 0);
  i2c.stop();
}

void CSE321_LCD::pulseEnable(unsigned char _data) {
  expanderWrite(_data | En); // En high
  wait_us(1);                // enable pulse must be >450ns

  expanderWrite(_data & ~En); // En low
  wait_us(50);                // commands need > 37us to settle
}

void CSE321_LCD::load_custom_character(unsigned char char_num,
                                       unsigned char *rows) {
  createChar(char_num, rows);
}

void CSE321_LCD::setBacklight(unsigned char new_val) {
  if (new_val) {
    backlight(); // turn backlight on
  } else {
    noBacklight(); // turn backlight off
  }
}

int CSE321_LCD::print(const char *text) {

  while (*text != 0) {
    send(*text, Rs);
    text++;
  }
  return 0;
}