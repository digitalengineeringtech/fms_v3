# FMS Framework

A modular firmware framework for fuel management systems running on ESP32 platforms.



## File Structure

```
fms_v3/
├── main/
│   ├── src/
│   │   ├── _fms_filemanager.h
│   │   ├── _fms_filemanager.cpp
│   │   ├── _fms_debug.h
│   │   ├── _fms_debug.cpp
│   │   └── _fms_cli.h
│   ├── data/
│   │   ├── index.html
│   │   ├── login.html
│   │   └── script.js
│   ├── main.ino
│   └── main.h
└── README.md
```

## Building and Flashing

1. Install Arduino IDE and ESP32 board support
2. Install required libraries:
   - ArduinoJson
   - PubSubClient
   - ModbusMaster
3. Select your ESP32 board in Arduino IDE
4. Upload the sketch

## Storage

- LittleFS: Used for web interface files
- SD Card: Configuration and data storage
- NVS: System settings and preferences

## Network Services

- OTA Update server on port 80
- MQTT client for remote management
- mDNS for device discovery

## Logging

Multiple log levels available:
- ERROR
- WARNING
- INFO
- DEBUG
- VERBOSE
- TASK

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a new Pull Request

## License

Copyright © 2025 Thailand. All rights reserved.
