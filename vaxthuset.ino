#include <DHT.h>
#include <DHT_U.h>
#include <dht11.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// Wemos D1/nodemcu pinout
#define D0 16
#define D1 5 // I2C Bus SCL (clock)
#define D2 4 // I2C Bus SDA (data)
#define D3 0 // 10k pullup
#define D4 2 // 10k pullup Same as "LED_BUILTIN", but inverted logic
#define D5 14 // SPI Bus SCK (clock)
#define D6 12 // SPI Bus MISO 
#define D7 13 // SPI Bus MOSI
#define D8 15 // 10k pullup SPI Bus SS (CS)
#define RX 03 // 
#define TX 01 //
// enbart nodemcu #define D9 3 // RX0 (Serial console)
// enbart nodemcu #define D10 1 // TX0 (Serial console)
#define DHTPININ D0
#define DHTPINUT D1
#define DHTTYPE DHT11
const int fuktmatn = A0;
const char* fuktijord;
const int tunnan = D2;
const int pumppin = D4;
const int ventpin1 = D3;
const char* ssid     = "xxxxxxx";
const char* password = "xxxxxx";
IPAddress ip(192, 168, 0, 16);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dnServer(192, 168, 0, 1);
const char* mqttServer = "192.168.0.15";
const int mqttPort = 1883;
const char* mqttuser = "xxxxxx";
const char* mqttpass = "xxxxxx";
WiFiClient espClient;
PubSubClient client(espClient);
DHT dhtin(DHTPININ, DHTTYPE);
DHT dhtut(DHTPINUT, DHTTYPE);
int tunna = 0;
String borfukt;
int borfukt1;
char message_buff[100];
int X;

void setup() {
  dhtin.begin();
  dhtut.begin();
  pinMode(tunnan, INPUT_PULLUP);
  pinMode(pumppin, OUTPUT);
  digitalWrite(pumppin, HIGH);
  pinMode(ventpin1, OUTPUT);
  digitalWrite(ventpin1, HIGH);
  Serial.begin(115200);
  delay(300);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet, dnServer);
  delay(200);
  WiFi.begin(ssid, password);
  delay(200);
  Serial.println(WiFi.localIP());
  client.setServer(mqttServer, mqttPort);
  client.connect("vaxtis", mqttuser, mqttpass);
  client.setCallback(callback);
  client.publish("vaxthus/tunnan", "1");
  client.publish("vaxthus/pump", "0");
  client.subscribe("vaxthus/borfukt");
  ArduinoOTA.setHostname("Vaxthuset");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;
  if (String(topic) == "vaxthus/borfukt") {
    for (i = 0; i < length; i++) {
      message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
    borfukt = String(message_buff);
    borfukt1 = borfukt.toInt();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("ansluter igen");
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gateway, subnet, dnServer);
    delay(500);
    WiFi.begin(ssid, password);
    delay(1000);
    //    client.setServer(mqttServer, mqttPort);
    if (client.connect("vaxtis", mqttuser, mqttpass));
    else {
      delay(2000);
    }
    client.subscribe("vaxthus/borfukt");
  }
}

void tempin()
{
// Läsa av lufttemp, luftfukt & jordfuktighet
  float h = dhtin.readHumidity();
  float t = dhtin.readTemperature();
  int j = analogRead(fuktmatn);
  String temperature = String(t);
  String humidity = String(h);
  String fuktijord = String(j);

  // mqtt sträng jordfuktighet
  fuktijord = map(j, 15, 900, 15, 900);
  int length = fuktijord.length();
  char msgBuffer[length];
  fuktijord.toCharArray(msgBuffer, length + 1);
  client.publish("vaxthus/jordfukt", msgBuffer);
  delay(300);
  // mqtt sträng för temp & luftfuktighet
  String payload = "{";
  payload += "\"temperature\":"; payload += temperature; payload += ",";
  payload += "\"humidity\":"; payload += humidity;
  payload += "}";
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  client.publish( "vaxthus/innetemp", attributes );
}
void temput()
{
  delay(250);
  float f = dhtut.readHumidity();
  float c = dhtut.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(f) || isnan(c)) {
    return;
  }
  String temperature = String(c);
  String humidity = String(f);
  String payload = "{";
  payload += "\"temperature\":"; payload += temperature; payload += ",";
  payload += "\"humidity\":"; payload += humidity;
  payload += "}";
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  client.publish( "vaxthus/utetemp", attributes );
  delay(250);
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  // Läsa mqtt från HA
  client.loop();
  if (borfukt1 > 0 && borfukt1 < 1024) {
    X = borfukt1;
  }
  ArduinoOTA.handle();
  tunna = digitalRead(tunnan);
  //   client.publish("vaxthus/tunnan", "1");
  delay(300);
  while (tunna == HIGH) {
    client.publish("vaxthus/tunnan", "0");
    delay(1000);
    client.publish("vaxthus/pump", "0");
    Serial.println(X);
    delay(1000);
    temput();
    delay(300);
    tempin();
    delay(300);
    tunna = digitalRead(tunnan);
  }
  {
    client.publish("vaxthus/pump", "0");
    delay(1000);
    client.publish("vaxthus/tunnan", "1");
    delay(1000);
    if (!client.connected()) {
      reconnect();
    }
    tempin();
    delay(300);
    temput();
    delay(300);
    float t = dhtin.readTemperature();
    delay(300);
    int j = analogRead(fuktmatn);
    delay(300);
    if (j < X && t < 23 )
    {
      digitalWrite(pumppin, LOW);
      delay(1000);
      digitalWrite(ventpin1, LOW);
      client.publish("vaxthus/pump", "1");
      delay(10000);
      client.publish("vaxthus/pump", "1");
      delay(10000);
      client.publish("vaxthus/pump", "1");
      delay(10000);
      client.publish("vaxthus/pump", "1");
      delay(10000);
      digitalWrite(ventpin1, HIGH);
      client.publish("vaxthus/pump", "1");
      delay(10000);
      client.publish("vaxthus/pump", "1");
      delay(10000);
      //Vilapumpen
      tempin();
      delay(300);
      temput();
      delay(300);
      digitalWrite(pumppin, HIGH);
      client.publish("vaxthus/pump", "0");
      delay(10000);
      client.publish("vaxthus/pump", "0");
      delay(10000);
      client.publish("vaxthus/pump", "0");
      delay(10000);
      client.publish("vaxthus/pump", "0");
      delay(10000);
      client.publish("vaxthus/pump", "0");
      delay(10000);
      float t = dhtin.readTemperature();
      delay(300);
      int j = analogRead(fuktmatn);
      delay(300);
    }
    else if (j > X + 10 )
    {
      digitalWrite(pumppin, HIGH);
      return;
    }
  }
}
