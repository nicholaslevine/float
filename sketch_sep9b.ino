#include <Wire.h>
#include "MS5837.h"
#include <Servo.h>
#include <WiFi.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

WiFiServer server(80);
WiFiClient client;

MS5837 sensor;

Servo servo1;
Servo servo2;

#define SERVO1_PIN 5
#define SERVO2_PIN 6


float Kp = 20.0;

struct DataPoint {
  float time;
  float depth;
};

DataPoint logData[1000];
int logIndex = 0;

enum State {
  WAIT_FOR_CLIENT,
  READY,
  GO_TO_2_5,
  HOLD_2_5,
  GO_TO_0_4,
  HOLD_0_4,
  DONE
};

State state = WAIT_FOR_CLIENT;

int profilesCompleted = 0;
unsigned long holdStart = 0;


float getDepth() {
  sensor.read();
  return sensor.depth();
}

void setBuoyancy(int control) {
  control = constrain(control, -30, 30);
  servo1.write(90 + control);
  servo2.write(90 - control);
}

void holdDepth(float targetDepth) {
  float depth = getDepth();
  float error = targetDepth - depth;
  int control = error * Kp;
  setBuoyancy(control);
}


void setup() {
  Serial.begin(115200);

  Wire.begin();

  while (!sensor.init()) {
    Serial.println("Sensor initialization failed!");
    delay(2000);
  }
  sensor.setFluidDensity(997);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  WiFi.begin(ssid, password);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  server.begin();
}


void loop() {

  float depth = getDepth();

  if (logIndex < 1000) {
    logData[logIndex++] = {millis() / 1000.0, depth};
  }

  
  if (depth < 0.3) {
    setBuoyancy(-15);
  }

  switch (state) {

    case WAIT_FOR_CLIENT:
      client = server.available();
      if (client) {
        client.println("Float is ready!");
        state = READY;
      }
      break;

    case READY:
      delay(2000); 
      state = GO_TO_2_5;
      break;

    case GO_TO_2_5:
      holdDepth(2.5);
      if (abs(depth - 2.5) < 0.1) {
        holdStart = millis();
        state = HOLD_2_5;
      }
      break;

    case HOLD_2_5:
      holdDepth(2.5);
      if (millis() - holdStart > 5000) {
        state = GO_TO_0_4;
      }
      break;

    case GO_TO_0_4:
      holdDepth(0.45); 
      if (abs(depth - 0.45) < 0.05) {
        holdStart = millis();
        state = HOLD_0_4;
      }
      break;

    case HOLD_0_4:
      holdDepth(0.45);
      if (millis() - holdStart > 5000) {
        profileCount++;
        if (profileCount >= 2) {
          state = DONE;
        } else {
          state = GO_TO_2_5;
        }
      }
      break;

    case DONE:
      setBuoyancy(0);

      if (client && client.connected()) {
        client.println("profile completed");
        client.println("time,depth");

        for (int i = 0; i < logIndex; i++) {
          client.print(logData[i].time);
          client.print(",");
          client.println(logData[i].depth);
        }
      }

      while (true); 
      break;
  }

  delay(50);
}