#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>
#include "EmonLib.h"

const char *ssid     = "";
const char *password = "";
const char *postHost = "10.0.0.101";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 2*60*60);

EnergyMonitor emon1;
double IavgTotal;
uint16_t IavgCount;
int oldMin = 99;
const uint32_t samplePeriod = 500; // ms
uint32_t lastSampleT;
int lastWifiStatus;

void setup() {
  WiFi.begin(ssid, password);

  waitForWifi();
  lastWifiStatus = WL_CONNECTED;

  timeClient.begin();
  timeClient.setUpdateInterval(60000*5);
  Serial.begin(9600);
  
  emon1.current(0, 42.5);
}

void loop() {
  uint32_t now = millis();
  uint32_t since = now - lastSampleT;
  if (since >= samplePeriod || since < 0) {
    double Irms = emon1.calcIrms(1480);  // Calculate Irms only
    IavgTotal += Irms;
    IavgCount++;
    lastSampleT = now;

    /*Serial.print(Irms*226.5); 
    Serial.print(" ");
    Serial.println(Irms);*/
  }
  
  int wStatus = WiFi.status();
  if (wStatus == WL_CONNECTED) {
    if (lastWifiStatus != wStatus) {
      Serial.print("WiFi Connected! RSSI: ");
      Serial.println(WiFi.RSSI());
    }
    timeClient.update();
 
    int minutes = timeClient.getMinutes();
    if (minutes != oldMin) {
      double Iavg = IavgTotal / IavgCount;
      Serial.print(Iavg*226.5); 
      Serial.print(" ");
      Serial.print(Iavg);
      Serial.print(" ");
      Serial.print(IavgCount);
      Serial.print(" ");
      Serial.println(timeClient.getFormattedTime());

      String postData = "{""Iavg"": " + String(Iavg) + ", ""ts"": " + timeClient.getEpochTime() + "}";
  
      HTTPClient http;
      http.begin("http://10.0.0.101:4501/");              //Specify request destination
      http.addHeader("Content-Type", "application/json");    //Specify content-type header
      int httpCode = http.POST(postData);   //Send the request
      String resp = http.getString();
      if (httpCode != 200) {
        Serial.print("HTTP Error: ");
        Serial.print(httpCode);
        Serial.print(" ");
        Serial.println(resp);
      }
      http.end();
      
      IavgTotal = 0;
      IavgCount = 0;
    }
  
    oldMin = minutes;
  } else {
    if (lastWifiStatus != wStatus) {
      Serial.print("WIFI Error! Status: ");
      Serial.println(wStatus);
    }
  }
  lastWifiStatus = wStatus;
}

void waitForWifi() {
  Serial.print("Waiting for WiFi connection");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi Connected! RSSI: ");
  Serial.println(WiFi.RSSI());
}
