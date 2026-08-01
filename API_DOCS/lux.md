# LUX Endpoint

## Overview

Get light intensity (illuminance) data from a light sensor device configured in the system. This endpoint retrieves real-time light level measurements in lux (lx) units from BH1750FVI or compatible sensors.

## Request

GET /lux?naturalId={naturalId}

### Headers

- Authorization: Bearer {token}
- Content-Type: application/json

### Query Parameters

- naturalId (string, required): Unique identifier of the light sensor device in the configuration.

## Response

### Success Response (200 OK)

```json
{
  "code": 200,
  "message": "Lay anh sang thanh cong",
  "data": {
    "lux": "350.50"
  }
}
```

### Error Responses

400 Bad Request

```json
{
  "code": 400,
  "message": "Query parameter bat buoc phai co: ?naturalId=..."
}
```

Cause: Missing or invalid naturalId parameter.

404 Not Found

```json
{
  "code": 404,
  "message": "Khong tim thay cam bien anh sang co naturalId tuong ung"
}
```

Cause: Device with the specified naturalId does not exist in the configuration, or it is not a light sensor.

500 Internal Server Error

```json
{
  "code": 500,
  "message": "Loi: Khong the ket noi den cam bien BH1750FVI"
}
```

Cause: Failed to initialize or connect to the BH1750FVI sensor on I2C bus.

```json
{
  "code": 500,
  "message": "Loi: Khong the doc du lieu tu cam bien BH1750FVI"
}
```

Cause: Failed to read light level data from the sensor.

```json
{
  "code": 500,
  "message": "Loi: Cam bien thieu thong tin module"
}
```

Cause: Device configuration is missing module information.

```json
{
  "code": 501,
  "message": "Module cam bien khong ho tro do anh sang"
}
```

Cause: The sensor module type does not support light intensity measurement.

## Implementation Details

- Communicates with BH1750FVI sensor via I2C protocol
- Sensor operates in continuous high-resolution mode (1 lux resolution)
- Response includes light intensity in lux (lx) units
- Range typically 1 - 65535 lux depending on sensor variant

## Example

Request:

```bash
curl -X GET "http://esp32-ip:8080/lux?naturalId=ESP32_LIGHT_01" \
  -H "Authorization: Bearer your_token" \
  -H "Content-Type: application/json"
```

Response:

```json
{
  "code": 200,
  "message": "Lay anh sang thanh cong",
  "data": {
    "lux": "425.75"
  }
}
```

## Notes

- The light sensor must be configured with category "SENSOR_LUX" in the device configuration
- BH1750FVI sensor requires 2 GPIO pins configured for I2C (SDA, SCL)
- I2C address for BH1750FVI is 0x23 (fixed)
- Sensor automatically adjusts sensitivity based on light conditions
- For accurate measurements, ensure proper light exposure to the sensor
