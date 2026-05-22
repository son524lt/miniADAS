#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

// ============ CONFIGURATION ============
#define SERIAL_BAUDRATE 921600
#define PACKET_START_BYTE1 0xAF
#define PACKET_START_BYTE2 0xFA
#define PACKET_START_BYTE3 0x55
#define PACKET_END_BYTE1 0x77
#define PACKET_END_BYTE2 0xAA
#define PACKET_DATA_LENGTH 12
#define PACKET_TOTAL_LENGTH (3 + PACKET_DATA_LENGTH + 2)  // start + data + end

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
  uint16_t distance;
};

// WebServer on port 80
WebServer server(80);

// Current data packet (updated in parseAndPrintPacket)
DataPacket currentPacket = {0};

// Packet buffer and state
uint8_t packetBuffer[PACKET_TOTAL_LENGTH];
uint8_t bufferIndex = 0;
bool receivingPacket = false;

void parseAndPrintPacket(uint8_t* data) {
  DataPacket packet;
  packet.steering = data[0];
  packet.user_throttle = data[1];
  packet.true_throttle = data[2];
  packet.brake = data[3];
  // Little-endian parsing for speed (int32_t)
  packet.speed = (int32_t)((data[4]) | (data[5] << 8) | (data[6] << 16) | ((int32_t)data[7] << 24));
  packet.PWM1 = data[8];
  packet.PWM2 = data[9];
  // Little-endian parsing for distance (uint16_t)
  packet.distance = (uint16_t)((data[10]) | (data[11] << 8));
  
  // Store to global currentPacket
  currentPacket = packet;
  
  // Print in format: st: 001, ut: 000, tt: 000, br: 000, sp: 12345, p1: 080, p2: 085, di: 00025 (us)
  Serial.printf("st: %03d, ut: %03d, tt: %03d, br: %03d, sp: %ld, p1: %03d, p2: %03d, di: %05d(us)\n",
    packet.steering, packet.user_throttle, packet.true_throttle, packet.brake,
    packet.speed, packet.PWM1, packet.PWM2, packet.distance);
}

// ===== HTTP HANDLERS =====
void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    server.send(404, "text/plain", "404: index.html not found");
    return;
  }
  server.streamFile(file, "text/html; charset=utf-8");
  file.close();
}

void handleCSS() {
  File file = LittleFS.open("/style.css", "r");
  if (!file) {
    server.send(404, "text/plain", "404: style.css not found");
    return;
  }
  server.streamFile(file, "text/css");
  file.close();
}

void handleJS() {
  File file = LittleFS.open("/app.js", "r");
  if (!file) {
    server.send(404, "text/plain", "404: app.js not found");
    return;
  }
  server.streamFile(file, "application/javascript");
  file.close();
}

void handleAPI() {
  // Compact JSON format
  String json = "{\"st\":";
  json += currentPacket.steering;
  json += ",\"ut\":";
  json += (currentPacket.user_throttle * 100 / 255);
  json += ",\"tt\":";
  json += (currentPacket.true_throttle * 100 / 255);
  json += ",\"br\":";
  json += (currentPacket.brake * 100 / 255);
  json += ",\"sp\":";
  json += (currentPacket.speed / 100);
  json += ",\"p1\":";
  json += currentPacket.PWM1;
  json += ",\"p2\":";
  json += currentPacket.PWM2;
  json += ",\"di\":";
  json += currentPacket.distance;
  json += "}";
  
  server.send(200, "application/json", json);
}

void onSerialReceive() {
  while (Serial1.available()) {
    uint8_t byte = Serial1.read();
    
    if (!receivingPacket) {
      // Look for start marker byte 1
      if (byte == PACKET_START_BYTE1) {
        bufferIndex = 0;
        receivingPacket = true;
        packetBuffer[bufferIndex++] = byte;
      }
    } else {
      packetBuffer[bufferIndex++] = byte;
      
      // Verify start marker (3 bytes)
      if (bufferIndex == 3) {
        if (packetBuffer[1] != PACKET_START_BYTE2 || packetBuffer[2] != PACKET_START_BYTE3) {
          // Invalid start marker, reset and rescan
          receivingPacket = false;
          bufferIndex = 0;
        }
      }
      // Check if we have complete packet (start + data + end)
      else if (bufferIndex == PACKET_TOTAL_LENGTH) {
        // Verify end marker
        if (packetBuffer[3 + PACKET_DATA_LENGTH] == PACKET_END_BYTE1 &&
            packetBuffer[3 + PACKET_DATA_LENGTH + 1] == PACKET_END_BYTE2) {
          // Valid packet, parse and print
          parseAndPrintPacket(&packetBuffer[3]);
        }
        // Reset for next packet
        receivingPacket = false;
        bufferIndex = 0;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(SERIAL_BAUDRATE, SERIAL_8N1, 5, 7);
  
  // Set interrupt handler for Serial1 receive
  Serial1.onReceive(onSerialReceive);
  
  Serial.println("\n=== S2mini WiFi WebServer ===");
  
  // Mount LittleFS
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] LittleFS mount failed!");
    delay(2000);
  } else {
    Serial.println("[OK] LittleFS mounted");
    
    // List files in LittleFS
    Serial.println("[INFO] Files in LittleFS:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
      Serial.print("  - ");
      Serial.print(file.name());
      Serial.print(" (");
      Serial.print(file.size());
      Serial.println(" bytes)");
      file = root.openNextFile();
    }
    
    // Check if index.html exists
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
  
  Serial.print("[OK] WiFi AP SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("[OK] WiFi AP IP: ");
  Serial.println(local_ip);
  
  // Setup HTTP routes
  server.on("/", handleRoot);
  server.on("/style.css", handleCSS);
  server.on("/app.js", handleJS);
  server.on("/api/data", handleAPI);
  
  // Start WebServer
  server.begin();
  Serial.println("[OK] WebServer started on http://192.168.1.1");
  Serial.println("S2mini started, waiting for packets...");
}

void loop() {
  server.handleClient();
  delay(10);
}