#include "Funkuhr.h"                  // DCF77 receiver library
#include <Wire.h>                     // Standard Arduino library
#include <LiquidCrystal_I2C.h>        // LCD I2C library
#include <DS3231.h>                   // RTC library

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // LCD I2C interface address 0x27

Funkuhr dcf;                          // DCF77 receiver declaration
struct Dcf77Time dcfTime = {0};
byte syncOK;
byte resyncOK;
uint8_t curSec;

#define DS3232_I2C_ADDRESS 0x68
DS3231 clock;
RTCDateTime rtcTime;

const byte dayBegin = 8;              // Day begin
const byte dayEnd = 22;               // Day end

const byte resyncFlagReset = 22;      // Resync flag is off to allow RTC reset at night
const byte resyncRTCtime = 1;         // Time when RTC is reset based on DCF77

const byte pinBuzzer = 9;             // pin for Buzzer
const int buzzerDelay1 = 100;         // Chime short delay
const int buzzerDelay2 = 600;         // Chime long delay
bool chimeSent = false;               // Chime flag

void setup() {
  lcd.begin(16, 2);                   // LCD 2x16 initialization
  lcd.backlight();                    // LCD backlight ON

  dcf.init();                         // DCF77 receiver initialization

  clock.begin();                      // DS3231 initialization

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);            // Set buzzer - pin 9 as an output

  syncOK = false;
}

void loop() {
  dcf.getTime(dcfTime);               // reading data from DCF77
  rtcTime = clock.getDateTime();      // reading data from RTC

  if (!dcf.synced()) {
    syncOK = false;
    lcd.backlight();                  // LCD backlight ON
    lcd.setCursor(0, 0);
    lcd.print(" DCF77 decoding ");
    lcd.setCursor(0, 1);
    lcd.print(" in progress... ");
  } else if (rtcTime.day > 0) {
    digitalWrite(LED_BUILTIN, HIGH);
    if (rtcTime.second != curSec)  {
      rtcLCD();                       // Show RTC date/time on LCD
      LCD_backlight();                // LCD backlight OFF during the night
      RadioStopChime();               // Chime at full hour
    }
    curSec = rtcTime.second;
  }

  // Set RTC based on decoded DCF77 signal
  if (dcf.synced()) {                             // if DCF77 was decoded correctly
    if ((syncOK == false && dcfTime.day > 0))  {  // if RTC was not set yet and day is greater than 0 (date and time from DCF is available)
      rtcSet();                                   // set RTC
      syncOK = true;
      resyncOK = true;
    } else if (resyncOK == false && dcfTime.hour == resyncRTCtime && dcfTime.day > 0) { // if reset RTC flag is off next RTC reset will be performed at night
      rtcSet();                                   // set RTC
      resyncOK = true;
    }
  } else {
    syncOK = false;                               // if RTC was not set yest than sync flag is set to off
  }

  if (dcfTime.hour >= resyncFlagReset) {
    resyncOK = false;                             // Resync flag set to off to allow RTC set next night
  }
}

// -- Funkcje pomocnicze --

void print2digits(int number) {
  // Add 0 to single digit number to have two digits for all
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void rtcSet ()  {
  // Setting RTC based on DCF77 date and time
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" * DCF77 RCVD * ");
  delay(500);
  clock.setDateTime(dcfTime.year + 2000, dcfTime.month, dcfTime.day, dcfTime.hour, dcfTime.min, dcfTime.sec);
  lcd.setCursor(0, 1);
  lcd.print(" ** RTC SET! ** ");
  delay(2000);
  lcd.clear();
}

void rtcLCD ()  {
  // Display date, time and temperature on LCD
  lcd.setCursor(0, 0);
  print2digits(rtcTime.hour);
  lcd.write(':');
  print2digits(rtcTime.minute);
  lcd.write(':');
  print2digits(rtcTime.second);

  clock.forceConversion();
  lcd.setCursor(9, 0);
  lcd.print(clock.readTemperature());
  lcd.print((char) 223);
  lcd.print("C");

  lcd.setCursor(3, 1);
  print2digits(rtcTime.day);
  lcd.write('.');
  print2digits(rtcTime.month);
  lcd.write('.');
  lcd.print(rtcTime.year);

  if (resyncOK == true)  {
    lcd.setCursor(0, 1);    // Second row, first character shows
    lcd.write('*');         // * if RTC reset at night was performed
  } else {
    lcd.setCursor(0, 1);
    lcd.write(' ');         // or nothing if RTC was not set at night (* mark is remove at 11pm)
  }
  
  if (dcf.synced())  {      // If synced, but it is always true after first sync and no change when DCF receiver is unplugged
    lcd.setCursor(15, 1);   // Second row, last character shows
    lcd.write('*');         // * if DCF77 is in sync
  } else {
    lcd.setCursor(15, 1);
    lcd.write(' ');         // or nothing if not sync
  }
}

void LCD_backlight()  {
  // Disabling LCD bakcklight during the night
  if (rtcTime.hour >= dayBegin && rtcTime.hour <= dayEnd) {
    lcd.backlight();
  } else {
    lcd.noBacklight();
  }
}


void RadioStopChime() {
  // Full hour chime
  if (rtcTime.hour >= dayBegin && rtcTime.hour <= dayEnd) {     // Night time silence
    if (rtcTime.minute == 0 && chimeSent == false) {             // if minute is 0 and no beep
      for (int i = 0; i < 3; i++) {                       // play chime sound 3 times at full hour
        tone(pinBuzzer, 1160);
        delay(buzzerDelay1);
        tone(pinBuzzer, 1400);
        delay(buzzerDelay1);
        tone(pinBuzzer, 1670);
        delay(buzzerDelay1);
        noTone(pinBuzzer);
        delay(buzzerDelay2);
      }
      chimeSent = true;                                     // Chime was played
    }

    if (rtcTime.minute > 0) {                               // If minute is greater than 0
      chimeSent = false;                                    // Chime flag was set to "not played"
    }
  }
}
