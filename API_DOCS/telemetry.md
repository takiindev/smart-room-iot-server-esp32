# Telemetry API

## GET `/telemetry` - Get sensor data from all configured devices

This endpoint retrieves telemetry data from all configured sensors including temperature, humidity, CO2, and light intensity.

### Request

```bash
curl -X GET "http://172.16.64.200:8080/telemetry" \
  -H "Authorization: Bearer <your_jwt_token>"
```

### Response

**Success (200 OK):**
```json
{
  "code": 200,
  "message": "Success",
  "data": {
    "roomCode": "R-VU",
    "devices": [
      {
        "naturalId": "TEMP_ESP32_01",
        "category": "TEMPERATURE",
        "tempC": "25.500"
      },
      {
        "naturalId": "TEMP_ESP32_02",
        "category": "TEMPERATURE",
        "tempC": "26.300"
      },
      {
        "naturalId": "HUM_ESP32_01",
        "category": "HUMIDITY",
        "humidity": "45.20"
      },
      {
        "naturalId": "ESP32_CO2_01",
        "category": "SENSOR_CO2",
        "co2": "425"
      },
      {
        "naturalId": "ESP32_LIGHT_01",
        "category": "SENSOR_LUX",
        "lux": "350.50"
      }
    ]
  },
  "timestamp": "2026-07-15T10:30:45Z"
}
```

**Error (401 Unauthorized):**
```json
{
  "code": 401,
  "message": "Token expired or invalid",
  "timestamp": "2026-07-15T10:30:45Z"
}
```

### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| roomCode | string | Room code from config |
| devices | array | Array of sensor readings |
| devices[].naturalId | string | Unique device identifier |
| devices[].category | string | Sensor type (TEMPERATURE, HUMIDITY, SENSOR_CO2, SENSOR_LUX) |
| devices[].tempC | string | Temperature value in Celsius (TEMPERATURE only) |
| devices[].humidity | string | Humidity value in percentage (HUMIDITY only) |
| devices[].co2 | string | CO2 concentration in ppm (SENSOR_CO2 only) |
| devices[].lux | string | Light intensity in lux (SENSOR_LUX only) |

### Supported Sensors

- TEMPERATURE: DS18B20, SCD40, SHT40
- HUMIDITY: SCD40, SHT40
- SENSOR_CO2: SCD40
- SENSOR_LUX: BH1750FVI

### Notes

- Endpoint scans all devices in config by sensor category
- Devices that fail to read are skipped (not included in response)
- SCD40 data comes from background cache task (no blocking wait)
- LUX sensor initializes fresh on each request
- Bearer token is required in Authorization header

### Usage Examples

**JavaScript/Fetch:**
```javascript
const response = await fetch('http://172.16.64.200:8080/telemetry', {
  method: 'GET',
  headers: {
    'Authorization': `Bearer ${token}`
  }
});

const data = await response.json();
console.log('Room:', data.data.roomCode);
data.data.devices.forEach(device => {
  console.log(`${device.naturalId} (${device.category}):`, device);
});
```

**Python:**
```python
import requests

headers = {
    'Authorization': f'Bearer {token}'
}

response = requests.get(
    'http://172.16.64.200:8080/telemetry',
    headers=headers
)

data = response.json()
if response.status_code == 200:
    print(f"Room: {data['data']['roomCode']}")
    for device in data['data']['devices']:
        print(f"{device['naturalId']}: {device}")
else:
    print(f"Error: {data['message']}")
```

### Error Handling

- 401: Invalid or expired token
- 500: JSON parse error or configuration issue
- Device read failures are logged but do not cause endpoint failure
