# Sensor Initialization Strategy

## Lý do thay đổi từ Lazy Init → Early Init

### ❌ Cũ: Lazy Initialization (Init khi API được gọi)

```
Boot
  ↓
Server Start
  ↓
API Call /humidity?naturalId=HUM_ESP32_01
  ↓
Check if (!isScd40Initialized)
  ↓
Wire.begin() + scd4x.begin() + startPeriodicMeasurement()
  ↓
⏳ WAIT 5 GIÂY
  ↓
Response (lâu!)
```

**Vấn đề:**
- 🔴 API call lần đầu timeout (5 giây chờ)
- 🔴 Client thinks server bị hang
- 🔴 Không thích hợp cho client timeout < 5s

### ✅ Mới: Early Initialization (Init trong setup())

```
Boot
  ↓
reinitializeRelayPins()
  ↓
initAllSensors() ← Đọc config + init từng sensor
  └─ Scan devices array
  └─ Tìm SCD40 devices
  └─ readSCD40Data() (init + 5s wait)
  └─ Xong trước khi server start
  ↓
Server Start
  ↓
API Call /humidity?naturalId=HUM_ESP32_01 ← Ready ngay! ✅
```

**Lợi ích:**
- ✅ Init xong trước server start
- ✅ API calls sẵn sàng từ lần đầu
- ✅ Không timeout
- ✅ Theo config (dynamic)

## Implementation Details

### 1. Hàm initAllSensors()

```cpp
void initAllSensors()
{
  // 1. Parse config JSON
  // 2. Iterate qua devices array
  // 3. Tìm devices có module="SCD40"
  // 4. Extract GPIO pins từ config
  // 5. Call readSCD40Data() để init
  // 6. Log kết quả
}
```

**Flow:**
```
FOR EACH device IN config.devices:
  IF device.internal.module == "SCD40":
    GET sdaPin = device.gpioPin[0]
    GET sclPin = device.gpioPin[1]
    readSCD40Data(sdaPin, sclPin)  ← Init + 5s wait
    Log result
```

### 2. Gọi trong setup()

```cpp
void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  // ... other init ...
  
  reinitializeRelayPins();
  initAllSensors();  ← ĐÂY
  
  // Server.begin() sau
  server.begin();
}
```

### 3. readSCD40Data() không thay đổi

- Vẫn check `isScd40Initialized` flag
- Vẫn khởi tạo 1 lần duy nhất
- Vẫn delay 5 giây lần đầu

## Serial Log Output

### Boot sequence:

```
13:45:00.123 -> Đang kết nối đến WiFi: A101CNTT
13:45:01.456 -> Kết nối WiFi thành công!
13:45:01.456 -> IP: 172.16.64.200
13:45:02.000 -> [TIME SYNC SUCCESS] ...
13:45:02.100 -> Re-init RELAY mới GPIO 13 -> HIGH
13:45:02.100 -> Re-init RELAY mới GPIO 14 -> HIGH

========================================
[SENSOR INITIALIZATION] Bắt đầu...
========================================
[SENSOR INIT] Found SCD40: HUM_ESP32_01 (SDA=21, SCL=22)
[SENSOR INIT] Initializing SCD40...
[SCD40] Serial Number: 0xC962B5073B26
[SCD40] Khởi tạo thành công lần đầu!
[SCD40] Chờ dữ liệu sẵn sàng (5 giây)...
[SCD40] I2C (SDA=21, SCL=22) -> CO2: 420 ppm, Temp: 25.50°C, Humidity: 45.20%RH
[SENSOR INIT] SCD40 initialized successfully!
[SENSOR INIT] First read: Temp=25.50°C, Humidity=45.20%
========================================
[SENSOR INITIALIZATION] Hoàn tất
========================================

========================================
[SERVER STARTED] Listening on port 8080
========================================
```

## API Call Flow After Init

### Lần 1 (lần đầu sau boot):
```
GET /humidity?naturalId=HUM_ESP32_01
  ↓
isScd40Initialized == true (từ setup())
  ↓
readSCD40Data()
  └─ getDataReadyStatus() ✅ (đã warm up)
  └─ readMeasurement() ✅
  ↓
Response 200 ← Nhanh! Không chờ
```

### Lần 2+:
```
GET /humidity?naturalId=HUM_ESP32_01
  ↓
isScd40Initialized == true
  ↓
readSCD40Data()
  └─ getDataReadyStatus() ✅
  └─ readMeasurement() ✅
  ↓
Response 200 ← Nhanh như cũ
```

## Configuration-Based

### Nếu config không có SCD40:

```json
{
  "devices": [
    {
      "naturalId": "TEMP_ESP32_01",
      "internal": {"module": "DS18B20"}
    }
  ]
}
```

**Result:**
```
[SENSOR INIT] Found SCD40: (không có)
[SENSOR INIT] Hoàn tất
```
→ Không init SCD40 (vì không có trong config)

### Nếu config có 2 SCD40:

Code sẽ init cái đầu tiên tìm được (vì một board ESP32 chỉ có 1 SCD40):
```
[SENSOR INIT] Found SCD40: HUM_ESP32_01 (SDA=21, SCL=22)
[SENSOR INIT] Found SCD40: HUM_ESP32_02 (SDA=21, SCL=22)
  ↓ (vì `break` sau cái đầu)
[SENSOR INIT] Initializing only HUM_ESP32_01
```

## Performance Impact

**Boot Time:**
- Trước: ~2 giây (skip init)
- Sau: ~7 giây (init + 5s wait)

**Trade-off:**
- ⏱️ Boot 5 giây lâu hơn
- ⚡ API calls sẵn sàng → Không timeout

## Troubleshooting

### SCD40 Init Failed:

```
[SENSOR INIT] SCD40 initialization failed!
[SENSOR INIT] Check hardware connections and I2C pins
```

**Fix:**
1. Kiểm tra GPIO pins: SDA=21, SCL=22
2. Kiểm tra Pull-up resistors 4.7kΩ
3. Kiểm tra VCC 3.3V + GND
4. Kiểm tra I2C address 0x62 (xem I2C Scanner)

### Cần skip init (không có hardware):

Sửa config để bỏ SCD40 device:
```json
{
  "devices": [
    // Không thêm SCD40 device nào
  ]
}
```

Hoặc comment out SCD40 init:
```cpp
// if (hasSCD40 && !isScd40Initialized)
// {
//   ...
// }
```

## Summary

| Aspect | Cũ (Lazy) | Mới (Early) |
|--------|-----------|------------|
| Init timing | Khi API call | Trong setup() |
| Boot time | ~2s | ~7s |
| API call 1 | 5s timeout ❌ | Nhanh ✅ |
| API call 2+ | Nhanh ✅ | Nhanh ✅ |
| Config based | Vẫn là ✅ | Tốt hơn ✅ |
| Scalable | Khó nếu nhiều sensor | Dễ (loop) ✅ |

→ **Early init tốt hơn cho production!**
