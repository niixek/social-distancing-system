/**
* Authors: Kexin Chen and Siquan Wang
* Purpose: A system that can detect the distance between a person and the system
* Extra Modules/Functions in File: QEI by Aaron Berk
* Assignment: Project 3
*********************************************************
* Inputs: Ultrasonic sensor, Rotary Encoder
* Outputs: Buzzer, LCD Display
* Constraints: Must keep the distance between 2 to 400 cm
               Adjustable distance is only adjusted by rotary encoder
               Ultrasonic sensor must have direct path to object
               Buzzer must be loud, but not too loud
               Must have response regarding object's distance to system on LCD
*********************************************************
* References:
*   QEI by Aaron Berk (Last Updated 09/02/2010) link:
*       https://os.mbed.com/users/aberk/code/QEI/docs/tip/classQEI.html
*   YT_001_HCSR04 (Ultrasonic Sensor) by Chris Powers (Last Update 06/03/2020)
*       link:
https://os.mbed.com/users/Powers/code/YT_001_HCSR04//file/777a2656a150/main.cpp/
*/

// Mbed header file
#include "mbed.h"

// LCD header file
#include "lcd1602.h"

// Rotary Encoder header file
#include "QEI.h"

// C standard IO header file
#include <cstdio>

/**
 * minDistance is the minimum "safe" distance from the system in centimeters
 * By default this is set to 183 cm, or 6 feet, as recommended by the CDC.
 */
int minDistance = 183;

// dist keeps track of the distance the ultrasonic sensor returns.
int dist;

// pulse keeps track of the previous state of the rotary encoder.
int pulse = 0;

// isWarning keeps track if the system is currently displaying the Warning text
bool isWarning = false;

/**
 * menu 1 and menu 2 are strings that are to be later displayed onto the
 * LCD Display.
 * warning is the warning message string.
 */
char menu1[] = "Social Distance";
char menu2[] = "Set new distance";
char warning[] = "Please Back Up! ";

/**
 * get_time stores the time between the ultrasonic sensor's outgoing wave and
 * reflected wave.
 */
Timer get_time;

/**
 * The below 3 variables are to be used with synchronization, as unplanned
 * changes to them can cause undesired results. They are set as "volatile".
 */
// pbcounter keeps track of the number of times the User Push button is pressed.
volatile int pbcounter = 0;

/**
 * printed is a boolean that determines whether or not the top line of the LCD
 * has printed.
 */
volatile bool printed = true;

/**
 * isChanging is a boolean that determines if the user has switched between the
 * menus. If it is false, then the user is at the default menu. If it is true,
 * then the user is at the "Change Distance" menu.
 */
volatile bool isChanging = false;

/*
 * lock is a mutex that prevents any change to the above 3 variables from other
 * threads when used. This technique was used for our synchronization
 * requirement.
 */
Mutex lock;

// Enable pin D9 (PD_15) as an output for the Ultrasonic sensor's trigger
DigitalOut trigger(D9);

// Enable pin D9 (PF_12) as an input for the Ultrasonic sensor's echo
DigitalIn echo(D8);

/**
 * Enable pin PB_8 as a PWM output. PWM was used to completely turn the buzzer
 * on and off.
 */
PwmOut Buzzer(PB_8);

/**
 * Initialization of a QEI object.
 * The first argument is "Channel A" (DT) and PE_10 is assigned.
 * The second argument is "Channel B" (CLK) and PE_12 is assigned.
 * The third argument is index and is not used so NC is assigned.
 * The fourth argument is pulses per revolution and is assigned to 1 pulse.
 */
QEI encoder(PE_10, PE_12, NC, 1);

/**
 * Initialization of LCD Object.
 * The first two parameters describe LCD size, columns x rows, which is 16x2.
 * The third parameter is the character dot size of the LCD.
 * The fourth parameter is SDA and PF_0 is assigned.
 * The fifth parameter is SCL and PF_1 is assigned.
 */
CSE321_LCD lcd(16, 2, LCD_5x8DOTS, PF_0, PF_1);

/**
 * Initialization of the User Push Button (PC_13) as an interrupt input.
 * PullDown is used to give it a default value of off.
 */
InterruptIn button(PC_13, PullDown);

// Initialization of a thread
Thread t;

/**
 * Initialization of an EventQueue, this is used for scheduling our threads.
 * Threads are queued up in the queue to run one after the other.
 * This technique was used for our scheduling requirement.
 */
EventQueue q(32 * EVENTS_EVENT_SIZE);

// Initialization of the WatchDog Timer
Watchdog &dog = Watchdog::get_instance();

/**
 * wdTimeout is the time for how long the WatchDog waits until it resets the
 * system. It is set to a value of 30000 milliseconds, or 30 seconds.
 */
#define wdTimeout 30000

// Below are the prototyping for all of the functions in the program.

// Function prototype for the Ultrasonic sensor code.
int Ultrasonic(void);

// Function prototype for system menu logic.
void ChangeDistance(void);

// Function prototype for WatchDog timer reset.
void resetDog();

// Function prototype for turning the Buzzer on.
void BuzzerOn();

// Function prototype for turning the Buzzer off.
void BuzzerOff();

// Function prototype for LCD display logic.
void printMenu(char[]);

// main method
int main() {
  // Used to separate instances.
  printf("------Start------\n");

  /**
   * Start the watchdog, have it restart the system after wdTimeout
   * milliseconds.
   */
  dog.start(wdTimeout);

  /**
   * Direct Bit-wise configuration to program the pin PC_13 (user push button)
   * to be an input.
   */
  // Enable the clock (RCC) for port C.
  RCC->AHB2ENR |= 0x4;

  // Set pin 13 on port C as an input.
  GPIOC->MODER &= ~(0xC000000);

  /**
   * Start the thread with the associated EventQueue. The EventQueue is passed
   * as a parameter and ensures proper scheduling with threads.
   */
  t.start(callback(&q, &EventQueue::dispatch_forever));

  /**
   * Set up the user push button to trigger an interrupt when pressed (on the
   * rise).
   */
  button.rise(q.event(&ChangeDistance));

  // Turn off the buzzer.
  Buzzer.suspend();

  // Set the Ultrasonic sensor's trigger to low for the start of the program.
  trigger = 0;

  // String buffers that are used to convert int to string later.
  char buffer[3];
  char Ebuffer[3];

  // Set up the LCD to start displaying text.
  lcd.begin();

  // Clear the LCD display.
  lcd.clear();

  // Print "Social Distance" to the first line of the LCD display.
  lcd.print("Social Distance");

  // Loop to run forever
  while (true) {
    /**
     * If push button has been pressed and isChanging is true, then the system
     * should be at the "Set new distance" menu. Being in this menu for too long
     * will trigger the Watchdog and reset minDistance to 183.
     */
    if (isChanging) {
      // Call printMenu to print "Set new distance" to LCD
      printMenu(menu2);

      // Turn the Buzzer off
      BuzzerOff();

      // Adjust delay for knob turning speed，currently set for 50 ms delay
      thread_sleep_for(50);

      // pulses stores the current state of the encoder
      int pulses = encoder.getPulses();

      /**
       * minDistance should not be smaller than the recommended distance by CDC
       * (currently 6 feet or 183 cm).
       * The maximum detectable distance is 400cm for the Ultrasonic sensor.
       * For Demoing purposes, minimum settable distance is 1 foot or 31 cm.
       */

      // If minDistance is less than 31, set it equal to 31.
      if (minDistance < 31) {
        minDistance = 31;
      }

      // If minDistance is greater than 400, set it equal to 400.
      else if (minDistance > 400) {
        minDistance = 400;
      }

      // If encoder has been turned, determine which direction it was turned.
      else if (pulse != pulses) {
        // If turned to the right, increase minDistance.
        if (pulse < pulses) {
          minDistance++;
        }

        // If turned to the left, decrease minDistance.
        else {
          minDistance--;
        }

        // Update pulse to equal the current encoder state.
        pulse = pulses;

        // Print the minDistance to the console.
        printf("%d\n", minDistance);
      }

      // Convert minDistance to a string.
      sprintf(Ebuffer, "%d", minDistance);

      // Print minDistance to the second line of LCD.
      lcd.setCursor(0, 1);
      lcd.print(Ebuffer);

      // If minDistance is not a 3 digit number, clear the third digit.
      if (minDistance < 100) {
        lcd.setCursor(2, 1);
        lcd.print(" ");
      }

      // If minDistance is not a 2 digit number, clear the second digit as well.
      if (minDistance < 10) {
        lcd.setCursor(1, 1);
        lcd.print(" ");
      }
    }

    /**
     * The default state of the system.
     * If push button has been pressed and isChanging is false, then the system
     * should be at the default menu.
     */
    else {
      // Check for the distance every 300 ms.
      thread_sleep_for(300);

      // Store the distance between object and sensor in dist.
      dist = Ultrasonic();

      // Print the distance to the console.
      printf("%d\n", dist);

      // Convert dist to a string.
      sprintf(buffer, "%d", dist);

      // Print dist to the second line of the LCD.
      lcd.setCursor(0, 1);
      lcd.print(buffer);

      /**
       * If dist is not a 4 digit number, clear the fourth digit (In case of
       * number formatting bugs).
       */
      if (dist < 1000) {
        lcd.setCursor(3, 1);
        lcd.print(" ");
      }

      // If dist is not a 3 digit number, clear the third digit.
      if (dist < 100) {
        lcd.setCursor(2, 1);
        lcd.print(" ");
      }

      // If dist is not a 2 digit number, clear the second digit as well.
      if (dist < 10) {
        lcd.setCursor(1, 1);
        lcd.print(" ");
      }

      // If object is closer than minDistance, turn the Buzzer on.
      if (dist < minDistance) {
        BuzzerOn();

        // Set the cursor to the top line of LCD, and print the warning message.
        lcd.setCursor(0, 0);
        lcd.print(warning);

        // Set printed to false, to allow default text to display later.
        printed = false;
      }

      // If not, turn the Buzzer off.
      else {
        BuzzerOff();

        /**
         * Call printMenu to print "Set new distance" to LCD if isChanging is
         * true.
         */
        if (isChanging) {
          printMenu(menu2);
        }

        // Else print "Social Distance".
        else {
          printMenu(menu1);
        }
      }

      // Reset the WatchDog Timer.
      resetDog();
    }
  }

  // Return for memory purposes.
  return 0;
}

// Resets the WatchDog Timer when called.
void resetDog() {
  // Refresh the timer.
  dog.kick();
}

// Turn the buzzer on if called.
void BuzzerOn() {
  // Turn the buzzer on.
  Buzzer.resume();
}

// Turn the buzzer off if called.
void BuzzerOff() {
  // Turn the buzzer off.
  Buzzer.suspend();
}

/**
 * Reference: YT_001_HCSR04
 * Author: Chris Powers
 * Link:
 * https://os.mbed.com/users/Powers/code/YT_001_HCSR04//file/777a2656a150/main.cpp/
 * Last Updated: 06/03/2020
 *
 * Returns an int that represents the distance measured by the Ultrasonic sensor
 * when called.
 */
int Ultrasonic(void) {

  // Temporary variable for distance measured.
  int distance;

  /**
   * Send a HI signal to start the Ultrasonic sensor's measurement by turning
   * on trigger.
   */
  trigger.write(1);

  // Wait for 10 us.
  wait_us(10);

  // Send a LO signal to turn off the trigger.
  trigger.write(0);

  // Do nothing while the echo is waiting for a HI signal.
  while (echo.read() == 0) {
  }

  // Reset the timer, then start the timer.
  get_time.reset();
  get_time.start();

  // Do nothing while echo is waiting to be LO again.
  while (echo.read() == 1) {
  }

  // Stop the timer.
  get_time.stop();

  // Store the time elapsed into distance.
  distance = get_time.read_us();

  // Calculate the distance using the time elapsed.
  distance = distance * 0.03432f / 2.0f;

  // Return distance.
  return distance;
}

/**
 * Library: QEI
 * Author: Chris Powers
 * Link: https://os.mbed.com/users/aberk/code/QEI/docs/tip/classQEI.html
 * Last Updated: 09/02/2010
 *
 * ISR function for the User Push Button.
 * This function changes the variables that determine which menu the user is
 * currently in.
 * The variables isChanging, printed, and pbcounter are also locked at the start
 * and unlocked after to ensure that threads other than the current one are not
 * changing those values.
 * The synchronization is implemented with a Mutex lock.
 */
void ChangeDistance(void) {
  // Lock the Mutex to prevent other threads from changing values.
  lock.lock();

  // Flip the value of isChanging: True -> False, False -> True.
  isChanging = !isChanging;

  // Set printed to false, a different menu text needs to display.
  printed = false;

  // Print to the console that the menu has changed.
  printf("switched menu\n");

  // If the button hasn't been pressed, then increment pbcounter.
  if (pbcounter == 0) {
    // Increase pbcounter
    pbcounter++;
  }
  // If the button has been pressed, then set pbcounter back to 0.
  else {
    // Set pbcounter to 0.
    pbcounter = 0;
  }

  // Unlock the Mutex, allowing other threads to access the variables again.
  lock.unlock();
}

/**
 * Prints the text that is passed in to the first line of the LCD Display.
 * The values that are passed in only change top row text, so only the first
 * line is affected.
 */
void printMenu(char text[]) {
  // If printed is false, then the menu text needs to change.
  if (printed == false) {
    // Clear the LCD Display and set the text to the passed in text.
    lcd.clear();
    lcd.print(text);

    // Set printed to true, so the menu text isn't constantly printing.
    printed = true;
  }
}