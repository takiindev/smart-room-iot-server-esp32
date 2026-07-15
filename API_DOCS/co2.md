# CO2 Endpoint

## Overview

Get CO2 concentration data from a CO2 sensor device configured in the system. This endpoint retrieves real-time CO2 measurement data cached from the background sensor reading task.

## Request

GET /co2?naturalId={naturalId}

### Headers

- Authorization: Bearer {token}
- Content-Type: application/json

### Query Parameters

- naturalId (string, required): Unique identifier of the CO2 sensor device in the configuration.

## Response

### Success Response (200 OK)

```json
{
  "code": 200,
  "message": "Lay CO2 thanh cong",
  "data": {
    "co2": 425
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
  "message": "Khong tim thay cam bien CO2 co naturalId tuong ung"
}
```

Cause: Device with the specified naturalId does not exist in the configuration, or it is not a CO2 sensor.

500 Internal Server Error

```json
{
  "code": 500,
  "message": "Loi: Du lieu SCD40 chua san sang, vui long cho"
}
```

Cause: The CO2 sensor data is not yet ready. This typically happens after device startup when the background task has not yet read initial sensor data.

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
  "message": "Module cam bien khong ho tro do CO2"
}
```

Cause: The sensor module type does not support CO2 measurement.

## Implementation Details

- CO2 data is fetched from cached values maintained by the SCD40 background task
- Cached data is updated every 1 second from the background task running on FreeRTOS core 1
- The endpoint returns cached data immediately without waiting for new measurements
- Response includes CO2 concentration in parts per million (ppm)

## Example

Request:

```bash
curl -X GET "http://esp32-ip:8080/co2?naturalId=SCD40_01" \
  -H "Authorization: Bearer your_token" \
  -H "Content-Type: application/json"
```

Response:

```json
{
  "code": 200,
  "message": "Lay CO2 thanh cong",
  "data": {
    "co2": 450
  }
}
```

## Notes

- The CO2 sensor must be configured with category "CO2" in the device configuration
- SCD40 sensor requires approximately 5 seconds initial warm-up time after device startup
- For best accuracy, ensure the device has proper ventilation
