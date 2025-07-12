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

#include "_fms_cli.h"

// Constructor
fms_cli::fms_cli(HardwareSerial& serial, const char* password) 
    : _serial(serial), 
      _buffer(""),
      _password(password ? password : ""),
      _prompt("FMS> "),
      _authenticated(password ? false : true),
      _authRequired(password ? true : false),
      _echoEnabled(true) {
}

// Initialize CLI
bool fms_cli::begin(unsigned long baudRate) {
    _serial.begin(baudRate);
    
    // Wait for serial to be ready
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (!_serial) {
        return false;
    }
    
    // Set up interrupt handler
    _serial.onReceive([this]() {
        this->process_input();
    });
    
    // Register built-in commands
    register_built_in_commands();
    
    // Print welcome message
    _serial.println("\n\r+----------------------------------+");
    _serial.println("|       FMS Firmware v3.0          |");
    _serial.println("+----------------------------------+");
    
    if (_authRequired && !_authenticated) {
        _serial.println("Please login with 'login <password>'");
    } else {
        _serial.print(_prompt);
    }
    
    return true;
}

// Register a command
void fms_cli::register_command(const String& name, const String& description, 
                            CommandCallback callback, uint8_t minArgs, uint8_t maxArgs) {
    cli_command_t cmd = {
        .name = name,
        .description = description,
        .callback = callback,
        .minArgs = minArgs,
        .maxArgs = maxArgs
    };
    
    _commands[name] = cmd;
}

// Process incoming data
void fms_cli::process_input() {
    
    while (_serial.available()) {
        char c = _serial.read();
        
        // Handle backspace
        if (c == '\b' || c == 127) {
            if (_buffer.length() > 0) {
                _buffer.remove(_buffer.length() - 1);
                if (_echoEnabled) {
                    _serial.print("\b \b"); // Erase character on terminal
                }
            }
            continue;
        }
        
        // Echo character if enabled
        if (_echoEnabled && c >= 32 && c < 127) {
            _serial.write(c);
        }
        
        // Process on newline
        if (c == '\n' || c == '\r') {
            if (_buffer.length() > 0) {
                _serial.println();
                
                String command;
                std::vector<String> args;
                
                parse_command(_buffer, command, args);
                
                // Handle login if authentication required
                if (_authRequired && !_authenticated) {
                    if (command == "login" && args.size() == 1) {
                        _authenticated = authenticate(args[0]);
                        if (_authenticated) {
                            respond("login", "Login successful", true);
                            //_serial.println("Login successful");
                        } else {
                            respond("login", "Invalid password", false);
                            //_serial.println("Invalid password");
                        }
                    } else {
                        _serial.println("Please login with 'login <password>'");
                    }
                } else {
                    execute_command(command, args);
                }
                
                _buffer = "";
                _serial.print(_prompt);
            } else if (c == '\r' && _serial.peek() != '\n') {
                // Just a carriage return without newline
                _serial.println();
                _serial.print(_prompt);
            }
        } else if (c >= 32) {
            // Add printable characters to buffer
            _buffer += c;
        }
    }
}

// Parse command line into command and arguments
void fms_cli::parse_command(const String& cmdLine, String& command, std::vector<String>& args) {
    String trimmed = cmdLine;
    trimmed.trim();
    
    int idx = 0;
    int lastIdx = 0;
    bool inQuotes = false;
    
    // Extract command (first word)
    idx = trimmed.indexOf(' ');
    if (idx == -1) {
        command = trimmed;
        return;
    }
    
    command = trimmed.substring(0, idx);
    lastIdx = idx + 1;
    
    // Extract arguments
    while (lastIdx < trimmed.length()) {
        // Skip spaces
        while (lastIdx < trimmed.length() && trimmed[lastIdx] == ' ') {
            lastIdx++;
        }
        
        if (lastIdx >= trimmed.length()) {
            break;
        }
        
        // Check for quoted argument
        if (trimmed[lastIdx] == '"') {
            inQuotes = true;
            lastIdx++; // Skip opening quote
            idx = trimmed.indexOf('"', lastIdx);
            
            if (idx == -1) {
                // No closing quote, take rest of string
                args.push_back(trimmed.substring(lastIdx));
                break;
            }
            
            args.push_back(trimmed.substring(lastIdx, idx));
            lastIdx = idx + 1;
            inQuotes = false;
        } else {
            // Regular argument
            idx = trimmed.indexOf(' ', lastIdx);
            
            if (idx == -1) {
                // Last argument
                args.push_back(trimmed.substring(lastIdx));
                break;
            }
            
            args.push_back(trimmed.substring(lastIdx, idx));
            lastIdx = idx + 1;
        }
    }
}

// Handle authentication
bool fms_cli::authenticate(const String& password) {
    return password == _password;
}

// Execute command
void fms_cli::execute_command(const String& command, const std::vector<String>& args) {
    auto it = _commands.find(command);
    
    if (it == _commands.end()) {
        respond(command, "Command not found", false);
        return;
    }
    
    const cli_command_t& cmd = it->second;
    
    // Check argument count
    if (args.size() < cmd.minArgs) {
        respond(command, "Too few arguments", false);
        return;
    }
    
    if (args.size() > cmd.maxArgs) {
        respond(command, "Too many arguments", false);
        return;
    }
    
    // Execute command callback
    cmd.callback(args);
}

// Escape JSON string values
String fms_cli::escape_json(const String& input) {
    String output;
    output.reserve(input.length() + 10); // Reserve some extra space for potential escapes
    
    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input[i];
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:
                if (c < 32) {
                    // For control characters, use \uXXXX format
                    char hex[7];
                    snprintf(hex, sizeof(hex), "\\u%04x", c);
                    output += hex;
                } else {
                    output += c;
                }
        }
    }
    
    return output;
}

// Format a JSON string manually
String fms_cli::format_json(const std::map<String, String>& fields) {
    String json = "{";
    bool first = true;
    
    for (const auto& field : fields) {
        if (!first) {
            json += ",";
        }
        first = false;
        
        json += "\"" + field.first + "\":";
        
        // Check if value is a boolean or number
        if (field.second == "true" || field.second == "false" || 
            field.second == "null" || 
            (field.second.length() > 0 && 
             ((field.second[0] >= '0' && field.second[0] <= '9') || 
              field.second[0] == '-'))) {
            json += field.second;
        } else {
            json += "\"" + escape_json(field.second) + "\"";
        }
    }
    
    json += "}";
    return json;
}

// Print response in JSON format
void fms_cli::respond(const String& command, const String& result, bool success) {
    std::map<String, String> fields;
    fields["command"] = command;
    fields["result"] = result;
    fields["success"] = success ? "true" : "false";
    
    String response = format_json(fields);
    _serial.println(response);
}

// Print help
void fms_cli::print_help() {
    _serial.println("+------------------+------------------+");
    _serial.println("| Command          | Description      |");
    _serial.println("+------------------+------------------+");
    
    for (const auto& cmd : _commands) {
        _serial.printf("| %-16s | %-16s |\n", cmd.second.name.c_str(), cmd.second.description.c_str());
    }
    
    _serial.println("+------------------+------------------+");
}

// Set authentication required flag
void fms_cli::set_auth_required(bool required) {
    _authRequired = required;
    if (!required) {
        _authenticated = true;
    }
}

// Set prompt
void fms_cli::set_prompt(const String& prompt) {
    _prompt = prompt;
}

// Enable/disable echo
void fms_cli::set_echo(bool enabled) {
    _echoEnabled = enabled;
}

// Register built-in commands
void fms_cli::register_built_in_commands() {
    // Help command
    register_command("help", "Show available commands", [this](const std::vector<String>&) {
        this->print_help();
    });
    
    // Echo command
    register_command("echo", "Toggle command echo", [this](const std::vector<String>& args) {
        if (args.size() > 0) {
            if (args[0] == "on") {
                this->set_echo(true);
                this->respond("echo", "Echo enabled");
            } else if (args[0] == "off") {
                this->set_echo(false);
                this->respond("echo", "Echo disabled");
            } else {
                this->respond("echo", "Invalid argument. Use 'on' or 'off'", false);
            }
        } else {
            this->set_echo(!_echoEnabled);
            this->respond("echo", _echoEnabled ? "Echo enabled" : "Echo disabled");
        }
    }, 0, 1);
    
    // Exit/logout command
    register_command("logout", "Logout from CLI", [this](const std::vector<String>&) {
        if (_authRequired) {
            _authenticated = false;
            this->respond("logout", "Logged out");
            _serial.println("Please login with 'login <password>'");
        } else {
            this->respond("logout", "Authentication not enabled", false);
        }
    });
}

// Begin a JSON response (prints the opening bracket)
void fms_cli::begin_json_response() {
    _serial.print("{");
}

// Add a part to the JSON response
void fms_cli::add_json_response_part(const String& part) {
    _serial.print(part);
    yield(); // Allow the system to process other tasks
}

// End the JSON response (prints the closing bracket)
void fms_cli::end_json_response() {
    _serial.println("}");
}

// Execute a command directly (for testing)
bool fms_cli::execute_test_command(const String& commandLine) {
    String command;
    std::vector<String> args;
    
    parse_command(commandLine, command, args);
    
    if (command.length() == 0) {
        return false;
    }
    
    auto it = _commands.find(command);
    if (it == _commands.end()) {
        respond(command, "Command not found", false);
        return false;
    }
    
    execute_command(command, args);
    return true;
}
