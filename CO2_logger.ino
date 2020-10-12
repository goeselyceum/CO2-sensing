#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>

hd44780_I2Cexp lcd; // declare lcd object: auto locate & auto config expander chip

// LCD geometry
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

RTC_DS3231 rtc;

const int chipSelect = 10;

const byte ABCenable[9]     = {0xFF, 0x01, 0x79, 0xA0, 0x00, 0x00, 0x00, 0x00, 0xE6};
const byte ABCdisable[9]    = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
const byte zeroPoint[9]     = {0xFF, 0x01, 0X87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};
const byte requestReading[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte result[9];

int incomingByte;

int uur = 0;
int minuut = 0;
int seconde = 0;

int counter = 0;

int currentMinute = 0;
int previousMinute = 0;
int currentSecond = 0;
int previousSecond = 0;
bool logFlag = false;



void setup()  {
  Serial.begin(9600);
  Serial1.begin(9600);
  int status;
  status = lcd.begin(LCD_COLS, LCD_ROWS);
  if (status) {
    hd44780::fatalError(status); // does not return
  }
  lcd.print("CO2 meter in startup");
  lcd.setCursor(0, 1);
  lcd.print("Revision 1.3");
  delay(1000);
  //Serial1.write(ABCdisable, 9);
  //Serial.println("Auto baseline calibration off");

  // init RTC
  if (! rtc.begin()) {
#ifdef DEBUG
    Serial.println("Couldn't find RTC");
#endif
    lcd.print("RTC!!");
  }

#ifdef DEBUG
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  Serial.println("RTC initialized.");
#endif
  lcd.setCursor(0, 2);
  lcd.print("RTC ok!");
  delay(1000);

  // init SD cardreader
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    lcd.print("SD-card reader!!");
    // don't do anything more:
    while (1);
  }

#ifdef DEBUG
  Serial.println("Card initialized.");
  Serial.println("Writing column headers SD-card.");
#endif
  lcd.setCursor(0, 3);
  lcd.print("SD  ok!");
  delay(2000);
  lcd.clear();

  File CO2log = SD.open("CO2log.txt", FILE_WRITE);
  // if the file is available, write to it:
  if (CO2log) {
    CO2log.print("Datum");
    CO2log.print(",");
    CO2log.print("Tijd");
    CO2log.print(',');
    CO2log.println("ppm");
    CO2log.close();
    // print to the serial port too:
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening CO2log.txt");
    lcd.clear();
    lcd.print("CO2log!!");
  }
  lcd.clear();
  lcd.print("Wait for 1st reading");
  DateTime now = rtc.now();
  previousMinute = now.minute(), DEC;
  currentMinute = now.minute(), DEC;
  previousSecond = now.second(), DEC;
  currentSecond = now.second(), DEC;
}

void loop() {
  DateTime now = rtc.now();
  uur = now.hour(), DEC;
  minuut = now.minute(), DEC;
  seconde = now.second(), DEC;
  currentMinute = now.minute(), DEC;
  currentSecond = now.second(), DEC;

  // send serial command to sensor
  if (Serial.available() > 0) {
    // read the oldest byte in the serial buffer:
    incomingByte = Serial.read();

    // if it's a capital Z,calibrate
    if (incomingByte == 'Z') {
      Serial1.write(zeroPoint, 9);
      Serial.println("ZeroPoint set to 400 ppm");
    }
    // if it's a capital A,Auto Baseline Correction ON
    if (incomingByte == 'O') {
      Serial1.write(ABCenable, 9);
      Serial.println("Auto Baseline Correction ON");
    }
  }

  if (currentSecond != previousSecond)  {
    DateTime now = rtc.now();
    klok();
    previousSecond = now.second(), DEC;
  }
#ifdef DEBUG
  //Serial.println(counter);
#endif
  // logging

  if (previousMinute != currentMinute)  {
    logFlag = true;
  }
  if (logFlag == true && currentMinute % 2 == 0)  {
    File CO2log = SD.open("CO2log.txt", FILE_WRITE);
    int ppmS = readPPMSerial();
    int temperature = readTEMPSerial();
    // if the file is available, write to it:
    if (CO2log) {
      CO2log.print(now.year(), DEC);
      CO2log.print('/');
      CO2log.print(now.month(), DEC);
      CO2log.print('/');
      CO2log.print(now.day(), DEC);
      CO2log.print(",");
      CO2log.print(now.hour(), DEC);
      CO2log.print(':');
      CO2log.print(now.minute(), DEC);
      CO2log.print(',');
      CO2log.println(ppmS);
      CO2log.close();
      Serial.println("logged");
      counter++;
      logFlag = false;
      previousMinute = now.minute(), DEC;
      lcd.clear();
      lcd.setCursor(6, 1);
      lcd.print(ppmS);
      lcd.print("ppm");
      lcd.setCursor(0, 3);
      lcd.print(temperature);
      lcd.print((char)223);
      lcd.print("C");
      
      if (counter < 18)  {
        lcd.setCursor(18, 0);
      }
      if (counter > 9 && counter < 100 )  {
        lcd.setCursor(17, 0);
      }
      if (counter > 99 && counter < 1000 )  {
        lcd.setCursor(16, 0);
      }
      if (counter > 999 && counter < 10000 )  {
        lcd.setCursor(15, 0);
      }
      lcd.print("#");
      lcd.print(counter);
    }
  }
}

void klok() {
  DateTime now = rtc.now();
  uur = now.hour(), DEC;
  minuut = now.minute(), DEC;
  seconde = now.second(), DEC;
  lcd.setCursor(12, 3);
  if (uur < 10) {
    lcd.print("0");
  }
  lcd.print(uur);
  lcd.print(":");
  if (minuut < 10) {
    lcd.print("0");
  }
  lcd.print(minuut);
  lcd.print(":");
  if (seconde < 10) {
    lcd.print("0");
  }
  lcd.print(seconde);
}

int readPPMSerial() {
  for (int i = 0; i < 9; i++) {
    Serial1.write(requestReading[i]);
  }
  //Serial.println("sent request");
  while (Serial1.available() < 9) {}; // wait for response
  for (int i = 0; i < 9; i++) {
    result[i] = Serial1.read();
  }
  int high = result[2];
  int low = result[3];
  return high * 256 + low;
}

int readTEMPSerial()  {
  for (int i = 0; i < 9; i++) {
    Serial1.write(requestReading[i]);
  }
  //Serial.println("sent request");
  while (Serial1.available() < 9) {}; // wait for response
  for (int i = 0; i < 9; i++) {
    result[i] = Serial1.read();
  }
  return result[4]-40;
}
