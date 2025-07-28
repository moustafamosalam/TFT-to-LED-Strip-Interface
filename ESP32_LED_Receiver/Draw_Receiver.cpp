#include <Arduino.h>
#include <FastLED.h>  
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#define LED_PIN  13
#define COLOR_ORDER GRB
#define LED_TYPE     WS2812
#define BRIGHTNESS 200
#define NUM_LEDS  573

CRGB leds[NUM_LEDS];

QueueHandle_t jsonQueue2;
// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int x;
  int y;
  uint32_t color;
} myData;

int invert(int y){
  return 23 - y;
}

//Function to convert uint16_t color to CRGB (uint32_t)
CRGB rgb565ToCRGB(uint16_t color) {
  uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((color >> 5)  & 0x3F) * 255 / 63;
  uint8_t b = (color & 0x1F) * 255 / 31;
  return CRGB(r, g, b);
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  // memcpy(&myData, incomingData, sizeof(myData));
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, incomingData);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  struct_message data;
  data.x = doc["x"];
  data.y = doc["y"];
  data.color = doc["color"];
  xQueueSend(jsonQueue2, &data, portMAX_DELAY);
}
void ledStripTask(void *) {
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  struct_message data;
  while (1) {
    if (xQueueReceive(jsonQueue2, &data, portMAX_DELAY)) {
      int x = data.x;
      int y = data.y;
      uint32_t color = data.color;
      
      int mappedXValue = map(x, 0, 280, 22, 0);
      int mappedYValue = map(y, 0, 239, 0, 25);

      if(mappedXValue % 2 != 0){
        mappedYValue = invert(mappedYValue);
      }
      if(color == 0xD69A ){
        FastLED.clear();
        FastLED.show();
      }
      else{
        int ledNumber = (mappedXValue*23) + mappedYValue;
        leds[ledNumber] = rgb565ToCRGB(color);
        FastLED.show();
      }
      /* uncomment for debugging*/
      // Serial.print(x);
      // Serial.print("    ");
      // Serial.print(y); 
      // Serial.print("    ");
      Serial.print(color);
      Serial.println("    ");
      // Serial.println(xPortGetCoreID());
    
      /* Write your code here  */
    }
  }
}
void IoTTask(void *) {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  while(1)
  {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  jsonQueue2 = xQueueCreate(10, sizeof(struct_message));
  if (jsonQueue2 == NULL) {
    Serial.println("Queue Creation Failed!");
  }

  xTaskCreatePinnedToCore(IoTTask, "Task A", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(ledStripTask, "Task B", 4096, NULL, 1, NULL, 1);
}

void loop() {
}