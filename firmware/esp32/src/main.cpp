#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <Adafruit_ADXL345_U.h>

#include "config.h"

namespace {
constexpr uint32_t kAccelSampleRate = 1600;  // Hz
constexpr size_t kAccelWindow = 256;
constexpr size_t kAudioWindow = 2048;
constexpr gpio_num_t kMicBclkPin = GPIO_NUM_26;
constexpr gpio_num_t kMicWsPin = GPIO_NUM_25;
constexpr gpio_num_t kMicDataPin = GPIO_NUM_33;

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

float accel_buffer[3][kAccelWindow];
int32_t audio_buffer[kAudioWindow];

uint32_t last_publish_ms = 0;
}  // namespace

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(config::kWifiSsid, config::kWifiPassword);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.print("\nIP: ");
  Serial.println(WiFi.localIP());
}

void ensureMqtt() {
  if (mqtt_client.connected()) {
    return;
  }
  while (!mqtt_client.connected()) {
    Serial.print("Connecting MQTT...");
    if (mqtt_client.connect(config::kDeviceId, config::kMqttUser, config::kMqttPassword)) {
      Serial.println("connected");
      mqtt_client.publish((String("factory/") + config::kDeviceId + "/status").c_str(), "online");
      break;
    }
    Serial.println("failed, retrying in 2s");
    delay(2000);
  }
}

void setupI2S() {
  i2s_config_t i2s_config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = 48000,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 256,
      .use_apll = true,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0,
  };

  i2s_pin_config_t pin_config = {
      .bck_io_num = kMicBclkPin,
      .ws_io_num = kMicWsPin,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = kMicDataPin,
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, nullptr);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void readAccelWindow() {
  sensors_event_t event;
  for (size_t i = 0; i < kAccelWindow; ++i) {
    accel.getEvent(&event);
    accel_buffer[0][i] = event.acceleration.x;
    accel_buffer[1][i] = event.acceleration.y;
    accel_buffer[2][i] = event.acceleration.z;
    delayMicroseconds(1000000UL / kAccelSampleRate);
  }
}

float computeRms(const float *buffer, size_t len) {
  double sum_sq = 0;
  for (size_t i = 0; i < len; ++i) {
    sum_sq += buffer[i] * buffer[i];
  }
  return sqrt(sum_sq / len);
}

void readAudioWindow() {
  size_t bytes_read = 0;
  i2s_read(I2S_NUM_0, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
  // bytes_read may be smaller; ignore for prototype
}

float computeSpl() {
  double sum_sq = 0;
  for (size_t i = 0; i < kAudioWindow; ++i) {
    sum_sq += pow((double)audio_buffer[i] / INT32_MAX, 2);
  }
  double rms = sqrt(sum_sq / kAudioWindow);
  const double ref = 0.00002;  // 20 uPa reference pressure
  return 20.0 * log10((rms + 1e-9) / ref);
}

void publishPayload(float rms_x, float rms_y, float rms_z, float spl) {
  DynamicJsonDocument doc(512);
  doc["device_id"] = config::kDeviceId;
  doc["timestamp_ms"] = millis();
  JsonArray accel_rms = doc.createNestedArray("accel_rms");
  accel_rms.add(rms_x);
  accel_rms.add(rms_y);
  accel_rms.add(rms_z);
  doc["spl_db"] = spl;
  doc["wifi_rssi"] = WiFi.RSSI();

  char buffer[512];
  auto len = serializeJson(doc, buffer);
  mqtt_client.publish((String("factory/") + config::kDeviceId + "/vibration").c_str(), buffer, len);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  if (!accel.begin()) {
    Serial.println("ADXL345 not detected!");
    while (true) {
      delay(1000);
    }
  }
  accel.setDataRate(ADXL345_DATARATE_1600_HZ);
  accel.setRange(ADXL345_RANGE_16_G);

  setupI2S();
  connectWiFi();
  mqtt_client.setServer(config::kMqttBroker, config::kMqttPort);
}

void loop() {
  ensureMqtt();
  mqtt_client.loop();

  readAccelWindow();
  readAudioWindow();

  float rms_x = computeRms(accel_buffer[0], kAccelWindow);
  float rms_y = computeRms(accel_buffer[1], kAccelWindow);
  float rms_z = computeRms(accel_buffer[2], kAccelWindow);
  float spl = computeSpl();

  const uint32_t now = millis();
  if (now - last_publish_ms > 2000) {
    publishPayload(rms_x, rms_y, rms_z, spl);
    last_publish_ms = now;
  }
}
