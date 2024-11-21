#include <WiFi.h>
#include <PubSubClient.h>

const int ldrPin = 34;    // Pin do sensor de luz LDR
const int pirPin = 14;    // Pin do sensor de movimento PIR
const int ledPin = 2;     // Pin do LED

const int maxNaturalLight = 4063;
const int minNaturalLight = 32;
const int naturalLightThreshold = 500;  // Limite para intensidade de luz (ajustável)
const int numLeituras = 10; // Quantidade de leituras realizadas pela média móvel

// variáveis da intensidade da luz
bool lightsOn = true;

int currentLight = 0;
int targetLight = 0;

int naturalLightLevel = 0;
int motionDetected = 0;

// Timer da Intensidade da Luz
bool canUpdate = true; 
int updateInterval = 15; //in miliseconds
unsigned long lastTime = 0;

//Timer Envio de Dados
bool canSendData = true;
int sendDataInterval = 800;
unsigned long lastSendData = 0;

// setting PWM properties
const int frequency = 5000;  // setting frequency to 5KHz
const int ledChannel = 0;    // using led channel 0
const int resolution = 8;    // using 8 bit resolution to control the brightness from 0 to 255.

// FIWARE Settings

const char* default_SSID = "Wokwi-GUEST"; // Nome da rede Wi-Fi
const char* default_PASSWORD = ""; // Senha da rede Wi-Fi
const char* default_BROKER_MQTT = "34.55.39.20"; // IP do Broker MQTT
const int default_BROKER_PORT = 1883; // Porta do Broker MQTT
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp003/cmd"; // Tópico MQTT de escuta
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp003/attrs"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp003/attrs/i"; // Tópico MQTT - Intensidade da Iluminação
const char* default_TOPICO_PUBLISH_3 = "/TEF/lamp003/attrs/l"; // Tópico MQTT - Luz Natural
const char* default_TOPICO_PUBLISH_4 = "/TEF/lamp003/attrs/m"; // Tópico MQTT - Sensor de Movimento
const char* default_ID_MQTT = "fiware_001"; // ID MQTT
const int default_D4 = 4; // Pino do LED onboard

// Declaração da variável para o prefixo do tópico
const char* topicPrefix = "lamp003";

// Variáveis para configurações editáveis
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* TOPICO_PUBLISH_3 = const_cast<char*>(default_TOPICO_PUBLISH_3);
char* TOPICO_PUBLISH_4 = const_cast<char*>(default_TOPICO_PUBLISH_4);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;

WiFiClient espClient;
PubSubClient MQTT(espClient);


void increaseBrightness() {
  // increase the LED brightness 
  currentLight++;
  ledcWrite(ledPin, currentLight);
  Serial.println("\nIncreasing Current Light");
}

void decreaseBrightness() {
  // decrease the LED brightness
  currentLight--;
  ledcWrite(ledPin, currentLight);
  Serial.println("\nDecreasing Current Light");
}

void setPWM() {
  // configure LED PWM functionalitites
  ledcAttach(ledPin, frequency, resolution);
  pinMode(ldrPin, INPUT);
  pinMode(pirPin, INPUT);
}

void initSerial() {
  Serial.begin(115200);
}

void setup() {
  setPWM();

  initSerial();
  initWiFi();
  initMQTT();
  MQTT.publish(TOPICO_PUBLISH_1, "s|on");
  
  


}

void loop() {
  int mediaMovel = 0;
  for (int i = 0; i < numLeituras; i++) {
    mediaMovel += analogRead(ldrPin);
  }
  naturalLightLevel = mediaMovel / numLeituras;      // Lê o valor de luz
  motionDetected = digitalRead(pirPin); // Lê o valor do PIR


  // Atualizar Intensidade da Luz  

  if (canUpdate == false && millis() >= lastTime + updateInterval) {
    canUpdate = true;
  }

  if(canUpdate == true && currentLight != targetLight) {
    if (currentLight < targetLight) {
      increaseBrightness();
      lastTime = millis();
      canUpdate = false;
    }
    else if (currentLight > targetLight) {
      decreaseBrightness();
      lastTime = millis();
      canUpdate = false;
    }
    
  }

  // Enviar dados para o servidor MQTT

  if (canSendData == false && millis() >= lastSendData + sendDataInterval) {
    canSendData = true;
  }
  else if (canSendData == true) {
    lastSendData = millis();
    canSendData = false;

    VerificaConexoesWiFIEMQTT();
    EnviaEstadoOutputMQTT();
    EnviaDadosMQTT(); // Envia os dados de Intensidade, Luz Natural e Sensor de Movimento para o servidor
  }

  // Condições para controle de luz
  if (motionDetected == HIGH && lightsOn == true) {
    // Acende as luzes se o ambiente estiver escuro e houver movimento

    targetLight = map(naturalLightLevel, maxNaturalLight, minNaturalLight, 0, 255);

  } else if (lightsOn == true) {
    // Apaga as luzes caso contrário
    
    targetLight = 0;

  }
  else {
    currentLight = 0;
    targetLight = 0;
    ledcWrite(ledPin, currentLight);

  }

  MQTT.loop(); // Importante

  // Debug no console serial
  //Serial.print("\nLuminosidade: \n");
  //Serial.print(naturalLightLevel);
  //Serial.print("\nMovimento detectado: \n");
  Serial.println("");
  Serial.print(motionDetected);
  Serial.println("\n");
  Serial.print(currentLight);
}

void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconectWiFi();
}

void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);
}

void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return;
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());

    // Garantir que o LED inicie desligado
    digitalWrite(D4, LOW);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    // Forma o padrão de tópico para comparação
    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    // Compara com o tópico recebido
    if (msg.equals(onTopic)) {
        lightsOn = true;
    }

    if (msg.equals(offTopic)) {
        lightsOn = false;
    }
}

void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();
    reconectWiFi();
}

void EnviaEstadoOutputMQTT() {
    if (lightsOn == true) {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on");
        Serial.println("- Lâmpada Ligada");
    }

    if (lightsOn == false) {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off");
        Serial.println("- Lâmpada Desligada");
    }
}

void InitOutput() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
    boolean toggle = false;

    for (int i = 0; i <= 10; i++) {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
}

void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        } else {
            Serial.println("Falha ao reconectar no broker.");
        }
    }
}

void EnviaDadosMQTT() {
    String mensagem;

    mensagem = String(currentLight);
    Serial.print("Intensidade: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str());

    mensagem = String(naturalLightLevel);
    Serial.print("Luz Natural: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_3, mensagem.c_str());

    mensagem = String(lightsOn);
    Serial.print("Lâmpada ligada: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_4, mensagem.c_str());
}
