/*
====================================================================================================================
  PROJECT      : DIY Solar PV Monitoring System V2.0
  VERSION      : v2.0
  UPDATED ON   : 08-Mar-2026
  AUTHOR       : Open Green Energy

  LICENSE
  ------------------------------------------------------------------------------------------------------------------
  Copyright (c) 2026 Open Green Energy

  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-nc-sa/4.0/

  HARDWARE USED
  ------------------------------------------------------------------------------------------------------------------
  MCU            : ESP32 / Seeed XIAO ESP32
  DISPLAY        : OLED 128x64 SSD1306 (I2C)
  ADC            : ADS1115 (16-bit external ADC)
  CURRENT SENSOR : ACS758 Hall Current Sensor
  TEMP SENSOR    : DS18B20
  CLOUD PLATFORM : Blynk IoT

  WHAT THIS DEVICE DOES
  ------------------------------------------------------------------------------------------------------------------
  1. Measures solar panel voltage using ADS1115 + voltage divider.
  2. Measures solar current using ACS758 Hall current sensor.
  3. Calculates real-time solar power.
  4. Integrates power over time to estimate generated energy (Wh).
  5. Measures panel temperature using DS18B20 sensor.
  6. Displays parameters locally on OLED display.
  7. Sends data to Blynk mobile dashboard via WiFi.

  MEASURED PARAMETERS
  ------------------------------------------------------------------------------------------------------------------
  Voltage (V)
  Current (A)
  Power (W)
  Energy (Wh)
  Temperature (°C)
  Estimated cost saving (₹)

====================================================================================================================
*/


// ============================================================================
// BLYNK CONFIGURATION
// Replace these values with your own Blynk project credentials
// ============================================================================
#define BLYNK_TEMPLATE_ID "TMPL3V-0wJfRH"
#define BLYNK_TEMPLATE_NAME "Solar PV Energy Meter V2"
#define BLYNK_AUTH_TOKEN "YOUR AUTH TOKEN"
#define BLYNK_PRINT Serial


// ============================================================================
// REQUIRED LIBRARIES
// ============================================================================
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>


// ============================================================================
// PIN DEFINITIONS
// ============================================================================
const int oneWirePin = D3;        // DS18B20 temperature sensor pin


// ============================================================================
// OLED DISPLAY OBJECT
// ============================================================================
Adafruit_SSD1306 display(128, 64, &Wire);


// ============================================================================
// ADS1115 ADC OBJECT
// Used for high resolution voltage/current measurement
// ============================================================================
Adafruit_ADS1115 ads;


// ============================================================================
// TEMPERATURE SENSOR OBJECT
// ============================================================================
OneWire oneWire(oneWirePin);
DallasTemperature sensors(&oneWire);


// ============================================================================
// MEASUREMENT VARIABLES
// ============================================================================
float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;
float temperature = 0;
float saving = 0;


// Time tracking variable used for energy calculation
unsigned long previousMillis = 0;


// Electricity cost per unit (₹ / kWh)
const float EnergyCost = 6.5;


// ============================================================================
// VOLTAGE DIVIDER RESISTOR VALUES
// Used to scale solar voltage to ADC range
// ============================================================================
const float R1 = 100;   // 100K
const float R2 = 10;    // 10K

// Current measurement divider
const float R3 = 1;     // 1K
const float R4 = 2;     // 2K


// ============================================================================
// ACS758 CURRENT SENSOR PARAMETERS
// ============================================================================
float OffsetVoltage = 2.5;      // Zero current output voltage
const float sensitivity = 40;   // mV per Amp


// ============================================================================
// WIFI & BLYNK SETTINGS
// ============================================================================
const char* ssid = "XXXXXXX";
const char* password = "XXXXX";
const char* blynkAuth = BLYNK_AUTH_TOKEN;


// ============================================================================
// SETUP FUNCTION
// Runs once when the microcontroller starts
// ============================================================================
void setup()
{
  Serial.begin(115200);

  // --------------------------------------------------------------------------
  // Connect to WiFi
  // --------------------------------------------------------------------------
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");

  // Start Blynk connection
  Blynk.begin(blynkAuth, ssid, password);


  // --------------------------------------------------------------------------
  // Initialize OLED Display
  // --------------------------------------------------------------------------
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();


  // --------------------------------------------------------------------------
  // Initialize ADS1115 ADC
  // --------------------------------------------------------------------------
  ads.begin();


  // --------------------------------------------------------------------------
  // Initialize temperature sensor
  // --------------------------------------------------------------------------
  sensors.begin();
}


// ============================================================================
// MAIN LOOP
// Runs continuously after setup()
// ============================================================================
void loop()
{
  // Maintain Blynk connection
  Blynk.run();


  // --------------------------------------------------------------------------
  // READ ADC VALUES
  // Channel 0 → Solar Voltage
  // Channel 1 → Current Sensor
  // --------------------------------------------------------------------------
  int16_t voltageRaw = ads.readADC_SingleEnded(0);
  int16_t currentRaw = ads.readADC_SingleEnded(1);


  // --------------------------------------------------------------------------
  // CONVERT ADC VALUE TO VOLTAGE
  // ADS1115 resolution = 0.1875 mV per bit
  // --------------------------------------------------------------------------
  float voltageVolts = voltageRaw * (0.1875 / 1000);
  voltage = voltageVolts * ((R1 + R2) / R2);


  // --------------------------------------------------------------------------
  // CURRENT CALCULATION
  // Convert sensor voltage into current value
  // --------------------------------------------------------------------------
  float currentVolts = currentRaw * ((R3 + R4) / R4) * (0.1875 / 1000);
  current = (currentVolts - OffsetVoltage) / (sensitivity / 1000.0);


  // --------------------------------------------------------------------------
  // POWER CALCULATION
  // P = V × I
  // --------------------------------------------------------------------------
  power = voltage * current;


  // --------------------------------------------------------------------------
  // ENERGY CALCULATION
  // Energy (Wh) = Power × Time
  // --------------------------------------------------------------------------
  unsigned long currentMillis = millis();

  energy += power * (currentMillis - previousMillis) / 3600000.0;

  saving = EnergyCost * (energy / 1000);

  previousMillis = currentMillis;


  // --------------------------------------------------------------------------
  // TEMPERATURE MEASUREMENT
  // --------------------------------------------------------------------------
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);


  // --------------------------------------------------------------------------
  // OLED DISPLAY OUTPUT
  // --------------------------------------------------------------------------
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(10, 10);
  display.print(voltage, 2);
  display.println(" V");

  display.setCursor(70, 10);
  display.print(current, 2);
  display.println(" A");

  display.setTextSize(2);

  display.setCursor(10, 25);
  display.print(power, 2);
  display.println(" W");

  display.setCursor(10, 45);
  display.print(energy, 2);
  display.println(" Wh");

  display.display();


  // --------------------------------------------------------------------------
  // SEND DATA TO BLYNK DASHBOARD
  // --------------------------------------------------------------------------
  Blynk.virtualWrite(V0, voltage);
  Blynk.virtualWrite(V1, current);
  Blynk.virtualWrite(V2, power);
  Blynk.virtualWrite(V3, energy / 1000);
  Blynk.virtualWrite(V4, temperature);
  Blynk.virtualWrite(V5, saving);


  // --------------------------------------------------------------------------
  // SERIAL MONITOR OUTPUT
  // --------------------------------------------------------------------------
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print("V");

  Serial.print(" Current: ");
  Serial.print(current);
  Serial.print("A");

  Serial.print(" Power: ");
  Serial.print(power);
  Serial.print("W");

  Serial.print(" Energy: ");
  Serial.print(energy, 2);
  Serial.print("Wh");

  Serial.print(" Saving: Rs.");
  Serial.print(saving, 2);

  Serial.print(" Temp: ");
  Serial.print(temperature);
  Serial.println("C");


  delay(100);
}
