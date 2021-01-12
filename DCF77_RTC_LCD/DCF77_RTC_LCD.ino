#include "Funkuhr.h"                  // DCF77 receiver library https://github.com/fiendie/Funkuhr
#include <Wire.h>                     // Standard Arduino library
#include <LiquidCrystal_I2C.h>        // LCD I2C library
#include <DS3231.h>                   // RTC library

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // LCD I2C interface address 0x27

Funkuhr dcf(0, 2, 13, false);          // DCF77 receiver declaration (Int, CDFpin, LEDpin, invertSig)

struct Dcf77Time dt = {0};
byte syncOK = false;
byte resyncOK = false;
uint8_t curSec;

#define DS3232_I2C_ADDRESS 0x68
DS3231 clock;
RTCDateTime rtcTime;

const byte dayBegin = 8;              // Day begin
const byte dayEnd = 22;               // Day end

byte resyncFlagResetDone = false;     // Resync flag set
const byte resyncFlagReset = 21;      // Resync flag is off to allow RTC reset at night
const byte resyncRTCtime = 22;        // Time when RTC is reset based on DCF77

const byte pinLED = 13;               // pin for DCF77 signal LED
const byte pinBuzzer = 9;             // pin for Buzzer
const int buzzerDelay1 = 100;         // Chime short delay
const int buzzerDelay2 = 600;         // Chime long delay
bool chimeSent = false;               // Chime flag

void setup() {
  lcd.begin(16, 2);                   // LCD 2x16 initialization
  lcd.backlight();                    // LCD backlight ON

  dcf.init();                         // DCF77 receiver initialization

  clock.begin();                      // DS3231 initialization

  pinMode(pinLED, OUTPUT);            // DCF77 bit receive indication LED
  pinMode(pinBuzzer, OUTPUT);         // Set buzzer - pin 9 as an output
}

void loop() {
  dcf.getTime(dt);               // reading data from DCF77
  rtcTime = clock.getDateTime();      // reading data from RTC

  if (!dcf.synced()) {                // if DCF77 is not synced
    syncOK = false;
    lcd.backlight();                  // LCD backlight ON
    lcd.setCursor(0, 0);
    lcd.print(" DCF77 decoding ");
    lcd.setCursor(0, 1);
    lcd.print(" in progress... ");
  } else {                                        // if DCF77 is synced
    if (syncOK == false && dt.day > 0)  {    // if RTC was not set yet and day is greater than 0 (date and time from DCF is available)
      rtcSet();                                   // set RTC
      syncOK = true;
      resyncOK = true;
      resyncFlagResetDone = false;
    } 
    if (resyncOK == false && resyncFlagResetDone == true && dt.hour == resyncRTCtime && dt.day > 0) { // if reset RTC flag is off next RTC reset will be performed at night
      rtcSet();                                   // set RTC
      resyncOK = true;
      resyncFlagResetDone = false;               // Resync flag was already set
    }

    if (rtcTime.second != curSec)  {
      // Show RTC date and time
      rtcLCD();                       // Show RTC date/time on LCD
      statusMark();                   // Show status marks * on LCD
      LCD_backlight();                // LCD backlight OFF during the night
      RadioStopChime();               // Chime at full hour
    }
    curSec = rtcTime.second;
  }

  if (dt.hour == resyncFlagReset && resyncFlagResetDone == false) {
    resyncOK = false;                 // Resync flag is disabled to allow RTC set next night
    resyncFlagResetDone = true;       // Resync flag was already reset
  }
}


// -- Funkcje pomocnicze --

void rtcSet ()  {
  // Setting RTC based on DCF77 date and time
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("** DCF77 RCVD **");
  delay(500);
  clock.setDateTime(dt.year + 2000, dt.month, dt.day, dt.hour, dt.min, dt.sec);
  lcd.setCursor(0, 1);
  lcd.print("*** RTC SET! ***");
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
}

void print2digits(byte number) {
  // Add 0 to single digit number to have two digits
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void statusMark() {
  // Use * asterisk mark on LCD to indicate sync and resync status
  if (resyncOK == true)  {
    lcd.setCursor(0, 1);    // Second row, first character shows
    lcd.write('*');         // * if RTC reset at night was performed
  } else {
    lcd.setCursor(0, 1);
    lcd.write(' ');
  }
  if (resyncFlagResetDone == true)  {
    lcd.setCursor(1, 1);    // Second row, second character shows
    lcd.write('*');         // * if RTC reset flag was set to allow RTC reset next time (next night)
  } else {
    lcd.setCursor(1, 1);
    lcd.write(' ');
  }

    lcd.setCursor(14, 1);   // Second row, last character shows
    lcd.print(dcf.synced());         // * if DCF77 is in sync

  if (dcf.synced())  {      // If DCF77 is synced (But it is always true after first sync and no change when DCF receiver is unplugged)
    lcd.setCursor(15, 1);   // Second row, last character shows
    lcd.write('*');         // * if DCF77 is in sync
  } else {
    lcd.setCursor(15, 1);
    lcd.write(' ');
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
    if (rtcTime.minute == 0 && chimeSent == false) {            // if minute is 0 and no beep
      for (int i = 0; i < 3; i++) {                             // play chime sound 3 times at full hour
        tone(pinBuzzer, 1160);
        delay(buzzerDelay1);
        tone(pinBuzzer, 1400);
        delay(buzzerDelay1);
        tone(pinBuzzer, 1670);
        delay(buzzerDelay1);
        noTone(pinBuzzer);
        delay(buzzerDelay2);
      }
      chimeSent = true;                                         // Chime was played
    }

    if (rtcTime.minute > 0) {                                   // If minute is greater than 0
      chimeSent = false;                                        // Chime flag was set to "not played"
    }
  }
}
