#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <WiFi.h>

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ----------------- Display and Touch Setup -----------------
TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// ----------------- Screen and Drawing Config -----------------
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define COLOR_BAR_WIDTH 40
#define CLEAR_HEIGHT 30

int x = -1, y = -1;
int selectedColorIndex = 0;
uint32_t currentColor = 0x102E50;

const uint32_t colors[] = {
  TFT_BLACK, TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_MAGENTA
  //0x000000,0xFF0000,0x90EE90,0x1E90FF,0xFFD700,0xFFFFFF
  //1060432, 16714581, 12554852, 12310691, 15790533, 7307882
};
const int numColors = sizeof(colors) / sizeof(colors[0]);

// ----------------- ESP-NOW Config -----------------
uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // Broadcast to all
QueueHandle_t jsonQueue;

typedef struct {
  int x;
  int y;
  uint32_t color;
} data_t;

esp_now_peer_info_t peerInfo;

// ----------------- Utility Functions -----------------

void printHexColor(uint32_t color) {
  uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((color >> 5) & 0x3F) * 255 / 63;
  uint8_t b = (color & 0x1F) * 255 / 31;

  Serial.printf("# %x", color);
}

uint32_t getColorFromTouch(int touchY) {
  int boxHeight = (SCREEN_HEIGHT - CLEAR_HEIGHT) / numColors;

  if (touchY >= SCREEN_HEIGHT - CLEAR_HEIGHT) {
    tft.fillRect(COLOR_BAR_WIDTH, 0, SCREEN_WIDTH - COLOR_BAR_WIDTH, SCREEN_HEIGHT, TFT_WHITE);
    drawColorPicker();
    data_t Buffer = { x, y, TFT_LIGHTGREY};

    if (xQueueSend(jsonQueue, &Buffer, portMAX_DELAY) == pdPASS) {
      Serial.println("Task A: Data sent to queue");
    }
    return currentColor;
  }

  int index = touchY / boxHeight;
  if (index >= 0 && index < numColors) {
    selectedColorIndex = index;
    drawColorPicker();
    return colors[index];
  }

  return currentColor;
}

void drawColorPicker() {
  int boxHeight = (SCREEN_HEIGHT - CLEAR_HEIGHT) / numColors;

  for (int i = 0; i < numColors; i++) {
    tft.fillRect(0, i * boxHeight, COLOR_BAR_WIDTH, boxHeight, colors[i]);
    if (i == selectedColorIndex) {
      tft.drawRect(0, i * boxHeight, COLOR_BAR_WIDTH, boxHeight, TFT_WHITE);
    }
  }

  tft.fillRect(0, SCREEN_HEIGHT - CLEAR_HEIGHT, COLOR_BAR_WIDTH, CLEAR_HEIGHT, TFT_LIGHTGREY);
  tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("CLEAR", COLOR_BAR_WIDTH / 2, SCREEN_HEIGHT - CLEAR_HEIGHT / 2, 1);
}

// ----------------- Callback for ESP-NOW -----------------

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("ESP-NOW Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ----------------- Task A: Read Touch + Queue Data -----------------

void TaskGenerateJSON(void *pvParameters) {
  // Touch + Screen Initialization on Core 1
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);
  drawColorPicker();

  while (true) {
    if (touchscreen.tirqTouched() && touchscreen.touched()) {
      TS_Point p = touchscreen.getPoint();

      x = map(p.x, 3720, 460, 1, SCREEN_WIDTH);
      y = map(p.y, 370, 3800, 1, SCREEN_HEIGHT);

      Serial.printf("X: %d  Y: %d  Color (HEX): ", x, y);
      printHexColor(currentColor);

      if (x < COLOR_BAR_WIDTH) {
        currentColor = getColorFromTouch(y);

      } else {
        tft.fillCircle(x, y, 10, currentColor);
      }

      data_t Buffer = { x, y, currentColor };

      if (xQueueSend(jsonQueue, &Buffer, portMAX_DELAY) == pdPASS) {
        Serial.println("Task A: Data sent to queue");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));  // check touch ~5 times per second
  }
}

// ----------------- Task B: Send JSON over ESP-NOW -----------------

void TaskSendJSON(void *pvParameters) {
  data_t Buffer;
  char jsonString[200];

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    vTaskDelete(NULL);
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer");
    vTaskDelete(NULL);
    return;
  }

  while (true) {
    if (xQueueReceive(jsonQueue, &Buffer, portMAX_DELAY) == pdPASS) {
      StaticJsonDocument<200> doc;
      doc["x"] = Buffer.x;
      doc["y"] = Buffer.y;
      doc["color"] = Buffer.color;

      serializeJson(doc, jsonString, sizeof(jsonString));
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)jsonString, strlen(jsonString));

      Serial.printf("Task B: Sent JSON -> %s\n", jsonString);
      if (result != ESP_OK) {
        Serial.println("ESP-NOW Send: Failed");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));  // faster response
  }
}

// ----------------- Arduino Setup -----------------

void setup() {
  Serial.begin(115200);
  delay(500);

  jsonQueue = xQueueCreate(10, sizeof(data_t));
  if (jsonQueue == NULL) {
    Serial.println("Queue creation failed!");
    while (true)
      ;
  }

  xTaskCreatePinnedToCore(TaskGenerateJSON, "TouchTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskSendJSON, "SendTask", 4096, NULL, 1, NULL, 0);
}

void loop() {
  // FreeRTOS handles everything
}