// Arduino sketch to read a BME680, display the values on a LCD and have values available through Serial
//
// Uses Grove modules from Seeed Studio: 
//    Temperature, Huidity, Pressure and Gas Sensor
//    LCD RGB Backlight
//
// Requires lbrary:
//    Timers, https://github.com/centaq/arduino-simple-timers
//    Adafruit BME680 Library, https://github.com/adafruit/Adafruit_BME680
//    Grove - LCD RGB Backlight, https://github.com/Seeed-Studio/Grove_LCD_RGB_Backlight
//    SHT31, https://github.com/RobTillaart/SHT31 
//
// Designed for Arduino Uno R4

#include "Timers.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "SHT31.h"

#include <Wire.h>
#include "rgb_lcd.h"

const int measurement_ms = 500; //time in ms between sensor read
const int lcd_ms = 2000;        //time in ms between display update

Timers timer_measurement; // read sensor
//Timers timer_lcd;         // change LCD screen
Adafruit_BME680 bme;      // I2C BME680
//rgb_lcd lcd;              // LCD Handle
SHT31 sht(0x45);          // I2C SHT35

// Variables
float temperature_bme;
float pressure_bme;
float humidity_bme;

float temperature_sht;
float humidity_sht;

// page to show on LCD
int lcd_page = 0;

void setup() {
  Serial.begin(115200);
  if (!bme.begin(0x76)) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1);
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME68X_OS_16X);
  bme.setHumidityOversampling(BME68X_OS_16X);
  bme.setPressureOversampling(BME68X_OS_16X);
  bme.setIIRFilterSize(BME68X_FILTER_SIZE_3);
  bme.setGasHeater(0, 0); // Disable Gas Measurement

  //begin sht
  sht.begin();

  //Start timers
  timer_measurement.start(measurement_ms);
  //timer_lcd.start(lcd_ms);

  //start LCD
  //lcd.begin(16, 2);
  //lcd.noBlinkLED();

  //set internal led to mark conversion time
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  //check time for a new measurement
  if (timer_measurement.available()) {
    timer_measurement.stop();
    //trigger a BME680 read
    unsigned long endTime = bme.beginReading();
    if (endTime == 0) {
      Serial.println(F("Failed to begin reading :("));
      return;
    }
    sht.requestData();

    digitalWrite(LED_BUILTIN, HIGH);
    timer_measurement.start(measurement_ms);
  }

  // save a concluded measurement
  if (bme.endReading()) {
    digitalWrite(LED_BUILTIN, LOW);
    temperature_bme = bme.temperature;
    pressure_bme = bme.pressure / 100.0;
    humidity_bme = bme.humidity;
  }

  // save a concluded measurement
  if(sht.dataReady()){
    bool success  = sht.readData(); 
    if (success == false)
    {
      Serial.println("SHT Failed read");
    }
    else
    {
      temperature_sht = sht.getTemperature();
      humidity_sht = sht.getHumidity();
    }
  }

  //check serial
  while (Serial.available()){
    char val = Serial.read();
    if(val == 'T'){
      //lcd.setRGB(25, 25, 25);
      Serial.print(temperature_bme);
      Serial.print(F(" *C, "));
      Serial.print(pressure_bme);
      Serial.print(F(" hPa, "));
      Serial.print(humidity_bme);
      Serial.println(F(" %"));
    }
    if(val == 'R'){
      Serial.print(temperature_sht);
      Serial.print(F(" *C, "));
      Serial.print(humidity_sht);
      Serial.println(F(" %"));
    }
  }

  //write to LCD
  /*  if (timer_lcd.available()) {
      timer_lcd.stop();
      // calculate next page
      lcd_page += 1;
      if(lcd_page > 2) lcd_page = 0;

      //write to LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      if(lcd_page == 0){
        lcd.setRGB(25, 0, 0);
        lcd.print(F("Temperature"));
        lcd.setCursor(0, 1);
        lcd.print(temperature);
        lcd.print(F(" *C"));
      }
      if(lcd_page == 1){
        lcd.setRGB(0, 0, 25);
        lcd.print(F("Humidity"));
        lcd.setCursor(0, 1);
        lcd.print(humidity);
        lcd.print(F(" %"));
      }
      if(lcd_page == 2){
        lcd.setRGB(0, 25, 0);
        lcd.print(F("Pressure"));
        lcd.setCursor(0, 1);
        lcd.print(pressure);
        lcd.print(F(" hPa"));
      }
      timer_lcd.start(lcd_ms);
  }
  */

  // some idle
  delay(100);
}
