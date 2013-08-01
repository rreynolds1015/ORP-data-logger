
/* Sketch to use the LCD shield + push buttons for data monitoring and logging

Pins used by the LCD + Keypad Shield:

  A0: Buttons, analog input from voltage ladder
  D4: LCD bit 4
  D5: LCD bit 5
  D6: LCD bit 6
  D7: LCD bit 7
  D8: LCD RS
  D9: LCD E
  D10: LCD Backlight
  
  The instantiation of the object uses the command:
  LiquidCrystal <variable name>(rs,e,d4,d5,d6,d7)
  i.e., 'LiquidCrystal lcd(8, 9, 4, 5, 6, 7)
  
ADC voltages seen on A0 for the 5 buttons:

  Button        Voltage        8bit          10bit
  __________    __________     _________     __________
  RIGHT         0.00V          0             0
  UP            0.71V          36            145
  DOWN          1.61V          82            329
  LEFT          2.47V          126           505
  SELECT        3.62V          184           741
  
*/

// Includes
#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>

// Defines
// Cursor positions for date and time
#define DATE_ROW 0
#define DATE_COL 11
#define TIME_ROW 0
#define TIME_COL 0

// Pins
#define BUTTON_ADC      A0
#define BACKLIGHT       10
#define rx               2
#define tx               3

// Define button pushes
#define RIGHT_10BIT      0
#define UP_10BIT       145
#define DOWN_10BIT     329
#define LEFT_10BIT     505
#define SELECT_10BIT   741
#define HYSTERESIS      10

// Define button values
#define BUTTON_NONE      0
#define RIGHT            1
#define UP               2
#define DOWN             3
#define LEFT             4
#define SELECT           5

// Fancy macros
#define BACKLIGHT_OFF()    digitalWrite( BACKLIGHT, LOW)
#define BACKLIGHT_ON()     digitalWrite( BACKLIGHT, HIGH)

// Variable definitions
RTC_DS1307 rtc;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Variables for button events
byte buttonJustPressed = false;
byte buttonJustReleased = false;
byte buttonWas = BUTTON_NONE;

// Setup section for Arduino framework
void setup()
{
 // Start the I2C functions for the RTC
 Wire.begin();
  
 // Set button input button
 pinMode( BUTTON_ADC, INPUT);
 digitalWrite( BUTTON_ADC, LOW);      // Ensure the internal pullup is off
 
 // Turn on the LCD backlight
 pinMode( BACKLIGHT, OUTPUT);
 BACKLIGHT_ON();
 
 // Initialize the RTC
 rtc.begin();
 
 // Start the LCD and print a quick message
 lcd.begin(16,2);
 lcd.print("Starting up...");
 delay(1000);
}


// Function to print a preceding 0 for values less than 10
void print_padded_digit(int value)
{
  if (value < 10) {
    lcd.print('0');
  }
  lcd.print(value,DEC);
}

// Function to print the date to the LCD
void print_date(DateTime *now)
{
  lcd.setCursor(DATE_COL,DATE_ROW);
  print_padded_digit(now->month());
  lcd.print('/');
  print_padded_digit(now->day());
}

// Function to print the time to the LCD
void print_time(DateTime *now)
{
  lcd.setCursor(TIME_COL, TIME_ROW);
  print_padded_digit(now->hour());
  lcd.print(':');
  print_padded_digit(now->minute());
  lcd.print(':');
  print_padded_digit(now->second());
}

byte ReadButtons()
{
 unsigned int buttonVoltage;
 byte button = BUTTON_NONE;
 
 // Read the button ADC pin voltage
 buttonVoltage = analogRead( BUTTON_ADC );
 // Is the voltage within a valid range - *** Conisder if this can be a switch instead***
 if (buttonVoltage < ( RIGHT_10BIT + HYSTERESIS ))
 {
   button = RIGHT;
 }
 else if(    buttonVoltage >= ( UP_10BIT - HYSTERESIS )
          && buttonVoltage <= ( UP_10BIT + HYSTERESIS ))
 {
   button = UP;
 }
 else if(    buttonVoltage >= ( DOWN_10BIT - HYSTERESIS )
          && buttonVoltage <= ( DOWN_10BIT + HYSTERESIS ))
 {
   button = DOWN; 
 }
 else if(    buttonVoltage >= ( LEFT_10BIT - HYSTERESIS )
          && buttonVoltage <= ( LEFT_10BIT + HYSTERESIS ))
 {
   button = LEFT;
 } 
 else if(    buttonVoltage >= ( SELECT_10BIT - HYSTERESIS )
          && buttonVoltage <= ( SELECT_10BIT + HYSTERESIS ))
 {
   button = SELECT;
 }
 
 // Handle button flags for just pressed and just released events
 if ( (buttonWas == BUTTON_NONE ) && (button != BUTTON_NONE ) )
 {
   buttonJustPressed = true;
   buttonJustReleased = false;
 }
 if ( (buttonWas != BUTTON_NONE ) && (button == BUTTON_NONE ) )
 {
   buttonJustPressed = false;
   buttonJustReleased = true;
 }
 
 buttonWas = button;
 
 return(button);
 
}

void printButton (String buttonText)
{
  lcd.setCursor(0,1);
  lcd.print(buttonText);
}

void loop()
{
  lcd.clear();
  DateTime now = rtc.now();
  
  print_date (&now);
  print_time (&now);
  
  byte button;
  button = ReadButtons();
  if ( buttonJustPressed || buttonJustReleased )
  {
    lcd.setCursor(0,1);
    lcd.print( "       " );
  }
  switch( button )
  {
    case BUTTON_NONE:
    {
      break;
    }
    case RIGHT:
    {
      printButton("RIGHT");
      break;
    }
    case UP:
    {
      printButton("UP");
      break;
    }
    case DOWN:
    {
      printButton("DOWN");
      break;
    }
    case LEFT:
    {
      printButton("LEFT");
      break;
    }
    case SELECT:
    {
      printButton("SELECT");
      BACKLIGHT_OFF();
      delay(150);
      BACKLIGHT_ON();
      delay(150);
      break;
    }
    default:
    {
      break;
    }
    
    if ( buttonJustPressed )
      buttonJustPressed = false;
    if ( buttonJustReleased )
      buttonJustReleased = false;
  }
  
  
  lcd.setCursor(14,1);
  lcd.print("mV");
  lcd.setCursor(9,1);
  lcd.print("0000");
  
  delay(100);
}
