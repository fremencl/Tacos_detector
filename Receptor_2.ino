#include <esp_now.h>
#include <WiFi.h>

// Estructura para recibir datos
typedef struct struct_message {
  int flowStatus; // 1 for FLOW ON, 0 for FLOW OFF
} struct_message;

struct_message myData;

// Variables para manejar el conteo y evaluación del flujo
unsigned long lastMillis = 0;
int countHigh = 0;
const int interval = 3000; // Intervalo de 3 segundos
const int threshold = 10;  // Umbral de 10 electrodos para determinar "FLOW ON"

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Flow Status Received: ");
  Serial.println(myData.flowStatus);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  pinMode(32, INPUT); // Pin del sensor
  pinMode(34, OUTPUT); // Pin de la alarma
}

void loop() {
  if (digitalRead(32) == HIGH) {
    countHigh++;
  }

  if (millis() - lastMillis >= interval) {
    if (countHigh > threshold && myData.flowStatus == 1) {
      // Flujo continuo y confirmado por el emisor
      digitalWrite(34, LOW); // Apagar la alarma
      Serial.println("Normal operation: Flow confirmed and sensor active.");
    } else if (countHigh <= threshold && myData.flowStatus == 1) {
      // El emisor dice "FLOW ON", pero el sensor no detecta suficientes electrodos
      digitalWrite(34, HIGH); // Activar alarma
      Serial.println("Warning: Flow discrepancy detected!");
    } else {
      // No hay suficientes electrodos o el emisor reporta "FLOW OFF"
      digitalWrite(34, LOW); // Asegurar que la alarma esté apagada
      Serial.println("Flow is OFF or insufficient electrodes detected.");
    }
    // Reseteo de variables para el próximo intervalo
    lastMillis = millis();
    countHigh = 0;
  }

  delay(10); // Retardo para reducir carga de CPU y evitar muestreo excesivo
}