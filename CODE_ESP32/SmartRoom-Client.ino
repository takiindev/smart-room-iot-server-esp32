#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <CustomJWT.h>
#include "time.h"
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <SensirionCore.h>
#include <BH1750.h>
#include <math.h>

// Không dùng Preferences nữa - config chỉ lưu trong RAM

String config = R"rawliteral(
{
    "timeApi": "http://172.16.57.136:8080/api/v1/public/time",
    "roomCode": "R-RND",
    "I2C": {
        "SDA": 8,
        "SCL": 9
    },
    "devices": [
        {
            "naturalId": "TEMP_ESP32_02",
            "category": "TEMPERATURE",
            "specificType": "GPIO",
            "controlType": "GPIO",
            "gpioPin": [
                8,
                9
            ],
            "translations": {
                "vi": {
                    "name": "Cảm biến nhiệt độ A101 2",
                    "description": "Cảm biến nhiệt độ SHT40 phòng học A101 sử dụng chuẩn giao tiếp I2C"
                },
                "en": {
                    "name": "Temperature Sensor A101 2",
                    "description": "SHT40 temperature sensor in room A101 using I2C protocol"
                }
            },
            "internal": {
                "peripheralType": "SENSOR",
                "module": "SCD40"
            }
        },
        {
          "naturalId": "CO2_ESP32_01",
          "category": "SENSOR_CO2",
          "specificType": "GPIO",
          "controlType": "GPIO",
          "gpioPin": [
            8,
            9
          ],
          "translations": {
            "vi": {
              "name": "Cảm biến CO2 phòng R&D",
              "description": "Nồng độ CO2"
            },
            "en": {
              "name": "Room R&D CO2 sensor",
              "description": "CO2 concentration"
            }
          },
          "internal": {
            "peripheralType": "SENSOR",
            "module": "SCD40"
          }
        },
        {
          "naturalId": "HUM_ESP32_01",
          "category": "HUMIDITY",
          "specificType": "GPIO",
          "controlType": "GPIO",
          "gpioPin": [
            8,
            9
          ],
          "translations": {
            "vi": {
              "name": "Cảm biến độ ẩm phòng R&D",
              "description": "Độ ẩm tương đối đo bằng cảm biến SCD40"
            },
            "en": {
              "name": "Room R&D humidity sensor",
              "description": "Relative humidity measured by SCD40"
            }
          },
          "internal": {
            "peripheralType": "SENSOR",
            "module": "SCD40"
          }
        },
        {
          "naturalId": "LUX_ESP32_01",
          "category": "SENSOR_LUX",
          "specificType": "GPIO",
          "controlType": "GPIO",
          "gpioPin": [
            8,
            9
          ],
          "translations": {
            "vi": {
              "name": "Cảm biến ánh sáng phòng R&D",
              "description": "Cường độ ánh sáng đo bằng cảm biến BH1750"
            },
            "en": {
              "name": "Room R&D illuminance sensor",
              "description": "Illuminance measured by BH1750"
            }
          },
          "internal": {
            "peripheralType": "SENSOR",
            "module": "BH1750",
            "i2cAddress": 35
          }
        }
    ]
}
)rawliteral";

int activeRelayPins[13];
int activeRelayCount = 0;

const int ledPin = 2;
const uint8_t RELAY_INITIAL_STATE = LOW; // Trạng thái ban đầu của relay: HIGH hoặc LOW
IRsend *irsend = nullptr;                 // Pointer to IRsend object, initialized from config

// SCD40 sensor - global object, initialized once
SensirionI2cScd4x scd4x;
bool isScd40Initialized = false;

// SCD40 background task - cache latest sensor data
struct SCD40CachedData {
  float temperature = 0.0;
  float humidity = 0.0;
  uint16_t co2 = 0;
  unsigned long lastReadTime = 0;
  bool isValid = false;
};
SCD40CachedData scd40CachedData;
int scd40CachedSdaPin = 8;
int scd40CachedSclPin = 9;
bool scd40BackgroundTaskRunning = false;

// BH1750 sensor - dùng chung bus I2C với SCD40
BH1750 bh1750;
bool isBh1750Initialized = false;
uint8_t bh1750I2cAddress = 0x23;

struct BH1750CachedData
{
  float lux = 0.0f;
  unsigned long lastReadTime = 0;
  bool isValid = false;
};

BH1750CachedData bh1750CachedData;

char secret_key[] = "ABC@123";
CustomJWT jwt(secret_key, 256);

const char *ssid = "A101CNTT";
const char *password = "fit@123456789";
IPAddress local_IP(172, 16, 65, 250);
IPAddress gateway(172, 16, 0, 1);
IPAddress subnet(255, 255, 0, 0);

IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

WebServer server(8080);

String serverUsername = "rndesp32gateway";
String serverPassword = "iuhrndesp32gateway";

const char *headerKeys[] = {"Authorization", "Origin"};
size_t headerKeysCount = sizeof(headerKeys) / sizeof(char *);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

struct DeviceMatch
{
  bool found = false;
  String peripheralType;
  int gpioPins[3] = {-1, -1, -1};
  int gpioCount = 0;
  int firstGpio = -1;
};

struct MotionSensorInfo
{
  bool found = false;
  String naturalId;
  String module;
  int gpioPin = -1;
  bool lastState = false;
};

// Global motion sensor info
MotionSensorInfo motionSensor;

// Forward declaration of BrandIrSender
struct BrandIrSender;

void reinitializeRelayPins();
bool turnOnAC(const String &naturalId = "");
bool turnOffAC(const String &naturalId = "");
bool toggleSwingAC(const String &naturalId = "");
bool setSwingAC(const String &naturalId, String state);
bool setModeAC(const String &naturalId, String mode);
bool setFanSpeedAC(const String &naturalId, int speed);
bool setTemperatureAC(const String &naturalId, int temp);
void controlRelay(int gpioPin, bool state);
unsigned long getEpochTime();
bool validateJWT(String tokenStr);
bool requireBearerToken(String &token);
bool parsePlainBody(JsonDocument &bodyDoc);
DeviceMatch findLightingDevice(const String &naturalId);
DeviceMatch findFanDevice(const String &naturalId);
float readDS18B20Temperature(int gpioPin);
bool readSCD40Data(int sdaPin, int sclPin, float &temperature, float &humidity);
bool syncTimeFromApi(const String &timeApiUrl);
void handleNotFound();
void handleGetConfig();
void handleControl();
void handleLogin();
void handleGetTemperature();
void handleGetHumidity();
void handleGetCO2();
void handleGetLux();
void handleTelemetry();
void initAllSensors();
void scd40BackgroundTask(void *parameter);
void startSCD40BackgroundTask();

bool syncTimeFromApi(const String &timeApiUrl)
{
  if (timeApiUrl.length() == 0 || timeApiUrl == "null")
  {
    Serial.println("[TIME SYNC] timeApiUrl rỗng, bỏ qua sync time từ API");
    return false;
  }

  Serial.printf("[TIME SYNC] Đang đồng bộ thời gian từ API: %s\n", timeApiUrl.c_str());

  HTTPClient http;
  unsigned long requestTime = millis();

  http.begin(timeApiUrl);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);

  int httpCode = http.GET();

  unsigned long responseTime = millis();
  unsigned long latency = (responseTime - requestTime) / 2; // Chia đôi (request + response)

  if (httpCode != HTTP_CODE_OK)
  {
    Serial.printf("[TIME SYNC FAILED] HTTP Code: %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, payload))
  {
    Serial.println("[TIME SYNC FAILED] JSON Parse Error");
    return false;
  }

  // Check for new format: { "data": { "time": "..." } }
  String timeStr;
  if (doc.containsKey("data") && doc["data"].containsKey("time"))
  {
    timeStr = doc["data"]["time"].as<String>();
    Serial.println("[TIME SYNC] Format: { data: { time: ... } }");
  }
  // Fallback to old format: { "time": "..." }
  else if (doc.containsKey("time"))
  {
    timeStr = doc["time"].as<String>();
    Serial.println("[TIME SYNC] Format: { time: ... }");
  }
  else
  {
    Serial.println("[TIME SYNC FAILED] Response không có field 'time' hoặc 'data.time'");
    return false;
  }

  Serial.printf("[TIME SYNC] Response time: %s\n", timeStr.c_str());
  Serial.printf("[TIME SYNC] Latency: %lu ms\n", latency);

  // Parse ISO 8601 time string: 2026-07-11T08:01:32.116Z
  struct tm timeinfo = {};
  char buf[30];
  timeStr.toCharArray(buf, 30);

  // Parse year, month, day, hour, minute, second
  sscanf(buf, "%d-%d-%dT%d:%d:%d",
         &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
         &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);

  timeinfo.tm_year -= 1900; // Years since 1900
  timeinfo.tm_mon -= 1;     // Months since January (0-11)

  time_t serverTime = mktime(&timeinfo);
  if (serverTime == -1)
  {
    Serial.println("[TIME SYNC FAILED] mktime() failed");
    return false;
  }

  // Thêm latency compensation vào thời gian
  time_t adjustedTime = serverTime + (latency / 1000);

  // Set system time
  timeval tv = {adjustedTime, 0};
  settimeofday(&tv, nullptr);

  Serial.printf("[TIME SYNC SUCCESS] Đã sync thời gian thành công\n");
  Serial.printf("[TIME SYNC] Original time: %lu\n", serverTime);
  Serial.printf("[TIME SYNC] Adjusted time: %lu\n", adjustedTime);

  return true;
}

String getCurrentTimestamp()
{
  struct tm timeinfo;
  time_t now;
  time(&now);
  gmtime_r(&now, &timeinfo);

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buffer);
}

void sendJson(int statusCode, const String &message)
{
  JsonDocument responseDoc;
  responseDoc["status"] = statusCode;
  responseDoc["message"] = message;
  responseDoc["timestamp"] = getCurrentTimestamp();

  String response;
  serializeJsonPretty(responseDoc, response);

  // Log response to serial
  Serial.println("[API RESPONSE]");
  Serial.println(response);
  Serial.println("[END RESPONSE]");

  server.send(statusCode, "application/json", response);
}

void sendJsonWithData(int statusCode, const String &message, JsonDocument &data)
{
  JsonDocument responseDoc;
  responseDoc["status"] = statusCode;
  responseDoc["message"] = message;
  responseDoc["data"] = data;
  responseDoc["timestamp"] = getCurrentTimestamp();

  String response;
  serializeJsonPretty(responseDoc, response);

  // Log response to serial
  Serial.println("[API RESPONSE]");
  Serial.println(response);
  Serial.println("[END RESPONSE]");

  server.send(statusCode, "application/json", response);
}

void sendCORSHeaders()
{
  if (server.hasHeader("Origin") && server.header("Origin") == "null")
  {
    server.sendHeader("Access-Control-Allow-Origin", "null");
  }
  else
  {
    server.sendHeader("Access-Control-Allow-Origin", "*");
  }
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS,PATCH");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type,Authorization");
}

void handleCORS()
{
  sendCORSHeaders();
  server.send(204);
}

void CheckConfigJSON()
{
  // Không dùng Preferences nữa, chỉ dùng config mặc định hard-code
  Serial.println("Sử dụng cấu hình mặc định từ code.");
}

String getRoomCodeFromConfigString(const String &configSource)
{
  JsonDocument doc;
  if (deserializeJson(doc, configSource))
  {
    return "";
  }

  if (doc.containsKey("roomCode"))
  {
    return doc["roomCode"].as<String>();
  }

  return "";
}

bool loadConfigDocument(JsonDocument &configDoc)
{
  return deserializeJson(configDoc, config) == DeserializationError::Ok;
}

bool findDeviceInConfig(const JsonDocument &configDoc, const String &naturalId, JsonObjectConst &deviceOut)
{
  JsonArrayConst devicesArray;
  if (configDoc.containsKey("devices"))
  {
    devicesArray = configDoc["devices"].as<JsonArrayConst>();
  }
  else if (configDoc.is<JsonArrayConst>())
  {
    devicesArray = configDoc.as<JsonArrayConst>();
  }
  else
  {
    return false;
  }

  for (JsonObjectConst device : devicesArray)
  {
    if (device["naturalId"].as<String>() == naturalId)
    {
      deviceOut = device;
      return true;
    }
  }

  return false;
}

bool readIrCodeValue(const JsonVariantConst &value, uint32_t &code)
{
  if (value.isNull())
  {
    return false;
  }

  if (value.is<const char *>())
  {
    const char *rawValue = value.as<const char *>();
    if (rawValue == nullptr || rawValue[0] == '\0')
    {
      return false;
    }

    char *endPtr = nullptr;
    unsigned long parsed = strtoul(rawValue, &endPtr, 0);
    if (endPtr == rawValue || (endPtr != nullptr && *endPtr != '\0'))
    {
      return false;
    }

    code = static_cast<uint32_t>(parsed);
    return true;
  }

  code = value.as<uint32_t>();
  return code != 0;
}

struct BrandIrSender
{
  const char *brand;
  bool (*send)(uint32_t code, uint16_t bits);
};

bool sendLgIrCode(uint32_t code, uint16_t bits)
{
  if (irsend == nullptr)
  {
    Serial.println("Lỗi: IRsend object chưa được khởi tạo!");
    return false;
  }
  irsend->sendLG(code, bits);
  return true;
}

bool sendSamsung36IrCode(uint32_t code, uint16_t bits)
{
  if (irsend == nullptr)
  {
    Serial.println("Lỗi: IRsend object chưa được khởi tạo!");
    return false;
  }
  // Samsung uses 36-bit encoding, we send the code as 36 bits
  irsend->sendSamsung36(code, 36);
  return true;
}

bool sendPanasonicIrCode(uint32_t code, uint16_t bits)
{
  if (irsend == nullptr)
  {
    Serial.println("Lỗi: IRsend object chưa được khởi tạo!");
    return false;
  }
  irsend->sendPanasonic(code, bits);
  return true;
}

// map - Thêm hãng mới ở đây
const BrandIrSender kBrandIrSenders[] = {
    {"LG", sendLgIrCode},
    {"SAMSUNG", sendSamsung36IrCode},
    {"PANASONIC", sendPanasonicIrCode},
};

const BrandIrSender *findBrandIrSender(const String &brand)
{
  for (const BrandIrSender &entry : kBrandIrSenders)
  {
    if (brand.equalsIgnoreCase(entry.brand))
    {
      return &entry;
    }
  }

  return nullptr;
}

String getDevicePeripheralTypeFromConfig(const JsonDocument &configDoc, const String &naturalId)
{
  if (!configDoc.containsKey("devices") || !configDoc["devices"].is<JsonArrayConst>())
  {
    return "";
  }

  JsonArrayConst devicesArray = configDoc["devices"].as<JsonArrayConst>();
  for (JsonObjectConst device : devicesArray)
  {
    if (naturalId.length() > 0 && device["naturalId"].as<String>() != naturalId)
    {
      continue;
    }

    if (device.containsKey("internal") && device["internal"].is<JsonObjectConst>() && device["internal"].as<JsonObjectConst>().containsKey("peripheralType"))
    {
      String peripheralType = device["internal"]["peripheralType"].as<String>();
      if (peripheralType.length() > 0)
      {
        return peripheralType;
      }
    }

    if (device.containsKey("peripheralType"))
    {
      return device["peripheralType"].as<String>();
    }
  }

  return "";
}

bool findAcCodeConfigsFromConfig(const JsonDocument &configDoc, const String &naturalId, JsonObjectConst &codeConfigsOut)
{
  if (!configDoc.containsKey("devices") || !configDoc["devices"].is<JsonArrayConst>())
  {
    return false;
  }

  JsonArrayConst devicesArray = configDoc["devices"].as<JsonArrayConst>();
  for (JsonObjectConst device : devicesArray)
  {
    String category = device["category"].as<String>();
    if (category != "AIR_CONDITION" && category != "AC")
    {
      continue;
    }

    String deviceNaturalId = device["naturalId"].as<String>();
    if (naturalId.length() > 0 && deviceNaturalId != naturalId)
    {
      continue;
    }

    JsonObjectConst internal = device["internal"].as<JsonObjectConst>();
    if (internal.isNull() || !internal.containsKey("codeConfigs") || !internal["codeConfigs"].is<JsonObjectConst>())
    {
      continue;
    }

    codeConfigsOut = internal["codeConfigs"].as<JsonObjectConst>();
    if (!codeConfigsOut.isNull())
    {
      return true;
    }
  }

  return false;
}

String getAcBrandFromCodeConfigs(const JsonObjectConst &codeConfigs)
{
  if (codeConfigs.isNull())
  {
    return "LG";
  }

  return codeConfigs["brand"] | "LG";
}

uint16_t getAcBitsFromCodeConfigs(const JsonObjectConst &codeConfigs)
{
  if (codeConfigs.isNull())
  {
    return 28;
  }

  return codeConfigs["bits"] | 28;
}

bool getAcCommandCodeFromConfig(const JsonDocument &configDoc, const String &naturalId, const String &section, const String &key, uint32_t &code, uint16_t &bits, String &brand)
{
  // Find device from config
  JsonArrayConst devicesArray;
  if (configDoc.containsKey("devices"))
  {
    devicesArray = configDoc["devices"].as<JsonArrayConst>();
  }
  else if (configDoc.is<JsonArrayConst>())
  {
    devicesArray = configDoc.as<JsonArrayConst>();
  }
  else
  {
    Serial.println("Lỗi: Không tìm thấy devices trong config!");
    return false;
  }

  JsonObjectConst targetDevice;
  bool deviceFound = false;
  for (JsonObjectConst device : devicesArray)
  {
    if (device["naturalId"].as<String>() == naturalId)
    {
      targetDevice = device;
      deviceFound = true;
      break;
    }
  }

  if (!deviceFound || !targetDevice.containsKey("internal"))
  {
    Serial.printf("Lỗi: Không tìm thấy device '%s' hoặc thiếu field 'internal'\n", naturalId.c_str());
    return false;
  }

  // Get internal object
  JsonObjectConst internalObj = targetDevice["internal"].as<JsonObjectConst>();
  // in ra nội dung của internalObj để debug
  String internalStr;
  serializeJson(internalObj, internalStr);
  // Serial.printf("-> internalObj: %s\n", internalStr.c_str());
  // Get and verify peripheralType
  if (!internalObj.containsKey("peripheralType"))
  {
    Serial.printf("Lỗi: Device '%s' thiếu field 'internal.peripheralType'\n", naturalId.c_str());
    return false;
  }

  String peripheralType = internalObj["peripheralType"].as<String>();
  if (peripheralType != "IR_SENDER")
  {
    Serial.printf("Lỗi: Device '%s' không phải IR_SENDER (là %s)\n", naturalId.c_str(), peripheralType.c_str());
    return false;
  }

  // Get brand
  if (!internalObj.containsKey("brand"))
  {
    brand = "LG"; // Default brand
    Serial.printf("Cảnh báo: Device '%s' không có field 'brand', sử dụng mặc định LG\n", naturalId.c_str());
  }
  else
  {
    brand = internalObj["brand"].as<String>();
  }

  // Verify brand is supported
  const BrandIrSender *sender = findBrandIrSender(brand);
  if (sender == nullptr)
  {
    Serial.printf("Lỗi: Hãng AC '%s' chưa được đăng ký trong registry!\n", brand.c_str());
    return false;
  }

  // Get codeConfigs from internal
  if (!internalObj.containsKey("codeConfigs") || !internalObj["codeConfigs"].is<JsonObjectConst>())
  {
    Serial.printf("Lỗi: Device '%s' thiếu field 'internal.codeConfigs'\n", naturalId.c_str());
    return false;
  }

  JsonObjectConst codeConfigs = internalObj["codeConfigs"].as<JsonObjectConst>();

  // Get bits from codeConfigs (top level)
  if (!codeConfigs.containsKey("bits"))
  {
    bits = 28; // Default bits
    Serial.printf("Cảnh báo: codeConfigs không có field 'bits', sử dụng mặc định 28\n");
  }
  else
  {
    bits = codeConfigs["bits"].as<uint16_t>();
  }

  if (!codeConfigs.containsKey(section) || !codeConfigs[section].is<JsonObjectConst>())
  {
    Serial.printf("Lỗi: Thiếu section '%s' trong codeConfigs\n", section.c_str());
    return false;
  }

  JsonObjectConst sectionObject = codeConfigs[section].as<JsonObjectConst>();
  if (!sectionObject.containsKey(key))
  {
    Serial.printf("Lỗi: Thiếu key '%s' trong section '%s'\n", key.c_str(), section.c_str());
    return false;
  }

  return readIrCodeValue(sectionObject[key], code);
}

bool sendAcCodeFromConfig(const JsonDocument &configDoc, const String &naturalId, const String &section, const String &key, const String &label)
{
  uint32_t code = 0;
  uint16_t bits = 28;
  String brand;

  if (!getAcCommandCodeFromConfig(configDoc, naturalId, section, key, code, bits, brand))
  {
    Serial.printf("Lỗi: Thiếu mã config cho devices[%s].internal.codeConfigs.%s.%s\n", naturalId.c_str(), section.c_str(), key.c_str());
    return false;
  }

  Serial.printf("[Thực thi] %s -> Hãng %s, mã 0x%07X, bits %u\n", label.c_str(), brand.c_str(), code, bits);
  const BrandIrSender *sender = findBrandIrSender(brand);
  if (sender == nullptr)
  {
    Serial.printf("Lỗi: Không có hàm gửi IR cho hãng '%s'!\n", brand.c_str());
    return false;
  }

  return sender->send(code, bits);
}

void reinitializeRelayPins()
{
  for (int i = 0; i < activeRelayCount; i++)
  {
    digitalWrite(activeRelayPins[i], RELAY_INITIAL_STATE);
    Serial.printf("Reset RELAY cũ GPIO %d -> %s\n", activeRelayPins[i], RELAY_INITIAL_STATE ? "HIGH" : "LOW");
  }
  activeRelayCount = 0;

  JsonDocument doc;
  if (deserializeJson(doc, config))
  {
    Serial.println("Lỗi khi parse JSON cấu hình relay!");
    return;
  }

  // Handle new format with { roomCode, devices }
  JsonArray devicesArray;
  if (doc.containsKey("devices"))
  {
    devicesArray = doc["devices"].as<JsonArray>();
  }
  else if (doc.is<JsonArray>())
  {
    // Fallback for old format (array only)
    devicesArray = doc.as<JsonArray>();
  }
  else
  {
    Serial.println("Không tìm thấy devices trong cấu hình!");
    return;
  }

  for (JsonObject device : devicesArray)
  {
    String peripheralType = getDevicePeripheralTypeFromConfig(doc, device["naturalId"].as<String>());
    if (peripheralType != "RELAY")
    {
      continue;
    }

    JsonArray gpioPins = device["gpioPin"];
    for (JsonVariant pinValue : gpioPins)
    {
      int gpioPin = pinValue.as<int>();
      pinMode(gpioPin, OUTPUT);
      digitalWrite(gpioPin, RELAY_INITIAL_STATE);
      activeRelayPins[activeRelayCount++] = gpioPin;
      Serial.printf("Re-init RELAY mới GPIO %d -> %s\n", gpioPin, RELAY_INITIAL_STATE ? "HIGH" : "LOW");
    }
  }
}

bool turnOnAC(const String &naturalId)
{
  JsonDocument configDoc;
  if (!loadConfigDocument(configDoc))
  {
    Serial.println("Lỗi: Không đọc được cấu hình AC!");
    return false;
  }

  return sendAcCodeFromConfig(configDoc, naturalId, "power", "ON", "turnOnAC()");
}

bool turnOffAC(const String &naturalId)
{
  JsonDocument configDoc;
  if (!loadConfigDocument(configDoc))
  {
    Serial.println("Lỗi: Không đọc được cấu hình AC!");
    return false;
  }

  return sendAcCodeFromConfig(configDoc, naturalId, "power", "OFF", "turnOffAC()");
}

bool toggleSwingAC(const String &naturalId)
{
  return setSwingAC(naturalId, "ON");
}

bool setSwingAC(const String &naturalId, String state)
{
  JsonDocument configDoc;
  if (!loadConfigDocument(configDoc))
  {
    Serial.println("Lỗi: Không đọc được cấu hình AC!");
    return false;
  }

  if (state != "ON" && state != "OFF")
  {
    Serial.printf("Lỗi: Trạng thái swing '%s' không hợp lệ!\n", state.c_str());
    return false;
  }

  return sendAcCodeFromConfig(configDoc, naturalId, "swing", state, "setSwingAC()");
}

bool setModeAC(const String &naturalId, String mode)
{
  JsonDocument configDoc;
  if (!loadConfigDocument(configDoc))
  {
    Serial.println("Lỗi: Không đọc được cấu hình AC!");
    return false;
  }

  if (!sendAcCodeFromConfig(configDoc, naturalId, "mode", mode, "setModeAC()"))
  {
    Serial.print("Lỗi: Tham số chế độ '");
    Serial.print(mode);
    Serial.println("' không hợp lệ hoặc thiếu cấu hình!");
    return false;
  }

  return true;
}

bool setFanSpeedAC(const String &naturalId, int speed)
{
  if (speed < 1 || speed > 5)
  {
    Serial.printf("Lỗi: Tốc độ %d không hợp lệ (Chỉ nhận 1 đến 5)!\n", speed);
    return false;
  }

  JsonDocument configDoc;
  if (!loadConfigDocument(configDoc))
  {
    Serial.println("Lỗi: Không đọc được cấu hình AC!");
    return false;
  }

  if (!sendAcCodeFromConfig(configDoc, naturalId, "speed", String(speed), "setFanSpeedAC()"))
  {
    Serial.printf("Lỗi: Thiếu cấu hình cho speed %d!\n", speed);
    return false;
  }

  return true;
}

bool setTemperatureAC(const String &naturalId, int temp)
{
  if (temp < 16 || temp > 30)
  {
    Serial.printf("Lỗi: Nhiệt độ %d°C nằm ngoài dải an toàn (16-30°C)!\n", temp);
    return false;
  }

  JsonDocument configDoc;
  if (!loadConfigDocument(configDoc))
  {
    Serial.println("Lỗi: Không đọc được cấu hình AC!");
    return false;
  }

  if (!sendAcCodeFromConfig(configDoc, naturalId, "temperature", String(temp), "setTemperatureAC()"))
  {
    Serial.printf("Lỗi: Thiếu cấu hình cho nhiệt độ %d!\n", temp);
    return false;
  }

  return true;
}

void controlRelay(int gpioPin, bool state)
{
  Serial.printf("[GPIO WRITE] Pin %d <- %d (%s)\n", gpioPin, state ? LOW : HIGH, state ? "LOW" : "HIGH");
  digitalWrite(gpioPin, state ? HIGH : LOW);
  Serial.printf("[GPIO READ] Pin %d = %d\n", gpioPin, digitalRead(gpioPin));
}

float readDS18B20Temperature(int gpioPin)
{
  OneWire oneWire(gpioPin);
  DallasTemperature sensors(&oneWire);

  sensors.begin();
  sensors.requestTemperatures();

  float temp = sensors.getTempCByIndex(0);

  if (temp == DEVICE_DISCONNECTED_C)
  {
    Serial.printf("Lỗi: Cảm biến DS18B20 trên GPIO %d không phản hồi!\n", gpioPin);
    return -999.0;
  }

  Serial.printf("[DS18B20] GPIO %d -> Nhiệt độ: %.2f°C\n", gpioPin, temp);
  return temp;
}

void printSCD40Error(uint16_t error)
{
  char errorMessage[256];
  errorToString(error, errorMessage, sizeof(errorMessage));
  Serial.println(errorMessage);
}

bool readSCD40Data(int sdaPin, int sclPin, float &temperature, float &humidity)
{
  // Chỉ khởi tạo I2C và cảm biến một lần duy nhất trong suốt vòng đời chip
  if (!isScd40Initialized)
  {
    Wire.begin(sdaPin, sclPin);
    
    scd4x.begin(Wire, 0x62); // SCD40 I2C address: 0x62

    uint16_t error;

    // Stop any previous measurements
    error = scd4x.stopPeriodicMeasurement();
    if (error)
    {
      Serial.print("[SCD40] Stop measurement error: ");
      printSCD40Error(error);
      return false;
    }

    // Get serial number to verify sensor is present
    uint64_t serialNumber;
    error = scd4x.getSerialNumber(serialNumber);
    if (error)
    {
      Serial.print("[SCD40] Sensor not found: ");
      printSCD40Error(error);
      return false;
    }

    Serial.print("[SCD40] Serial Number: 0x");
    Serial.println((unsigned long long)serialNumber, HEX);

    // Start periodic measurement
    error = scd4x.startPeriodicMeasurement();
    if (error)
    {
      Serial.print("[SCD40] Start measurement error: ");
      printSCD40Error(error);
      return false;
    }

    isScd40Initialized = true;
    Serial.println("[SCD40] Khởi tạo thành công lần đầu!");
    Serial.println("[SCD40] Chờ dữ liệu sẵn sàng (5 giây)...");
    delay(5000); // Wait for first measurement
  }

  // Check if data is ready - retry up to 10 times (10 seconds max)
  uint16_t error;
  bool dataReady = false;
  int retries = 0;
  const int MAX_RETRIES = 10;

  while (retries < MAX_RETRIES)
  {
    error = scd4x.getDataReadyStatus(dataReady);
    
    if (error)
    {
      Serial.print("[SCD40] Data ready error: ");
      printSCD40Error(error);
      return false;
    }

    if (dataReady)
    {
      Serial.printf("[SCD40] Data ready after %d retries\n", retries);
      break;
    }

    Serial.printf("[SCD40] Data not ready yet, retry %d/%d...\n", retries + 1, MAX_RETRIES);
    delay(1000);
    retries++;
  }

  if (!dataReady)
  {
    Serial.println("[SCD40] Data timeout after 10 seconds!");
    return false;
  }

  // Read measurement
  uint16_t co2;
  error = scd4x.readMeasurement(co2, temperature, humidity);

  if (error)
  {
    Serial.print("[SCD40] Read measurement error: ");
    printSCD40Error(error);
    Serial.printf("[SCD40] GPIO pins: SDA=%d, SCL=%d\n", sdaPin, sclPin);
    return false;
  }

  Serial.printf("[SCD40] I2C (SDA=%d, SCL=%d) -> CO2: %u ppm, Temp: %.2f°C, Humidity: %.2f%%\n",
                sdaPin, sclPin, co2, temperature, humidity);

  return true;
}

unsigned long getEpochTime()
{
  struct tm timeinfo;
  time_t now;

  if (!getLocalTime(&timeinfo, 10) || timeinfo.tm_year < 126)
  {
    Serial.println("Lỗi! Chưa đồng bộ được thời gian hệ thống.");
    return 0;
  }

  time(&now);
  return now;
}

bool validateJWT(String tokenStr)
{
  int tokenLen = tokenStr.length() + 1;
  char *tokenChar = new char[tokenLen];
  tokenStr.toCharArray(tokenChar, tokenLen);

  jwt.allocateJWTMemory();
  int status = jwt.decodeJWT(tokenChar);
  delete[] tokenChar;

  if (status != 0)
  {
    jwt.clear();
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jwt.payload);
  if (error)
  {
    jwt.clear();
    return false;
  }

  String jwtUsername = doc["username"].as<String>();
  unsigned long expTime = doc["exp"].as<unsigned long>();
  jwt.clear();

  unsigned long currentTime = getEpochTime();
  if (currentTime == 0 || currentTime > expTime)
  {
    Serial.print("Token lỗi hoặc hết hạn! Current: ");
    Serial.print(currentTime);
    Serial.print(" | Exp: ");
    Serial.println(expTime);
    return false;
  }

  return jwtUsername == serverUsername;
}

bool requireBearerToken(String &token)
{
  if (!server.hasHeader("Authorization"))
  {
    sendJson(401, "Phải có Header Authorization");
    return false;
  }

  String authHeader = server.header("Authorization");
  if (!authHeader.startsWith("Bearer "))
  {
    sendJson(401, "Header Authorization phải bắt đầu bằng 'Bearer + token'");
    return false;
  }

  token = authHeader.substring(7);
  if (!validateJWT(token))
  {
    sendJson(401, "Token hết hạn hoặc không đúng");
    return false;
  }

  return true;
}

bool parsePlainBody(JsonDocument &bodyDoc)
{
  if (!server.hasArg("plain"))
  {
    sendJson(400, "Body yêu cầu trống (Missing Plain Text)");
    return false;
  }

  String body = server.arg("plain");
  if (deserializeJson(bodyDoc, body))
  {
    sendJson(400, "Định dạng JSON trong Body không hợp lệ");
    return false;
  }

  return true;
}

DeviceMatch findLightingDevice(const String &naturalId)
{
  DeviceMatch match;
  JsonDocument doc;
  if (deserializeJson(doc, config))
  {
    return match;
  }

  // Handle new format with { roomCode, devices }
  JsonArray devicesArray;
  if (doc.containsKey("devices"))
  {
    devicesArray = doc["devices"].as<JsonArray>();
  }
  else if (doc.is<JsonArray>())
  {
    // Fallback for old format (array only)
    devicesArray = doc.as<JsonArray>();
  }
  else
  {
    return match;
  }

  for (JsonObject device : devicesArray)
  {
    if (device["category"].as<String>() != "LIGHT")
    {
      continue;
    }

    if (naturalId == device["naturalId"].as<String>())
    {
      match.found = true;
      match.peripheralType = getDevicePeripheralTypeFromConfig(doc, naturalId);
      match.firstGpio = device["gpioPin"][0].as<int>();
      break;
    }
  }

  return match;
}

DeviceMatch findFanDevice(const String &naturalId)
{
  DeviceMatch match;
  JsonDocument doc;
  if (deserializeJson(doc, config))
  {
    return match;
  }

  // Handle new format with { roomCode, devices }
  JsonArray devicesArray;
  if (doc.containsKey("devices"))
  {
    devicesArray = doc["devices"].as<JsonArray>();
  }
  else if (doc.is<JsonArray>())
  {
    // Fallback for old format (array only)
    devicesArray = doc.as<JsonArray>();
  }
  else
  {
    return match;
  }

  for (JsonObject device : devicesArray)
  {
    if (device["category"].as<String>() != "FAN")
    {
      continue;
    }

    if (naturalId == device["naturalId"].as<String>())
    {
      match.found = true;
      match.gpioCount = device["gpioPin"].size();
      match.peripheralType = getDevicePeripheralTypeFromConfig(doc, naturalId);
      for (int i = 0; i < match.gpioCount && i < 3; i++)
      {
        match.gpioPins[i] = device["gpioPin"][i].as<int>();
      }
      break;
    }
  }

  return match;
}

void handleNotFound()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("[404 NOT FOUND]");
  Serial.printf("[REQUEST METHOD] %s\n", server.method() == HTTP_GET ? "GET" : 
                                         server.method() == HTTP_POST ? "POST" : 
                                         server.method() == HTTP_OPTIONS ? "OPTIONS" : "OTHER");
  Serial.printf("[REQUEST URI] %s\n", server.uri().c_str());
  Serial.printf("[REQUEST ARGS] %s\n", server.arg("plain").c_str());
  Serial.println("------------------------------------------");
  sendJson(404, "Không tìm thấy");
}

void handleGetTemperature()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/temperature");
  Serial.println("[DEBUG] Request received!");

  String token;
  if (!requireBearerToken(token))
  {
    Serial.println("------------------------------------------");
    return;
  }

  // Get naturalId from query parameter instead of body
  String naturalId = "";
  if (server.hasArg("naturalId"))
  {
    naturalId = server.arg("naturalId");
    Serial.printf("[DEBUG] Query param naturalId found: %s\n", naturalId.c_str());
  }
  else
  {
    Serial.println("[DEBUG] Query param naturalId NOT found!");
    Serial.printf("[DEBUG] Available args: %d\n", server.args());
    for (uint8_t i = 0; i < server.args(); i++)
    {
      Serial.printf("[DEBUG] Arg %d: %s = %s\n", i, server.argName(i).c_str(), server.arg(i).c_str());
    }
  }

  if (naturalId == "null" || naturalId.length() == 0)
  {
    sendJson(400, "Query parameter bắt buộc phải có: ?naturalId=...");
    Serial.println("------------------------------------------");
    return;
  }

  Serial.printf("[REQUEST] naturalId: %s\n", naturalId.c_str());

  JsonDocument configDoc;
  if (deserializeJson(configDoc, config))
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  JsonArrayConst devicesArray;
  if (configDoc.containsKey("devices"))
  {
    devicesArray = configDoc["devices"].as<JsonArrayConst>();
  }
  else
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  JsonObjectConst targetDevice;
  bool deviceFound = false;
  for (JsonObjectConst device : devicesArray)
  {
    String category = device["category"].as<String>();
    if (category != "TEMPERATURE")
    {
      continue;
    }

    if (device["naturalId"].as<String>() == naturalId)
    {
      targetDevice = device;
      deviceFound = true;
      Serial.printf("[DEVICE FOUND] naturalId: %s\n", naturalId.c_str());
      break;
    }
  }

  if (!deviceFound)
  {
    Serial.printf("[DEVICE NOT FOUND] naturalId: %s\n", naturalId.c_str());
    sendJson(404, "Không tìm thấy cảm biến nhiệt độ có naturalId tương ứng");
    Serial.println("------------------------------------------");
    return;
  }

  if (!targetDevice.containsKey("internal") || !targetDevice["internal"].containsKey("module"))
  {
    sendJson(500, "Lỗi: Cảm biến thiếu thông tin module");
    Serial.println("------------------------------------------");
    return;
  }

  String module = targetDevice["internal"]["module"].as<String>();

  if (!targetDevice["gpioPin"].is<JsonArrayConst>() || targetDevice["gpioPin"].size() == 0)
  {
    sendJson(500, "Lỗi: Cảm biến thiếu thông tin GPIO pin");
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument data;

  if (module == "DS18B20")
  {
    int gpioPin = targetDevice["gpioPin"][0].as<int>();
    float temperature = readDS18B20Temperature(gpioPin);

    if (temperature == -999.0)
    {
      sendJson(500, "Lỗi: Không thể đọc dữ liệu từ cảm biến DS18B20");
      Serial.println("------------------------------------------");
      return;
    }

    data["tempC"] = serialized(String(temperature, 3));
  }
  else if (module == "SHT40")
  {
    if (targetDevice["gpioPin"].size() < 2)
    {
      sendJson(500, "Lỗi: SHT40 cần 2 GPIO pins (SDA, SCL)");
      Serial.println("------------------------------------------");
      return;
    }

    int sdaPin = targetDevice["gpioPin"][0].as<int>();
    int sclPin = targetDevice["gpioPin"][1].as<int>();
    float temperature, humidity;

    if (!readSCD40Data(sdaPin, sclPin, temperature, humidity))
    {
      sendJson(500, "Lỗi: Không thể đọc dữ liệu từ cảm biến SHT40");
      Serial.println("------------------------------------------");
      return;
    }

    data["tempC"] = serialized(String(temperature, 3));
  }
  else if (module == "SCD40")
  {
    if (targetDevice["gpioPin"].size() < 2)
    {
      sendJson(500, "Lỗi: SCD40 cần 2 GPIO pins (SDA, SCL)");
      Serial.println("------------------------------------------");
      return;
    }

    // Use cached data from background task
    if (!scd40CachedData.isValid)
    {
      sendJson(500, "Lỗi: Dữ liệu SCD40 chưa sẵn sàng, vui lòng chờ");
      Serial.println("------------------------------------------");
      return;
    }

    data["tempC"] = serialized(String(scd40CachedData.temperature, 3));
    Serial.printf("[SCD40 CACHED] Using cached data: Temp=%.2f°C\n", scd40CachedData.temperature);
  }
  else
  {
    Serial.printf("Lỗi: Module '%s' hiện chưa được hỗ trợ\n", module.c_str());
    sendJson(501, "Module cảm biến chưa được hỗ trợ");
    Serial.println("------------------------------------------");
    return;
  }

  sendJsonWithData(200, "Lấy nhiệt độ thành công", data);
  Serial.println("------------------------------------------");
}

void handleGetHumidity()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/humidity");
  Serial.println("[DEBUG] Request received!");

  String token;
  if (!requireBearerToken(token))
  {
    Serial.println("------------------------------------------");
    return;
  }

  // Get naturalId from query parameter instead of body
  String naturalId = "";
  if (server.hasArg("naturalId"))
  {
    naturalId = server.arg("naturalId");
    Serial.printf("[DEBUG] Query param naturalId found: %s\n", naturalId.c_str());
  }
  else
  {
    Serial.println("[DEBUG] Query param naturalId NOT found!");
    Serial.printf("[DEBUG] Available args: %d\n", server.args());
    for (uint8_t i = 0; i < server.args(); i++)
    {
      Serial.printf("[DEBUG] Arg %d: %s = %s\n", i, server.argName(i).c_str(), server.arg(i).c_str());
    }
  }

  if (naturalId == "null" || naturalId.length() == 0)
  {
    sendJson(400, "Query parameter bắt buộc phải có: ?naturalId=...");
    Serial.println("------------------------------------------");
    return;
  }

  Serial.printf("[REQUEST] naturalId: %s\n", naturalId.c_str());

  JsonDocument configDoc;
  if (deserializeJson(configDoc, config))
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  JsonArrayConst devicesArray;
  if (configDoc.containsKey("devices"))
  {
    devicesArray = configDoc["devices"].as<JsonArrayConst>();
  }
  else
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  JsonObjectConst targetDevice;
  bool deviceFound = false;
  for (JsonObjectConst device : devicesArray)
  {
    String category = device["category"].as<String>();
    if (category != "HUMIDITY")
    {
      continue;
    }

    if (device["naturalId"].as<String>() == naturalId)
    {
      targetDevice = device;
      deviceFound = true;
      Serial.printf("[DEVICE FOUND] naturalId: %s\n", naturalId.c_str());
      break;
    }
  }

  if (!deviceFound)
  {
    Serial.printf("[DEVICE NOT FOUND] naturalId: %s\n", naturalId.c_str());
    sendJson(404, "Không tìm thấy cảm biến độ ẩm có naturalId tương ứng");
    Serial.println("------------------------------------------");
    return;
  }

  if (!targetDevice.containsKey("internal") || !targetDevice["internal"].containsKey("module"))
  {
    sendJson(500, "Lỗi: Cảm biến thiếu thông tin module");
    Serial.println("------------------------------------------");
    return;
  }

  String module = targetDevice["internal"]["module"].as<String>();

  if (!targetDevice["gpioPin"].is<JsonArrayConst>() || targetDevice["gpioPin"].size() < 2)
  {
    sendJson(500, "Lỗi: SHT40 cần 2 GPIO pins (SDA, SCL)");
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument data;

  if (module == "SHT40")
  {
    int sdaPin = targetDevice["gpioPin"][0].as<int>();
    int sclPin = targetDevice["gpioPin"][1].as<int>();
    float temperature, humidity;

    if (!readSCD40Data(sdaPin, sclPin, temperature, humidity))
    {
      sendJson(500, "Lỗi: Không thể đọc dữ liệu từ cảm biến SHT40");
      Serial.println("------------------------------------------");
      return;
    }

    data["humidity"] = serialized(String(humidity, 2));
  }
  else if (module == "SCD40")
  {
    // Use cached data from background task
    if (!scd40CachedData.isValid)
    {
      sendJson(500, "Lỗi: Dữ liệu SCD40 chưa sẵn sàng, vui lòng chờ");
      Serial.println("------------------------------------------");
      return;
    }

    // data["humidity"] = serialized(String(scd40CachedData.humidity, 2));
    data["humidity"] = scd40CachedData.humidity;
    Serial.printf("[SCD40 CACHED] Using cached data: Humidity=%.2f%%\n", scd40CachedData.humidity);
  }
  else
  {
    Serial.printf("Lỗi: Module '%s' không hỗ trợ đo độ ẩm\n", module.c_str());
    sendJson(501, "Module cảm biến không hỗ trợ đo độ ẩm");
    Serial.println("------------------------------------------");
    return;
  }

  sendJsonWithData(200, "Lấy độ ẩm thành công", data);
  Serial.println("------------------------------------------");
}

void handleGetCO2()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/co2");
  Serial.println("[DEBUG] Request received!");

  String token;
  if (!requireBearerToken(token))
  {
    Serial.println("------------------------------------------");
    return;
  }

  // Get naturalId from query parameter
  String naturalId = "";
  if (server.hasArg("naturalId"))
  {
    naturalId = server.arg("naturalId");
    Serial.printf("[DEBUG] Query param naturalId found: %s\n", naturalId.c_str());
  }
  else
  {
    Serial.println("[DEBUG] Query param naturalId NOT found!");
    Serial.printf("[DEBUG] Available args: %d\n", server.args());
    for (uint8_t i = 0; i < server.args(); i++)
    {
      Serial.printf("[DEBUG] Arg %d: %s = %s\n", i, server.argName(i).c_str(), server.arg(i).c_str());
    }
  }

  if (naturalId == "null" || naturalId.length() == 0)
  {
    sendJson(400, "Query parameter bắt buộc phải có: ?naturalId=...");
    Serial.println("------------------------------------------");
    return;
  }

  Serial.printf("[REQUEST] naturalId: %s\n", naturalId.c_str());

  JsonDocument configDoc;
  if (deserializeJson(configDoc, config))
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  JsonArrayConst devicesArray;
  if (configDoc.containsKey("devices"))
  {
    devicesArray = configDoc["devices"].as<JsonArrayConst>();
  }
  else
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  JsonObjectConst targetDevice;
  bool deviceFound = false;
  for (JsonObjectConst device : devicesArray)
  {
    String category = device["category"].as<String>();
    if (category != "SENSOR_CO2")
    {
      continue;
    }

    if (device["naturalId"].as<String>() == naturalId)
    {
      targetDevice = device;
      deviceFound = true;
      Serial.printf("[DEVICE FOUND] naturalId: %s\n", naturalId.c_str());
      break;
    }
  }

  if (!deviceFound)
  {
    Serial.printf("[DEVICE NOT FOUND] naturalId: %s\n", naturalId.c_str());
    sendJson(404, "Không tìm thấy cảm biến CO2 có naturalId tương ứng");
    Serial.println("------------------------------------------");
    return;
  }

  if (!targetDevice.containsKey("internal") || !targetDevice["internal"].containsKey("module"))
  {
    sendJson(500, "Lỗi: Cảm biến thiếu thông tin module");
    Serial.println("------------------------------------------");
    return;
  }

  String module = targetDevice["internal"]["module"].as<String>();

  if (!targetDevice["gpioPin"].is<JsonArrayConst>() || targetDevice["gpioPin"].size() < 2)
  {
    sendJson(500, "Lỗi: SCD40 cần 2 GPIO pins (SDA, SCL)");
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument data;

  if (module == "SCD40")
  {
    // Use cached data from background task
    if (!scd40CachedData.isValid)
    {
      sendJson(500, "Lỗi: Dữ liệu SCD40 chưa sẵn sàng, vui lòng chờ");
      Serial.println("------------------------------------------");
      return;
    }

    data["co2"] = scd40CachedData.co2;
    Serial.printf("[SCD40 CACHED] Using cached data: CO2=%d ppm\n", scd40CachedData.co2);
  }
  else
  {
    Serial.printf("Lỗi: Module '%s' không hỗ trợ đo CO2\n", module.c_str());
    sendJson(501, "Module cảm biến không hỗ trợ đo CO2");
    Serial.println("------------------------------------------");
    return;
  }

  sendJsonWithData(200, "Lấy CO2 thành công", data);
  Serial.println("------------------------------------------");
}

void handleGetLux()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/lux");
  Serial.println("[DEBUG] Request received!");

  String token;
  if (!requireBearerToken(token))
  {
    Serial.println("------------------------------------------");
    return;
  }

  String naturalId = "";
  if (server.hasArg("naturalId"))
  {
    naturalId = server.arg("naturalId");
    Serial.printf("[DEBUG] Query param naturalId found: %s\n", naturalId.c_str());
  }

  if (naturalId == "null" || naturalId.length() == 0)
  {
    sendJson(400, "Query parameter bắt buộc phải có: ?naturalId=...");
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument configDoc;
  if (deserializeJson(configDoc, config))
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  JsonArrayConst devicesArray = configDoc["devices"].as<JsonArrayConst>();
  JsonObjectConst targetDevice;
  bool deviceFound = false;

  for (JsonObjectConst device : devicesArray)
  {
    if (device["category"].as<String>() != "SENSOR_LUX")
    {
      continue;
    }

    if (device["naturalId"].as<String>() == naturalId)
    {
      targetDevice = device;
      deviceFound = true;
      break;
    }
  }

  if (!deviceFound)
  {
    sendJson(404, "Không tìm thấy cảm biến ánh sáng có naturalId tương ứng");
    Serial.println("------------------------------------------");
    return;
  }

  String module = targetDevice["internal"]["module"] | "";
  if (module != "BH1750")
  {
    sendJson(501, "Module cảm biến không hỗ trợ đo Lux");
    Serial.println("------------------------------------------");
    return;
  }

  if (!isBh1750Initialized || !bh1750CachedData.isValid)
  {
    sendJson(500, "Lỗi: Dữ liệu BH1750 chưa sẵn sàng, vui lòng chờ");
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument data;
  data["lux"] = bh1750CachedData.lux;

  Serial.printf(
      "[BH1750 CACHED] Using cached data: Lux=%.2f lx\n",
      bh1750CachedData.lux
  );

  sendJsonWithData(200, "Lấy cường độ ánh sáng thành công", data);
  Serial.println("------------------------------------------");
}

void handleGetConfig()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/setup");

  String token;
  if (!requireBearerToken(token))
  {
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument configDoc;
  deserializeJson(configDoc, config);

  JsonDocument data;
  String roomCode = getRoomCodeFromConfigString(config);

  if (configDoc.is<JsonArray>())
  {
    // Fallback for old format
    if (roomCode.length() > 0)
    {
      data["roomCode"] = roomCode;
    }
    data["devices"] = configDoc;
  }
  else
  {
    data = configDoc;
  }

  sendJsonWithData(200, "Lấy JSON cấu hình thiết bị thành công", data);
  Serial.println("------------------------------------------");
}

void handleControl()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/control");

  String token;
  if (!requireBearerToken(token))
  {
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument jsonBody;
  if (!parsePlainBody(jsonBody))
  {
    Serial.println("------------------------------------------");
    return;
  }

  String naturalId = jsonBody["naturalId"].as<String>();
  String category = jsonBody["category"].as<String>();

  // Log request details
  Serial.printf("[REQUEST] naturalId: %s, category: %s\n", naturalId.c_str(), category.c_str());

  if (naturalId == "null" || naturalId.length() == 0 || category == "null" || category.length() == 0)
  {
    sendJson(400, "Body bắt buộc phải có các trường: naturalId, category");
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument configDoc;
  if (deserializeJson(configDoc, config))
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  // Extract devices array from new format { roomCode, devices } or old format (array)
  JsonArray devicesArray;
  if (configDoc.containsKey("devices"))
  {
    devicesArray = configDoc["devices"].as<JsonArray>();
  }
  else if (configDoc.is<JsonArray>())
  {
    devicesArray = configDoc.as<JsonArray>();
  }
  else
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  JsonObject targetDevice;
  bool deviceFound = false;
  for (JsonObject device : devicesArray)
  {
    if (device["naturalId"].as<String>() == naturalId && device["category"].as<String>() == category)
    {
      targetDevice = device;
      deviceFound = true;
      Serial.printf("[DEVICE FOUND] naturalId: %s\n", naturalId.c_str());
      break;
    }
  }

  if (!deviceFound)
  {
    Serial.printf("[DEVICE NOT FOUND] naturalId: %s, category: %s\n", naturalId.c_str(), category.c_str());
    sendJson(404, "Không tìm thấy thiết bị có naturalId tương ứng");
    Serial.println("------------------------------------------");
    return;
  }

  if (category == "LIGHT")
  {
    if (!targetDevice["gpioPin"].is<JsonArray>() || targetDevice["gpioPin"].size() == 0)
    {
      sendJson(404, "Không tìm thấy thiết bị có naturalId tương ứng");
      Serial.println("------------------------------------------");
      return;
    }

    int targetGpio = targetDevice["gpioPin"][0].as<int>();
    String peripheralType = getDevicePeripheralTypeFromConfig(configDoc, naturalId);
    String power = jsonBody["power"].as<String>();

    Serial.printf("[LIGHT CONTROL] GPIO: %d, Power: %s\n", targetGpio, power.c_str());

    if (jsonBody.containsKey("power") && power != "ON" && power != "OFF")
    {
      sendJson(400, "power chỉ nhận giá trị là ON/OFF");
      Serial.println("------------------------------------------");
      return;
    }

    if (peripheralType == "RELAY")
    {
      if (jsonBody.containsKey("power"))
      {
        Serial.printf("[RELAY ACTION] Setting GPIO %d to %s\n", targetGpio, power.c_str());
        controlRelay(targetGpio, power == "ON");
        Serial.printf("[RELAY DONE] GPIO %d is now %s\n", targetGpio, power == "ON" ? "HIGH" : "LOW");
      }
      else
      {
        Serial.println("[WARNING] power field not provided in request");
      }
      sendJson(200, "Điều khiển thành công");
    }

    Serial.println("------------------------------------------");
    return;
  }

  if (category == "AIR_CONDITION")
  {
    bool actionExecuted = false;
    bool actionFailed = false;

    if (jsonBody.containsKey("power"))
    {
      String power = jsonBody["power"].as<String>();
      if (power == "ON")
      {
        actionFailed = !turnOnAC(naturalId);
        actionExecuted = true;
      }
      else if (power == "OFF")
      {
        actionFailed = !turnOffAC(naturalId);
        actionExecuted = true;
      }
      else
      {
        sendJson(400, "power chỉ nhận giá trị là ON/OFF");
        Serial.println("------------------------------------------");
        return;
      }
    }

    if (jsonBody.containsKey("temperature"))
    {
      if (actionFailed)
      {
        sendJson(500, "Lỗi đọc cấu hình AC");
        Serial.println("------------------------------------------");
        return;
      }

      if (actionExecuted)
      {
        delay(100);
      }

      int temperature = jsonBody["temperature"];
      if (temperature < 16 || temperature > 30)
      {
        sendJson(400, "temperature chỉ nhận giá trị từ 16 đến 30");
        Serial.println("------------------------------------------");
        return;
      }

      actionFailed = !setTemperatureAC(naturalId, temperature);
      actionExecuted = true;
    }

    if (jsonBody.containsKey("mode"))
    {
      if (actionFailed)
      {
        sendJson(500, "Lỗi đọc cấu hình AC");
        Serial.println("------------------------------------------");
        return;
      }

      if (actionExecuted)
      {
        delay(100);
      }

      String mode = jsonBody["mode"].as<String>();
      if (mode != "COOL" && mode != "HEAT" && mode != "DRY" && mode != "AUTO" && mode != "FAN")
      {
        sendJson(400, "mode chỉ nhận giá trị COOL, HEAT, DRY, AUTO, FAN");
        Serial.println("------------------------------------------");
        return;
      }

      actionFailed = !setModeAC(naturalId, mode);
      actionExecuted = true;
    }

    if (jsonBody.containsKey("speed") || jsonBody.containsKey("fanSpeed"))
    {
      if (actionFailed)
      {
        sendJson(500, "Lỗi đọc cấu hình AC");
        Serial.println("------------------------------------------");
        return;
      }

      if (actionExecuted)
      {
        delay(100);
      }

      int speed = jsonBody.containsKey("speed") ? jsonBody["speed"] : jsonBody["fanSpeed"];
      if (speed < 1 || speed > 3)
      {
        sendJson(400, "speed chỉ nhận giá trị từ 1 đến 3");
        Serial.println("------------------------------------------");
        return;
      }

      actionFailed = !setFanSpeedAC(naturalId, speed);
      actionExecuted = true;
    }

    if (jsonBody.containsKey("swing"))
    {
      if (actionFailed)
      {
        sendJson(500, "Lỗi đọc cấu hình AC");
        Serial.println("------------------------------------------");
        return;
      }

      if (actionExecuted)
      {
        delay(100);
      }

      String swing = jsonBody["swing"].as<String>();
      if (swing != "ON" && swing != "OFF")
      {
        sendJson(400, "swing chỉ nhận giá trị ON/OFF");
        Serial.println("------------------------------------------");
        return;
      }

      actionFailed = !setSwingAC(naturalId, swing);
      actionExecuted = true;
    }

    if (actionFailed)
    {
      sendJson(500, "Lỗi đọc cấu hình AC");
      Serial.println("------------------------------------------");
      return;
    }

    String message = actionExecuted ? "Điều khiển AC thành công" : "Không có cấu hình nào được thay đổi";
    sendJson(200, message);

    Serial.println("[Thành công] Phản hồi dữ liệu kết quả về Client hoàn tất.");
    Serial.println("------------------------------------------");
    return;
  }

  if (category == "FAN")
  {
    int targetGpioCount = targetDevice["gpioPin"].size();
    int targetGpioPins[3] = {-1, -1, -1};
    for (int i = 0; i < targetGpioCount && i < 3; i++)
    {
      targetGpioPins[i] = targetDevice["gpioPin"][i].as<int>();
    }

    String power = jsonBody["power"].as<String>();
    bool hasPower = jsonBody.containsKey("power");
    bool hasSpeed = jsonBody.containsKey("speed");

    if (hasPower && power != "ON" && power != "OFF")
    {
      sendJson(400, "power chỉ nhận giá trị là ON/OFF");
      Serial.println("------------------------------------------");
      return;
    }

    if (hasSpeed)
    {
      int speed = jsonBody["speed"] | 0;
      if (speed < 1 || speed > 3)
      {
        sendJson(400, "speed chỉ nhận giá trị 1, 2, hoặc 3");
        Serial.println("------------------------------------------");
        return;
      }

      for (int i = 0; i < targetGpioCount; i++)
      {
        if (targetGpioPins[i] != -1)
        {
          controlRelay(targetGpioPins[i], false);
        }
      }

      if (speed <= targetGpioCount)
      {
        controlRelay(targetGpioPins[speed - 1], true);
      }
    }
    else if (hasPower)
    {
      if (power == "OFF")
      {
        for (int i = 0; i < targetGpioCount; i++)
        {
          if (targetGpioPins[i] != -1)
          {
            controlRelay(targetGpioPins[i], false);
          }
        }
        sendJson(200, "Điều khiển thành công");
        Serial.println("------------------------------------------");
        return;
      }

      for (int i = 0; i < targetGpioCount; i++)
      {
        if (targetGpioPins[i] != -1)
        {
          controlRelay(targetGpioPins[i], false);
        }
      }
      if (targetGpioCount > 0)
      {
        controlRelay(targetGpioPins[0], true);
      }
    }
    else
    {
      for (int i = 0; i < targetGpioCount; i++)
      {
        if (targetGpioPins[i] != -1)
        {
          controlRelay(targetGpioPins[i], false);
        }
      }
      if (targetGpioCount > 0)
      {
        controlRelay(targetGpioPins[0], true);
      }
    }

    sendJson(200, "Điều khiển thành công");
    Serial.println("------------------------------------------");
    return;
  }

  sendJson(400, "category chỉ nhận giá trị LIGHT | FAN | AIR_CONDITION");
  Serial.println("------------------------------------------");
}

void handleLogin()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/auth/login");

  JsonDocument jsonBody;
  if (!parsePlainBody(jsonBody))
  {
    Serial.println("------------------------------------------");
    return;
  }

  String username = jsonBody["username"].as<String>();
  String passwordValue = jsonBody["password"].as<String>();

  // Log login attempt
  Serial.printf("[LOGIN ATTEMPT] username: %s\n", username.c_str());

  if (username.length() == 0 || username == "null" || passwordValue.length() == 0 || passwordValue == "null")
  {
    Serial.println("[LOGIN FAILED] Missing username or password");
    sendJson(400, "Thiếu body request hoặc body không đúng định dạng");
    Serial.println("------------------------------------------");
    return;
  }

  if (username != serverUsername || passwordValue != serverPassword)
  {
    Serial.println("[LOGIN FAILED] Invalid credentials");
    sendJson(401, "username hoặc password không đúng");
    Serial.println("------------------------------------------");
    return;
  }

  unsigned long currentEpoch = getEpochTime();
  if (currentEpoch == 0)
  {
    Serial.println("[LOGIN FAILED] Time not synced");
    sendJson(500, "Server chưa đồng bộ xong thời gian, hãy thử lại sau vài giây!");
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument payloadDoc;
  payloadDoc["username"] = username;
  payloadDoc["role"] = "super_user";
  payloadDoc["exp"] = currentEpoch + 3600;

  String payloadStr;
  serializeJson(payloadDoc, payloadStr);

  char payloadChar[256];
  payloadStr.toCharArray(payloadChar, 256);

  jwt.allocateJWTMemory();
  jwt.encodeJWT(payloadChar);
  String finalToken = String(jwt.out);
  jwt.clear();

  Serial.println("[LOGIN SUCCESS]");
  Serial.printf("[TOKEN] %s\n", finalToken.c_str());

  JsonDocument data;
  data["token"] = finalToken;
  sendJsonWithData(200, "Đăng nhập thành công", data);
  Serial.println("------------------------------------------");
}

void handleTelemetry()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/telemetry");

  String token;
  if (!requireBearerToken(token))
  {
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument configDoc;
  if (deserializeJson(configDoc, config))
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  JsonArrayConst devicesArray;
  if (configDoc.containsKey("devices"))
  {
    devicesArray = configDoc["devices"].as<JsonArrayConst>();
  }
  else
  {
    sendJson(500, "Lỗi thiết bị khi parse JSON");
    Serial.println("------------------------------------------");
    return;
  }

  // Get room code
  String roomCode = "";
  if (configDoc.containsKey("roomCode"))
  {
    roomCode = configDoc["roomCode"].as<String>();
  }

  // Collect all TEMPERATURE devices and their data
  JsonDocument responseData;
  responseData["roomCode"] = roomCode;

  JsonArray devicesArray_out = responseData.createNestedArray("devices");

  for (JsonObjectConst device : devicesArray)
  {
    String category = device["category"].as<String>();
    String naturalId = device["naturalId"].as<String>();

    if (category == "SENSOR_CO2")
    {
        if (!scd40CachedData.isValid)
        {
            Serial.printf(
                "[TELEMETRY] CO2 device %s: cached data not ready\n",
                naturalId.c_str()
            );
            continue;
        }

        JsonObject deviceItem = devicesArray_out.createNestedObject();
        deviceItem["naturalId"] = naturalId;
        deviceItem["category"] = "SENSOR_CO2";

        JsonObject dataObj = deviceItem.createNestedObject("data");
        dataObj["co2"] = scd40CachedData.co2;

        Serial.printf(
            "[TELEMETRY] Device %s: CO2 = %u ppm\n",
            naturalId.c_str(),
            scd40CachedData.co2
        );

        continue;
    }

    if (category == "HUMIDITY")
    {
      if (!scd40CachedData.isValid)
      {
        Serial.printf(
            "[TELEMETRY] Humidity device %s: cached data not ready\n",
            naturalId.c_str()
        );
        continue;
      }

      JsonObject deviceItem =
          devicesArray_out.createNestedObject();

      deviceItem["naturalId"] = naturalId;
      deviceItem["category"] = "HUMIDITY";

      JsonObject dataObj =
          deviceItem.createNestedObject("data");

      // Gửi dưới dạng số thực
      dataObj["humidity"] = scd40CachedData.humidity;

      Serial.printf(
          "[TELEMETRY] Device %s: humidity = %.2f %%RH\n",
          naturalId.c_str(),
          scd40CachedData.humidity
      );

      continue;
    }

    if (category == "SENSOR_LUX")
    {
      if (!isBh1750Initialized || !bh1750CachedData.isValid)
      {
        Serial.printf(
            "[TELEMETRY] Lux device %s: cached data not ready\n",
            naturalId.c_str()
        );
        continue;
      }

      JsonObject deviceItem = devicesArray_out.createNestedObject();
      deviceItem["naturalId"] = naturalId;
      deviceItem["category"] = "SENSOR_LUX";

      JsonObject dataObj = deviceItem.createNestedObject("data");
      dataObj["lux"] = bh1750CachedData.lux;

      Serial.printf(
          "[TELEMETRY] Device %s: lux = %.2f lx\n",
          naturalId.c_str(),
          bh1750CachedData.lux
      );

      continue;
    }

    if (category != "TEMPERATURE")
    {
      continue;
    }

    // String naturalId = device["naturalId"].as<String>();
    Serial.printf("[TELEMETRY] Processing device: %s\n", naturalId.c_str());

    // Check if device has internal.module
    if (!device.containsKey("internal") || !device["internal"].containsKey("module"))
    {
      Serial.printf("[TELEMETRY] Device %s: Missing module info\n", naturalId.c_str());
      continue;
    }

    String module = device["internal"]["module"].as<String>();

    // Get GPIO pin
    if (!device["gpioPin"].is<JsonArrayConst>() || device["gpioPin"].size() == 0)
    {
      Serial.printf("[TELEMETRY] Device %s: No GPIO pin found\n", naturalId.c_str());
      continue;
    }

    float temperature = 0;
    bool readSuccess = false;

    if (module == "DS18B20")
    {
      int gpioPin = device["gpioPin"][0].as<int>();
      temperature = readDS18B20Temperature(gpioPin);
      readSuccess = (temperature != -999.0);
    }
    else if (module == "SHT40")
    {
      if (device["gpioPin"].size() < 2)
      {
        Serial.printf("[TELEMETRY] Device %s: SHT40 needs 2 GPIO pins\n", naturalId.c_str());
        continue;
      }

      int sdaPin = device["gpioPin"][0].as<int>();
      int sclPin = device["gpioPin"][1].as<int>();
      float humidity;
      readSuccess = readSCD40Data(sdaPin, sclPin, temperature, humidity);
    }
    else if (module == "SCD40")
    {
      if (device["gpioPin"].size() < 2)
      {
        Serial.printf("[TELEMETRY] Device %s: SCD40 needs 2 GPIO pins\n", naturalId.c_str());
        continue;
      }

      // Use cached data from background task
      if (!scd40CachedData.isValid)
      {
        Serial.printf("[TELEMETRY] Device %s: SCD40 cached data not ready\n", naturalId.c_str());
        continue;
      }

      temperature = scd40CachedData.temperature;
      readSuccess = true;
      Serial.printf("[TELEMETRY] Device %s: Using cached SCD40 data\n", naturalId.c_str());
    }
    else
    {
      Serial.printf("[TELEMETRY] Device %s: Module %s not supported\n", naturalId.c_str(), module.c_str());
      continue;
    }

    if (!readSuccess)
    {
      Serial.printf("[TELEMETRY] Device %s: Failed to read temperature\n", naturalId.c_str());
      continue;
    }

    // Add device data to response
    JsonObject deviceItem = devicesArray_out.createNestedObject();
    deviceItem["naturalId"] = naturalId;
    deviceItem["category"] = "TEMPERATURE";

    JsonObject dataObj = deviceItem.createNestedObject("data");
    dataObj["tempC"] = serialized(String(temperature, 3));

    Serial.printf("[TELEMETRY] Device %s: tempC = %.2f\n", naturalId.c_str(), temperature);
  }

  sendJsonWithData(200, "Success", responseData);
  Serial.println("------------------------------------------");
}

void initAllSensors()
{
  Serial.println("\n========================================");
  Serial.println("[SENSOR INITIALIZATION] Bắt đầu...");
  Serial.println("========================================");

  JsonDocument configDoc;
  if (deserializeJson(configDoc, config))
  {
    Serial.println("[SENSOR INIT] Lỗi parse config JSON!");
    return;
  }

  JsonArrayConst devicesArray;
  if (configDoc.containsKey("devices"))
  {
    devicesArray = configDoc["devices"].as<JsonArrayConst>();
  }
  else
  {
    Serial.println("[SENSOR INIT] Không tìm thấy devices trong config!");
    return;
  }

  // Scan config để init SCD40 sensors
  bool hasSCD40 = false;
  int scd40SdaPin = 8;
  int scd40SclPin = 9;

  for (JsonObjectConst device : devicesArray)
  {
    if (!device.containsKey("internal") || !device["internal"].containsKey("module"))
    {
      continue;
    }

    String module = device["internal"]["module"].as<String>();
    
    if (module != "SCD40")
    {
      continue;
    }

    // Found SCD40 device, get GPIO pins
    if (device.containsKey("gpioPin") && device["gpioPin"].size() >= 2)
    {
      scd40SdaPin = device["gpioPin"][0].as<int>();
      scd40SclPin = device["gpioPin"][1].as<int>();
      hasSCD40 = true;

      String naturalId = device["naturalId"].as<String>();
      Serial.printf("[SENSOR INIT] Found SCD40: %s (SDA=%d, SCL=%d)\n", 
                    naturalId.c_str(), scd40SdaPin, scd40SclPin);
      break; // Only one SCD40 per board
    }
  }

  // Init SCD40 if found in config
  if (hasSCD40 && !isScd40Initialized)
  {
    Serial.println("[SENSOR INIT] Initializing SCD40...");
    float temp, humidity;
    
    // Call readSCD40Data to initialize (will set isScd40Initialized = true)
    if (readSCD40Data(scd40SdaPin, scd40SclPin, temp, humidity))
    {
      Serial.printf("[SENSOR INIT] SCD40 initialized successfully!\n");
      Serial.printf("[SENSOR INIT] First read: Temp=%.2f°C, Humidity=%.2f%%\n", temp, humidity);
    }
    else
    {
      Serial.println("[SENSOR INIT] SCD40 initialization failed!");
      Serial.println("[SENSOR INIT] Check hardware connections and I2C pins");
    }
  }

  // Scan config và khởi tạo BH1750
  for (JsonObjectConst device : devicesArray)
  {
    if (!device.containsKey("internal") ||
        device["internal"]["module"].as<String>() != "BH1750")
    {
      continue;
    }

    int bh1750SdaPin = configDoc["I2C"]["SDA"] | 8;
    int bh1750SclPin = configDoc["I2C"]["SCL"] | 9;
    bh1750I2cAddress = static_cast<uint8_t>(
        device["internal"]["i2cAddress"] | 0x23
    );

    // Nếu không có SCD40, BH1750 chịu trách nhiệm khởi tạo bus I2C.
    // Nếu có SCD40, Wire đã được khởi tạo trong readSCD40Data().
    if (!hasSCD40)
    {
      Wire.begin(bh1750SdaPin, bh1750SclPin);
    }

    Serial.printf(
        "[SENSOR INIT] Initializing BH1750: %s "
        "(SDA=%d, SCL=%d, address=0x%02X)\n",
        device["naturalId"].as<const char *>(),
        bh1750SdaPin,
        bh1750SclPin,
        bh1750I2cAddress
    );

    isBh1750Initialized = bh1750.begin(
        BH1750::CONTINUOUS_HIGH_RES_MODE,
        bh1750I2cAddress,
        &Wire
    );

    if (!isBh1750Initialized)
    {
      Serial.println("[SENSOR INIT] BH1750 initialization failed!");
      break;
    }

    // High-resolution mode cần thời gian đo đầu tiên.
    delay(200);

    float lux = bh1750.readLightLevel();
    if (lux >= 0.0f && isfinite(lux))
    {
      bh1750CachedData.lux = lux;
      bh1750CachedData.lastReadTime = millis();
      bh1750CachedData.isValid = true;

      Serial.printf(
          "[SENSOR INIT] BH1750 initialized successfully: %.2f lx\n",
          lux
      );
    }
    else
    {
      Serial.printf(
          "[SENSOR INIT] BH1750 initialized but first reading is invalid: %.2f\n",
          lux
      );
    }

    break; // Chỉ dùng một BH1750 trên board này
  }

  Serial.println("========================================");
  Serial.println("[SENSOR INITIALIZATION] Hoàn tất");
  Serial.println("========================================\n");
}

// SCD40 Background Task - liên tục đọc data và cache
void scd40BackgroundTask(void *parameter)
{
  Serial.println("[SENSOR BACKGROUND] Task started");

  while (scd40BackgroundTaskRunning)
  {
    // Đọc SCD40 nếu đã khởi tạo
    if (isScd40Initialized)
    {
      uint16_t error;
      bool dataReady = false;

      error = scd4x.getDataReadyStatus(dataReady);

      if (error)
      {
        Serial.print("[SCD40 BACKGROUND] Data ready error: ");
        Serial.println(error);
      }
      else if (dataReady)
      {
        uint16_t co2;
        float temperature;
        float humidity;

        error = scd4x.readMeasurement(co2, temperature, humidity);

        if (error)
        {
          Serial.print("[SCD40 BACKGROUND] Read measurement error: ");
          Serial.println(error);
        }
        else
        {
          scd40CachedData.co2 = co2;
          scd40CachedData.temperature = temperature;
          scd40CachedData.humidity = humidity;
          scd40CachedData.lastReadTime = millis();
          scd40CachedData.isValid = true;

          Serial.printf(
              "[SCD40 BACKGROUND] CO2: %u ppm, "
              "Temp: %.2f°C, Humidity: %.2f%%\n",
              co2,
              temperature,
              humidity
          );
        }
      }
    }

    // Đọc BH1750 tuần tự trên cùng bus I2C
    if (isBh1750Initialized)
    {
      float lux = bh1750.readLightLevel();

      if (lux >= 0.0f && isfinite(lux))
      {
        bh1750CachedData.lux = lux;
        bh1750CachedData.lastReadTime = millis();
        bh1750CachedData.isValid = true;

        Serial.printf(
            "[BH1750 BACKGROUND] Lux: %.2f lx\n",
            lux
        );
      }
      else
      {
        Serial.printf(
            "[BH1750 BACKGROUND] Invalid reading: %.2f\n",
            lux
        );
      }
    }

    delay(30000); // Cập nhật cache mỗi 30 giây
  }

  Serial.println("[SENSOR BACKGROUND] Task ended");
  vTaskDelete(NULL);
}

// Khởi động background task
void startSCD40BackgroundTask()
{
  if (!scd40BackgroundTaskRunning &&
      (isScd40Initialized || isBh1750Initialized))
  {
    scd40BackgroundTaskRunning = true;
    
    // Tạo FreeRTOS task (priority 1, stack size 4096, core 1)
    xTaskCreatePinnedToCore(
      scd40BackgroundTask,           // Task function
      "SCD40 Background",            // Task name
      4096,                          // Stack size
      NULL,                          // Parameters
      1,                             // Priority
      NULL,                          // Task handle
      1                              // Core (1 = second core)
    );
    
    Serial.println("[SENSOR] Background task started successfully");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  WiFi.disconnect(true);
  delay(1000);

  pinMode(ledPin, OUTPUT);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    Serial.println("Cấu hình IP tĩnh thất bại!");
  }

  Serial.print("Đang kết nối đến WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  int timeout_counter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
    timeout_counter++;

    if (timeout_counter > 4)
    {
      Serial.println("\nKết nối quá thời gian! Đang khởi động lại...");
      ESP.restart();
    }
  }

  Serial.println("\nKết nối WiFi thành công!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Tải config để lấy timeApi
  CheckConfigJSON();

  // Thử sync time từ API trước
  JsonDocument configDoc;
  String timeApiUrl = "";
  if (deserializeJson(configDoc, config) == DeserializationError::Ok)
  {
    if (configDoc.containsKey("timeApi"))
    {
      timeApiUrl = configDoc["timeApi"].as<String>();
      Serial.printf("[SETUP] timeApi từ config: %s\n", timeApiUrl.c_str());
    }
    else
    {
      Serial.println("[SETUP] Config không có trường timeApi");
    }
  }
  else
  {
    Serial.println("[SETUP] Lỗi parse config JSON");
  }

  bool timeSyncSuccess = false;
  if (timeApiUrl.length() > 0 && timeApiUrl != "null")
  {
    timeSyncSuccess = syncTimeFromApi(timeApiUrl);
  }
  else
  {
    Serial.println("[SETUP] timeApi rỗng hoặc null, bỏ qua API sync");
  }

  // Nếu API sync thất bại, fallback sang NTP
  if (!timeSyncSuccess)
  {
    Serial.println("[TIME SYNC] API sync thất bại, fallback sang NTP...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Đang đồng bộ thời gian từ NTP...");

    struct tm timeinfo;
    int retryCounter = 0;
    while ((!getLocalTime(&timeinfo) || timeinfo.tm_year < 126) && retryCounter < 2)
    {
      delay(500);
      Serial.print("*");
      retryCounter++;
    }

    if (retryCounter >= 2)
    {
      Serial.println("\n[LỖI BẢO MẬT] Không thể đồng bộ thời gian từ API hoặc NTP!");
      Serial.println("Để đảm bảo an toàn, hệ thống sẽ tự động khởi động lại sau 1 giây...");
      delay(1000);
      ESP.restart();
    }
    else
    {
      Serial.println("\nĐồng bộ thời gian từ NTP thành công!");
    }
  }
  else
  {
    Serial.println("Đồng bộ thời gian từ API thành công!");
  }

  server.collectHeaders(headerKeys, headerKeysCount);

  server.on("/auth/login", HTTP_POST, handleLogin);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/temperature", HTTP_GET, handleGetTemperature);
  server.on("/humidity", HTTP_GET, handleGetHumidity);
  server.on("/co2", HTTP_GET, handleGetCO2);
  server.on("/lux", HTTP_GET, handleGetLux);
  server.on("/setup", HTTP_GET, handleGetConfig);
  server.on("/telemetry", HTTP_GET, handleTelemetry);

  server.on("/auth/login", HTTP_OPTIONS, handleCORS);
  server.on("/control", HTTP_OPTIONS, handleCORS);
  server.on("/temperature", HTTP_OPTIONS, handleCORS);
  server.on("/humidity", HTTP_OPTIONS, handleCORS);
  server.on("/co2", HTTP_OPTIONS, handleCORS);
  server.on("/lux", HTTP_OPTIONS, handleCORS);
  server.on("/setup", HTTP_OPTIONS, handleCORS);
  server.on("/telemetry", HTTP_OPTIONS, handleCORS);

  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("========================================");
  Serial.println("[SERVER STARTED] Listening on port 8080");
  Serial.println("========================================");

  // Initialize IRsend from config
  if (deserializeJson(configDoc, config) == DeserializationError::Ok)
  {
    JsonArrayConst devicesArray = configDoc["devices"].as<JsonArrayConst>();
    int irLedPin = 18; // Default fallback

    for (JsonObjectConst device : devicesArray)
    {
      String category = device["category"].as<String>();
      if (category == "AIR_CONDITION" && device.containsKey("gpioPin"))
      {
        irLedPin = device["gpioPin"][0].as<int>();
        Serial.printf("IR LED pin từ config: %d\n", irLedPin);
        break;
      }
    }

    irsend = new IRsend(irLedPin);
    irsend->begin();
    Serial.printf("IRsend đã khởi tạo với GPIO pin %d\n", irLedPin);
  }
  else
  {
    Serial.println("Cảnh báo: Không đọc được config, sử dụng IR LED pin mặc định 18");
    irsend = new IRsend(18);
    irsend->begin();
  }

  digitalWrite(ledPin, HIGH);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, config);
  if (error)
  {
    Serial.println("Lỗi khi parse JSON cấu hình thiết bị!");
    return;
  }

  reinitializeRelayPins();

  // Init sensors from config
  initAllSensors();
  
  // Start SCD40 background task
  startSCD40BackgroundTask();
}

void loop()
{
  server.handleClient();
  delay(1); // Avoid WDT timeout
}