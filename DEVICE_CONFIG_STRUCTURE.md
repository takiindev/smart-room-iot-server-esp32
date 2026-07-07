# Cấu trúc Device Config - Tham khảo nhanh

Tài liệu này mô tả chi tiết cấu trúc config cho các loại thiết bị được hỗ trợ.

---

## Cấu trúc chung

```json
{
  "roomCode": "R-VU",
  "devices": [ /* mảng các device objects */ ]
}
```

---

## Device Type 1: RELAY (Đèn, Quạt)

### LIGHT - Thiết bị đèn

```json
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
  "gpioPin": [13],
  "internal": {
    "peripheralType": "RELAY"
  }
}
```

**Giải thích fields:**
- `naturalId`: ID duy nhất của thiết bị (string, bắt buộc)
- `category`: Loại thiết bị `"LIGHT"` (bắt buộc)
- `translations`: Object chứa tên và mô tả đa ngôn ngữ
- `specificType`: `"GPIO"` cho relay
- `controlType`: `"GPIO"` cho relay
- `gpioPin`: Mảng số pin GPIO (relay đèn dùng 1 pin)
- `internal.peripheralType`: `"RELAY"` (bắt buộc)

### FAN - Thiết bị quạt

```json
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
  "gpioPin": [14, 27, 26],
  "internal": {
    "peripheralType": "RELAY"
  }
}
```

**Giải thích fields:**
- `category`: `"FAN"` (bắt buộc)
- `gpioPin`: Mảng 1-3 pins (mỗi pin = 1 tốc độ quạt)
  - 1 pin: chỉ có ON/OFF
  - 2 pins: 2 tốc độ
  - 3 pins: 3 tốc độ
- Các fields khác tương tự LIGHT

---

## Device Type 2: IR_SENDER (Điều hòa)

### AIR_CONDITION - Thiết bị điều hòa

```json
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
}
```

**Giải thích fields:**
- `category`: `"AIR_CONDITION"` (bắt buộc)
- `specificType`: `"IR_SEND"` (bắt buộc)
- `gpioPin`: Mảng 1 pin (pin kết nối IR LED sender)
- `internal.peripheralType`: `"IR_SENDER"` (bắt buộc)
- `internal.brand`: Hãng AC (bắt buộc)
  - Các hãng được hỗ trợ: `"LG"`, `"SAMSUNG"`, `"PANASONIC"`
- `internal.codeConfigs`: Object chứa tất cả mã IR (bắt buộc)

### Cấu trúc `codeConfigs`

#### Section: `power`
```json
"power": {
  "ON": "0x8800F43",
  "OFF": "0x88C0051"
}
```
- `ON`: Mã bật AC (bắt buộc)
- `OFF`: Mã tắt AC (bắt buộc)

#### Section: `mode`
```json
"mode": {
  "COOL": "0x8808F4B",
  "HEAT": "0x880C556",
  "DRY": "0x880910A",
  "FAN": "0x880A30D",
  "AUTO": "0x880B151"
}
```
- `COOL`: Chế độ làm lạnh (bắt buộc)
- `HEAT`: Chế độ sưởi ấm (tùy chọn)
- `DRY`: Chế độ hút ẩm (tùy chọn)
- `FAN`: Chế độ quạt (tùy chọn)
- `AUTO`: Chế độ tự động (tùy chọn)

#### Section: `speed`
```json
"speed": {
  "1": "0x880A30D",
  "2": "0x880A32F",
  "3": "0x880A341"
}
```
- Keys: `"1"`, `"2"`, `"3"`, `"4"`, `"5"` (tùy theo AC hỗ trợ bao nhiêu tốc độ)
- Thường LG có 3 tốc độ, Samsung có 5 tốc độ

#### Section: `temperature`
```json
"temperature": {
  "16": "0x880A14F",
  "17": "0x880A163",
  ...
  "30": "0x880A267"
}
```
- Keys: từ `"16"` đến `"30"` (16°C - 30°C)
- Mỗi nhiệt độ phải có mã IR tương ứng

#### Section: `swing`
```json
"swing": {
  "ON": "0x8810001"
}
```
hoặc
```json
"swing": {
  "ON": "0x8810001",
  "OFF": "0x8810002"
}
```
- `ON`: Bật lưới tượng (bắt buộc)
- `OFF`: Tắt lưới tượng (tùy chọn, một số AC chỉ có toggle)

---

## Lưu ý quan trọng

### Về mã IR hex

1. **Format mã IR:**
   - Phải bắt đầu với `0x`
   - Theo sau là các ký tự hex (0-9, A-F)
   - Ví dụ: `"0x8800F43"` ✅, `"8800F43"` ❌

2. **Số bit (bits field):**
   - **KHÔNG CẦN** field `bits` trong config
   - Firmware tự động xác định số bit theo brand:
     - LG: 28 bits
     - SAMSUNG: 36 bits
     - PANASONIC: 48 bits

3. **Tìm mã IR:**
   - Sử dụng remote IR analyzer/scanner
   - Hoặc tham khảo database IR codes trên internet
   - Hoặc dùng thư viện IRremoteESP8266 examples để scan

### Về validation

1. **Device phải có đầy đủ:**
   - `naturalId` (unique string)
   - `category` (LIGHT, FAN, AIR_CONDITION)
   - `gpioPin` (array of integers)
   - `internal` object
   - `internal.peripheralType` (RELAY hoặc IR_SENDER)

2. **IR_SENDER phải có thêm:**
   - `internal.brand` (string, hoa hoặc thường đều được)
   - `internal.codeConfigs` (object)
   - Các sections trong `codeConfigs`: power, mode, speed, temperature, swing

3. **Nếu thiếu field nào:**
   - Firmware sẽ log lỗi ra Serial Monitor
   - API sẽ trả về status 500 với message cụ thể
   - Ví dụ: `"Lỗi: Thiếu mã config cho devices[LABAC1].internal.codeConfigs.temperature.16"`

---

## Template config hoàn chỉnh

```json
{
  "roomCode": "R-VU",
  "devices": [
    {
      "naturalId": "LABAC1",
      "category": "AIR_CONDITION",
      "controlType": "GPIO",
      "specificType": "IR_SEND",
      "gpioPin": [18],
      "translations": {
        "en": { "name": "Lab AC 1" },
        "vi": { "name": "Máy lạnh phòng lab 1" }
      },
      "internal": {
        "peripheralType": "IR_SENDER",
        "brand": "LG",
        "codeConfigs": {
          "power": { "ON": "0x8800F43", "OFF": "0x88C0051" },
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
            "16": "0x880A14F", "17": "0x880A163", "18": "0x880A177",
            "19": "0x880A18B", "20": "0x880A19F", "21": "0x880A1B3",
            "22": "0x880A1C7", "23": "0x880A1DB", "24": "0x880A1EF",
            "25": "0x880A203", "26": "0x880A217", "27": "0x880A22B",
            "28": "0x880A23F", "29": "0x880A253", "30": "0x880A267"
          },
          "swing": { "ON": "0x8810001" }
        }
      }
    },
    {
      "naturalId": "LIGHT_01",
      "category": "LIGHT",
      "translations": {
        "vi": { "name": "Đèn A101 1", "description": "Đèn số 01 của phòng lab A101" },
        "en": { "name": "Light A101 1", "description": "Light 01 of Lab A101" }
      },
      "specificType": "GPIO",
      "controlType": "GPIO",
      "gpioPin": [13],
      "internal": { "peripheralType": "RELAY" }
    },
    {
      "naturalId": "Fan_01",
      "category": "FAN",
      "translations": {
        "vi": { "name": "Quạt A101 1", "description": "Quạt số 01 của phòng lab A101" },
        "en": { "name": "Fan A101 1", "description": "Fan 01 of Lab A101" }
      },
      "specificType": "GPIO",
      "controlType": "GPIO",
      "gpioPin": [14, 27, 26],
      "internal": { "peripheralType": "RELAY" }
    }
  ]
}
```

---

## Mở rộng - Thêm hãng AC mới

### Bước 1: Viết hàm gửi IR

Thêm vào file `.ino`:

```cpp
bool sendDaikinIrCode(uint32_t code, uint16_t bits)
{
  irsend.sendDaikin(code, bits);
  return true;
}
```

### Bước 2: Đăng ký brand

Thêm vào array `kBrandIrSenders`:

```cpp
const BrandIrSender kBrandIrSenders[] = {
    {"LG", sendLgIrCode},
    {"SAMSUNG", sendSamsung36IrCode},
    {"PANASONIC", sendPanasonicIrCode},
    {"DAIKIN", sendDaikinIrCode},  // Thêm dòng này
};
```

### Bước 3: Sử dụng trong config

```json
{
  "naturalId": "OFFICE_AC",
  "category": "AIR_CONDITION",
  "...": "...",
  "internal": {
    "peripheralType": "IR_SENDER",
    "brand": "DAIKIN",
    "codeConfigs": { /* mã IR của Daikin */ }
  }
}
```

---

**Xem thêm:**
- [README.md](README.md) - Tổng quan hệ thống
- [config.md](config.md) - API lấy và cập nhật config
- [control.md](control.md) - API điều khiển thiết bị
- [auth.md](auth.md) - API đăng nhập
