#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include <EEPROM.h>

#define mqtt_server "mqtt1.gpstech.com.br"
#define mqtt_port 1883
#define mqtt_user "teste_gerson_pluvi"
#define mqtt_password "1234"
#define mqtt_client_id "teste_gerson_pluvi"
#define Pluviometro "gerson/Gerson_teste/pluviometro"


const int REED = 14;              //The reed switch outputs to digital pin 9

// Variáveis:
int val = 0;
int old_val = 0;
int REEDCOUNT = 0;
float chuva = 0;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP udp;//Cria um objeto "UDP".
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000);

String hora;
int flag;
int parte1, parte2;

void setup() {
  pinMode (REED, INPUT_PULLUP);
  Serial.begin(9600);
  EEPROM.begin(50);
  parte1 = EEPROM.read(0);
  parte2 = EEPROM.read(1);
  REEDCOUNT =   (parte1 * 256) + parte2;
  delay(100);
  Serial.print(REEDCOUNT);
  Serial.println ("Chuva");
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout ( 60);
  wifiManager.autoConnect("Pluviometro");
  client.setServer(mqtt_server, mqtt_port);
  ArduinoOTA.onStart([]() {
    Serial.println("Inicio...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("nFim!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Autenticacao Falhou");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha no Inicio");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha na Conexao");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha na Recepcao");
    else if (error == OTA_END_ERROR) Serial.println("Falha no Fim");
  });
  ArduinoOTA.begin();
  Serial.println("Pronto");
  ntp.begin();//Inicia o NTP.
  ntp.forceUpdate();//Força o Update.
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    break;
  }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();


  hora = ntp.getFormattedTime();
  Serial.println(hora);
  if ((hora >= "23:30:30") && (hora <= "23:59:30") && (flag == 0))
  {
    if (REEDCOUNT != 0) {
      chuva = 0;
      flag = 1;
      REEDCOUNT = 0;
      EEPROM.write(0, REEDCOUNT);
      EEPROM.write(1, REEDCOUNT);
      EEPROM.commit();
      Serial.println("flag 1");
      client.publish(Pluviometro, String(chuva).c_str(), true);
    }
  }

  if ((hora >= "18:30:30") && (hora <= "19:30:30") && (flag == 1))
  {
    flag = 0;
    Serial.println("flag 0");

  }

  val = digitalRead(REED);
  if ((val == LOW) && (old_val == HIGH)) {   //Check to see if the status has changed

    delay(10);                   // Delay put in to deal with any "bouncing" in the switch.

    REEDCOUNT = REEDCOUNT + 1;
    EEPROM.write(0, REEDCOUNT / 256);
    EEPROM.write(1, REEDCOUNT % 256);
    EEPROM.commit();//Add 1 to the count of bucket tips

    old_val = val;              //Make the old value equal to the current value
    Serial.print("Medida de chuva (contagem): ");
    Serial.print(REEDCOUNT);//*0.2794);
    Serial.println(" pulso");
    Serial.print("Medida de chuva (calculado): ");
    chuva = (REEDCOUNT * 0.25);
    Serial.print(chuva);
    Serial.println(" mm");
    client.publish(Pluviometro, String(chuva).c_str(), true);
  }

  else {

    old_val = val;


  }
}
