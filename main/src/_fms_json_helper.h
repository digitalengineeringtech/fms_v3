/*
  * light weight json lib for fms
  * copyright@2025 iih 
*/
#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <Arduino.h>

// A lightweight JSON string builder to replace ArduinoJSON
class JsonBuilder {
private:
  String json;
  bool firstItem;

public:
  JsonBuilder() {
    json = "{";
    firstItem = true;
  }

  // Add a string value
  void addString(const char* key, const String& value) {
    if (!firstItem) {
      json += ",";
    }
    json += "\"";
    json += key;
    json += "\":\"";
    json += value;
    json += "\"";
    firstItem = false;
  }

  // Add an integer value
  void addInt(const char* key, int value) {
    if (!firstItem) {
      json += ",";
    }
    json += "\"";
    json += key;
    json += "\":";
    json += value;
    firstItem = false;
  }

  // Add a long value
  void addLong(const char* key, long value) {
    if (!firstItem) {
      json += ",";
    }
    json += "\"";
    json += key;
    json += "\":";
    json += value;
    firstItem = false;
  }

  // Add a boolean value
  void addBool(const char* key, bool value) {
    if (!firstItem) {
      json += ",";
    }
    json += "\"";
    json += key;
    json += "\":";
    json += value ? "true" : "false";
    firstItem = false;
  }

  // Get the final JSON string
  String toString() {
    return json + "}";
  }
};

#endif // JSON_HELPER_H
