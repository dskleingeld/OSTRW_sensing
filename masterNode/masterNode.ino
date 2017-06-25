#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "sensitiveConfig.h"

WiFiClientSecure client;

void setup() {
  //Connecting to wifi
  Serial.begin(9600);
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
  String url = "/test";
  int postData = 42;

  while(1) {
    Serial.println("sending POST request");
    
    client.println(String("POST ") + url + " HTTP/1.1");
    client.println(String("Host: ") + host);
    client.println("User-Agent: BuildFailureDetectorESP8266");
    client.println("Content-Type: text/plain");
    client.print("Content-Length: ");
    client.println(2);
    client.println();
    client.println(postData);
    
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

    delay(10000);
  }
}
