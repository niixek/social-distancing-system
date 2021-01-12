//modified from https://os.mbed.com/users/Yar/code/LiquidCrystal_I2C_for_Nucleo/

 #include "mbed.h"
 
// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80
 
// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00
 
// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00
 
// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00
 
// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00
 
// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00
 
#define LCD_ADDRESS_1602 0x4E 
#define En 0x04//B00000100  // Enable bit
#define Rw 0x02 // B00000010  // Read/Write bit
#define Rs 0x01 //B00000001  // Register select bit
 
/**
 * This is the driver for the Liquid Crystal LCD displays that use the I2C bus.
 *
 * After creating an instance of this class, first call begin() before anything else.
 * The backlight is on by default, since that is the most likely operating mode in
 * most cases.
 */
class CSE321_LCD {
public:
      /**
     * Constructor
     *
     * @param lcd_cols  Number of columns your LCD display has.
     * @param lcd_rows  Number of rows your LCD display has.
     * @param charsize  The size in dots that the display has, use LCD_5x10DOTS or LCD_5x8DOTS.
     * @param sda       Pin to use for SDA connection of I2C for LCD
     * @param scl       Pin to use for the SCL connection of I2C for LCD          
     */
    CSE321_LCD( unsigned char lcd_cols, unsigned char lcd_rows, unsigned char charsize = LCD_5x8DOTS,PinName sda=PB_9, PinName scl=PB_8);
 
    /**
     * Set the LCD display in the correct begin state, must be called before anything else is done.
     */
    void begin();
 
     /**
      * Remove all the characters currently shown. Next print/write operation will start
      * from the first position on LCD display.
      */
    void clear();
 
    /**
     * Next print/write operation will will start from the first position on the LCD display.
     */
    void home();
 
     /**
      * Do not show any characters on the LCD display. Backlight state will remain unchanged.
      * Also all characters written on the display will return, when the display in enabled again.
      */
    void noDisplay();
 
    /**
     * Show the characters on the LCD display, this is the normal behaviour. This method should
     * only be used after noDisplay() has been used.
     */
    void display();
 
    /**
     * Do not blink the cursor indicator.
     */
    void noBlink();
 
    /**
     * Start blinking the cursor indicator.
     */
    void blink();
 
    /**
     * Do not show a cursor indicator.
     */
    void noCursor();
 
    /**
     * Show a cursor indicator, cursor can blink on not blink. Use the
     * methods blink() and noBlink() for changing cursor blink.
     */
    void cursor();
    void scrollDisplayLeft();
    void scrollDisplayRight();
    void printLeft();
    void printRight();
    void leftToRight();
    void rightToLeft();
    void shiftIncrement();
    void shiftDecrement();
    void noBacklight();
    void backlight();
    bool getBacklight();
    void autoscroll();
    void noAutoscroll();
    void createChar(unsigned char, unsigned char[]);
    void setCursor(unsigned char, unsigned char);
    virtual int write(unsigned char);
    void command(unsigned char);
    inline void blink_on() { blink(); }
    inline void blink_off() { noBlink(); }
    inline void cursor_on() { cursor(); }
    inline void cursor_off() { noCursor(); }
 
// Compatibility API function aliases
    void setBacklight(unsigned char new_val);             // alias for backlight() and nobacklight()
    void load_custom_character(unsigned char char_num, unsigned char *rows);    // alias for createChar()
    int print(const char* text);
private:
    void send(unsigned char, unsigned char);
    void write4bits(unsigned char);
    void expanderWrite(unsigned char);
    void pulseEnable(unsigned char);
    unsigned char _addr;
    unsigned char _displayfunction;
    unsigned char _displaycontrol;
    unsigned char _displaymode;
    unsigned char _cols;
    unsigned char _rows;
    unsigned char _charsize;
    unsigned char _backlightval;

       //MBED I2C object used to transfer data to LCD
    I2C i2c;       
};
 