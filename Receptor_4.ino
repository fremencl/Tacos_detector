#include <esp_now.h>
#include <WiFi.h>

// Estructura para recibir datos
typedef struct struct_message {
  int flowStatus; // 1 for FLOW ON, 0 for FLOW OFF
} struct_message;

struct_message myData;

// Variables para manejar el conteo y evaluación del flujo
unsigned long lastMillis = 0;
unsigned long transitStartTime = 0; // Tiempo cuando se detecta FLOW ON en entrada y FLOW OFF en salida
unsigned long lastElectrodeTime = 0; // Tiempo del último cambio a FLOW OFF
unsigned long lastElectrodeDetectedTime = 0; // Tiempo del último electrodo detectado
unsigned long alarmStartTime = 0; // Tiempo de inicio de la alarma
const unsigned long alarmDuration = 30000; // Duración de la alarma en milisegundos
int countHigh = 0;
const int interval = 3000; // Intervalo de 3 segundos
const int threshold = 10;  // Umbral de 10 electrodos para determinar "FLOW ON"
const int transitTime = 30000; // Tiempo de tránsito normal en milisegundos
const unsigned long gracePeriod = transitTime + 5000; // Período de gracia

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    int previousStatus = myData.flowStatus;
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.print("Flow Status Received: ");
    Serial.println(myData.flowStatus);
    if (previousStatus == 1 && myData.flowStatus == 0) {
        lastElectrodeTime = millis();  // Marcar tiempo cuando el flujo cambia de ON a OFF
    }
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

  digitalWrite(34, LOW); // Asegurarse de que la alarma está apagada al iniciar
  countHigh = 0; 
}

void loop() {
  // Leer el estado del sensor
  if (digitalRead(32) == HIGH) {
        countHigh++;
        if (myData.flowStatus == 0) {
            lastElectrodeDetectedTime = millis();  // Actualizar solo si el estado es FLOW OFF
        }
  }
  if (millis() - lastMillis >= interval) {
    if (countHigh > threshold && myData.flowStatus == 1) {
      digitalWrite(34, LOW); // Apagar la alarma
      Serial.println("Normal operation: Flow confirmed and sensor active.");
      transitStartTime = 0; // Reseteamos el tiempo de tránsito
    } else if (countHigh <= threshold && myData.flowStatus == 1) {
      if (transitStartTime == 0) {
        transitStartTime = millis(); // Empezamos a contar el tiempo de tránsito
      } else if (millis() - transitStartTime > transitTime) {
        digitalWrite(34, HIGH); // Activar alarma si el tiempo excede el tiempo de tránsito
        alarmStartTime = millis(); // Comienza a contar el tiempo de alarma
        Serial.println("Warning: Potential blockage detected!");
      }
    } else if (myData.flowStatus == 0) {
      if (countHigh > 0 && millis() - lastElectrodeDetectedTime < gracePeriod) {
        digitalWrite(34, HIGH); // Activar alarma
        alarmStartTime = millis(); // Comienza a contar el tiempo de alarma
        Serial.println("Warning: Flow detected after FLOW OFF!");
      } else if (countHigh == 0 && millis() - lastElectrodeDetectedTime > gracePeriod) {
        digitalWrite(34, LOW); // Apagar la alarma
        Serial.println("Flow OFF condition cleared.");
        transitStartTime = 0;
        lastElectrodeDetectedTime = 0;
      }
    }
    // Reseteo de variables para el próximo intervalo
    lastMillis = millis();
    countHigh = 0;
  }

  // Apagar automáticamente la alarma después de 30 segundos
  if (alarmStartTime != 0 && millis() - alarmStartTime > alarmDuration) {
    digitalWrite(34, LOW);
    alarmStartTime = 0;
    Serial.println("Alarm auto-reset after 30 seconds.");
  }
  
  delay(10); // Retardo para reducir carga de CPU y evitar muestreo excesivo
}
