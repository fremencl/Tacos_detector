#include <esp_now.h>
#include <WiFi.h>

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0x0C, 0xB8, 0x15, 0x83, 0x8A, 0x70};

typedef struct struct_message {
  int flowStatus; // 1 for FLOW ON, 0 for FLOW OFF
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  pinMode(32, INPUT); // Sensor Pin
}

void loop() {
  static int countHigh = 0;
  static unsigned long lastMillis = 0;

  if (digitalRead(32) == HIGH) {
    countHigh++;
  }

  if (millis() - lastMillis >= 3000) {
    myData.flowStatus = (countHigh > 10) ? 1 : 0;
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    lastMillis = millis();
    countHigh = 0;
  }

  delay(10); // Small delay to avoid too frequent polling
}
