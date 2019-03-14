#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32RTC.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define NUM_READS 50 // number of readings in sample

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

// analog
const int analogInPin = PB1;
const float R = 3.0; // resistance of ballast resistor + open mosfet, Ohm
const float coefficientADC = 0.00681; // constant to convert ADC reading to voltage
const float alpha = 0.0392; // alpha = 2 / (num_reads + 1)
const float beta = 1 - alpha; // beta = 1 - alpha
const float minVoltage = 3; // minimal voltage on deplited battery
float voltageADC = 0;
float V = 0;
float I = 0;
float Wh = 0; // capacity of battery in Watt*hours
float capacity = 0; // capacity of battery in mA*hours

// MOSFET driver
const int mosfetOutPin = PB0;

// button
const int buttonInPin = PB10;
int buttonState = HIGH;

// buzzer
const int buzzerOutPin = PB11;

// LED
const int ledOutPin = PC13;

// state of test
int test = 0;

// test time in millis
unsigned long prevMillis = 0;
//unsigned long currentMillis = 0;

// debug info
int debug = HIGH;

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
  u8g2.setFont(u8g2_font_haxrcorp4089_tr);

  rtc.setClockSource(STM32RTC::LSE_CLOCK);
  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(weekDay, day, month, year);

  // play on buzzer and LED - init done
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

float readVoltage () {
// Exponentially weighted moving average
  for  (int i = 0; i < NUM_READS; i++) {
    voltageADC = alpha * analogRead(analogInPin) + beta * voltageADC;
  }
  return coefficientADC * int(voltageADC);
}

int readButtonDebounce () {
  // read button with debouncing
  int reading = digitalRead(buttonInPin);
  // if the switch changed, due to noise or pressing:
  if (reading == LOW) {
    // delay
    delay(100);
    reading = digitalRead(buttonInPin);    
  }
  return reading;  
}

void showTimeSinceStart () {
  // show time since start from RTC
  u8g2.setCursor(87,32);
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

void showVoltage () {
  u8g2.setCursor(0,8);
  u8g2.print("V: ");
  u8g2.print(V);
}

void showCurrent () {  
  u8g2.setCursor(64,8);
  u8g2.print("A: ");
  u8g2.print(I);
}

void showWattage () {  
  u8g2.setCursor(0,16);
  u8g2.print("Wh: ");
  u8g2.print(Wh);
}

void showCapacity () {  
  u8g2.setCursor(64,16);
  u8g2.print("mAh: ");
  u8g2.print(capacity);
}

void showDebug () {
  u8g2.setCursor(120,8);
  u8g2.print(test);
}

void beepStop () {
  digitalWrite(buzzerOutPin, 0);
  digitalWrite(ledOutPin, 0);
  delay(100);
  digitalWrite(buzzerOutPin, 1);
  digitalWrite(ledOutPin, 1);
  delay(100);
  digitalWrite(buzzerOutPin, 0);
  digitalWrite(ledOutPin, 0);
  delay(100);
  digitalWrite(buzzerOutPin, 1);
  digitalWrite(ledOutPin, 1);
  delay(100);
  digitalWrite(buzzerOutPin, 0);
  digitalWrite(ledOutPin, 0);
  delay(100);
  digitalWrite(buzzerOutPin, 1);
  digitalWrite(ledOutPin, 1);
}

void loop() {
  // put your main code here, to run repeatedly:

  u8g2.clearBuffer();

  V = readVoltage();
  buttonState = readButtonDebounce();

  switch (test) {
    case 0:
      // Press button to start
      u8g2.setCursor(0,24);
      u8g2.print("Press button");
      u8g2.setCursor(0,32);
      u8g2.print("to start test!");
      showTimeSinceStart();
      showVoltage();
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
      if (V < minVoltage) {
        test = 3;
        digitalWrite(mosfetOutPin, LOW);
        beepStop();
      }
      break;
    case 2:
      // Testing
      I = V / R;
      Wh += V * I * (millis() - prevMillis) / 3600000;
      capacity += I * (millis() - prevMillis) / 3600000000;
      u8g2.setCursor(0,24);
      u8g2.print("Testing battery...");
      showVoltage();
      showCurrent();
      showWattage();
      showCapacity();
      if (V < minVoltage) {
        test = 3;
        digitalWrite(mosfetOutPin, LOW);
        beepStop();
      }
      if (buttonState == LOW) {
        // button is pressed
        test = 4;
      }
      break;
    case 3:
      // Stop testing - battery deplited
      u8g2.setCursor(0,24);
      u8g2.print("Battery deplited! Charge battery");
      u8g2.setCursor(0,32);
      u8g2.print("and press button");
      showVoltage();
      showWattage();
      showCapacity();
      if (buttonState == LOW) {
        // button is pressed
        test = 0;
        I = 0;
        Wh = 0;
        capacity = 0;
      }
      break;
    case 4:
      // Stop testing - button pressed
      u8g2.setCursor(0,24);
      u8g2.print("Test stopped by user");
      u8g2.setCursor(0,32);
      u8g2.print("Press button");      
      showVoltage();
      showWattage();
      showCapacity();
      if (buttonState == LOW) {
        // button is pressed
        test = 0;
        I = 0;
        Wh = 0;
        capacity = 0;
      }
      break;    
    default:
      // statements
      break;
  }
  
  showTimeSinceStart();
  if (debug == HIGH) {
    showDebug();
  }
  u8g2.sendBuffer();  
   
}
