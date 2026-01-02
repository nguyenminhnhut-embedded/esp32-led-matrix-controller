#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

const int POT_PIN = 34;   // chân ADC biến trở
const int NUM_READS = 10; // số mẫu trung bình
int readings[NUM_READS];  // mảng lưu giá trị đọc
int readIndex = 0;        // vị trí đọc hiện tại
long total = 0;           // tổng các giá trị đọc
int scrollSpeed = 20;     // tốc độ cuộn hiện tại

unsigned long lastReadTime = 0;
const unsigned long readInterval = 20; // 20 ms

// ================= LED MATRIX =================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4   // 32x8

#define DATA_PIN 15
#define CLK_PIN  18
#define CS_PIN   5

MD_Parola matrix = MD_Parola(
  HARDWARE_TYPE,
  DATA_PIN,
  CLK_PIN,
  CS_PIN,
  MAX_DEVICES
);

// ================= WIFI AP =================
const char* ssid = "ESP32-MATRIX";
const char* password = "123456789";

WebServer server(80);

// Nội dung hiển thị
String text = "Hello ESP32";

// ================= HTML =================
String webpage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 LED Matrix</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 50px; }
    input { font-size: 20px; width: 80%; padding: 10px; }
    button { font-size: 20px; padding: 10px 20px; }
  </style>
</head>
<body>
  <h2>LED Matrix 32x8</h2>
  <form action="/set">
    <input name="msg" placeholder="Nhap chu..." autofocus>
    <br><br>
    <button type="submit">GUI</button>
  </form>
</body>
</html>
)rawliteral";
}

// ================= HANDLER =================
void handleRoot() {
  server.send(200, "text/html", webpage());
}

void handleSet() {
  if (server.hasArg("msg")) {
    text = server.arg("msg");
    matrix.displayClear();
    matrix.displayScroll(
      text.c_str(),
      PA_LEFT,
      PA_SCROLL_LEFT,
      80
    );
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);

  pinMode(POT_PIN, INPUT);
  for (int i = 0; i < NUM_READS; i++) {
    readings[i] = 0;
  }

  // LED
  matrix.begin();
  matrix.setIntensity(2);
  matrix.displayClear();
  matrix.displayScroll(
    text.c_str(),
    PA_LEFT,
    PA_SCROLL_LEFT,
    80
  );

  // WIFI AP
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP started");
  Serial.println(WiFi.softAPIP());

  // WEB
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}

void loop() {
  server.handleClient();

  unsigned long currentTime = millis();
  if (currentTime - lastReadTime >= readInterval) {
    lastReadTime = currentTime;

    // Loại bỏ giá trị cũ ra khỏi tổng
    total -= readings[readIndex];

    // Đọc giá trị mới từ ADC
    int newReading = analogRead(POT_PIN);
    readings[readIndex] = newReading;

    // Cộng giá trị mới vào tổng
    total += newReading;

    // Cập nhật vị trí đọc tiếp theo
    readIndex = (readIndex + 1) % NUM_READS;

    // Tính giá trị trung bình
    int averageValue = total / NUM_READS;

    // Map giá trị ADC trung bình thành tốc độ cuộn
    scrollSpeed = map(averageValue, 0, 4095, 20, 200);

    // Cập nhật tốc độ cho ma trận LED
    matrix.setSpeed(scrollSpeed);
  }

  if (matrix.displayAnimate()) {
    matrix.displayReset();
  }
}
