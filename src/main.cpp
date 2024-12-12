#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <inttypes.h>
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"


#define RST_PIN 17 
#define SS_PIN 33  


Servo myservo;
MFRC522 rfid(SS_PIN, RST_PIN);

char ssid[50]; // Network SSID
char pass[50]; // Network password

const int kNetworkTimeout = 30 * 1000; // 30 seconds
const int kNetworkDelay = 1000;    

const int servoPin = 32; 
const int authorizedUID[4] = {0xC1, 0xCD, 0x24, 0x03};
int tempKey[4];
bool unauth = false;

void nvs_access() {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase and retry NVS initialization
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open NVS handle
    Serial.println("\nOpening Non-Volatile Storage (NVS) handle...");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        Serial.println("Done");
        Serial.println("Retrieving SSID/PASSWD");

        // Retrieve stored SSID and password
        size_t ssid_len = sizeof(ssid);
        size_t pass_len = sizeof(pass);
        err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
        err |= nvs_get_str(my_handle, "pass", pass, &pass_len);

        switch (err) {
            case ESP_OK:
                Serial.println("Done");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                Serial.println("The value is not initialized yet!");
                break;
            default:
                Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        // Close NVS handle
        nvs_close(my_handle);
    }
}

void setup() {
  Serial.begin(9600);
  // delay(1000);


    // Retrieve SSID and password from flash storage
    nvs_access();

    // Connect to WiFi
    delay(1000);
    Serial.println("\nConnecting to WiFi...");
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());

  SPI.begin(25,27,26);         
  rfid.PCD_Init();     
  myservo.attach(servoPin);

  Serial.println("place tag next to reader");
  myservo.write(0); 
}

void loop() {
  int err = 0;
  String value = "";
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
      value = "unauth";
    }
  }


  if (isAuthorized) {
    value = "auth";
    Serial.println("Authorized tag detected. Moving servo...");
    myservo.write(179); 
    delay(3000);       
    myservo.write(0);  
  } else {
    Serial.println("Unauthorized tag.");
}


   String path = "/auth?value=" + value;

    // Send HTTP GET request
    WiFiClient c;
    HttpClient http(c);
    err = http.get("54.183.251.111", 5000, path.c_str() , NULL);
    if (err == 0) {
        Serial.println("Started request successfully");
        err = http.responseStatusCode();

        if (err >= 0) {
            Serial.print("Got status code: ");
            Serial.println(err);

            // Skip response headers
            err = http.skipResponseHeaders();
            if (err >= 0) {
                int bodyLen = http.contentLength();
                Serial.print("Content length: ");
                Serial.println(bodyLen);
                Serial.println("\nBody returned follows:");

                // Print response body
                unsigned long timeoutStart = millis();
                char c;

                while ((http.connected() || http.available()) && 
                       ((millis() - timeoutStart) < kNetworkTimeout)) {
                    if (http.available()) {
                        c = http.read();
                        Serial.print(c);
                        bodyLen--;

                        // Reset timeout counter
                        timeoutStart = millis();
                    } else {
                        delay(kNetworkDelay);
                    }
                }
            } else {
                Serial.printf("Failed to skip response headers: %d\n", err);
            }
        } else {
            Serial.printf("Getting response failed: %d\n", err);
        }
    } else {
        Serial.printf("Connection failed: %d\n", err);
    }

    // Stop HTTP client
    http.stop();


  rfid.PICC_HaltA();
}
