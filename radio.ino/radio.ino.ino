
/*

 N6QW / KK6FUT Super Simple DDS - Si5351 version

 2015 Feb 26


 This is a super simple DDS sketch to run a single band AD9850-based DDS.
 Kept super-super simple to make it easy for the non-software folks.


 Inputs: Rotary encoder
 Outputs: LCD display, DDS frequency

 Uses a a (16*2) LCD Display on the I2C Interface for TwRobot.
 Using the Si5351 breakout board from Adafruit.



 Changes:

 2015 Feb 26 - shifted default radix to 1000 Hz, initial frequency to General

 2915 March 2

 Switched order of USB/LSB

 Added full USB/LSB to display

 Only showed 5 decimal places

 Changed the radix step function

 Set BFO to 4.913700 and 4.916700

 added lcd.init();

 added lcd.backlight();



 */

#include<stdlib.h>
#include <Wire.h>
#include "DS1307.h"
#include <LiquidCrystal_I2C.h> //twrobot
#include <si5351.h>
#define IF           0L

DS1307 clock; // Define a clock object of the DS1307 class.
Si5351 si5351;

boolean keystate = 0;
boolean changed_f = 0;
int old_vfo = 0;
int old_bfo = 0;
int val = 0; // used for dip
const uint32_t bandStart = 7000000L;  // start of 40m
const uint32_t bandEnd =   7300000L; // end of 40m
const uint32_t bandInit =  7175000L; // where to initially set the frequency
const uint32_t offset =    4915200L; // amount to add for IF offset
const uint32_t USB = 4916700L;

//const uint32_t LSB = 4916140L; // great clarity, but muffled signal

//const uint32_t LSB = 4915040L; // louder

//const uint32_t LSB = 4914540L; // louder but a little rattle

const uint32_t LSB = 4913700L; // another try at BFO

volatile uint32_t freq = bandInit ;  // this is a variable (changes) - set it to the beginning of the band
volatile uint32_t vfo = bandInit + LSB ; // this is a variable (changes) - set it to the beginning of the band
volatile uint32_t bfo = LSB; // or LSB later make it selectable with the SSB Select Switch
volatile uint32_t radix = 1000; // how much to change the frequency by, clicking the rotary encoder will change this.

int blinkpos = 3; // position for blinking cursor


// Set pins for ROTARY ENCODER

const int RotEncAPin = 10;
const int RotEncBPin = 11;
const int RotEncSwPin = A3;

const int SW = 9; //selects upper or lower sideband

// Display library assumes use of A4 for clock, A5 for data. No code needed.

// Variables for Rotary Encoder handling

boolean OldRotEncA = true;
boolean RotEncA = true;
boolean RotEncB = true;
boolean RotEncSw = true;
boolean oldSideband = false;
boolean newSideband = false;

// Instantiate the LCD display...

//LiquidCrystal_I2C lcd(0x027, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address TwRobot

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup() {

  Serial.begin(9600);
  
  // Setup DS1307 RTC
  clock.begin();
  clock.fillByYMD(2013,1,19);//Jan 19,2013
  clock.fillByHMS(15,28,30);//15:28 30"
  clock.fillDayOfWeek(SAT);//Saturday
  clock.setTime();//write time to the RTC chip
  // Set up LCD

//  lcd.begin(20, 4);  // initialize the lcd for 16 chars 2 lines, turn on backlight TwRobot
  lcd.init();

  // Print a message to the LCD.

  lcd.backlight();
  lcd.setCursor(0, 1);
  lcd.print(" N6QW & KK6FUT");



  // Set up ROTARY ENCODER

  pinMode(RotEncAPin, INPUT);

  pinMode(RotEncBPin, INPUT);

  pinMode(RotEncSwPin, INPUT);

  // set up pull-up resistors on inputs...

  digitalWrite(RotEncAPin, HIGH);

  digitalWrite(RotEncBPin, HIGH);

  digitalWrite(RotEncSwPin, HIGH);



  pinMode(SW, INPUT);   // Selects either USB or LSB````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````[

  digitalWrite(SW, HIGH);

  newSideband = digitalRead(SW);

  lcd.setCursor(13, 0); // to the right of the frequency

  if (newSideband) { //********LSB

    bfo = LSB;

    lcd.print("LSB");

  } else {               // USB

    bfo = USB;

    lcd.print("USB");

  }



  // Start serial and initialize the Si5351

  si5351.init(SI5351_CRYSTAL_LOAD_10PF);

  // si5351.set_correction(100);

  delay(1000);

  // Set CLK0 to output vfo  frequency with a fixed PLL frequency

  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);

  si5351.set_freq(vfo , SI5351_PLL_FIXED, SI5351_CLK0);

  si5351.set_freq(bfo , 0, SI5351_CLK2);

  //set power

  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);

  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA );

  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);



  // start the oscillator...

  send_frequency(freq);

  display_frequency(freq);



}



void loop() {



  // Read the inputs...

  RotEncA = digitalRead(RotEncAPin);

  RotEncB = digitalRead(RotEncBPin);

  RotEncSw = digitalRead(RotEncSwPin);

  newSideband = digitalRead(SW);
  printTime();

  if (newSideband != oldSideband) {

    lcd.setCursor(13, 0); // to the right of the frequency

    if (newSideband) { //********LSB

      bfo = LSB;

      lcd.print("LSB");

    } else {               // USB

      bfo = USB;

      lcd.print("USB");

    }

    si5351.set_freq( bfo, 0, SI5351_CLK2);

    oldSideband = newSideband;

  }

  if (Serial.available()) {

    // if (0) {

    int c = Serial.read();

    if (c == ',') {

      Serial.print(" increase bfo:  ");

      bfo = bfo + 05L;

      freq = freq + 05L;

    } else if (c == '.') {

      Serial.print(" decrease bfo:  ");

      bfo = bfo - 05L;

      freq = freq - 05L;

    }

    Serial.print(bfo);

    send_frequency(freq);  // set the DDS to the new frequency

  }



  // check the rotary encoder values

  if ((RotEncA == HIGH) && (OldRotEncA == LOW)) {

    // adjust frequency

    if (RotEncB == LOW) {

      freq = constrain(freq + radix, bandStart, bandEnd);

    } else {

      freq = constrain(freq - radix, bandStart, bandEnd);

    }

    OldRotEncA = RotEncA; // store rotary encoder position for next go around



    // Now, update the LCD with frequency

    display_frequency(freq); // push the frequency to LCD display

    send_frequency(freq);  // set the DDS to the new frequency

    // delay(400); // let the frequency/voltages settle out after changing the frequency

  }



  // check for the click of the rotary encoder

  if (RotEncSw == LOW) {

    // if user clicks rotary encoder, change the size of the increment

    // use an if then loop, but this could be more elegantly done with some shifting math



    if (radix == 10000) {

      radix = 1000;

    } else if (radix == 1000) {

      radix = 100;

    } else if (radix == 100) {

      radix = 10;

    } else if (radix == 10) {

      //   radix = 1;

      //} else if (radix == 1) {

      radix = 10000;

    } else { // this is either 100 or we goofed somewhere else, so set it back to the big change

      radix = 10000;

    }

  }



  OldRotEncA = RotEncA;

  // End of loop()

}







// subroutine to display the frequency...

void display_frequency(uint32_t frequency) {

  lcd.noBlink();

  lcd.setCursor(0, 0); //was 17

  if (frequency < 10000000) {

    lcd.print(" ");

  }

  lcd.print(frequency / 1e6, 5);

  lcd.print(" MHz");



}





// Subroutine to generate a positive pulse on 'pin'...

void pulseHigh(int pin) {

  digitalWrite(pin, HIGH);

  digitalWrite(pin, LOW);

}



// calculate and send frequency code to Si5351...

void send_frequency(uint32_t frequency) {



  vfo = frequency + LSB;

  si5351.set_freq(vfo , SI5351_PLL_FIXED, SI5351_CLK0);

  si5351.set_freq(bfo , 0, SI5351_CLK2);

  Serial.print("freq: ");

  Serial.print(frequency);

  Serial.print(" vfo: ");

  Serial.print(vfo);

  Serial.print(" bfo:  ");

  Serial.print(bfo);

  Serial.print(" vfo-bfo: ");

  Serial.print(vfo - bfo);

  Serial.println("  ");
}

/*Function: Display time on the serial monitor*/
void printTime(){
	clock.getTime();
        lcd.setCursor(0, 1); 
	lcd.print(clock.hour, DEC);
	lcd.print(":");
	lcd.print(clock.minute, DEC);
	lcd.print(":");
	lcd.print(clock.second, DEC);
//	lcd.print("	");
//	lcd.print(clock.month, DEC);
//	lcd.print("/");
//	lcd.print(clock.dayOfMonth, DEC);
//	lcd.print("/");
//	lcd.print(clock.year+2000, DEC);
//	lcd.print(" ");
//	lcd.print(clock.dayOfMonth);
//	lcd.print("*");
//	switch (clock.dayOfWeek)// Friendly printout the weekday
//	{
//		case MON:
//		  lcd.print("MON");
//		  break;
//		case TUE:
//		  lcd.print("TUE");
//		  break;
//		case WED:
//		  lcd.print("WED");
//		  break;
//		case THU:
//		  lcd.print("THU");
//		  break;
//		case FRI:
//		  lcd.print("FRI");
//		  break;
//		case SAT:
//		  lcd.print("SAT");
//		  break;
//		case SUN:
//		  lcd.print("SUN");
//		  break;
//	}
//	lcd.println(" ");
}










