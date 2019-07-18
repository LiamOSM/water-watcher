// Version 1.1 (initial public release)
// Copyright © 2019 Liam Smolenaars
// If you require a license, see
// https://opensource.org/licenses/BSD-3-Clause

/* This program runs on an Arduino Nano microcontroller.
   The device is a shower water monitor, which displays
   the total water, in litres, the cost of the water, and
   the real-time flow rate. The flow sensor used gives
   450 pulses per litre, therefore each pulse is 1/450th
   of a litre added.

   If the shower is running, the front-panel button toggles
   between the two display modes. Mode 0 shows the flow
   rate in litres per second. Mode 1 shows the cost of water
   used, in cents (¢). The total water, in litres, is always
   shown on the top of the screen regardless of the mode.

   If the shower is not running, the front-panel button
   clears the total volume of water and total cost. This
   prevents the user from accidentally clearing the total
   while the shower is running.

   The hardware connections are described below:
    - Sensor on pin D2 (int0)
    - Button on pin D3 (int1)
    - LCD SCL on pin A5
    - LCD SDA on pin A4
*/

#include <LiquidCrystal_I2C.h>

const byte SENSOR = 2;     // sensor on digital pin 2 (int0)
const byte BUTTON = 3;     // button on digital pin 3 (int1)
const byte ADDRESS = 0x27; // i2c address for the LCD (0x3F & 0x27 are common)

unsigned long count = 0;      // number of pulses from flow sensor (one pulse is 1/450th of a litre)
unsigned long last_count = 0; // timer to track last pulse from sensor
unsigned long last_time = 0;  // timer to track last LCD update
const int INTERVAL = 200;     // refresh rate for screen readout
const float COST_PER_LITRE = 0.2523; // cost per litre, in cents, from city website
unsigned long debounce = 0;   // timer for button debounce

bool mode_changed = true; // a flag to update the text label "Rate:" or "Cost:"
bool watering = false;    // a flag to indicate if the shower is running
bool mode = false;        // show flow rate if true, or cost if false

LiquidCrystal_I2C lcd(ADDRESS, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// The cent sign (¢) is not included in the default character set.
// Therefore, we must create a custom character.
// This was done with the help of this online tool:
// https://maxpromer.github.io/LCD-Character-Creator/
byte cent_sign[] = {
  B00100,
  B00100,
  B01111,
  B10100,
  B10100,
  B01111,
  B00100,
  B00100
};

void setup() {

  // Set pin functions & enable internal pull-up resistors
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);

  // Initialize interrupts
  attachInterrupt(digitalPinToInterrupt(SENSOR), sensor_pulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON), button_press, FALLING);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.createChar(0, cent_sign);
  lcd.backlight();
  lcd.clear();
  lcd.print("Total:");
}

void loop() {

  // if the mode has changed, print the updated text label
  if (mode_changed) {
    lcd.setCursor(0, 1); // move to second line
    if (mode)
      lcd.print("Rate:           "); // white space is to clear any extraneous characters
    else
      lcd.print("Cost:           "); // white space is to clear any extraneous characters
    mode_changed = false; // clear flag
  }

  // more than INTERVAL-time has passed, so update the screen
  if (abs(millis() - last_time) > INTERVAL) {
    last_time = millis(); // set last update time to present time
    lcd.setCursor(7, 0);
    lcd.print(String(count / 450.0) + " L"); // calcluate and print total litres
    lcd.setCursor(7, 1);
    if (mode) { // flow-rate mode, display current flow rate
      lcd.print(String(1000 * (count - last_count) / 450.0 / INTERVAL) + (" L/s   "));
    }
    else { // cost mode, display total cost
      lcd.print(String(COST_PER_LITRE * count / 450.0) + " ");
      lcd.write(byte(0)); // print cents sign (¢)
      lcd.print("   "); // print trailing spaces to clear extraneous chars
    }

    // this is how we determine if the shower is running
    if (last_count == count)
      watering = false; // shower is off
    else
      watering = true; // shower is on

    last_count = count;
  }

}

void sensor_pulse() {
  count++; // increase count by one
}

void button_press() {
  if (abs(millis() - debounce) > 250) { // debounce delay to prevent unwanted button presses
    debounce = millis(); // reset debounce timer
    if (watering) { // if shower is running, the button toggles the mode
      mode = !mode;
      mode_changed = true; // raise flag
    }
    else { // if the shower is off, the button clears the total
      count = 0;
      last_count = 0;
    }
  }
}
