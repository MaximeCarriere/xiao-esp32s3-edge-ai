#include <Arduino.h>
#include "esp_camera.h"
#include "base64.h"

// ──────────────────────────────────────────────────────────────────────
// XIAO ESP32-S3 Sense — camera pin map (from Seeed wiki, do not change)
// ──────────────────────────────────────────────────────────────────────
#define PWDN_GPIO_NUM -1  // power-down not wired
#define RESET_GPIO_NUM -1 // reset not wired
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 40 // I²C SDA to camera
#define SIOC_GPIO_NUM 39 // I²C SCL to camera

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

void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== Phase 2 / Milestone 1: capture one JPEG ===\n");

  // Build the camera config struct.
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
  config.xclk_freq_hz = 20000000;          // 20 MHz clock
  config.pixel_format = PIXFORMAT_JPEG;    // hardware JPEG compression
  config.frame_size = FRAMESIZE_QVGA;      // 320×240, small + fast
  config.jpeg_quality = 12;                // 0=best, 63=worst; 12 is good
  config.fb_count = 2;                     // double-buffer in PSRAM
  config.fb_location = CAMERA_FB_IN_PSRAM; // store frames in PSRAM
  config.grab_mode = CAMERA_GRAB_LATEST;

  // Initialize.
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init FAILED with error 0x%x\n", err);
    while (true)
      delay(1000); // halt
  }
  Serial.println("Camera initialized.\n");
}

void loop()
{
  Serial.println("Capturing frame...");

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Capture failed!");
    delay(2000);
    return;
  }

  Serial.printf("Got %u bytes (%ux%u)\n", fb->len, fb->width, fb->height);

  // Encode the JPEG bytes as base64 and dump between markers.
  String b64 = base64::encode(fb->buf, fb->len);

  Serial.println("---BEGIN JPEG BASE64---");
  Serial.println(b64);
  Serial.println("---END JPEG BASE64---\n");

  esp_camera_fb_return(fb); // CRITICAL: free the frame buffer

  delay(10000); // wait 10s before next capture
}