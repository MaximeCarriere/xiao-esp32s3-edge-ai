#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "secrets.h"

// ──────────────────────────────────────────────────────────────────────
// XIAO ESP32-S3 Sense — camera pin map
// ──────────────────────────────────────────────────────────────────────
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 40
#define SIOC_GPIO_NUM 39
#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 11
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 16
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13

WebServer server(80);

static const char *STREAM_BOUNDARY = "\r\n--frame\r\n";
static const char *STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace; boundary=frame";
static const char *STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// ──────────────────────────────────────────────────────────────────────
bool initCamera()
{
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; // 320x240 — lighter on memory & bandwidth
  config.jpeg_quality = 15;           // higher = smaller files (12 was sharper)
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  return esp_camera_init(&config) == ESP_OK;
}

// ──────────────────────────────────────────────────────────────────────
void handleRoot()
{
  String html =
      "<!DOCTYPE html><html><head>"
      "<meta charset=\"utf-8\">"
      "<title>XIAO Camera</title>"
      "<style>body{margin:0;background:#111;color:#eee;"
      "font-family:system-ui,sans-serif;text-align:center}"
      "h1{padding:1em}img{max-width:100%;height:auto}</style>"
      "</head><body>"
      "<h1>XIAO ESP32-S3 Sense &mdash; live stream</h1>"
      "<img src=\"/stream\" />"
      "</body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void handleStream()
{
  Serial.println(">> /stream requested");

  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.printf("Content-Type: %s\r\n", STREAM_CONTENT_TYPE);
  client.println("Cache-Control: no-cache");
  client.println("Connection: close");
  client.println();

  char part_buf[64];
  int frame = 0;
  while (client.connected())
  {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      delay(100);
      continue;
    }

    client.print(STREAM_BOUNDARY);
    size_t hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, fb->len);
    client.write((const uint8_t *)part_buf, hlen);
    size_t sent = client.write(fb->buf, fb->len);

    if (sent != fb->len)
    {
      Serial.printf("Short write: %u of %u, dropping client\n", sent, fb->len);
      esp_camera_fb_return(fb);
      break;
    }

    esp_camera_fb_return(fb);

    frame++;
    if (frame % 30 == 0)
    {
      Serial.printf("   streamed %d frames\n", frame);
    }
    delay(100); // ~10 fps cap
  }
  Serial.println("<< /stream client disconnected");
}

void handleSnap()
{
  Serial.println(">> /snap requested");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    server.send(500, "text/plain", "capture failed");
    return;
  }
  server.sendHeader("Content-Disposition", "inline; filename=snap.jpg");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// ──────────────────────────────────────────────────────────────────────
void setup()
{
  // Disable brownout detector — prevents random resets under heavy load
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Phase 2 / Milestone 2: live Wi-Fi stream ===\n");

  Serial.print("Initializing camera... ");
  if (!initCamera())
  {
    Serial.println("FAILED");
    while (true)
      delay(1000);
  }
  Serial.println("OK");

  Serial.printf("Connecting to Wi-Fi '%s' (SSID len=%d)\n",
                WIFI_SSID, strlen(WIFI_SSID));
  Serial.printf("Password length: %d\n", strlen(WIFI_PASSWORD));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000)
    {
      Serial.println("\nWi-Fi connect TIMEOUT");
      Serial.printf("Final status code: %d\n", WiFi.status());
      while (true)
        delay(1000);
    }
  }
  Serial.println();
  Serial.printf("Connected. IP address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("RSSI (signal): %d dBm\n", WiFi.RSSI());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/snap", HTTP_GET, handleSnap);
  server.begin();

  Serial.printf("\nOpen in your browser: http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Or grab a single frame: http://%s/snap\n\n", WiFi.localIP().toString().c_str());
}

void loop()
{
  server.handleClient();
}