#include "driver/rtc_io.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 4      // Pin al que está conectado el DHT22
#define DHTTYPE DHT22 // DHT 22 (AM2302)
#define LED_BUILTIN 2 // Pin del LED integrado, generalmente GPIO 2

const char* ssid = "";
const char* password = "";
const char* serverNameButton = "http://homeassistant.local:8123/api/webhook/caida_detectada";
const char* serverNameSensor = "http://homeassistant.local:8123/api/webhook/recibir_datos_esp32";

const int buttonPin = 12; // Pin del botón
DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0; // Almacena el tiempo desde la última lectura del sensor
const long interval = 60000; // Intervalo de tiempo para la lectura del sensor (1 minuto)

void setup() {
  Serial.begin(115200);

  // Conectar a Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a la red WiFi");

  // Desactivar la funcionalidad táctil en GPIO 12
  rtc_gpio_deinit(GPIO_NUM_12);
  
  // Configurar el pin del botón como entrada con resistencia pull-up interna
  pinMode(buttonPin, INPUT_PULLUP);

  // Configurar el pin del LED integrado como salida y apagar el LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Apagar el LED
  
  // Inicializar el sensor DHT22
  dht.begin();
  Serial.println("Inicialización completa.");
}

void loop() {
  // Asegurarse de que el LED permanece apagado
  digitalWrite(LED_BUILTIN, LOW);
  
  // Leer el estado del botón
  int buttonState = digitalRead(buttonPin);

  if (buttonState == LOW) { // Botón presionado
    Serial.println("Botón pulsado, enviando solicitud HTTP...");

    // Enviar solicitud HTTP al webhook de Home Assistant para notificar la caída
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverNameButton);
      int httpResponseCode = http.POST("");

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
      } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    } else {
      Serial.println("Error en la conexión WiFi");
    }

    // Esperar un momento para evitar múltiples lecturas rápidas
    delay(1000);
  }

  // Enviar datos de temperatura y humedad cada minuto
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Leer la temperatura y la humedad del DHT22
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Verificar si la lectura es válida
    if (!isnan(h) && !isnan(t)) {
      // Crear un payload JSON
      String payload = "{\"temperatura\":" + String(t) + ",\"humedad\":" + String(h) + "}";
      Serial.println("Payload: " + payload);

      // Enviar datos a Home Assistant mediante una solicitud HTTP
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverNameSensor);
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST(payload);

        if (httpResponseCode > 0) {
          String response = http.getString();
          Serial.println(httpResponseCode);
          Serial.println(response);
        } else {
          Serial.print("Error en la solicitud: ");
          Serial.println(httpResponseCode);
        }
        http.end();
      } else {
        Serial.println("Error en la conexión WiFi");
      }
    } else {
      Serial.println("Fallo al leer del sensor DHT22");
    }
  }
}
