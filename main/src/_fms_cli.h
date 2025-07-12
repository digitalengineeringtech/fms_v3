
/*
 * FMS CLI - Command Line Interface for FMS (Firmware Management System)
 * 
 * This library provides a simple command line interface for FMS projects.
 * It allows you to register commands, process input, and respond in JSON format.
 *
 * @copyright 2025 FMS Project
 * @author Sir Thiha Kyaw, Trion
 * original author:
 * @date 2025
 * @version 0.1.0
 * @Innovatic IOT House
 */

#ifndef _FMS_CLI_H_
#define _FMS_CLI_H_

#include <Arduino.h>
#include <functional>
#include <vector>
#include <map>
#include <string>

// Increase the stack size for the CLI task
#define FMS_CLI_TASK_STACK_SIZE 8192

// CLI Command callback function type
typedef std::function<void(const std::vector<String>&)> CommandCallback;

// Command structure
struct cli_command_t {
    String name;
    String description;
    CommandCallback callback;
    uint8_t minArgs;
    uint8_t maxArgs;
};

class fms_cli {
public:
    fms_cli(HardwareSerial& serial, const char* password = nullptr);
    
    // Initialize CLI
    bool begin(unsigned long baudRate = 115200);
    
    // Register a command
    void register_command(const String& name, const String& description, 
                         CommandCallback callback, uint8_t minArgs = 0, uint8_t maxArgs = 255);
    
    // Process incoming data
    void process_input();
    
    // Print response in JSON format
    void respond(const String& command, const String& result, bool success = true);
    
    // Format a JSON string manually
    String format_json(const std::map<String, String>& fields);
    
    // Print help
    void print_help();
    
    // Set authentication required flag
    void set_auth_required(bool required);
    
    // Set prompt
    void set_prompt(const String& prompt);
    
    // Enable/disable echo
    void set_echo(bool enabled);

    // Send a large response in chunks to avoid memory issues
    void begin_json_response();
    void add_json_response_part(const String& part);
    void end_json_response();
    
    // Execute a command directly (for testing)
    bool execute_test_command(const String& commandLine);

private:
    HardwareSerial& _serial;
    String _buffer;
    String _password;
    String _prompt;
    bool _authenticated;
    bool _authRequired;
    bool _echoEnabled;
    std::map<String, cli_command_t> _commands;
    
    // Parse command line into command and arguments
    void parse_command(const String& cmdLine, String& command, std::vector<String>& args);
    
    // Handle authentication
    bool authenticate(const String& password);
    
    // Execute command
    void execute_command(const String& command, const std::vector<String>& args);
    
    // Register built-in commands
    void register_built_in_commands();
    
    // Escape JSON string values
    String escape_json(const String& input);
};

#endif // _FMS_CLI_H_
