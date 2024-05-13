#include <Arduino.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

const int ledPin = 27;
const int switch1 = 12;
const int switch2 = 14;
const int switch3 = 26;

bool sw01State = false;
bool sw02State = false;
bool sw03State = false;

int receiveData = 1;

bool sendBlue = false;

void setup() {

  Serial.begin(115200);
  Serial.println("OryArm-01");
  
  SerialBT.begin("OryArm-01", true); 
  Serial.println("The device started in master mode, make sure remote BT device is on!");
  
  // Attempt to connect to the specified device
  if(SerialBT.connect("OryArm-01")) { // ここで直接デバイス名を指定
    Serial.println("Connected Successfully!");
  } else {
    Serial.println("Failed to connect. Make sure remote device is available and in range, then restart app.");
    // No need to disconnect here; just attempt to reconnect or prompt for manual restart
  }


  pinMode(ledPin, OUTPUT);
  pinMode(switch1, INPUT_PULLUP);
  pinMode(switch2, INPUT_PULLUP);
  pinMode(switch3, INPUT_PULLUP);

  // Sign for the end of bluetooth setup.
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
  }

}

void loop() {

  if (Serial.available()) {
    receiveData = Serial.read();
  }
  sw01State = digitalRead(switch1);
  sw02State = digitalRead(switch2);
  sw03State = digitalRead(switch3);

  if (sw01State == LOW || receiveData == 49) {
      // SerialBT.write('A');
    SerialBT.write(49);
    // Serial.println("A");
    digitalWrite(ledPin, HIGH); // Feedback that the button was pressed.
    sendBlue = true;
  } else if (sw02State == LOW || receiveData == 50) {
    // SerialBT.write('B');
    SerialBT.write(50);
    // Serial.println("B");
    digitalWrite(ledPin, HIGH); // Feedback that the button was pressed.
  }
  else if (sw03State == LOW || receiveData == 51) {
    // SerialBT.write('C');
    SerialBT.write(51);
    // Serial.println("C");
    digitalWrite(ledPin, HIGH); // Feedback that the button was pressed.
  }
  
  if (digitalRead(switch1) == HIGH && digitalRead(switch2) == HIGH) {
    digitalWrite(ledPin, LOW);
    // SerialBT.write('Z');
    // Serial.println("Z");
  }
  receiveData = 0;
  
  delay(500);
}
