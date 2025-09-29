// const char* ssid     = "I'm_watching_u";   // <-- change to your SSID
//const char* password = "Sylvia2021";  

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"

// ===== WiFi (fill these) =====
const char* ssid     = "I'm_watching_u"; 
const char* password = "Sylvia2021"; 

// ===== NTP (UTC+3 for Finland summer time) =====
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3 * 3600, 60000); // offset=UTC+3, update each 60s

// ===== OLED (SSD1306, I2C 0x3C; SDA=GPIO12, SCL=GPIO14) =====
SSD1306Wire display(0x3c, 12, 14);
OLEDDisplayUi ui(&display);

// ===== Geometry =====
const int screenW = 128;
const int screenH = 64;

// No extra header offset (keep content higher)
const int clockCenterX = screenW / 2;
const int clockCenterY = 36;       // a bit below top to leave room for title/time
const int analogRadius = 30;       // larger round clock

// ---------- helpers ----------
String twoDigits(int d) { return (d < 10) ? ("0" + String(d)) : String(d); }

void drawHand(OLEDDisplay* d, int cx, int cy, int angleDeg, int length) {
  float rad = angleDeg * 0.0174533f; // PI/180
  int x = cx + (int)(length * sinf(rad));
  int y = cy - (int)(length * cosf(rad));
  d->drawLine(cx, cy, x, y);
}

void drawTicks(OLEDDisplay* d, int cx, int cy, int r) {
  for (int i = 0; i < 12; ++i) {
    int a = i * 30;
    float rad = a * 0.0174533f;
    int r1 = r - 1;
    int r2 = r - ((i % 3 == 0) ? 6 : 3); // longer ticks at 12/3/6/9
    int x1 = cx + (int)(r1 * sinf(rad));
    int y1 = cy - (int)(r1 * cosf(rad));
    int x2 = cx + (int)(r2 * sinf(rad));
    int y2 = cy - (int)(r2 * cosf(rad));
    d->drawLine(x1, y1, x2, y2);
  }
}

// ---------- Frame 1: Digital time with header ----------
void digitalClockFrame(OLEDDisplay *d, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  timeClient.update();
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  int s = timeClient.getSeconds();

  // Header (bigger)
  d->setTextAlignment(TEXT_ALIGN_CENTER);
  d->setFont(ArialMT_Plain_16);            // was 10 → now larger
  d->drawString(64 + x, 0 + y, "Hei, Razib");

  // Big time (move it a bit lower to fill the screen)
  String t = twoDigits(h) + ":" + twoDigits(m) + ":" + twoDigits(s);
  d->setFont(ArialMT_Plain_24);            // 24 is the largest built-in font in this lib
  d->drawString(64 + x, 22 + y, t);        // was ~14 → now lower to use more space
}


// ---------- Frame 2: Larger analog clock ----------
void analogClockFrame(OLEDDisplay *d, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  timeClient.update();
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  int s = timeClient.getSeconds();

  const int cx = clockCenterX + x;
  const int cy = clockCenterY + y;

  // Face + ticks
  d->drawCircle(cx, cy, analogRadius);
  drawTicks(d, cx, cy, analogRadius);

  // Smooth hand angles
  int hourAngle   = (h % 12) * 30 + m / 2 + s / 120;
  int minuteAngle = m * 6 + s / 10;
  int secondAngle = s * 6;

  drawHand(d, cx, cy, hourAngle,   analogRadius - 12); // hour
  drawHand(d, cx, cy, minuteAngle, analogRadius - 6);  // minute
  drawHand(d, cx, cy, secondAngle, analogRadius - 3);  // second

  d->fillCircle(cx, cy, 2);
}

// Register frames
FrameCallback frames[] = { digitalClockFrame, analogClockFrame };
int frameCount = 2;

void setup() {
  Serial.begin(115200);
  delay(100);

  // WiFi connect
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.printf("\nWiFi OK, IP: %s\n", WiFi.localIP().toString().c_str());

  // I2C + Display
  Wire.begin(12, 14);         // SDA=12, SCL=14 (your board)
  display.init();
  display.flipScreenVertically();  // comment out if upside-down
  ui.setTargetFPS(60);
  ui.disableAllIndicators();       // no dots at top
  ui.setFrames(frames, frameCount);
  ui.init();

  // NTP
  timeClient.begin();
  timeClient.forceUpdate();
}

void loop() {
  int remaining = ui.update();
  if (remaining > 0) delay(remaining);
}
