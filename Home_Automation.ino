#include <WiFi.h>
#include <PZEM004Tv30.h>
#include <PubSubClient.h>

long lastMsg = 0;
// char msg[50];
// int value = 0;

//Ganti dengan credentials ditempat kalian
const char* ssid = "Audidiv";
const char* password = "12341234";

//MQTT Broker Setup
const char* mqtt_broker = "192.168.42.14";        //host broker
const char *topic = "/topik/mqtt/out";            //outTopic
const char *inTopic = "/topik/mqtt/in";           //inTopic
const int mqtt_port = 1883;                       //Port broker

WiFiClient client;
PubSubClient mqtt(client);

#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

#define NUM_PZEMS 4

PZEM004Tv30 pzems[NUM_PZEMS];

#if defined(USE_SOFTWARE_SERIAL) && defined(ESP32)
    #error "Can not use SoftwareSerial with ESP32"
#elif defined(USE_SOFTWARE_SERIAL)

#include <SoftwareSerial.h>

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
#endif

#define relay1 23
#define relay2 22
#define relay3 19
#define relay4 18

float voltage[NUM_PZEMS];
float current[NUM_PZEMS];
float power[NUM_PZEMS];
float energy[NUM_PZEMS];
float frequency[NUM_PZEMS];
float pF[NUM_PZEMS];  

void setup() {
  Serial.begin(115200);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);

  digitalWrite(relay4,HIGH);
  digitalWrite(relay3,HIGH);
  digitalWrite(relay2,HIGH);
  digitalWrite(relay1,HIGH);

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");    
  }
  Serial.println("Wifi Connect");

  mqtt.setServer(mqtt_broker, 1883);
  mqtt.setCallback(callback);

  for(int i = 0; i < NUM_PZEMS; i++) {

#if defined(USE_SOFTWARE_SERIAL)
    pzems[i] = PZEM004Tv30(pzemSWSerial, (i+1)*0x10);
#elif defined(ESP32)
    pzems[i] = PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, (i+1)*0x10);
#else
    pzems[i] = PZEM004Tv30(PZEM_SERIAL, (i+1)*0x10);
#endif
  }
  Serial.println("Ready to monitoring!");
  delay(250);
}

void loop() {
  Serial.println("loop");
  if(!client.connected()){
    Serial.println("reconnect");
    reconnect();
  }
  mqtt.loop();

  long now = millis();
  if(now - lastMsg > 2000){
    lastMsg = now;
    // ++value;
    // snprintf(msg, 75, "hello world #%ld", value);
    // Serial.print("Publish message : ");
    // Serial.println(msg);
    // mqtt.publish(topic, msg);

    for(int i = 0; i < NUM_PZEMS; i++){
      Serial.print("PZEM ");
      Serial.print(i);
      Serial.print(" - Address:");
      Serial.println(pzems[i].getAddress(), HEX);
      Serial.println("===================");

      voltage[i] = pzems[i].voltage();
      current[i] = pzems[i].current();
      power[i] = pzems[i].power();
      energy[i] = pzems[i].energy();
      frequency[i] = pzems[i].frequency();
      pF[i] = pzems[i].pf();

      if(isnan(voltage[i])){
        Serial.println("Error reading voltage");
      } else if (isnan(current[i])) {
        Serial.println("Error reading current");
      } else if (isnan(power[i])) {
        Serial.println("Error reading power");
      } else if (isnan(energy[i])) {
        Serial.println("Error reading energy");
      } else if (isnan(frequency[i])) {
        Serial.println("Error reading frequency");
      } else if (isnan(pF[i])) {
        Serial.println("Error reading power factor");
      } else {
        Serial.print("Voltage: ");      Serial.print(voltage[i]);      Serial.println("V");
        Serial.print("Current: ");      Serial.print(current[i]);      Serial.println("A");
        Serial.print("Power: ");        Serial.print(power[i]);        Serial.println("W");
        Serial.print("Energy: ");       Serial.print(energy[i],3);     Serial.println("kWh");
        Serial.print("Frequency: ");    Serial.print(frequency[i], 1); Serial.println("Hz");
        Serial.print("PF: ");           Serial.println(pF[i]);
      }
      Serial.println("-------------------");
      Serial.println();
    }

    String PZEM0 = "\"PZEM0\":{\"voltage\":\""+ String(voltage[0])+ "\",\"current\":\"" + String(current[0]) + "\",\"power\":\"" + String(power[0]) + "\",\"energy\":\"" + String(energy[0]) + "\",\"freq\":\"" + String(frequency[0]) + "\",\"pf\":\"" + String(pF[0]) + "\"}";
    String PZEM1 = "\"PZEM1\":{\"voltage\":\""+ String(voltage[1])+ "\",\"current\":\"" + String(current[1]) + "\",\"power\":\"" + String(power[0]) + "\",\"energy\":\"" + String(energy[1]) + "\",\"freq\":\"" + String(frequency[1]) + "\",\"pf\":\"" + String(pF[1]) + "\"}";
    String PZEM2 = "\"PZEM2\":{\"voltage\":\""+ String(voltage[2])+ "\",\"current\":\"" + String(current[2]) + "\",\"power\":\"" + String(power[0]) + "\",\"energy\":\"" + String(energy[2]) + "\",\"freq\":\"" + String(frequency[2]) + "\",\"pf\":\"" + String(pF[2]) + "\"}";
    String PZEM3 = "\"PZEM3\":{\"voltage\":\""+ String(voltage[3])+ "\",\"current\":\"" + String(current[3]) + "\",\"power\":\"" + String(power[0]) + "\",\"energy\":\"" + String(energy[3]) + "\",\"freq\":\"" + String(frequency[3]) + "\",\"pf\":\"" + String(pF[3]) + "\"}";
    String responseBody = "{" + PZEM0 + "," + PZEM1 + "," + PZEM2 + "," + PZEM3 + "}";
    mqtt.publish(topic, responseBody.c_str());
  }
  delay(250);
}

//Pesan masuk untuk mengatur Relay============================================================
void callback(char *topic, byte *payload, unsigned int length){
  Serial.print("Message Arrived in :");
  Serial.println(topic);
  Serial.print("Message : ");
  for(int i = 0; i < length; i++){
    Serial.print((char) payload[i]);
  }
  Serial.println("\n-------------------------------");

}

void reconnect(){
  while(!mqtt.connected()){
    if(mqtt.connect("ESP32Test")){
      Serial.println("Connected! 2");
      mqtt.subscribe(inTopic);
      mqtt.publish(topic, "test 2");
    } else {
      Serial.println(mqtt.state());
      delay(5000);
    }
  }
}