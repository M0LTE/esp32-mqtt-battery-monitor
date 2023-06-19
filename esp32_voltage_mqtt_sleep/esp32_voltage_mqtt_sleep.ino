/* circuit:
 *   gnd -> 12k -> VP pin
 *   VP pin -> 55k -> bat+
 *   (VP pin is ADC ch 0)
 *   
 *   module is ESP32 WROOM dev board
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include <PubSubClient.h>

const char *ssid = "mywifi";
const char *password = "mywifipassword"; 

const char *mqtt_broker = "mymqttserver";
const char *topic = "mytopic";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;

const int sensorPin = A0;
int value;
double volts;

static int sum;
const int samples = 2000;

const double m = 0.004380370945;
const double c = 0.911157725;

static char outstr[10];

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60       /* Time ESP32 will go to sleep (in seconds) */

WiFiClient espClient;
PubSubClient client(espClient);

int sample() {
  sum = 0;
  for (int i = 0; i < samples; i++){
    sum += analogRead(sensorPin);
    tryGiveUp();
  }
  return sum / samples;
}

void disconnectMqtt() {
  client.disconnect(); 
  espClient.flush();
  while(client.state() != -1 && millis() < 60000){
    delay(10);
  }
}

void goToSleep(){
  disconnectMqtt();
  esp_wifi_stop();
  esp_deep_sleep_start();
}

void tryGiveUp(){
  if (millis() > 60000){
    goToSleep();
  }
}

void setup() {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    tryGiveUp();
    delay(500);
  }
  client.setServer(mqtt_broker, mqtt_port);
  String client_id = "esp32-client-";
  client_id += String(WiFi.macAddress());  
  while (!client.connected()) {
    tryGiveUp();
    if (!client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.print("MQTT connect failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  value = sample();
  if (value == 0){
    volts = 0;
  } else {
    volts = m * value + c;
  }
  Serial.print("read ");
  Serial.print(value);
  Serial.print(" = ");
  Serial.print(volts);
  Serial.println("V");
  Serial.flush();
  dtostrf(volts, 5, 2, outstr);
  client.publish(topic, outstr);
  goToSleep();
}

void loop() {
}
