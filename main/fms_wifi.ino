bool initialize_fms_wifi(bool flag) {
  if (flag) {
    // get ssid and password from nvs storage
    // using preference library
    fms_nvs_storage.begin("fms_config", false);
    String ssid_str = fms_nvs_storage.getString("ssid");
    String pass_str = fms_nvs_storage.getString("pass");
    
    if(ssid_str.length() == 0 || pass_str.length() == 0) {
      gpio_set_level(LED_YELLOW, LOW);
      vTaskDelay(pdMS_TO_TICKS(500));
      FMS_LOG_ERROR("[fms_wifi.ino:11] [DEBUG WiFi] wifi .. credential .. value is empty");
      fms_nvs_storage.end();
      return false;
    }

    fms_nvs_storage.end();
    FMS_LOG_DEBUG("SSID : %s , PASS : %s", ssid_str, pass_str);
    strncpy(sysCfg.wifi_ssid, ssid_str.c_str(), sizeof(sysCfg.wifi_ssid) - 1);
    strncpy(sysCfg.wifi_password, pass_str.c_str(), sizeof(sysCfg.wifi_password) - 1);
    if (sysCfg.wifi_ssid == " " || sysCfg.wifi_password == " ") {
      FMS_LOG_ERROR("[fms_wifi.ino:21] [DEBUG WiFi] wifi .. credential .. value is empty");
      return false;
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);  // auto reconnect function
    WiFi.begin(sysCfg.wifi_ssid, sysCfg.wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
      gpio_set_level(LED_YELLOW, LOW);
      vTaskDelay(pdMS_TO_TICKS(100));
      gpio_set_level(LED_YELLOW, HIGH);
      vTaskDelay(pdMS_TO_TICKS(100));

      FMS_LOG_INFO("[fms_wifi.ino:35] WiFi initialized, connecting to %s... wpa:%s", sysCfg.wifi_ssid, sysCfg.wifi_password);
      vTaskDelay(pdMS_TO_TICKS(1000));  // Wait for 1 second before repeating
    }
    return true;
  }
}


uint8_t count = 1;
static void wifi_task(void *arg) {
  BaseType_t rc;
  while (1) {
    if (WiFi.status() != WL_CONNECTED) {
      FMS_LOG_WARNING("[fms_wifi.ino:53] Failed to connect to WiFi");
      gpio_set_level(LED_YELLOW, LOW);
      vTaskDelay(pdMS_TO_TICKS(100));
      gpio_set_level(LED_YELLOW, HIGH);
      vTaskDelay(pdMS_TO_TICKS(100));

      #ifdef USE_TOUCH
      fms_uart2_serial.write(Hide_cloud_icon, 8); // send hide cloud icon command to touch
      vTaskDelay(pdMS_TO_TICKS(250)); // wait for 250 milliseconds
      fms_uart2_serial.write(Show_cloud_icon, 8); // send show cloud icon command to touch
      vTaskDelay(pdMS_TO_TICKS(250)); // wait for 250 milliseconds
      #endif
    } else {
      #ifdef USE_TOUCH
        fms_uart2_serial.write(Show_wifi_icon, 8);
        fms_uart2_serial.write(Show_wifi_icon, 8);
        Icon_fun();  // icon function
      #endif
      // FMS_LOG_INFO("[fms_wifi.ino:59] Connected to WiFi, IP: %s", WiFi.localIP().toString().c_str());
      gpio_set_level(LED_YELLOW, LOW);
    }
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait for 1 second before repeating
  }
}
