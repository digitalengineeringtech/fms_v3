
// cli command function
void handle_wifi_command(const std::vector<String>& args) {
  if (args.size() != 2) {
    fms_cli.respond("wifi", "Usage: wifi <ssid> <password>", false);
    return;
  }
  String ssid = args[0];
  String password = args[1];
  // Save to preferences
  fms_nvs_storage.begin("fms_config", false);
  fms_nvs_storage.putString("ssid", ssid);
  fms_nvs_storage.putString("pass", password);
  fms_nvs_storage.end();

  fms_cli.respond("wifi", "WiFi settings updated. SSID: " + ssid);
}

void handle_restart_command(const std::vector<String>& args) {
  fms_cli.respond("restart", "Restarting system...");
  delay(1000);
  ESP.restart();
}

// Alternative WiFi scan implementation that uses even less memory
void handle_wifi_scan_safe_command(const std::vector<String>& args) {
  const int MAX_NETWORKS = 5;
  // Start scan
  fms_cli.respond("wifiscan_safe", "Scanning for networks...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  vTaskDelay(pdMS_TO_TICKS(100));
  // Use synchronous scan with limited time per channel
  int numNetworks = WiFi.scanNetworks(false, true, false, 200);
  if (numNetworks <= 0) {
    fms_cli.respond("wifiscan_safe", numNetworks == 0 ? "No networks found" : "Scan failed");
    return;
  }
  // Limit the number of networks to prevent memory issues
  if (numNetworks > MAX_NETWORKS) {
    numNetworks = MAX_NETWORKS;
  }
  // Send a simplified response with minimal memory usage
  fms_cli.respond("wifiscan_safe", "Found " + String(numNetworks) + " networks");

  // Process each network individually
  for (int i = 0; i < numNetworks; i++) {
    // Send SSID
    Serial.print("Network ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(WiFi.SSID(i));

    // Send RSSI
    Serial.print("  Signal: ");
    Serial.print(WiFi.RSSI(i));
    Serial.println(" dBm");

    // Send Channel
    Serial.print("  Channel: ");
    Serial.println(WiFi.channel(i));

    // Send Encryption
    Serial.print("  Security: ");
    Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "WPA/WPA2");

    Serial.println("---");

    // Give the system time to process
    delay(10);
    yield();
  }

  // Free the memory used by the scan
  WiFi.scanDelete();

  // Final message
  fms_cli.respond("wifiscan_safe", "Scan complete");
}

void handle_wifi_connect_command(const std::vector<String>& args) {
  if (args.size() != 2) {
    fms_cli.respond("wifi_connect", "Usage: wifi_connect <ssid> <password>", false);
    return;
  }

  String ssid = args[0];
  String password = args[1];

  // Disconnect from any current WiFi connection
  WiFi.disconnect();
  delay(100);

  // Set WiFi mode to station
  WiFi.mode(WIFI_STA);

  // Begin connection attempt
  fms_cli.respond("wifi_connect", "Connecting to " + ssid + "...");

  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait for connection with timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Check connection result
  if (WiFi.status() == WL_CONNECTED) {
    // Use individual prints instead of building a large JSON string
    Serial.println("{");
    Serial.print("  \"command\": \"wifi_connect\",");
    Serial.print("  \"result\": \"Connected successfully\",");
    Serial.print("  \"success\": true,");
    Serial.print("  \"ip\": \"");
    Serial.print(WiFi.localIP().toString());
    Serial.print("\",");
    Serial.print("  \"ssid\": \"");
    Serial.print(WiFi.SSID());
    Serial.print("\",");
    Serial.print("  \"rssi\": ");
    Serial.print(WiFi.RSSI());
    Serial.println("}");
  } else {
    // Connection failed, report error code
    String errorMsg;
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        errorMsg = "SSID not available";
        break;
      case WL_CONNECT_FAILED:
        errorMsg = "Connection failed (wrong password?)";
        break;
      case WL_CONNECTION_LOST:
        errorMsg = "Connection lost";
        break;
      case WL_DISCONNECTED:
        errorMsg = "Disconnected";
        break;
      default:
        errorMsg = "Unknown error: " + String(WiFi.status());
    }

    fms_cli.respond("wifi_connect", "Failed to connect: " + errorMsg, false);
  }
}

void handle_wifi_read_command(const std::vector<String>& args) {
  if (WiFi.status() == WL_CONNECTED) {
    // Use individual prints instead of building a large string
    Serial.print("{");
    Serial.print("\"ssid\": \"");
    Serial.print(WiFi.SSID());
    Serial.print("\",");
    Serial.print("  \"rssi\": ");
    Serial.print(WiFi.RSSI());
    Serial.print(",");
    Serial.print("  \"ip\": \"");
    Serial.print(WiFi.localIP().toString());
    Serial.print("\"");
    Serial.println("}");
  } else {
    fms_cli.respond("wifiread", "WiFi not connected", false);
  }
}

// Test command to run a series of tests
void handle_test_command(const std::vector<String>& args) {
  fms_cli.respond("test", "Running fms_cli tests...");

  // Test help command
  Serial.println("\n--- Testing help command ---");
  fms_cli.execute_test_command("help");

  // Test echo command
  Serial.println("\n--- Testing echo command ---");
  fms_cli.execute_test_command("echo on");
  fms_cli.execute_test_command("echo off");
  fms_cli.execute_test_command("echo");

  // Test wifi read command
  Serial.println("\n--- Testing wifiread command ---");
  fms_cli.execute_test_command("wifiread");

  // Test invalid command
  Serial.println("\n--- Testing invalid command ---");
  fms_cli.execute_test_command("invalidcommand");

  fms_cli.respond("test", "Tests completed");
}

// WiFi connection test command
void handle_wifi_test_command(const std::vector<String>& args) {
  fms_cli.respond("wifi_test", "Running WiFi connection tests...");

  // Get stored WiFi credentials
  fms_nvs_storage.begin("fms_config", true);  // Read-only mode
  String ssid = fms_nvs_storage.getString("ssid", "");
  String password = fms_nvs_storage.getString("pass", "");
  fms_nvs_storage.end();

  if (ssid.length() == 0 || password.length() == 0) {
    fms_cli.respond("wifi_test", "No WiFi credentials stored. Use 'wifi <ssid> <password>' first.", false);
    return;
  }

  // Test WiFi connection
  Serial.println("\n--- Testing WiFi connection ---");
  Serial.printf("Connecting to %s...\n", ssid.c_str());

  // Disconnect from any current WiFi connection
  WiFi.disconnect();
  delay(100);

  // Set WiFi mode to station
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait for connection with timeout
  unsigned long startTime = millis();
  bool connected = false;

  while (millis() - startTime < WIFI_TIMEOUT) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (connected) {
    Serial.printf("Connected to %s\n", ssid.c_str());
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal strength (RSSI): %d dBm\n", WiFi.RSSI());

    // Test ping to gateway
    IPAddress gateway = WiFi.gatewayIP();
    Serial.printf("Gateway IP: %s\n", gateway.toString().c_str());

    fms_cli.respond("wifi_test", "WiFi connection test successful");
  } else {
    String errorMsg;
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        errorMsg = "SSID not available";
        break;
      case WL_CONNECT_FAILED:
        errorMsg = "Connection failed (wrong password?)";
        break;
      case WL_CONNECTION_LOST:
        errorMsg = "Connection lost";
        break;
      case WL_DISCONNECTED:
        errorMsg = "Disconnected";
        break;
      default:
        errorMsg = "Unknown error: " + String(WiFi.status());
    }

    fms_cli.respond("wifi_test", "WiFi connection test failed: " + errorMsg, false);
  }
}

// Device Id Change Command
void handle_device_id_change_command(const std::vector<String>& args) {
  if (args.size() != 1) {
    fms_cli.respond("device", "Usage: UUID <id> ", false);
    return;
  }
  String uuid = args[0];
  // Save to preferences
  fms_nvs_storage.begin("fms_config", false);
  fms_nvs_storage.putString("uuid", uuid);
  fms_nvs_storage.end();
  fms_cli.respond("UUID", "UUID  updated. UUID: " + uuid);
}

void handle_protocol_command(const std::vector<String>& args) {
  if (args.size() != 1) {
    fms_cli.respond("protocol", "Usage: protocol <redstar|tatsuno>", false);
    return;
  }
  String protocol = args[0];
  if (protocol == "redstar") {
    #define USE_RESTAR
    //fms_set_protocol_config("redstar");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Allow time for changes to take effect
    ESP.restart();
    // Set Redstar protocol
    fms_cli.respond("protocol", "Protocol set to Redstar");
    // Add any additional setup for Redstar here
  } else if (protocol == "tatsuno") {
    #define USE_TATSUNO
    //fms_set_protocol_config("tatsuno");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Allow time for changes to take effect
    ESP.restart();
    // Set Tatsuno protocol
    fms_cli.respond("protocol", "Protocol set to Tatsuno");
    // Add any additional setup for Tatsuno here
  } else {
    fms_cli.respond("protocol", "Unknown protocol: " + protocol, false);
  }
}

// Custom print function that captures output for the web interface
size_t custom_print(const uint8_t* buffer, size_t size) {
  if (testModeActive) {
    // Add to buffer
    for (size_t i = 0; i < size; i++) {
      serialOutputBuffer += (char)buffer[i];
    }
    // Trim buffer if it gets too large
    if (serialOutputBuffer.length() > MAX_BUFFER_SIZE) {
      serialOutputBuffer = serialOutputBuffer.substring(serialOutputBuffer.length() - MAX_BUFFER_SIZE);
    }
  }
  // Pass through to original Serial
  return Serial.write(buffer, size);
}

// Protocol config command
void handle_protocol_config_command(const std::vector<String>& args) {
  if (args.size() < 11) {
    fms_cli.respond("protocol_config", "Usage: protocol_config <protocol> <device_id> <nozzle_count> <pump_id1> ... <pump_id8>", false);
    return;
  }

  String protocol = args[0];
  // Validate protocol 
  if (protocol != "tatsuno" && protocol != "gilbarco" && protocol != "redstar" && protocol != "haungyang") {
    fms_cli.respond("protocol_config", "Invalid protocol. Must be tatsuno, gilbarco, redstar, or haungyang", false);
    return;
  }

  // Validate device and nozzle numbers
  uint8_t devn = args[1].toInt();
  uint8_t noz = args[2].toInt();
  if (devn == 0 || devn > 255) {
    fms_cli.respond("protocol_config", "Device ID must be between 1 and 255", false);
    return;
  }
  if (noz == 0 || noz > 8) {
    fms_cli.respond("protocol_config", "Nozzle count must be between 1 and 8", false);
    return;
  }

  // Validate and store pump IDs
  uint8_t pumpids[8] = {0};
  for (int i = 0; i < 8; i++) {
    pumpids[i] = args[i + 3].toInt();
    if (pumpids[i] > 255) {
      fms_cli.respond("protocol_config", "Pump ID " + String(i+1) + " must be between 0 and 255", false);
      return;
    }
  }

  // All validation passed, update configuration
  dcfg.pt = protocol;
  dcfg.devn = devn;
  dcfg.noz = noz;
  memcpy(dcfg.pumpids, pumpids, sizeof(pumpids));

  fms_set_protocol_config(dcfg);
  config_eeprom_writeInt(92,111111); /* test mode pin */

  
  fms_cli.respond("protocol_config", 
    "Protocol configuration saved:\n"
    "Protocol: " + protocol + "\n"
    "Device ID: " + String(devn) + "\n"
    "Nozzle count: " + String(noz), true);
}

void handle_mqtt_command(const std::vector<String>& args) {

if (args.size() != 2) {
    fms_cli.respond("mqtt_config", "Usage: mqtt_config <host> <port>", false);
    return;
  }
  char mqtt_host[32];
  char mqtt_port[6];

  String host = args[0];
  String port = args[1];

    // Save to preferences
  fms_nvs_storage.begin("fms_config", false);
  fms_nvs_storage.putString("host", host);
  fms_nvs_storage.putString("port", port);
  fms_nvs_storage.end();

  fms_cli.respond("mqtt_config", "config successfully saved", true);
}


void config_writeStringToEEPROM(int address, String data) {  // to store wifi ssid and pass in EEprom
  int data_length = data.length();
  for (int i = 0; i < data_length; i++) {
    EEPROM.write(address + i, data[i]);
    EEPROM.commit();
  }
  EEPROM.write(address + data_length, '\0');
  EEPROM.commit();
  Serial.println("EEPROM write complete");
}


void config_eeprom_writeInt(int add, long data) {
  byte byte0 = (data >> 24) & 0xff;
  byte byte1 = (data >> 16) & 0xff;
  byte byte2 = (data >> 8) & 0xff;
  byte byte3 = data & 0xff;

  EEPROM.write(add, byte0);
  EEPROM.write(add + 1, byte1);
  EEPROM.write(add + 2, byte2);
  EEPROM.write(add + 3, byte3);
  EEPROM.commit();
}


void handle_nozzle_command(const std::vector<String>& args) {
  if (args.size() < 16) {
    fms_cli.respond("nozzle_config", "Usage: nozzle_config <> <>", false);
    return;
  }

  String nozzle_1_fuel_type  = args[0];
  int nozzle_1_fuel_price    = args[1].toInt();

  String nozzle_2_fuel_type  = args[2];
  int nozzle_2_fuel_price    = args[3].toInt();

  String nozzle_3_fuel_type  = args[4];
  int nozzle_3_fuel_price    = args[5].toInt();

  String nozzle_4_fuel_type  = args[6];
  int nozzle_4_fuel_price    = args[7].toInt();

  String nozzle_5_fuel_type  = args[8];
  int nozzle_5_fuel_price    = args[9].toInt();

  String nozzle_6_fuel_type  = args[10];
  int nozzle_6_fuel_price    = args[11].toInt();

  String nozzle_7_fuel_type  = args[12];
  int nozzle_7_fuel_price    = args[13].toInt();

  String nozzle_8_fuel_type  = args[14];
  int nozzle_8_fuel_price    = args[15].toInt();

/* for touch controller eeprom storage */
  config_writeStringToEEPROM(200, nozzle_1_fuel_type);
  config_writeStringToEEPROM(230, nozzle_2_fuel_type);
  config_writeStringToEEPROM(260, nozzle_3_fuel_type);
  config_writeStringToEEPROM(290, nozzle_4_fuel_type);
  config_writeStringToEEPROM(330, nozzle_5_fuel_type);
  config_writeStringToEEPROM(360, nozzle_6_fuel_type);
  config_writeStringToEEPROM(390, nozzle_7_fuel_type);
  config_writeStringToEEPROM(420, nozzle_8_fuel_type);

  config_eeprom_writeInt(114, 2900);
  config_eeprom_writeInt(118, 3000);
  config_eeprom_writeInt(122, 2700);
  config_eeprom_writeInt(81, 3300);
  config_eeprom_writeInt(85, 0);
  config_eeprom_writeInt(132, 0);
  config_eeprom_writeInt(138, 0);
  config_eeprom_writeInt(144, 0);


  fms_cli.respond("nozzle_config" ,"fuel" + String(nozzle_1_fuel_type) , true);
}


static void cli_task(void* arg) {
  BaseType_t rc;
  // cli command
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
