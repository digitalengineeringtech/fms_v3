#ifndef FMS_FILEMANAGER_H
#define FMS_FILEMANAGER_H

#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WebServer.h>
  typedef ESP8266WebServer WebServerClass;
#else
  #include <WebServer.h>
  typedef WebServer WebServerClass;
#endif
#include <FS.h>

// Use LittleFS by default, but allow SPIFFS if needed
#ifndef USE_SPIFFS
  #if defined(ESP8266)
    #include <LittleFS.h>
    #define FILESYSTEM LittleFS
  #elif defined(ESP32)
   #include <LittleFS.h>
    #define FILESYSTEM LittleFS
  #endif
#else
  #include <SPIFFS.h>
  #define FILESYSTEM SPIFFS
#endif

class FMS_FileManager {
public:
  FMS_FileManager();
  
  // Initialize the file manager
  bool begin(WebServerClass* server);
  
  // Set the directory to serve files from
  void setDirectory(const String& directory);
  
  // Set the maximum upload size
  void setMaxUploadSize(size_t maxSize);
  
  // Get the HTML for the file manager UI
  String getFileManagerHTML();
  
  // Check and repair file system if needed
  bool checkFileSystem();
  
private:
  // Configuration
  WebServerClass* _server;
  String _directory;
  size_t _maxUploadSize;
  
  // Web handlers
  void handleFileList();
  void handleFileUpload();
  void handleFileDelete();
  void handleFileDownload();
  void handleNotFound();
  
  // Helper methods
  String getContentType(const String& filename);
  String formatBytes(size_t bytes);
};

#endif // FMS_FILEMANAGER_H