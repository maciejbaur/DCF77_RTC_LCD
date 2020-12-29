#include "Funkuhr.h"            // biblioteka dekodera sygnalu DCF77
#include <Wire.h>               // standardowa biblioteka Arduino
#include <LiquidCrystal_I2C.h>  // dolaczenie pobranej biblioteki I2C dla LCD
#include <DS3231.h>             // biblioteka modułu RTC


LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Ustawienie adresu ukladu na 0x27

Funkuhr dcf;  // definicja zegara
struct Dcf77Time dt = {0};
byte syncOK;
uint8_t curSec;

#define DS3232_I2C_ADDRESS 0x68
DS3231 clock;
RTCDateTime tm;

const byte buzzer = 9;          // buzzer to arduino pin 9
const int buzzerDelay1 = 100;   // przerwa w pikaniu
const int buzzerDelay2 = 600;   // przerwa miedzy sekwencja pikania
bool piknelo = false;           // flaga czy piknelo

void setup() {
  lcd.begin(16, 2);   // Inicjalizacja LCD 2x16
  lcd.backlight();    // zalaczenie podwietlenia

  dcf.init();         // inicjalizacja odbiornika DCF77

  clock.begin();      // Inicjalizacja DS3231

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buzzer, OUTPUT); // Set buzzer - pin 9 as an output
  
  syncOK = false;
}

void loop() {
  dcf.getTime(dt);          // sprawdzenie sygnału z radia
  tm = clock.getDateTime(); // odczyt danych z RTC

  if (!dcf.synced()) {
    syncOK = false;
    lcd.backlight();    // zalaczenie podwietlenia
    lcd.setCursor(0, 0);
    lcd.print(" DCF77 decoding ");
    lcd.setCursor(0, 1);
    lcd.print(" in progress... ");
  } else if (tm.day > 0) {
    digitalWrite(LED_BUILTIN, HIGH);
    if (tm.second != curSec)  {
      rtcLCD();           // Wyswietlanie danych RTC na LCD
      podswietlenie();    // podswietlenie wylaczane w nocy
      RadioStopChime();          // pikanie na pelna godzine
    }
    curSec = dt.sec;
  }

  if (syncOK == false && dt.day > 0)  {
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print(" * DCF77 RCVD * ");
    delay(500);
    clock.setDateTime(dt.year + 2000, dt.month, dt.day, dt.hour, dt.min, dt.sec);
    lcd.setCursor(0, 1);
    lcd.print(" ** RTC SET! ** ");
    delay(2000);
    lcd.clear();
    syncOK = true;
  }
}

// -- Funkcje pomocnicze --

// Uzupelnianie do liczb dwucyfrowych na potrzeby wyswietlacza
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void rtcLCD ()  {
  //tm = clock.getDateTime();

  lcd.setCursor(0, 0); // Ustawienie kursora w pozycji 0,0 (pierwszy wiersz, pierwsza kolumna)
  print2digits(tm.hour);
  lcd.write(':');
  print2digits(tm.minute);
  lcd.write(':');
  print2digits(tm.second);
  //lcd.print(" ");

  clock.forceConversion();
  lcd.setCursor(9, 0); //Ustawienie kursora w pozycji 0,12 (drugi wiersz, 12 kolumna)
  lcd.print(clock.readTemperature());
  lcd.print((char) 223);
  lcd.print("C");

  lcd.setCursor(3, 1); //Ustawienie kursora w pozycji 0,1 (drugi wiersz, pierwsza kolumna)
  //lcd.print("   ");
  print2digits(tm.day);
  lcd.write('.');
  print2digits(tm.month);
  lcd.write('.');
  lcd.print(tm.year);
  //lcd.print("   ");

}

void podswietlenie()  {
  if (tm.hour >= 8 && tm.hour <= 22) {    // wylaczenie podswietlenia od 23 do 8
    lcd.backlight();                      // zalaczenie podwietlenia
  } else {
    lcd.noBacklight();                    // wylaczenie podwietlenia
  }
}

void RadioStopChime() {
  if (tm.minute == 0 && piknelo == false) { // jesli minuta jest rowna 0 i jeszcze nie piknelo
    for (int i = 0; i < 3; i++) {           // dwa pikniecia
      tone(buzzer, 1160);
      delay(buzzerDelay1);
      tone(buzzer, 1400);
      delay(buzzerDelay1);
      tone(buzzer, 1670);
      delay(buzzerDelay1);
      noTone(buzzer);
      delay(buzzerDelay2);
    }
    piknelo = true;                         // flaga, ze piknelo
  }
  
  if (tm.minute > 0) {                      // jesli minuta jest wieksza od 0
    piknelo = false;                        // flaga, ze nie piknelo
  }
}
