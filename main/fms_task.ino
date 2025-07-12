

bool create_task(TaskFunction_t task_func, const char* name, uint32_t stack_size, UBaseType_t priority, TaskHandle_t* handle, BaseType_t& rc) {
  rc = xTaskCreatePinnedToCore(
    task_func,   // Task function
    name,        // Name
    stack_size,  // Stack size
    nullptr,     // Parameters
    priority,    // Priority
    handle,      // Handle
    app_cpu      // CPU
  );
  assert(rc == pdPASS);
  if (rc != pdPASS) {
    FMS_LOG_ERROR("[TASK]%s task created fail", name);
    return false;
  }
  
  FMS_LOG_INFO("[TASK] %s task created done", name);
  return true;
}

bool fms_task_create() {
  BaseType_t sd_rc, wifi_rc, mqtt_rc, cli_rc, uart2_rc, webserver_rc;

  if (!create_task(sd_task, "sdcard", 3000, 2, &hsdCardTask, sd_rc)) return false;
  if (!create_task(wifi_task, "wifi", 3000, 3, &hwifiTask, wifi_rc)) return false;
  if (!create_task(mqtt_task, "mqtt", 3000, 3, &hmqttTask, mqtt_rc)) return false;
  if (!create_task(fms_uart2_task, "uart2", 3000, 1, &huart2Task, uart2_rc)) return false;
  if (!create_task(cli_task, "cli", 3000, 1, &hcliTask, cli_rc)) return false;
  if (!create_task(web_server_task, "webserver", 4096, 4, &hwebServerTask, webserver_rc)) return false;

  return true;
}

