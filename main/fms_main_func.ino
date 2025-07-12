
void fms_boot_count(bool bootsave) {
  if (!bootsave) {
    return;
  }

  if (!fms_nvs_storage.begin("fms_config", false)) {
    FMS_LOG_ERROR("[fms_main_func.ino:13] Failed to initialize NVS storage");
    return;
  }
  sysCfg.bootcount = fms_nvs_storage.getUInt("bootcount", 0) + 1;
  app_cpu = xPortGetCoreID();
  FMS_LOG_INFO("[fms_main_func.ino:19] CPU %d: Boot count: %lu", app_cpu, sysCfg.bootcount);
  fms_nvs_storage.putUInt("bootcount", sysCfg.bootcount);
  fms_nvs_storage.end();  // Close NVS storage
}

void log_chip_info() {
#if SHOW_FMS_CHIP_INFO_LOG
  fms_chip_info_log();
  fms_memory_info_log();
#endif
}

bool fms_initialize_uart2(int baudrate) {
  if (fms_uart2_begin(use_serial1, baudrate)) {
    //fms_uart2_serial.onReceive(fm_rx_irq_interrupt);  // uart interrupt function
    FMS_LOG_INFO("[FMSUART2] UART2.. DONE");
    return true;
  } else {
    FMS_LOG_ERROR("[FMSUART2] UART2.. FAIL");
    return false;
  }
}

bool fms_initialize_wifi() {
  if (initialize_fms_wifi(wifi_start_event)) {
    FMS_LOG_INFO("[fms_main_func:45] Connected to WiFi, IP: %s", WiFi.localIP().toString().c_str());
    return true;
  } else {
    FMS_LOG_WARNING("[fms_main_func:48] Failed to connect to WiFi");
    return false;
  }
}

void fms_run_sd_test() {
#if true
  fms_config_load_sd_test();
  if (!LittleFS.begin(true)) {  // preference storage (1MB)
    Serial.println("[fms_main_func.ino:57] [STORAGE] Failed to mount file system");
  } else {
    Serial.println("[fms_main_func.ino:59] [STORAGE] File system mounted");
  }
  // load system config file
#endif
}

void log_debug_info() {
#if SHOW_DEBUG_FMS_CHIP_INFO_LOG
  fms_print_after_setup_info();
  fms_log_task_list();
#endif
}

void fms_pin_mode(int pin, int mode) { // pin out declare
  pinMode(pin, mode);
}

/* led output config */
void init_status_leds() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_RED) | (1ULL << LED_GREEN) |
                        (1ULL << LED_BLUE) | (1ULL << LED_YELLOW),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_RED, 1);
    gpio_set_level(LED_GREEN, 1);
    gpio_set_level(LED_BLUE, 1);
    gpio_set_level(LED_YELLOW, 1);
}

 

void fms_set_protocol_config(DisConfig& cfg) {
  if (!fms_nvs_storage.begin("fms_config", false)) {
    FMS_LOG_ERROR("[Protocol Config] Failed to initialize NVS storage");
    return;
  }

  // Save configuration to NVS storage
  bool success = true;
  success &= fms_nvs_storage.putString("protocol", cfg.pt);
  success &= fms_nvs_storage.putUChar("devn", cfg.devn);
  success &= fms_nvs_storage.putUChar("noz", cfg.noz);

  // Save pump IDs
  char key[12];
  for (int i = 0; i < 8; i++) {
    snprintf(key, sizeof(key), "pumpid%d", i + 1);
    success &= fms_nvs_storage.putUChar(key, cfg.pumpids[i]);
  }
  if (success) {
    FMS_LOG_INFO("[Protocol Config] %s configuration saved successfully", cfg.pt.c_str());
    Serial.printf(
      "Protocol: %s, Device ID: %d, Nozzle count: %d\n"
      "Pump IDs: %d %d %d %d %d %d %d %d\n",
      cfg.pt.c_str(), cfg.devn, cfg.noz,
      cfg.pumpids[0], cfg.pumpids[1], cfg.pumpids[2], cfg.pumpids[3],
      cfg.pumpids[4], cfg.pumpids[5], cfg.pumpids[6], cfg.pumpids[7]
    );
    
    // Update system configuration
    sysCfg.protocol = cfg.pt;

      EEPROM.write(109, cfg.devn);  // device id
      EEPROM.commit();
      EEPROM.write(110, cfg.noz);  // nozzle count
      EEPROM.commit();
      EEPROM.write(101, cfg.pumpids[0]);  // pump id 1
      EEPROM.commit();
      EEPROM.write(102, cfg.pumpids[1]);  // pump id 2
      EEPROM.commit();
      EEPROM.write(103, cfg.pumpids[2]);  // pump id 3
      EEPROM.commit();
      EEPROM.write(104, cfg.pumpids[3]);  // pump id 4
      EEPROM.commit();
      EEPROM.write(105, cfg.pumpids[4]);  // pump id 5
      EEPROM.commit();
      EEPROM.write(106, cfg.pumpids[5]);  // pump id 6
      EEPROM.commit();
      EEPROM.write(107, cfg.pumpids[6]);  // pump id 7
      EEPROM.commit();
      EEPROM.write(108, cfg.pumpids[7]);  // pump id 8
      EEPROM.commit();
    FMS_LOG_INFO("[Protocol Config] Configuration values saved to EEPROM");
  } else {
    FMS_LOG_ERROR("[Protocol Config] Failed to save some configuration values");
  }
  fms_nvs_storage.end();
}

void fms_load_config() {
  if (!fms_nvs_storage.begin("fms_config", false)) {
    FMS_LOG_ERROR("[fms_main_func:205] Failed to initialize NVS storage");
    return;
  }
  deviceName = fms_nvs_storage.getString("uuid", "ultm_25505v01_");
  FMS_LOG_INFO("[fms_main_func:209] Device UUID: %s", deviceName.c_str());
  fms_nvs_storage.end();  // Close NVS storage
}


