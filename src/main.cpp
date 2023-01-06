#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <UltraSonic.h>
#include <arduino.h>
#include <ArduinoJson.h>

//ULTRASONIC
#define TRIGGER_PIN  D6
#define ECHO_PIN     D7
Ultrasonic sonar(TRIGGER_PIN, ECHO_PIN);

//WATER LEVEL
#define waterLevel A0
int waterLevelSensorValue = 0; // variable untuk menampung nilai baca dari sensor dalam bentuk integer
float tinggiAir = 0; // variabel untuk menampung ketinggian air
float WaterLevelsensorVoltage = 0; // untuk menampung nilai ketinggian air

int nilaiMaxWaterLevel = 1023; // nilai "sensorValue" saat sensor terendam penuh kedalam air, bisa dirubah sesuai sensor dan jenis air yang anda pakai
float panjangWaterLevelSensor = 4.0 ; // 4.0 cm, bisa dirubah, menyesuikan dengan panjang sensor yang kalian gunakan

//buzzer
#define buzzer D1

//relay
#define relay D5

// WiFi
const char *ssid = "Golongan kami"; // Enter your WiFi name
const char *password = "1sampai8";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "34.66.102.226";
const char *topicUltraSonic = "tubes/ultrasonic";
const char *topicWaterLevel = "tubes/waterlevel";
const char *topicKondisiHardware = "tubes/kondisiHardware";
const char *topic = "tubes/callbackRelay";
const char *mqtt_username = "admin";
const char *mqtt_password = "password";
const int mqtt_port = 1883;

//buffer mqtt
char message_buff[100];
long lastMsg = 0;
char msg[50];
int value = 0;

WiFiClient espClient;
PubSubClient client(espClient);


// WiFi Connect
void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT Connect
void mqttConnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }

}

// Callback MQTT FROM TELEGRAM
void callback(char* topic, byte* payload, unsigned int length) {

  StaticJsonDocument<256> doc;                                                              // Allocate memory for the doc array.
  deserializeJson(doc, payload, length);                                                    // Convert JSON string to something useable.

  Serial.print("Message arrived: ["); 
  Serial.print(topic); 
  Serial.println("]");             // Prints out anything that's arrived from broker and from the topic that we've subscribed to.

  int i; 
  for (i = 0; i < length; i++) 
  message_buff[i] = payload[i]; 
  message_buff[i] = '\0';  // We copy payload to message_buff because we can't make a string out of payload.
  
  String msgString = String(message_buff);                                                  // Finally, converting our payload to a string so we can compare it.
  // Serial.println(msgString);                                                                // Prints out the message.                                                                       // Call the JSONencoder function and pass it the doc array.


  if (msgString == "on") {                                                                  // If the message is "on", turn on the LED.
    digitalWrite(relay, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
  } else if (msgString == "off") {                                                          // If the message is "off", turn off the LED.
    digitalWrite(relay, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

float waterLevelFunction() {
  waterLevelSensorValue = analogRead(waterLevel); // baca nilai sensor
  WaterLevelsensorVoltage = (waterLevelSensorValue * 5.0) / nilaiMaxWaterLevel; // konversi nilai sensor ke nilai tegangan
  tinggiAir = (WaterLevelsensorVoltage * panjangWaterLevelSensor) / 5.0; // konversi nilai tegangan ke nilai ketinggian air
  Serial.print("Tinggi air : ");
  Serial.print(tinggiAir);
  Serial.println(" cm");
  StaticJsonDocument<200> doc;
  doc["waterLevel"] = tinggiAir;
  char buffer[200];
  serializeJson(doc, buffer);
  client.publish(topicWaterLevel, buffer);
  delay(1000);
  return tinggiAir;
}

// Setup
void setup() {
  Serial.begin(9600);
  wifiConnect();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  pinMode(relay, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buzzer, OUTPUT);
  client.setServer(mqtt_broker, mqtt_port);
  mqttConnect();
  client.setCallback(callback);
}

// Loop

void loop() {
  client.loop();
  long ketinggiAir = waterLevelFunction();
  long distance = sonar.read();
  digitalWrite(buzzer,HIGH);
  delay(1000);
  digitalWrite(buzzer,LOW);
  delay(1000);

  if(ketinggiAir <= 0.9) {
    digitalWrite(relay, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
  } else if (distance <=6) {
    digitalWrite(relay, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    
  } else {
    //do nothing

  }


  // if (distance <= 5 && ketinggiAir <= 3) {
  //   digitalWrite(relay, LOW);
  //   digitalWrite(LED_BUILTIN, HIGH);
  //   // noTone(buzzer);
  //   // delay(1000);

  // } else if (distance <= 10 && ketinggiAir > 3) {
  //   digitalWrite(relay, HIGH);
  //   digitalWrite(LED_BUILTIN, LOW);
  //   // tone(buzzer, 1000);
  //   // delay(1000);
  // }

  StaticJsonDocument<200> doc;
  doc["distance"] = distance;
  Serial.print("distance: ");
  Serial.println(distance);
  char buffer[200];
  serializeJson(doc, buffer);
  client.publish(topicUltraSonic, buffer);
  delay(1000);

 
}




