#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "driver/uart.h"

// ============ CONFIGURATION ============
#define SERIAL_BAUDRATE 921600
#define PACKET_START_BYTE1 0xAF
#define PACKET_START_BYTE2 0xFA
#define PACKET_START_BYTE3 0x55
#define PACKET_END_BYTE1 0x77
#define PACKET_END_BYTE2 0xAA
#define PACKET_DATA_LENGTH 14  // steering(1) + ut(1) + tt(1) + br(1) + speed(4) + PWM1(1) + PWM2(1) + distance(2) + sample_rate(1)
#define PACKET_TOTAL_LENGTH 18  // start(3) + data(14) + end(2) = 18 bytes

// ============ WiFi CONFIG ============
#define WIFI_SSID "miniADAS"
#define WIFI_PASSWORD "12345678"
#define WIFI_IP 192, 168, 1, 1
#define WIFI_GATEWAY 192, 168, 1, 1
#define WIFI_SUBNET 255, 255, 255, 0
// =====================================

// DataPacket struct
struct DataPacket {
  uint8_t steering;
  uint8_t user_throttle;
  uint8_t true_throttle;
  uint8_t brake;
  int32_t speed;
  uint8_t PWM1;
  uint8_t PWM2;
  uint8_t sample_rate;
  uint16_t distance;
};

// WebServer on port 80
WebServer server(80);

// Current data packet (updated in parseAndPrintPacket)
DataPacket currentPacket = {0};

// File caches (P3: Cache files in RAM)
String indexHtml;
String styleCss;
String appJs;

// DMA + Ring buffer for UART1 RX
#define UART_RX_BUF_SIZE 1024
#define UART_TX_BUF_SIZE 256
uint8_t uartRxBuffer[UART_RX_BUF_SIZE];

// Packet buffer for parsing
uint8_t packetBuffer[PACKET_TOTAL_LENGTH];
uint8_t bufferIndex = 0;
bool receivingPacket = false;

// Event queue for UART events
QueueHandle_t uartEventQueue;

// WiFi event counter
int wifiClientCount = 0;
int httpRequestCount = 0;

// P5: UART DMA Event Handler - parse packet from DMA buffer
static void IRAM_ATTR uartEventTask(void *pvParameters) {
  uart_event_t event;
  uint8_t dtmp[256];  // Temporary buffer for reading
  
  while (1) {
    // Wait for UART event from queue
    if (xQueueReceive(uartEventQueue, (void *)&event, (TickType_t)portMAX_DELAY)) {
      switch (event.type) {
        case UART_DATA:
          // RX data available - read from DMA buffer
          {
            int len = uart_read_bytes(UART_NUM_1, dtmp, event.size, 0);
            
            // Parse packet for Web API
            for (int i = 0; i < len; i++) {
              uint8_t byte = dtmp[i];
              
              if (!receivingPacket) {
                if (byte == PACKET_START_BYTE1) {
                  bufferIndex = 0;
                  receivingPacket = true;
                  packetBuffer[bufferIndex++] = byte;
                }
              } else {
                if (bufferIndex < PACKET_TOTAL_LENGTH) {
                  packetBuffer[bufferIndex++] = byte;
                }
                
                // Verify start marker at byte 3
                if (bufferIndex == 3) {
                  if (packetBuffer[1] != PACKET_START_BYTE2 || packetBuffer[2] != PACKET_START_BYTE3) {
                    receivingPacket = false;
                    bufferIndex = 0;
                  }
                }
                // Check complete packet
                else if (bufferIndex == PACKET_TOTAL_LENGTH) {
                  if (packetBuffer[16] == PACKET_END_BYTE1 && packetBuffer[17] == PACKET_END_BYTE2) {
                    // Valid packet - extract and update currentPacket
                    uint8_t* data = &packetBuffer[3];
                    currentPacket.steering = data[0];
                    currentPacket.user_throttle = data[1];
                    currentPacket.true_throttle = data[2];
                    currentPacket.brake = data[3];
                    currentPacket.speed = (int32_t)((data[4]) | (data[5] << 8) | (data[6] << 16) | ((int32_t)data[7] << 24));
                    currentPacket.PWM1 = data[8];
                    currentPacket.PWM2 = data[9];
                    currentPacket.distance = (uint16_t)((data[10]) | (data[11] << 8));
                    currentPacket.sample_rate = data[12];
                  }
                  receivingPacket = false;
                  bufferIndex = 0;
                }
              }
            }
          }
          break;
        
        case UART_FIFO_OVF:
          Serial.println("[WARN] UART FIFO overflow");
          uart_flush_input(UART_NUM_1);
          break;
        
        case UART_BUFFER_FULL:
          Serial.println("[WARN] UART RX buffer full");
          uart_flush_input(UART_NUM_1);
          break;
        
        default:
          break;
      }
    }
  }
}

// P3: Cache files in RAM - optimized file serving
void IRAM_ATTR handleRoot() {
  httpRequestCount++;
  Serial.printf("[HTTP #%d] GET / from %s\n", httpRequestCount, server.client().remoteIP().toString().c_str());
  if (indexHtml.length() > 0) {
    server.send(200, "text/html; charset=utf-8", indexHtml);
  } else {
    server.send(404, "text/plain", "404: index.html not found");
  }
}

void IRAM_ATTR handleCSS() {
  httpRequestCount++;
  Serial.printf("[HTTP #%d] GET /style.css\n", httpRequestCount);
  if (styleCss.length() > 0) {
    server.send(200, "text/css", styleCss);
  } else {
    server.send(404, "text/plain", "404: style.css not found");
  }
}

void IRAM_ATTR handleJS() {
  httpRequestCount++;
  Serial.printf("[HTTP #%d] GET /app.js\n", httpRequestCount);
  if (appJs.length() > 0) {
    server.send(200, "application/javascript", appJs);
  } else {
    server.send(404, "text/plain", "404: app.js not found");
  }
}

void IRAM_ATTR handleAPI() {
  httpRequestCount++;
  Serial.printf("[HTTP #%d] GET /api/data\n", httpRequestCount);
  
  // P6: Binary protocol (13 bytes) - 75% smaller than JSON
  // Format: steering(1) + user_throttle(1) + true_throttle(1) + brake(1) + speed(4 LE) + PWM1(1) + PWM2(1) + distance(2 LE) + sample_rate(1)
  uint8_t binData[13];
  binData[0] = currentPacket.steering;
  binData[1] = currentPacket.user_throttle;
  binData[2] = currentPacket.true_throttle;
  binData[3] = currentPacket.brake;
  // speed (int32_t, little endian)
  binData[4] = (currentPacket.speed >> 0) & 0xFF;
  binData[5] = (currentPacket.speed >> 8) & 0xFF;
  binData[6] = (currentPacket.speed >> 16) & 0xFF;
  binData[7] = (currentPacket.speed >> 24) & 0xFF;
  binData[8] = currentPacket.PWM1;
  binData[9] = currentPacket.PWM2;
  binData[10] = currentPacket.sample_rate;
  // distance (uint16_t, little endian)
  binData[11] = (currentPacket.distance >> 0) & 0xFF;
  binData[12] = (currentPacket.distance >> 8) & 0xFF;
  
  // Send binary response using WebServer (handles HTTP headers automatically)
  server.sendHeader("Content-Type", "application/octet-stream");
  server.send(200, "application/octet-stream", String((const char*)binData, 13));
}

void setup() {
  // Init USB CDC (Serial) for logging only
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== S2mini WiFi WebServer (UART DMA) ===");
  
  // P5: UART DMA Setup using esp-idf
  // Install UART1 driver with DMA enabled
  uart_driver_install(UART_NUM_1, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 10, &uartEventQueue, 0);
  
  // Configure UART1
  uart_config_t uart1_config = {
    .baud_rate = SERIAL_BAUDRATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
  };
  uart_param_config(UART_NUM_1, &uart1_config);
  uart_set_pin(UART_NUM_1, 7, 5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_enable_rx_intr(UART_NUM_1);  // Enable RX interrupt with DMA
  
  // Create task for UART event handling
  xTaskCreate(uartEventTask, "uart_event_task", 2048, NULL, 12, NULL);
  
  // Mount LittleFS
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] LittleFS mount failed!");
    delay(2000);
  } else {
    Serial.println("[OK] LittleFS mounted");
    if (LittleFS.exists("/index.html")) {
      Serial.println("[OK] index.html found!");
    } else {
      Serial.println("[ERROR] index.html NOT found!");
    }
  }
  
  // Start WiFi AP with static IP
  WiFi.mode(WIFI_AP);
  IPAddress local_ip(WIFI_IP);
  IPAddress gateway(WIFI_GATEWAY);
  IPAddress subnet(WIFI_SUBNET);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("[OK] WiFi AP started on 192.168.1.1");
  
  // Setup HTTP routes
  server.on("/", handleRoot);
  server.on("/style.css", handleCSS);
  server.on("/app.js", handleJS);
  server.on("/api/data", handleAPI);
  
  // P3: Pre-load files into RAM
  File f = LittleFS.open("/index.html", "r");
  if (f) indexHtml = f.readString();
  f.close();
  
  f = LittleFS.open("/style.css", "r");
  if (f) styleCss = f.readString();
  f.close();
  
  f = LittleFS.open("/app.js", "r");
  if (f) appJs = f.readString();
  f.close();
  
  // Start WebServer
  server.begin();
  Serial.println("[OK] WebServer started");
  
  // WiFi AP event logging
  WiFi.onEvent([](WiFiEvent_t event) {
    switch(event) {
      case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        wifiClientCount++;
        Serial.printf("[WiFi] Client connected (Total: %d)\n", wifiClientCount);
        break;
      case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        if (wifiClientCount > 0) wifiClientCount--;
        Serial.printf("[WiFi] Client disconnected (Total: %d)\n", wifiClientCount);
        break;
      default:
        break;
    }
  });
}

void loop() {
  // P5: DMA + FIFO + ISR handles all UART forwarding automatically
  // Main loop only handles HTTP requests
  server.handleClient();
  
  // P1: 50ms delay (matches 50ms UART packet cycle)
  delay(50);
}