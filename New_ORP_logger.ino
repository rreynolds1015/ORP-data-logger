
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
#include <SoftwareSerial.h>
#include <Time.h>

// Defines
// Cursor positions for date and time
#define DATE_ROW 0
#define DATE_COL 11
#define TIME_ROW 0
#define TIME_COL 0

// Pins
#define BUTTON_ADC      A0
#define BACKLIGHT       10

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

// Variables for ORP and data logger
const byte rx = 2;
const byte tx = 3;
const byte logPin = 12;
SoftwareSerial orpSerial (rx,tx);
char sensor_data[20];
float ORP = 0;
float ORPvalue = 0;
float avgORP = 0;
byte string_received = 0;
boolean logData=false;
unsigned long nextDataPoint;
long logFreq = 5;
byte received_from_sensor = 0;
byte counter = 1;

// Variables for debugging with PC
byte pc_debug = 1;
byte received_from_computer = 0;
char computerdata[20];

time_t syncProvider() { return rtc.now().unixtime(); }

// Setup section for Arduino framework
void setup()
{
 // Start the I2C functions for the RTC
 Wire.begin();
  
 // Set button input
 pinMode( BUTTON_ADC, INPUT );
 digitalWrite( BUTTON_ADC, LOW );      // Ensure the internal pullup is off
 
 // Set the pin for supplying power to the OpenLog board
 pinMode( logPin, OUTPUT );
 digitalWrite( logPin, LOW);
 
 // Turn on the LCD backlight
 pinMode( BACKLIGHT, OUTPUT );
 BACKLIGHT_ON();
 
 // Initialize the RTC
 rtc.begin();
 setSyncProvider(syncProvider);
 
 // Start the on-board serial and the software serial
 Serial.begin(38400);
 orpSerial.begin(38400);
 
 // Ensure the ORP circuit is not in continuous mode
 orpSerial.print("e\r");
 delay(50);
 // Write the command a second time to make sure we're out of continuous mode
 orpSerial.print("e\r");
 delay(50);
 
 // Start the LCD and print a quick message
 lcd.begin(16,2);
 lcd.print("Starting up...");
 
}

// Function that looks for events on the serial interface.  Easiest method to 
// send calibration events to the Atlas ORP
void serialEvent()
{
  if (pc_debug == 1)
  {
    received_from_computer = Serial.readBytesUntil(13,computerdata,20);
    computerdata[received_from_computer] = 0;
    orpSerial.print(computerdata);
    orpSerial.print('\r');
  }
}

// Function to print a preceding 0 to the LCD for date/time less than 10
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

// Function to determine which button was pressed
byte ReadButtons()
{
 unsigned int buttonVoltage;
 byte button = BUTTON_NONE;
 
 // Read the button ADC pin voltage
 buttonVoltage = analogRead( BUTTON_ADC );
 // Is the voltage within a valid range
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

// Function to write data to the SD card via the serial port
void LogData(float ORPvalue, DateTime *now)
{
  if (now->month()<10) Serial.print('0');
  Serial.print(now->month());
  Serial.print('/');
  if (now->day()<10) Serial.print('0');
  Serial.print(now->day());
  Serial.print('/');
  Serial.print(now->year());
  
  Serial.print(',');
  if (now->hour()<10) Serial.print('0');
  Serial.print(now->hour());
  Serial.print(':');
  if (now->minute()<10) Serial.print('0');
  Serial.print(now->minute());
  Serial.print(':');
  if (now->second()<10) Serial.print('0');
  Serial.print(now->second());
  Serial.print(',');
  
  Serial.print(ORPvalue,2);
  Serial.print('\n');
}

void loop()
{
  // Start with a fresh canvas every time
  lcd.clear();
  
  // Get the time for this cycle and print it to the LCD
  DateTime now = rtc.now();
  print_date (&now);
  print_time (&now);
  
  // Continually look for a button press event and react to it
  byte button;
  button = ReadButtons();
  
  // On a button press event, clear the area of the LCD for feedback
  if ( buttonJustPressed || buttonJustReleased )
  {
    lcd.setCursor(0,1);
    lcd.print( "       " );
  }
  
  // What's the status of the button press
  switch( button )
  {
    case BUTTON_NONE:
    { break; }
    case RIGHT:
    {
      startCalibration();
      break;
    }
    case UP:
    {
      /* 
      Insert code to allow PC debugging to be turned on/off from the main screen
      but need a physical sign for the operator to know if it's on or off
      */
      // pc_debug = !pc_debug
      break;
    }
    case DOWN:
    { break; }
    case LEFT:
    {
      setLogFreq();
      break;
    }
    case SELECT:
    {
      logData = !logData;
      if (logData == true)
      { 
        digitalWrite(logPin, HIGH);
        LogData(ORPvalue,&now);
        nextDataPoint = now.unixtime() + logFreq;
      }
      else
      { digitalWrite(logPin, LOW); }
      break;
    }
    default:
    { break; }
    
    // Reset the flags for if a button was pressed
    if ( buttonJustPressed )
      buttonJustPressed = false;
    if ( buttonJustReleased )
      buttonJustReleased = false;
   }
   
   // We want to average the ORP data coming in
   // Averaging 3 points at 320 ms/point gives a response every ~1 second
   if (counter < 4)
   {
     avgORP = (avgORP + getORPdata()) / counter;
     counter ++;
   }
   else 
   {
     counter = 1;
     ORPvalue = avgORP; 
   }
   
   byte orpPos;
   if (abs((ORPvalue / 1000)) >= 1)
     orpPos = 9;
   else if (abs((ORPvalue / 100)) >= 1)
     orpPos = 10;
   else if (abs((ORPvalue / 10)) >= 1)
     orpPos = 11;
   else
     orpPos = 12;
   
   if (ORPvalue < 0) orpPos = orpPos--;
   
   lcd.setCursor(orpPos,1);
   lcd.print(ORPvalue,0);
   lcd.setCursor(14,1);
   lcd.print("mV");
  
  // Check if we should be logging data to the SD card
  if (logData == true)
  {
    lcd.setCursor(0,1);
    lcd.print("Logging");
    // Okay, we should be logging data, but is it the time for the next datapoint?
    if (now.unixtime() >= nextDataPoint)
    {
      LogData(ORPvalue,&now);
      nextDataPoint = now.unixtime() + logFreq;
    }
  }
  delay(320);
}

// Function to get (and return) the value from the ORP sensor
float getORPdata()
{
  if (orpSerial.available() > 0)
  {
    received_from_sensor=orpSerial.readBytesUntil(13,sensor_data,20);
    sensor_data[received_from_sensor]=0;
    string_received=1;
  }

  orpSerial.print("R\r");
  if (string_received==1)
  {
    ORP=atof(sensor_data);
/*    byte orpPos = (13 - strlen(sensor_data));
    lcd.setCursor(orpPos,1);
    lcd.print(ORP,0);
    lcd.setCursor(14,1);
    lcd.print("mV");  */
  }
  return ORP;
  string_received = 0;
}

// Function to send calibration commands to the ORP board. May still be buggy.
void startCalibration()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Calibration");
  lcd.setCursor(0,1);
  lcd.print("LFT=EXIT");
  orpSerial.print("C\r");
  byte button;
  do 
  {
    if (counter < 4)
    {
      avgORP = (avgORP + getORPdata()) / counter;
      counter ++;
    }
    else 
    {
      counter = 1;
      ORPvalue = avgORP; 
    }
   
    byte orpPos;
    if (abs((ORPvalue / 1000)) >= 1)
      orpPos = 9;
    else if (abs((ORPvalue / 100)) >= 1)
      orpPos = 10;
    else if (abs((ORPvalue / 10)) >= 1)
      orpPos = 11;
    else
      orpPos = 12;
   
    if (ORPvalue < 0) orpPos = orpPos--;
   
    lcd.setCursor(orpPos,1);
    lcd.print(ORPvalue,0);
    lcd.setCursor(14,1);
    lcd.print("mV");
    
    
    button = ReadButtons();
    if (button == UP) orpSerial.print("+\r");
    if (button == DOWN) orpSerial.print("-\r");
    if (button == SELECT) orpSerial.print("X\r");
    delay(320);
  }
  while (button != LEFT);
  orpSerial.print("E\r");
}

// Function to use the button shield to set the frequency for data logging.
void setLogFreq()
{ 
  byte button;
  do
  {
    button = ReadButtons();
    if (button == UP) logFreq = logFreq + 5;
    if (button == DOWN) logFreq = logFreq - 5;
    if (logFreq < 5) logFreq = 5;
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Set log freq");
    lcd.setCursor(0,1);
    lcd.print("RT=EXIT");
    lcd.setCursor(13,1);
    lcd.print("sec");
    if (logFreq < 10)
    { lcd.setCursor(11,1); }
    else if (logFreq < 100)
    { lcd.setCursor(10,1); }
    else
    { lcd.setCursor(9,1); }
    lcd.print(logFreq);
    delay(200);
  }
  while (button != RIGHT);
}
