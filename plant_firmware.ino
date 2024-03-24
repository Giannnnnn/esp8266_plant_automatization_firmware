#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// CONFIGURAÇÕES DE WIFI, MQTT E PORTAS LÓGICAS
const char* ssid = "Giovani";
const char* password = "sucodeuva";
const char* mqtt_server = "193.133.195.176";
#define MAX_DISTANCE 700
float timeOut = MAX_DISTANCE * 60;
int soundVelocity = 340;
WiFiClient espClient;
PubSubClient client(espClient);

const int pinControleBomba = 2;
const int moisturePin = A0;
const int trigPin = 13;
const int echoPin = 12;

const unsigned long pumpDuration = 3000; // DURAÇÃO DA ATIVAÇÃO DA BOMBA DE ÁGUA
unsigned long lastPumpActivationTime = 0; // SEGUNDOS DESDE A ÚLTIMA ATIVAÇÃO
bool pumpActivated = false; // FLAG DEMARCANDO SE A BOMBA DE ÁGUA ESTÁ ATIVA

// TAMANHO DA MENSAGEM A SER PUBLICADA
char msg[50];

// INICIALIZACAO E CONFIGURACAO INICIAL
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pinControleBomba, OUTPUT);
  delay(100);

  digitalWrite(pinControleBomba, HIGH); // REVALIDAÇÃO DE QUE A BOMBA DE ÁGUA FOI DESATIVADA
}

// LOOP PRINCIPAL
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(5000);
  // Verifica se a bomba está ativada e se passou o tempo de ativação especificado
  if (pumpActivated && (millis() - lastPumpActivationTime >= pumpDuration)) {
    // Desativa a bomba d'água
    Serial.println("DESATIVANDO BOMBA D'ÁGUA");
    digitalWrite(pinControleBomba, HIGH);
    pumpActivated = false; // RESETE A FLAG
  }

// CONFIGURACAO E CONEXAO WI-FI
void setup_wifi() {
  delay(100);
  Serial.println();
  Serial.print("CONECTANDO A ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(5000);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WI-FI CONECTADO");
  Serial.print("ENDEREÇO IP: ");
  Serial.println(WiFi.localIP());
}

// TENTA RECONEXÃO AO SERVIDOR MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("TENTANDO CONEXÃO MQTT...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("CONECTADO");
      client.publish("outTopic", "OLÁ, MUNDO");
      client.subscribe("waterPump");
    } else {
      Serial.print("FALHA, RC=");
      Serial.print(client.state());
      Serial.println(" TENTANDO NOVAMENTE EM 5 SEGUNDOS");
      delay(5000);
    }
  }
}

float getSonar() {
  unsigned long pingTime;
  float distance;
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  pingTime = pulseIn(echoPin, HIGH, timeOut);
  distance = (float)pingTime * soundVelocity / 2 / 10000;
  return distance;
}

  int waterMoisture = analogRead(moisturePin);
  float moisturePercentage = (static_cast<float>(1024 - waterMoisture) / 1024) * 100;
  snprintf(msg, sizeof(msg), "%.2f", moisturePercentage);
  client.publish("moisture", msg);
  publishMoisture(moisturePercentage);
  int distance = getSonar(); // Obtém a distância em cm
  publishDistance(distance);
}

void publishMoisture(float moistureValue) {
  char msg[10];
  snprintf(msg, sizeof(msg), "%.2f", moistureValue);
  client.publish("moisture", msg);
}

void publishDistance(int distanceValue) {
  char msg[10];
  snprintf(msg, sizeof(msg), "%d", distanceValue);
  client.publish("distance", msg);
}

void publishWaterPumpActivationLog() {
  client.publish("waterPumpActivation", "1");
}

void activateWaterPump() {
  digitalWrite(pinControleBomba, LOW);
  lastPumpActivationTime = millis();
  pumpActivated = true; 
  publishWaterPumpActivationLog();
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  if (strcmp(topic, "waterPump") == 0) {
    if ((char)payload[0] == '1' && !pumpActivated) {
      activateWaterPump();
    }
  }
}

