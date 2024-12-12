#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>

#define RST_PIN 17 
#define SS_PIN 33  

Servo myservo;
MFRC522 rfid(SS_PIN, RST_PIN);

const int servoPin = 32; 
const int authorizedUID[4] = {0xC1, 0xCD, 0x24, 0x03};
int tempKey[4];
bool unauth = false;

void setup() {
  Serial.begin(9600);
  SPI.begin(25,27,26);         
  rfid.PCD_Init();     
  myservo.attach(servoPin);

  Serial.println("place tag next to reader");
  myservo.write(0); 
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println("tag detected!");
  Serial.print("UID: ");
  bool isAuthorized = true;
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
    if (rfid.uid.uidByte[i] != authorizedUID[i]) {
      isAuthorized = false;
    }
  }


  if (isAuthorized) {
    Serial.println("Authorized tag detected. Moving servo...");
    myservo.write(179); 
    delay(3000);       
    myservo.write(0);  
  } else {
    Serial.println("Unauthorized tag.");
}


  rfid.PICC_HaltA();
}
