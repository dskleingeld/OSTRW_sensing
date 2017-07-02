#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "sensitiveConfig.h"
#include <SHT1x.h>

/*NodeMCU has weird pin mapping.*/
/*Pin numbers written on the board itself do not correspond to ESP8266 GPIO pin numbers.*/
#define D0 16
#define D1 5 // I2C Bus SCL (clock)
#define D2 4 // I2C Bus SDA (data)
#define D3 0
#define D4 2 // Same as "LED_BUILTIN", but inverted logic
#define D5 14 // SPI Bus SCK (clock)
#define D6 12 // SPI Bus MISO 
#define D7 13 // SPI Bus MOSI
#define D8 15 // SPI Bus SS (CS)
#define D9 3 // RX0 (Serial console)
#define D10 1 // TX0 (Serial console)

int _dataPin = D0;
int _clockPin = D3;
String url = "/test";
float temp = -1;
float humid = -1;

SHT1x sht1x(_dataPin, _clockPin);
WiFiClientSecure client;

void setup() {
  Serial.begin(9600);
  
  //Connecting to wifi
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(WiFi.status());
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Use WiFiClientSecure class to create TLS connection
  Serial.print("connecting to ");
  Serial.println(host);
  while (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    delay(2000);
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
}

void loop() {
  Serial.println("polling sensor");
  temp = sht1x.readTemperatureC();
  humid = sht1x.readHumidity();
  Serial.println("temp: " + (String)temp + " humid: " + (String)humid);

  Serial.println("sending POST request");
  
  client.println(String("POST ") + url + " HTTP/1.1");
  client.println(String("Host: ") + host);
  client.println("User-Agent: BuildFailureDetectorESP8266");
  client.println("Content-Type: text/plain");
  client.print("Content-Length: ");
  client.println(5);
  client.println();
  client.println(temp);
  
  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");

  delay(100);
}
