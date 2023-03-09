#include <WiFi.h>
#include <HTTPClient.h>
#include <PZEM004Tv30.h>

const char* ssid = "Padepokan79_bootcamp";
const char* password = "Majubersama2020";

const char* URLOrigin = "http://10.10.20.223:8084/power-monitor";

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

  digitalWrite(relay4,LOW);
  digitalWrite(relay3,LOW);
  digitalWrite(relay2,LOW);
  digitalWrite(relay1,LOW);

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println(".");    
  }

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
}

void loop() {
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

  Serial.println();
  delay(2000);
}

void sendToServer() {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, URLOrigin);

  // If you need an HTTP request with a content type: application/json, use the following:
  http.addHeader("Content-Type", "application/json");
  String PZEM1 = "\"PZEM0\":\"voltage\":\""+ String(voltage[0])+ "\"}";
  int httpResponseCode = http.POST("{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}");


}

