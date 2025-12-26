#include <SoftwareSerial.h>
#include <Servo.h>

Servo s1;
SoftwareSerial espSerial(2, 3); // RX, TX
#define IR 7

bool lastIRState = HIGH;   // for edge detection

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  pinMode(IR, INPUT_PULLUP);
  s1.attach(5);
  s1.write(90);
}

void loop() {
  bool currentIRState = digitalRead(IR);
  if (lastIRState == HIGH && currentIRState == LOW) {
    Serial.println("WASTE DETECTED");
    espSerial.println("DETECT");
  }
  lastIRState = currentIRState;

  if (espSerial.available()) {
    String result = espSerial.readStringUntil('\n');
    result.trim();
    Serial.print("Received classification: ");
    Serial.println(result);

    if (result == "PLASTIC") {
      Serial.println("DRY detected — moving servo RIGHT");
      moveServoSmooth(90, 150);
      delay(500);
      moveServoSmooth(150, 90);
    } 
    else if (result == "WET") {
      Serial.println("WET detected — moving servo LEFT");
      moveServoSmooth(90, 30);
      delay(500);
      moveServoSmooth(30, 90);
    }
  }
}

void moveServoSmooth(int startAngle, int endAngle) {
  if (startAngle < endAngle) {
    for (int pos = startAngle; pos <= endAngle; pos++) {
      s1.write(pos);
      delay(25);
    }
  } else {
    for (int pos = startAngle; pos >= endAngle; pos--) {
      s1.write(pos);
      delay(25);
    }
  }
}








