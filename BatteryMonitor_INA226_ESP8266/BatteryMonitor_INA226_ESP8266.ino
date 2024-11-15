#include <Wire.h>
#include <ESP8266WiFi.h>
#include "INA226_WE.h"
#define I2C_ADDRESS 0x40
 
String apiKey = "";
const char* ssid = "";          // Enter your WiFi Network's SSID
const char* pass = "";  // Enter your WiFi Network's Password
const char* server = "api.thingspeak.com";

//From: https://naturesgenerator.com/blogs/news/lead-acid-battery-voltage-chart
const float voltage_to_percent[] = {23.25, 23.40, 23.61, 23.91, 24.21, 24.45, 24.81, 25.02, 25.31, 25.56, 25.77}; //10% step iteration, position 0 of the array is 0%
const int numLevels = sizeof(voltage_to_percent) / sizeof(voltage_to_percent[0]);
float getBatteryPercentage(float voltage);
INA226_WE ina226 = INA226_WE(I2C_ADDRESS);
 
WiFiClient client;
 
void setup()
{
  Serial.begin(115200);
  while (!Serial); // wait until serial comes up on Arduino Leonardo or MKR WiFi 1010
  Wire.begin();
  ina226.init();
 
  /* Set Resistor and Current Range
     if resistor is 5.0 mOhm, current range is up to 10.0 A
     default is 100 mOhm and about 1.3 A*/
 
  ina226.setResistorRange(0.1, 1.3); // choose resistor 0.1 Ohm and gain range up to 1.3A
 
  /* If the current values delivered by the INA226 differ by a constant factor
     from values obtained with calibrated equipment you can define a correction factor.
     Correction factor = current delivered from calibrated equipment / current delivered by INA226*/
 
  ina226.setCorrectionFactor(0.93);
 
  Serial.println("INA226 Current Sensor Example Sketch - Continuous");
 
  ina226.waitUntilConversionCompleted(); //if you comment this line the first data might be zero
 
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print("*");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}
 
void loop()
{
  float shuntVoltage_mV = 0.0;
  float loadVoltage_V = 0.0;
  float batteryVoltage_V = 0.0;
  float current_mA = 0.0;
  float power_mW = 0.0;
 
  ina226.readAndClearFlags();
  shuntVoltage_mV = ina226.getShuntVoltage_mV();
  batteryVoltage_V = ina226.getBusVoltage_V();
  current_mA = ina226.getCurrent_mA();
  power_mW = ina226.getBusPower();
  loadVoltage_V  = batteryVoltage_V + (shuntVoltage_mV / 1000);
 
  float batteryVoltage_Percent = getBatteryPercentage(batteryVoltage_V);
  Serial.print("Battery Voltage: ");
  Serial.print(batteryVoltage_V);
  Serial.println("V");
  Serial.print("Battery Voltage Percent");
  Serial.print(batteryVoltage_Percent);
  Serial.println("%");
 
  Serial.print("Load Voltage: ");
  Serial.print(loadVoltage_V);
  Serial.println("V");
 
  Serial.print("Current: ");
  Serial.print(current_mA);
  Serial.println("mA");
 
  Serial.print("Power: ");
  Serial.print(power_mW);
  Serial.println("mW");
 
  if (!ina226.overflow)
  {
    Serial.println("Values OK - no overflow");
  }
  else
  {
    Serial.println("Overflow! Choose higher current range");
  }
 
  if (client.connect(server, 80)) {
 
    String postStr = apiKey;
    postStr += "&field8=";
    postStr += String(batteryVoltage_V);
    postStr += "&field7=";
    postStr += String(batteryVoltage_Percent);
    postStr += "\r\n\r\n\r\n\r\n";
 
    client.print("POST /update HTTP/1.1\n");
    delay(100);
    client.print("Host: api.thingspeak.com\n");
    delay(100);
    client.print("Connection: close\n");
    delay(100);
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    delay(100);
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    delay(100);
    client.print("Content-Length: ");
    delay(100);
    client.print(postStr.length());
    delay(100);
    client.print("\n\n\n\n");
    delay(100);
    client.print(postStr);
    delay(100);
  }
  client.stop();
  Serial.println("Sent Successfully :)");
  Serial.println();
  delay(5000UL);
}

float getBatteryPercentage(float voltage) {
    // If the voltage is below or above the range, clamp to 0% or 100%
    if (voltage <= voltage_to_percent[0]) return 0;
    if (voltage >= voltage_to_percent[numLevels - 1]) return 100;

    // Find the closest interval in the array
    for (int i = 0; i < numLevels - 1; i++) {
        if (voltage < voltage_to_percent[i + 1]) {
            // Interpolate between voltage_to_percent[i] and voltage_to_percent[i + 1]
            float range = voltage_to_percent[i + 1] - voltage_to_percent[i];
            float percentPerVolt = 100.0 / (numLevels - 1); // 100% spread over number of levels
            return i * percentPerVolt + ((voltage - voltage_to_percent[i]) / range) * percentPerVolt;
        }
    }
    return 100; // Should never reach here
}
