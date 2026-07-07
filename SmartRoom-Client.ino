#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <CustomJWT.h>
#include "DHT.h"
#include "time.h"
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <Preferences.h>

Preferences preferences;
const char *PREF_NAMESPACE = "Configs";
const char *JSON_KEY = "config_json";

String config = R"rawliteral({
  "roomCode": "R-VU",
  "devices": [
    {
      "naturalId": "LABAC1",
      "category": "AIR_CONDITION",
      "controlType": "GPIO",
      "specificType": "IR_SEND",
      "gpioPin": [18],
      "translations": {
        "en": {
          "name": "Lab AC 1"
        },
        "vi": {
          "name": "Máy lạnh phòng lab 1"
        }
      },
      "internal": {
        "peripheralType": "IR_SENDER",
        "brand": "LG",
        "codeConfigs": {
          "power": {
            "ON": "0x8800F43",
            "OFF": "0x88C0051"
          },
          "mode": {
            "COOL": "0x8808F4B",
            "HEAT": "0x880C556",
            "DRY": "0x880910A",
            "FAN": "0x880A30D",
            "AUTO": "0x880B151"
          },
          "speed": {
            "1": "0x880A30D",
            "2": "0x880A32F",
            "3": "0x880A341"
          },
          "temperature": {
            "16": "0x880A14F",
            "17": "0x880A163",
            "18": "0x880A177",
            "19": "0x880A18B",
            "20": "0x880A19F",
            "21": "0x880A1B3",
            "22": "0x880A1C7",
            "23": "0x880A1DB",
            "24": "0x880A1EF",
            "25": "0x880A203",
            "26": "0x880A217",
            "27": "0x880A22B",
            "28": "0x880A23F",
            "29": "0x880A253",
            "30": "0x880A267"
          },
          "swing": {
            "ON": "0x8810001"
          }
        }
      }
    },
        {
            "naturalId": "LIGHT_01",
            "category": "LIGHT",
            "translations": {
                "vi": {
                    "name": "Đèn A101 1",
                    "description": "Đèn số 01 của phòng lab A101"
                },
                "en": {
                    "name": "Light A101 1",
                    "description": "Light 01 of Lab A101"
                }
            },
            "specificType": "GPIO",
            "controlType": "GPIO",
            "gpioPin": [
                13
            ]
            ,
            "internal": {
              "peripheralType": "RELAY"
            }
        },
        {
            "naturalId": "Fan_01",
            "category": "FAN",
            "translations": {
                "vi": {
                    "name": "Quạt A101 1",
                    "description": "Quạt số 01 của phòng lab A101"
                },
                "en": {
                    "name": "Fan A101 1",
                    "description": "Fan 01 of Lab A101"
                }
            },
            "specificType": "GPIO",
            "controlType": "GPIO",
            "gpioPin": [
                14,
                27,
                26
            ],
            "internal": {
              "peripheralType": "RELAY"
            }
        }
    ]
})rawliteral";

int activeRelayPins[13];
int activeRelayCount = 0;

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const int ledPin = 2;
const uint16_t kIrLedPin = 18;
IRsend irsend(kIrLedPin);

char secret_key[] = "Vudeptrai@123";
CustomJWT jwt(secret_key, 256);

const char *ssid = "A101CNTT";
const char *password = "fit@123456789";
IPAddress local_IP(172, 16, 64, 200);
IPAddress gateway(172, 16, 0, 1);
IPAddress subnet(255, 255, 0, 0);

// const char *ssid = "ThanhLoi";
// const char *password = "bichloi123";
// IPAddress local_IP(192, 168, 1, 200);
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);


IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

WebServer server(8080);

String serverUsername = "vuesp";
String serverPassword = "123456789";

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
float getTemperature();
void controlRelay(int gpioPin, bool state);
unsigned long getEpochTime();
bool validateJWT(String tokenStr);
bool requireBearerToken(String &token);
bool parsePlainBody(JsonDocument &bodyDoc);
DeviceMatch findLightingDevice(const String &naturalId);
DeviceMatch findFanDevice(const String &naturalId);
void handleNotFound();
void handleGetTemperature();
void handleGetConfig();
void handleUpdateConfig();
void handleControl();
void handleLogin();
void handleDebugClearConfig();

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
  preferences.begin(PREF_NAMESPACE, true);
  if (!preferences.isKey(JSON_KEY))
  {
    Serial.println("Không tìm thấy JSON cấu hình thiết bị trong bộ nhớ...");
    Serial.println("Sử dụng cấu hình mặc định từ code.");
  }
  else
  {
    config = preferences.getString(JSON_KEY, "{}");
    Serial.println("Đã tải JSON cấu hình từ bộ nhớ!");
    Serial.printf("Kích thước: %d bytes\n", config.length());
  }
  preferences.end();
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
  irsend.sendLG(code, bits);
  return true;
}

bool sendSamsung36IrCode(uint32_t code, uint16_t bits)
{
  // Samsung uses 36-bit encoding, we send the code as 36 bits
  irsend.sendSamsung36(code, 36);
  return true;
}

bool sendPanasonicIrCode(uint32_t code, uint16_t bits)
{
  irsend.sendPanasonic(code, bits);
  return true;
}

//map - Thêm hãng mới ở đây
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

void clearConfiguration()
{
  preferences.begin(PREF_NAMESPACE, false);
  preferences.remove(JSON_KEY);
  Serial.println("-> Đã xóa dữ liệu cấu hình trong bộ nhớ!");
  preferences.end();
}

int UpdateConfigJSON(String newConfig)
{
  if (newConfig.length() == 0 || newConfig == "null")
  {
    Serial.println("-> Lỗi: newConfig rỗng hoặc không hợp lệ!");
    return 0;
  }

  preferences.begin(PREF_NAMESPACE, false);
  delay(10);

  size_t sch = preferences.putString(JSON_KEY, newConfig);
  if (sch > 0)
  {
    Serial.println("-> Đã lưu cấu hình mới vào bộ nhớ thành công!");
    config = newConfig;
    Serial.printf("   Kích thước: %d bytes\n", sch);
    reinitializeRelayPins();
  }
  else
  {
    Serial.println("-> Lỗi: Không thể lưu vào bộ nhớ!");
  }

  preferences.end();
  return sch;
}

void reinitializeRelayPins()
{
  for (int i = 0; i < activeRelayCount; i++)
  {
    digitalWrite(activeRelayPins[i], LOW);
    Serial.printf("Reset RELAY cũ GPIO %d\n", activeRelayPins[i]);
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
      digitalWrite(gpioPin, LOW);
      activeRelayPins[activeRelayCount++] = gpioPin;
      Serial.printf("Re-init RELAY mới GPIO %d\n", gpioPin);
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

float getTemperature()
{
  return dht.readTemperature();
}

void controlRelay(int gpioPin, bool state)
{
  Serial.printf("[GPIO WRITE] Pin %d <- %d (%s)\n", gpioPin, state ? HIGH : LOW, state ? "HIGH" : "LOW");
  digitalWrite(gpioPin, state ? HIGH : LOW);
  Serial.printf("[GPIO READ] Pin %d = %d\n", gpioPin, digitalRead(gpioPin));
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
  sendJson(404, "Không tìm thấy");
}

void handleGetTemperature()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/get/temperature");

  String token;
  if (!requireBearerToken(token))
  {
    Serial.println("------------------------------------------");
    return;
  }

  JsonDocument data;
  data["temperature"] = getTemperature();
  sendJsonWithData(200, "Lấy nhiệt độ thành công", data);
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

void handleUpdateConfig()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/config");

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

  JsonDocument currentConfigDoc;
  bool hasCurrentConfig = loadConfigDocument(currentConfigDoc);

  String newConfig;

  // Log request details
  Serial.println("[CONFIG UPDATE REQUEST]");

  // Check if request has the new format { roomCode, devices }
  if (jsonBody.containsKey("roomCode") && jsonBody.containsKey("devices"))
  {
    // New format - save entire object with roomCode and devices
    Serial.println("[FORMAT] New format with roomCode");
    serializeJson(jsonBody, newConfig);
  }
  else if (jsonBody.containsKey("devices"))
  {
    // Old format with just devices array - preserve existing roomCode when possible
    Serial.println("[FORMAT] Legacy format (devices only)");
    JsonDocument wrappedConfig;
    if (jsonBody.containsKey("roomCode") && !jsonBody["roomCode"].isNull())
    {
      wrappedConfig["roomCode"] = jsonBody["roomCode"].as<String>();
    }
    else if (hasCurrentConfig && currentConfigDoc.containsKey("roomCode"))
    {
      wrappedConfig["roomCode"] = currentConfigDoc["roomCode"].as<String>();
    }
    wrappedConfig["devices"] = jsonBody["devices"];
    serializeJson(wrappedConfig, newConfig);
  }

  if (newConfig.length() > 0 && newConfig != "null")
  {
    Serial.printf("Dữ liệu nhận được: %s\n", newConfig.c_str());
    UpdateConfigJSON(newConfig);
    Serial.println("[CONFIG UPDATE SUCCESS]");
    sendJson(200, "Cập nhật JSON cấu hình thiết bị thành công");
  }
  else
  {
    Serial.println("[CONFIG UPDATE FAILED] Missing roomCode or devices");
    sendJson(400, "Thiếu trường roomCode hoặc devices");
  }

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

void handleDebugClearConfig()
{
  sendCORSHeaders();
  Serial.println("------------------------------------------");
  Serial.println("Endpoint:/debug/clear-config");
  
  clearConfiguration();
  
  sendJson(200, "Đã xóa config từ preferences, sẽ sử dụng config mặc định");
  Serial.println("------------------------------------------");
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
    delay(500);
    Serial.print(".");
    timeout_counter++;

    if (timeout_counter > 10)
    {
      Serial.println("\nKết nối quá thời gian! Đang khởi động lại...");
      ESP.restart();
    }
  }

  Serial.println("\nKết nối WiFi thành công!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

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
    Serial.println("\n[LỖI BẢO MẬT] Không thể đồng bộ thời gian từ NTP!");
    Serial.println("Để đảm bảo an toàn, hệ thống sẽ tự động khởi động lại sau 1 giây...");
    delay(1000);
    ESP.restart();
  }
  else
  {
    Serial.println("\nĐồng bộ thời gian thành công!");
  }

  CheckConfigJSON();
  server.collectHeaders(headerKeys, headerKeysCount);

  server.on("/auth/login", HTTP_POST, handleLogin);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/get/temperature", HTTP_POST, handleGetTemperature);
  server.on("/config", HTTP_PATCH, handleUpdateConfig);
  server.on("/setup", HTTP_GET, handleGetConfig);
  server.on("/debug/clear-config", HTTP_GET, handleDebugClearConfig);

  server.on("/auth/login", HTTP_OPTIONS, handleCORS);
  server.on("/control", HTTP_OPTIONS, handleCORS);
  server.on("/get/temperature", HTTP_OPTIONS, handleCORS);
  server.on("/config", HTTP_OPTIONS, handleCORS);
  server.on("/setup", HTTP_OPTIONS, handleCORS);
  server.on("/debug/clear-config", HTTP_OPTIONS, handleCORS);

  server.onNotFound(handleNotFound);
  server.begin();
  irsend.begin();
  digitalWrite(ledPin, HIGH);
  dht.begin();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, config);
  if (error)
  {
    Serial.println("Lỗi khi parse JSON cấu hình thiết bị!");
    return;
  }

  reinitializeRelayPins();
}

void loop()
{
  server.handleClient();
}