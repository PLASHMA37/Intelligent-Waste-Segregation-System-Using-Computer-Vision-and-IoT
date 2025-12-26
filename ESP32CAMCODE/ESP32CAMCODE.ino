#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "board_config.h"
#ifndef LED_GPIO_NUM
#define LED_GPIO_NUM 4
#endif

HardwareSerial SoftUart(1); // UART1
const char* ssid = "SSID";
const char* password = "PASSWORD";

WebServer server(80);
static camera_fb_t* last_fb = nullptr;
const uint16_t FLASH_WARMUP_MS = 220;  


void release_last_fb() {
  if (last_fb) {
    esp_camera_fb_return(last_fb);
    last_fb = nullptr;
  }
}

void handleRoot() {
  String ip = WiFi.localIP().toString();
  String html = "<html><body>"
                "<h3>ESP32-CAM host</h3>"
                "<p>Latest image: <a href=\"/image.jpg\">/image.jpg</a></p>"
                "<p>POST classification (text/plain) to /result</p>"
                "<p>Device IP: " + ip + "</p>"
                "</body></html>";
  server.send(200, "text/html", html);
}

void handleGetImage() {
  if (!last_fb) {
    server.send(500,"text/plain","no_image_available");
    return;
  }
  server.setContentLength(last_fb->len);
  server.send(200,"image/jpeg");
  WiFiClient client = server.client();
  client.write(last_fb->buf, last_fb->len);
}

void handlePostResult() {
  if (!server.hasArg("plain")) {
    server.send(400,"text/plain","missing_body");
    return;
  }
  String cls = server.arg("plain");
  cls.trim();
  if (cls.isEmpty()) {
    server.send(400,"text/plain","empty");
    return;
  }
  SoftUart.println(cls);
  server.send(200, "text/plain", "ok");
}

void startCameraServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/image.jpg", HTTP_GET, handleGetImage);
  server.on("/result", HTTP_POST, handlePostResult);
  server.begin();
}

void setupLedFlash() {
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);
}


camera_fb_t* captureImage() {
  digitalWrite(LED_GPIO_NUM, HIGH);
  delay(FLASH_WARMUP_MS);

  camera_fb_t* fb = esp_camera_fb_get();
  digitalWrite(LED_GPIO_NUM, LOW);

  if (!fb || fb->format != PIXFORMAT_JPEG) {
    if (fb) esp_camera_fb_return(fb);
    return nullptr;
  }
  return fb;
}


void setup() {
  Serial.begin(115200);
  delay(100);
  SoftUart.begin(9600, SERIAL_8N1, 13, 12);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size   = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode    = CAMERA_GRAB_LATEST;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count     = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
  } else {
    sensor_t* s = esp_camera_sensor_get();
    if (s && s->id.PID == OV3660_PID) {
      s->set_vflip(s, 1);
      s->set_brightness(s, 1);
      s->set_saturation(s, -2);
    }
    if (config.pixel_format == PIXFORMAT_JPEG) {
      s->set_framesize(s, FRAMESIZE_QVGA);
    }
  }

  setupLedFlash();

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) delay(300);

  startCameraServer();

  last_fb = captureImage();
  if (last_fb) Serial.println("Initial image captured");
  Serial.print("Ready at http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();

  if (SoftUart.available()) {
    String cmd = SoftUart.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("DETECT")) {
      release_last_fb();
      last_fb = captureImage();
      if (last_fb) {
        Serial.println("SHOT_READY");
        Serial.print("IMAGE_URL: http://");
        Serial.print(WiFi.localIP());
        Serial.println("/image.jpg");
      } else {
        SoftUart.println("ERROR");
      }
    }
  }
}




