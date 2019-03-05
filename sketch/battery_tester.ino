#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32RTC.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define NUM_READS 10 // number of readings in sample

U8G2_SSD1306_128X32_UNIVISION_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ PA0, /* dc=*/ PA1, /* reset=*/ PA2);

STM32RTC& rtc = STM32RTC::getInstance();
/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 0;
const byte hours = 0;

/* Change these values to set the current initial date */
/* Monday 1st January 2019 */
const byte weekDay = 2;
const byte day = 1;
const byte month = 1;
const byte year = 19;

// Analog
const int analogInPin = PB1;
const float coefficientADC = 0.007; // constant to convert ADC reading to voltage
const int minVoltage = 3 / coefficientADC; // ADC readout = minimal voltage on battery divided on coefficientADC
int voltageADC = 0;

// MOSFET driver
const int mosfetOutPin = PB0;

// Button
const int buttonInPin = PB10;
int buttonState = HIGH;

// Buzzer
const int buzzerOutPin = PB11;

// LED
const int ledOutPin = PC13;

// test condition
int test = 0;
unsigned long prevMillis = 0;
unsigned long currentMillis = 0;

//unsigned long prevMillis;

void setup() {
  // put your setup code here, to run once:
  pinMode(analogInPin, INPUT);
  pinMode(mosfetOutPin, OUTPUT);
  pinMode(buttonInPin, INPUT_PULLUP);
  pinMode(ledOutPin, OUTPUT);

  pinMode(buzzerOutPin, OUTPUT);
  digitalWrite(buzzerOutPin, 1);  

  digitalWrite(mosfetOutPin, LOW);

  u8g2.begin();
  u8g2.setFont(u8g2_font_helvB08_tr);

  rtc.setClockSource(STM32RTC::LSE_CLOCK);
  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(weekDay, day, month, year);

  // Init done
  digitalWrite(buzzerOutPin, 0);
  digitalWrite(ledOutPin, 0);
  delay(50);
  digitalWrite(buzzerOutPin, 1);
  digitalWrite(ledOutPin, 1);
  delay(100);
  digitalWrite(buzzerOutPin, 0);
  digitalWrite(ledOutPin, 0);
  delay(50);
  digitalWrite(buzzerOutPin, 1);
  digitalWrite(ledOutPin, 1);

}

int readVoltage () {
  int sumOfReadedValues = 0;
  for (int i = 0; i < NUM_READS; i++) {
    sumOfReadedValues += analogRead(analogInPin);
  }
  return sumOfReadedValues / NUM_READS; // arithmetic mean
}

int readButtonDebounce () {
  // read button with debouncing
  int reading = digitalRead(buttonInPin);
  // if the switch changed, due to noise or pressing:
  if (reading == LOW) {
    // delay
    delay(50);
    reading = digitalRead(buttonInPin);    
  }
  return reading;  
}

void showTimeSinceStart () {
  // show time since start
  u8g2.setCursor(85,31);
  if (rtc.getHours() < 10) {
    u8g2.print("0"); // print a 0 before if the number is < than 10
  }
  u8g2.print(rtc.getHours());
  u8g2.print(":");
  if (rtc.getMinutes() < 10) {
    u8g2.print("0"); // print a 0 before if the number is < than 10
  }
  u8g2.print(rtc.getMinutes());
  u8g2.print(":");
  if (rtc.getSeconds() < 10) {
    u8g2.print("0"); // print a 0 before if the number is < than 10
  }
  u8g2.print(rtc.getSeconds());
}

void showVoltage (int voltage) {
  u8g2.setCursor(0,18);
  u8g2.print("Voltage: ");
  u8g2.print(voltage * coefficientADC, 3);
}

void loop() {
  // put your main code here, to run repeatedly:

  voltageADC = readVoltage();
  buttonState = readButtonDebounce();

  u8g2.clearBuffer();
  
  switch (test) {
    case 0:
      // Press button to start
      u8g2.setCursor(0,9);
      u8g2.print("Press button");
      u8g2.setCursor(0,18);
      u8g2.print("to start test!");
      showTimeSinceStart();
      showVoltage(voltageADC);
      if (buttonState == LOW) {
        // button is pressed
        test = 1;
      }
      break;
    case 1:
      // Start test      
      digitalWrite(mosfetOutPin, HIGH);
      test = 2;
      prevMillis = millis();
      if (voltageADC <= minVoltage) {
        test = 3;
      }
      break;
    case 2:
      // Testing

      break;
    case 3:
      // Stop testing - battery deplited

      break;
    case 4:
      // Stop testing - button pressed

      break;
    case 5:
      // 

      break;
    default:
      // statements
      break;
  }

  u8g2.sendBuffer();  
   
}
