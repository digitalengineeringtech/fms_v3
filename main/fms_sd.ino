/*
  *  fms_sd.cpp
  *  Created on: 2020. 12. 10.
  *  author : thet htar khaing
*/

/* upgrade in version 2 */
/* not included in version 1 */

bool fms_sd_init() {
  if (!SD.begin(SD_CARD_CS_PIN)) {
    FMS_LOG_ERROR("Card Mount Failed");
    return false;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    FMS_LOG_ERROR("No SD card attached");
 
    vTaskDelay(pdMS_TO_TICKS(1000));
    return false;
  }
  FMS_LOG_INFO("SD CARD MOUNTED");
  return true;
}

void fms_sd_dir(fs::FS& fs, const char* dirname, uint8_t levels) {
  FMS_LOG_INFO("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    FMS_LOG_ERROR("Failed to open directory");
    //return false;
  }
  if (!root.isDirectory()) {
    FMS_LOG_ERROR("Not a directory");
    //return false;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      FMS_LOG_INFO("  DIR : ");
      FMS_LOG_INFO(String(file.name()).c_str());
      if (levels) {
        fms_sd_dir(fs, file.name(), levels - 1);
      }
    } else {
      FMS_LOG_INFO("  FILE: ");
      FMS_LOG_INFO(String(file.name()).c_str());
      FMS_LOG_INFO("  SIZE: ");
      FMS_LOG_INFO(String(file.size()).c_str());
    }
    file = root.openNextFile();
  }
}

void fms_config_load_sd_test() {
  fms_sd_init();
  fms_sd_dir(SD, "/", 0);
  //return true;
}

bool write_data_sd(char* input) {
  //to write code to save data to sd.
  //step 1. simple write
  //step 2. encrypt and write
  //setp 3. sd formarting (clicommand)


  return true;
}

static void sd_task(void* arg) {
  BaseType_t rc;
  while (1) {

    vTaskDelay(pdMS_TO_TICKS(100));

  }
}
