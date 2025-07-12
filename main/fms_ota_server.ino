/* 
ota server 
  * author : Trion
    Guided By : Sir Thiha Kyaw
    Copyright : @2025 iih
    ota server & dns reponseder on network 
    using esp ota lib
    file : _fms_main.h
    file : fms_ota_server.ino (rtos task) 
    reference from : mongoose wizard
    description : lite_version ota 
    v 0.1 ota server
*/

/* version 2 ota */
#define USE_V1_OTA_SERVER
#undef  USE_V2_OTA_SERVER

#ifdef USE_V1_OTA_SERVER

#define HTTP_UPLOAD_BUFLEN 4096         // Increased from default 1460
#define WDT_TIMEOUT_S 30

void fms_info_response() {            // mini version show in ota page
  JsonBuilder json;
  json.addString("deviceName",        deviceName);
  json.addString("firmwareVersion",   firmwareVersion);
  json.addString("ipAddress",         WiFi.localIP().toString());
  json.addString("macAddress",        WiFi.macAddress());
  json.addInt("rssi",                 WiFi.RSSI());
  json.addLong("uptime",              uptime);
  json.addInt("freeHeap",             ESP.getFreeHeap());
  json.addInt("totalHeap",            ESP.getHeapSize());
  json.addInt("cpuFreqMHz",           ESP.getCpuFreqMHz());
  json.addString("sdkVersion",        ESP.getSdkVersion());
  json.addString("status",            updateStatus);
  json.addInt("progress",             otaProgress);
  json.addBool("otaInProgress",       otaInProgress);
 
  //json.addLong("flashChipSize", ESP.getFlashChipSize());
  cachedInfoResponse = json.toString();
}

void handleDashboard() { // login auth
  if (!isAuthenticated) {
    // Redirect to login if not authenticated
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting to login...");
    return;
  }
  File file = LittleFS.open("/index.html", "r");
  if (file) {
    server.sendHeader("Cache-Control", "max-age=86400");
    server.streamFile(file, "text/html");
    file.close();
  }
}

void handleLogin() {
  if (server.hasArg("user") && server.hasArg("pass")) {
    String user = server.arg("user");
    String pass = server.arg("pass");

    if (user == correctUsername && pass == correctPassword) {
      isAuthenticated = true;  // Mark as logged in
      server.send(200, "text/plain", "Login Successful!");
    } else {
      server.send(401, "text/plain", "Invalid Username or Password");
    }
  } else {
    server.send(400, "text/plain", "Missing credentials");
  }
}

void handleLogout() {
  isAuthenticated = false;
  server.send(200, "text/plain", "Logged out successfully!");
}

void fms_set_ota_server() {
  FMS_LOG_INFO("[fms_ota_server.ino:75] ota server created");
  server.enableCORS(true);
  const char* cacheControl = "max-age=86400";
  server.serveStatic("/", LittleFS, "/login.html", cacheControl);

  server.on("/login", handleLogin);
  server.on("/dashboard", handleDashboard);

  server.serveStatic("/script.js", LittleFS, "/script.js", cacheControl);
  server.on("/api/info", HTTP_GET, []() {
    if (millis() - lastInfoRequest < INFO_CACHE_TIME && cachedInfoResponse.length() > 0) {
      server.send(200, "application/json", cachedInfoResponse);
      return;
    }
    fms_info_response();
    lastInfoRequest = millis();
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send(200, "application/json", cachedInfoResponse);
  });
  server.on("/logout", handleLogout);  // logout ota server
  server.on(
    "/api/update", HTTP_POST, []() {
      // This handler is called after the upload is complete
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");

      // Re-enable task watchdog if it was disabled
      esp_task_wdt_config_t wdtConfig;
      wdtConfig.timeout_ms = WDT_TIMEOUT_S * 1000;
      wdtConfig.idle_core_mask = 0;
      wdtConfig.trigger_panic = true;
      esp_task_wdt_init(&wdtConfig);

      vTaskDelay(pdMS_TO_TICKS(1000));
      ESP.restart();
    },
    []() {
      // This handler processes the actual upload
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        // Disable task watchdog for update process
        esp_task_wdt_deinit();
        FMS_LOG_INFO("Update: %s\n", upload.filename.c_str());
        updateStatus = "Update started";
        otaInProgress = true;
        otaProgress = 0;
        contentLength = 0;
        uploadedBytes = 0;

        // Get content length from headers if available
        if (server.hasHeader("Content-Length")) {
          contentLength = server.header("Content-Length").toInt();
          FMS_LOG_INFO("Content-Length: %d bytes\n", contentLength);
        }
        // Free up as much memory as possible
        FMS_LOG_INFO("Free heap before update: %d bytes\n", ESP.getFreeHeap());
        // Start the update with the correct partition type
        if (!Update.begin(contentLength > 0 ? contentLength : UPDATE_SIZE_UNKNOWN, U_FLASH)) {
          Update.printError(Serial);
          updateStatus = "OTA Error";
          otaInProgress = false;
          // Re-enable task watchdog
          esp_task_wdt_config_t wdtConfig;
          wdtConfig.timeout_ms = WDT_TIMEOUT_S * 1000;
          wdtConfig.idle_core_mask = 0;
          wdtConfig.trigger_panic = true;
          esp_task_wdt_init(&wdtConfig);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        // Process upload chunks in larger batches for better performance
        uploadedBytes += upload.currentSize;
        // Calculate progress
        if (contentLength > 0) {
          otaProgress = (uploadedBytes * 100) / contentLength;
        } else if (Update.size()) {
          otaProgress = (uploadedBytes * 100) / Update.size();
        }
        // Write the received bytes to flash
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
          updateStatus = "OTA Error";
        }
        // Log progress less frequently to reduce overhead
        if (otaProgress % 10 == 0) {
          FMS_LOG_INFO("Progress: %u%% (%u / %u bytes)\n",
                       otaProgress,
                       uploadedBytes,
                       contentLength > 0 ? contentLength : Update.size());
        }
        // Yield to avoid watchdog trigger
        yield();
      } else if (upload.status == UPLOAD_FILE_END) {
        // Finalize the update
        if (Update.end(true)) {
          FMS_LOG_INFO("Update Success: %u bytes\nRebooting...\n", uploadedBytes);
          updateStatus = "Update successful";
          otaProgress = 100;
        } else {
          Update.printError(Serial);
          updateStatus = "OTA Error";
        }
        otaInProgress = false;
      } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        otaInProgress = false;
        updateStatus = "Update aborted";
        FMS_LOG_INFO("Update aborted");
        // Re-enable task watchdog
        esp_task_wdt_config_t wdtConfig;
        wdtConfig.timeout_ms = WDT_TIMEOUT_S * 1000;
        wdtConfig.idle_core_mask = 0;
        wdtConfig.trigger_panic = true;
        esp_task_wdt_init(&wdtConfig);
      }
      yield();
    });

  // Handle restart request
  server.on("/api/restart", HTTP_POST, []() {
    server.send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
  });

  // Set up mDNS responder
  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });
}

static void web_server_task(void* arg) {
  if (!MDNS.begin(deviceName)) {                // Set up mDNS responder
    Serial.println("[DNS] Error setting up MDNS responder!");
  } else {
    Serial.println("[DNS] mDNS responder started");
    MDNS.addService("esp-ota", "tcp", 80);      // Add service to MDNS
    MDNS.addService("http", "tcp", 80);         // Add standard HTTP service for better discovery
  }
  deviceName = deviceName + String(WiFi.macAddress()).c_str();
  fms_set_ota_server();
  server.begin();

  while (1) {
    server.handleClient();
    // Update uptime counter (every second)
    if (millis() - lastUptimeUpdate >= 1000) {
      uptime++;
      lastUptimeUpdate = millis();
    }
   
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

#endif // USE_V1_OTA
