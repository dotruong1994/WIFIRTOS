#include <Arduino.h>
#include <MQTT.h>
#include "WiFi.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EasyButton.h>
#include <Wire.h>
#include "time.h"

#define BUTTON_PIN 33
#define LED 27
int i=0;
EasyButton button(BUTTON_PIN);

// #define WIFI_NETWORK "Redmi K30 5G"
// #define WIFI_PASSWORD "11223344"

#define WIFI_NETWORK "factory-J1-1F-1"
#define WIFI_PASSWORD "qwerasdf"

// #define WIFI_NETWORK "factory-A17-1F-1"
// #define WIFI_PASSWORD "qwerasdf"


#define WIFI_TIMEOUT_MS 10000 // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 10000 // Wait 30 seconds after a failed connection attempt

TaskHandle_t Task1;
TaskHandle_t Task2;

const char* ntpServer = "time.pouchen.com";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 0;
char timeHour[9];

const char* mqtt_server = "172.21.149.102";
WiFiClient espClient;
PubSubClient client(espClient);

typedef void (*MQTTClientCallbackSimple)(String &topic, String &payload);

void keepWiFiAlive(void * parameter);
void sencuttingTime(void * parameter);
void blink(void * parameter);
void printLocalTime();
void cutting();
void messageReceived(String &topic, String &payload);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  button.begin();
  client.setServer(mqtt_server, 8000);
  pinMode(LED,OUTPUT);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  xTaskCreatePinnedToCore(
    keepWiFiAlive,
    "keepWiFiAlive",  // Task name
    10000,             // Stack size (bytes)
    NULL,             // Parameter
    1,                // Task priority
    NULL,             // Task handle
    1
);

  xTaskCreatePinnedToCore(
    sencuttingTime,
    "sendCutting",
    64000,
    NULL,
    1,
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    blink,
    "blink",
    11000,
    NULL,
    1,
    NULL,
    1
  );

}

void sencuttingTime(void * parameter){
  for(;;){
    client.loop();
    vTaskDelay(10);
    button.read();
    button.onPressed(cutting);
    // digitalWrite(LED,HIGH);
    // vTaskDelay(1000/ portTICK_PERIOD_MS);
    // digitalWrite(LED,LOW);
    // vTaskDelay(1000/ portTICK_PERIOD_MS);
    //vTaskDelete(NULL);
  }
}

void blink(void * parameter){
  for(;;){
    digitalWrite(LED,HIGH);
    vTaskDelay(1000/ portTICK_PERIOD_MS);
    digitalWrite(LED,LOW);
    vTaskDelay(1000/ portTICK_PERIOD_MS);
    
    //vTaskDelete(NULL);
  }
}


void loop() {
  // put your main code here, to run repeatedly:
}




/**
 * Task: monitor the WiFi connection and keep it alive!
 * 
 * When a WiFi connection is established, this task will check it every 10 seconds 
 * to make sure it's still alive.
 * 
 * If not, a reconnect is attempted. If this fails to finish within the timeout,
 * the ESP32 will wait for it to recover and try again.
 */
void keepWiFiAlive(void * parameter){
    for(;;){
        if(WiFi.status() == WL_CONNECTED){
            printLocalTime();
            while(!client.connect("esp32_J2")){
              Serial.print("Attempting MQTT connection...");
              vTaskDelay(1000);
            }
            
            if(client.connect("esp32_J1")){
                Serial.println("MQTT Connected");
            }
            Serial.println("WiFi still connected");
            // Serial.println(WiFi.SSID());
            // Serial.print("IP address: ");           
            // Serial.println(WiFi.localIP());
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }
        Serial.println("[WIFI] Connecting");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
        
        unsigned long startAttemptTime = millis();

        // Keep looping while we're not connected and haven't reached the timeout
        while (WiFi.status() != WL_CONNECTED && 
                millis() - startAttemptTime < WIFI_TIMEOUT_MS){}

        // When we couldn't make a WiFi connection (or the timeout expired)
		  // sleep for a while and then retry.
        if(WiFi.status() != WL_CONNECTED){
            Serial.println("[WIFI] FAILED");           
            vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
			  continue;
        }

        Serial.println("[WIFI] Connected: " + WiFi.localIP());
    }
  //  vTaskDelete(NULL);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(timeHour,9, "%T", &timeinfo);
  String timeHour1 = String(timeHour);
  String timeresetShift1 = "06:00:00"; //06:00:00
  String timeresetShift2 = "14:00:00"; //14:00:00
  String timeresetShift3 = "22:00:00";
  if (timeHour1 == timeresetShift1){Serial.println("ESP reset chứ còn cái nịt gì nữa");ESP.restart();}
  if (timeHour1 == timeresetShift2){Serial.println("ESP reset chứ còn cái nịt gì nữa");i=0;}
  if (timeHour1 == timeresetShift3){Serial.println("ESP reset chứ còn cái nịt gì nữa");i=0;}
}

void cutting(){
  if(!client.connect("esp32_J1")){
    Serial.println("MQTT failed");
  }
  
  i++;
  StaticJsonDocument<64> doc;
  doc["Machine"] = "machine_1";
  doc["Dept"] = "J1";
  doc["cuttingTime"] = i;
  char buffer[256];
  serializeJson(doc, buffer);
  client.publish("esp32_J1", buffer);
  Serial.println("Button sent");
}
