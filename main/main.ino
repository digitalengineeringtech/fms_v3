
/* device login page */
const String correctUsername = "admin";           /* change your login username here*/
const String correctPassword = "admin";           /* change your login pass here*/
const String firmwareVersion = "3.0.0.0";           /* Current firmware version*/
String deviceName            = "ultm_25505v01_";  /* device ID (for)  change here like this user can change with configpanel*/
#define CLI_PASSWORD         "admin"              /* cli password change this password*/
/* end change note  */

#include "main.h"
#include "src/_fms_cli.h"
#include "src/_fms_debug.h"
#include "src/_fms_json_helper.h"
#include <src/_fms_filemanager.h>        /* test features */


#define USE_CLI
fms_cli fms_cli(fms_cli_serial, CLI_PASSWORD);      // Use "admin" as the default password change your admin pass here


void setup() {

#ifdef USE_CLI
  fms_cli.begin(115200);  // Initialize the CLI with a baud rate of 115200
  fms_cli.register_command("wifi",         "Configure WiFi settings",       handle_wifi_command, 2, 2);
  fms_cli.register_command("wifi_connect", "Connect to WiFi network",       handle_wifi_connect_command, 2, 2);
  fms_cli.register_command("restart",      "Restart the system",            handle_restart_command);
  fms_cli.register_command("wifiscan_safe", "Scan for WiFi networks (safe mode)", handle_wifi_scan_safe_command);
  fms_cli.register_command("wifiread",      "Read current WiFi status",     handle_wifi_read_command);
  fms_cli.register_command("wifi_test",     "Test WiFi connection",         handle_wifi_test_command);
  fms_cli.register_command("uuid_change",   "Change Your Device Id unique address", handle_device_id_change_command, 1, 1);
  fms_cli.register_command("protocol",      "Set Protocol",                 handle_protocol_command, 1, 1);
  fms_cli.register_command("protocol_config","Set Protocol Config",       handle_protocol_config_command, 11, 11);
  fms_cli.register_command("mqtt_config"   ,"Configure Mqtt settings",     handle_mqtt_command,2,2);
  fms_cli.register_command("noz_config", "Configure Nozzle settings",   handle_nozzle_command, 16, 16);
#endif
 
  fms_run_sd_test();                        // demo test fix this load configure data from sd card
  fmsEnableSerialLogging(true);             // show serial logging data on Serial Monitor
  fms_boot_count(true);                     // boot count
  fms_load_config();                        // load config from nvs storage
 

/* task create */
  if (fms_initialize_wifi()) {             // wifi is connected create all task s
    fms_task_create();
  }




}

void loop() {
}
