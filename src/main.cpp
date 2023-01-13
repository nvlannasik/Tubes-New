#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <arduino.h>
#include <ArduinoJson.h>
#include <NewPing.h>

//ULTRASONIC
#define TRIGGER_PIN  D6
#define ECHO_PIN     D7
#define MAX_DISTANCE 30
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

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
const char *ssid = "Tubes"; // Enter your WiFi name
const char *password = "tubes12345678";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "34.66.102.226";
const char *topicSensor = "tubes/sensor";
const char *topicKondisiSensor = "tubes/kondisiSensor";
const char *topic = "tubes/callbackRelay";
const char *mqtt_username = "admin";
const char *mqtt_password = "password";
const int mqtt_port = 1883;

//buffer mqtt
char message_buff[100];
long lastMsg = 0;
char msg[50];
int value = 0;

bool buzzerhidup = true;

WiFiClient espClient;
PubSubClient client(espClient);


// WiFi Connect
void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    tone(buzzer, 2000);
    delay(500);
    noTone(buzzer);
    tone(buzzer, 2000);
    delay(100);
    noTone(buzzer);
    delay(100);
    tone(buzzer, 2000);
    noTone(buzzer);
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  tone(buzzer, 2000);
  delay(100);
  noTone(buzzer);
  delay(100);
  tone(buzzer, 2000);
  delay(100);
  noTone(buzzer);
  delay(1000);

}

// MQTT Connect
void mqttConnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(topic);
      tone(buzzer, 2000);
      delay(100);
      noTone(buzzer);
      delay(100);
      tone(buzzer, 2000);
      delay(100);
      noTone(buzzer);
      delay(100);
      tone(buzzer, 2000);
      delay(100);
      noTone(buzzer);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      tone(buzzer, 2000);
      delay(500);
      noTone(buzzer);
      delay(100);
      tone(buzzer, 2000);
      delay(100);
      noTone(buzzer);
      delay(100);
      tone(buzzer, 2000);
      delay(100);
      noTone(buzzer);
      delay(100);
      tone(buzzer, 2000);
      delay(100);
      noTone(buzzer);
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }

}

// Callback MQTT FROM TELEGRAM
void callback(char* topic, byte* payload, unsigned int length) {

  StaticJsonDocument<200> doc;                                                              // Allocate memory for the doc array.
  deserializeJson(doc, payload, length);                                                    // Convert JSON string to something useable.

  Serial.print("Message arrived: ["); 
  Serial.print(topic); 
  Serial.println("]");             // Prints out anything that's arrived from broker and from the topic that we've subscribed to.

  int i; 
  for (i = 0; i < length; i++) 
  message_buff[i] = payload[i]; 
  message_buff[i] = '\0';  // We copy payload to message_buff because we can't make a string out of payload.
  
  String msgString = String(message_buff);// Finally, converting our payload to a string so we can compare it.
  // Serial.println(msgString);                                                                // Prints out the message.                                                                       // Call the JSONencoder function and pass it the doc array.


  if (msgString == "on") { // If the message is "on", turn on the LED.
    digitalWrite(relay, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("relay on");
    tone(buzzer, 600);
    delay(500);
    noTone(buzzer);
  } else if (msgString == "off") { // If the message is "off", turn off the LED.
    digitalWrite(relay, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("relay off");
    // buzzerhidup=false;
    tone(buzzer, 2000);
    delay(500);
    noTone(buzzer);
    delay(200);
    tone(buzzer, 3000);
    delay(200);
    noTone(buzzer);
    delay(200);
    tone(buzzer, 4000);
    delay(200);
    noTone(buzzer);
  }
}

float waterLevelFunction() {
  waterLevelSensorValue = analogRead(waterLevel); // baca nilai sensor
  WaterLevelsensorVoltage = (waterLevelSensorValue * 5.0) / nilaiMaxWaterLevel; // konversi nilai sensor ke nilai tegangan
  tinggiAir = round(WaterLevelsensorVoltage * panjangWaterLevelSensor) / 5.0; // konversi nilai tegangan ke nilai ketinggian air
  Serial.print("Tinggi air : ");
  Serial.print(tinggiAir);
  Serial.println(" cm");
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
  long distance = sonar.ping_cm();

  if (distance>6){
    digitalWrite(relay, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("relay on");
    tone(buzzer, 600);
    delay(500);
    noTone(buzzer);
    buzzerhidup=true;

    //send to mqtt kondisi sensor
    StaticJsonDocument<200> doc;
    doc["kondisiSensor"] = "ON";
    char buffer[200];
    serializeJson(doc, buffer);
    client.publish(topicKondisiSensor, buffer);
    Serial.println("kondisi sensor on");
    delay(1000);
  
  }

  if (distance<5 && buzzerhidup == true){
    digitalWrite(relay, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("relay off");
    buzzerhidup=false;
    tone(buzzer, 2000);
    delay(500);
    noTone(buzzer);
    delay(200);
    tone(buzzer, 3000);
    delay(200);
    noTone(buzzer);
    delay(200);
    tone(buzzer, 4000);
    delay(200);
    noTone(buzzer);

    //send to mqtt kondisi sensor
    StaticJsonDocument<200> doc;
    doc["kondisiSensor"] = "OFF";
    char buffer[200];
    serializeJson(doc, buffer);
    client.publish(topicKondisiSensor, buffer);
    Serial.println("kondisi sensor off");
    delay(1000);
  }

  StaticJsonDocument<200> doc;
  char buffer[200];
  doc["distance"]= distance;
  doc["ketinggiAir"]= ketinggiAir;
  serializeJson(doc, buffer);
  client.publish(topicSensor, buffer);
  Serial.print("distance: ");
  Serial.println(distance);
  delay(1000);

    //reconnect wifi

  if (WiFi.status() != WL_CONNECTED) {
    wifiConnect();
    mqttConnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }


}




