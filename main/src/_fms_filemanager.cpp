#include "_fms_filemanager.h"

FMS_FileManager::FMS_FileManager() {
  _server = NULL;
  _directory = "/";
  _maxUploadSize = 1024 * 1024; // 1MB default
}

bool FMS_FileManager::begin(WebServerClass* server) {
  _server = server;
  
  if (!_server) {
    return false;
  }
  // Set up web server routes
  _server->on("/list", HTTP_GET, [this]() { this->handleFileList(); });
  _server->on("/upload", HTTP_POST, []() {}, [this]() { this->handleFileUpload(); });
  _server->on("/delete", HTTP_POST, [this]() { this->handleFileDelete(); });
  _server->on("/download", HTTP_GET, [this]() { this->handleFileDownload(); });
  
  return true;
}

bool FMS_FileManager::checkFileSystem() {
  if (!FILESYSTEM.begin(true)) {  // true = format on failure
    Serial.println("File system mount failed");
    return false;
  }
  return true;
}

void FMS_FileManager::setDirectory(const String& directory) {
  _directory = directory;
  
  // Ensure directory starts with a slash
  if (!_directory.startsWith("/")) {
    _directory = "/" + _directory;
  }
  
  // Ensure directory ends with a slash
  if (!_directory.endsWith("/")) {
    _directory += "/";
  }
}

void FMS_FileManager::setMaxUploadSize(size_t maxSize) {
  _maxUploadSize = maxSize;
}

String FMS_FileManager::getFileManagerHTML() {
  String html = "<!DOCTYPE html>\n";
  html += "<html lang=\"en\">\n";
  html += "<head>\n";
  html += "    <meta charset=\"UTF-8\">\n";
  html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  html += "    <title>ESP File Manager</title>\n";
  html += "    <style>\n";
  html += "        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }\n";
  html += "        h1 { color: #0066cc; }\n";
  html += "        .container { border: 1px solid #ccc; border-radius: 5px; padding: 20px; margin-top: 20px; }\n";
  html += "        .file-list { max-height: 300px; overflow-y: auto; border: 1px solid #ccc; margin-top: 10px; }\n";
  html += "        .file-item { padding: 8px; border-bottom: 1px solid #eee; display: flex; justify-content: space-between; align-items: center; }\n";
  html += "        .file-item:hover { background-color: #f0f0f0; }\n";
  html += "        .file-name { flex-grow: 1; }\n";
  html += "        .file-size { color: #666; margin-right: 10px; }\n";
  html += "        .file-actions { display: flex; gap: 5px; }\n";
  html += "        .btn { background-color: #0066cc; color: white; padding: 5px 10px; border: none; border-radius: 4px; cursor: pointer; text-decoration: none; font-size: 12px; }\n";
  html += "        .btn-delete { background-color: #dc3545; }\n";
  html += "        .btn-download { background-color: #28a745; }\n";
  html += "        .upload-form { margin-top: 20px; }\n";
  html += "        .progress { width: 100%; height: 20px; background-color: #f0f0f0; border-radius: 4px; margin-top: 10px; overflow: hidden; display: none; }\n";
  html += "        .progress-bar { height: 100%; background-color: #28a745; width: 0%; transition: width 0.3s; }\n";
  html += "        .alert { padding: 10px; border-radius: 4px; margin-top: 10px; display: none; }\n";
  html += "        .alert-success { background-color: #d4edda; color: #155724; }\n";
  html += "        .alert-danger { background-color: #f8d7da; color: #721c24; }\n";
  html += "        .system-info { margin-top: 20px; font-size: 12px; color: #666; }\n";
  html += "    </style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "    <h1>ESP File Manager</h1>\n";
  html += "    <div class=\"container\">\n";
  html += "        <h2>Files</h2>\n";
  html += "        <button id=\"refreshBtn\" class=\"btn\">Refresh</button>\n";
  html += "        <div id=\"fileList\" class=\"file-list\">\n";
  html += "            <div class=\"file-item\">Loading files...</div>\n";
  html += "        </div>\n";
  html += "        <div class=\"upload-form\">\n";
  html += "            <h2>Upload File</h2>\n";
  html += "            <form id=\"uploadForm\" enctype=\"multipart/form-data\">\n";
  html += "                <input type=\"file\" id=\"fileInput\" name=\"file\">\n";
  html += "                <button type=\"submit\" class=\"btn\">Upload</button>\n";
  html += "            </form>\n";
  html += "            <div id=\"progress\" class=\"progress\">\n";
  html += "                <div id=\"progressBar\" class=\"progress-bar\"></div>\n";
  html += "            </div>\n";
  html += "            <div id=\"alertSuccess\" class=\"alert alert-success\"></div>\n";
  html += "            <div id=\"alertDanger\" class=\"alert alert-danger\"></div>\n";
  html += "        </div>\n";
  html += "        <div class=\"system-info\">\n";
  html += "            <p>Free Space: <span id=\"freeSpace\">Loading...</span></p>\n";
  html += "            <p>Used Space: <span id=\"usedSpace\">Loading...</span></p>\n";
  html += "            <p>Total Space: <span id=\"totalSpace\">Loading...</span></p>\n";
  html += "        </div>\n";
  html += "    </div>\n";
  html += "    <script>\n";
  html += "        // DOM Elements\n";
  html += "        const fileList = document.getElementById('fileList');\n";
  html += "        const refreshBtn = document.getElementById('refreshBtn');\n";
  html += "        const uploadForm = document.getElementById('uploadForm');\n";
  html += "        const fileInput = document.getElementById('fileInput');\n";
  html += "        const progress = document.getElementById('progress');\n";
  html += "        const progressBar = document.getElementById('progressBar');\n";
  html += "        const alertSuccess = document.getElementById('alertSuccess');\n";
  html += "        const alertDanger = document.getElementById('alertDanger');\n";
  html += "        const freeSpace = document.getElementById('freeSpace');\n";
  html += "        const usedSpace = document.getElementById('usedSpace');\n";
  html += "        const totalSpace = document.getElementById('totalSpace');\n";
  html += "        \n";
  html += "        // Load file list and system info\n";
  html += "        function loadFileList() {\n";
  html += "            fetch('/list')\n";
  html += "                .then(response => response.json())\n";
  html += "                .then(data => {\n";
  html += "                    // Update file list\n";
  html += "                    const files = data.files || [];\n";
  html += "                    fileList.innerHTML = '';\n";
  html += "                    \n";
  html += "                    if (files.length === 0) {\n";
  html += "                        fileList.innerHTML = '<div class=\"file-item\">No files found</div>';\n";
  html += "                    } else {\n";
  html += "                        files.forEach(file => {\n";
  html += "                            const item = document.createElement('div');\n";
  html += "                            item.className = 'file-item';\n";
  html += "                            \n";
  html += "                            const name = document.createElement('div');\n";
  html += "                            name.className = 'file-name';\n";
  html += "                            name.textContent = file.name;\n";
  html += "                            \n";
  html += "                            const size = document.createElement('div');\n";
  html += "                            size.className = 'file-size';\n";
  html += "                            size.textContent = file.size;\n";
  html += "                            \n";
  html += "                            const actions = document.createElement('div');\n";
  html += "                            actions.className = 'file-actions';\n";
  html += "                            \n";
  html += "                            const downloadBtn = document.createElement('a');\n";
  html += "                            downloadBtn.className = 'btn btn-download';\n";
  html += "                            downloadBtn.textContent = 'Download';\n";
  html += "                            downloadBtn.href = '/download?file=' + encodeURIComponent(file.name);\n";
  html += "                            \n";
  html += "                            const deleteBtn = document.createElement('button');\n";
  html += "                            deleteBtn.className = 'btn btn-delete';\n";
  html += "                            deleteBtn.textContent = 'Delete';\n";
  html += "                            deleteBtn.addEventListener('click', () => deleteFile(file.name));\n";
  html += "                            \n";
  html += "                            actions.appendChild(downloadBtn);\n";
  html += "                            actions.appendChild(deleteBtn);\n";
  html += "                            \n";
  html += "                            item.appendChild(name);\n";
  html += "                            item.appendChild(size);\n";
  html += "                            item.appendChild(actions);\n";
  html += "                            \n";
  html += "                            fileList.appendChild(item);\n";
  html += "                        });\n";
  html += "                    }\n";
  html += "                    \n";
  html += "                    // Update system info\n";
  html += "                    if (data.system) {\n";
  html += "                        freeSpace.textContent = data.system.free || 'Unknown';\n";
  html += "                        usedSpace.textContent = data.system.used || 'Unknown';\n";
  html += "                        totalSpace.textContent = data.system.total || 'Unknown';\n";
  html += "                    }\n";
  html += "                })\n";
  html += "                .catch(error => {\n";
  html += "                    console.error('Error loading file list:', error);\n";
  html += "                    fileList.innerHTML = '<div class=\"file-item\">Error loading files</div>';\n";
  html += "                });\n";
  html += "        }\n";
  html += "        \n";
  html += "        // Delete file\n";
  html += "        function deleteFile(filename) {\n";
  html += "            if (!confirm('Are you sure you want to delete ' + filename + '?')) {\n";
  html += "                return;\n";
  html += "            }\n";
  html += "            \n";
  html += "            const formData = new FormData();\n";
  html += "            formData.append('file', filename);\n";
  html += "            \n";
  html += "            fetch('/delete', {\n";
  html += "                method: 'POST',\n";
  html += "                body: formData\n";
  html += "            })\n";
  html += "            .then(response => response.text())\n";
  html += "            .then(result => {\n";
  html += "                showAlert(result, true);\n";
  html += "                loadFileList();\n";
  html += "            })\n";
  html += "            .catch(error => {\n";
  html += "                console.error('Error deleting file:', error);\n";
  html += "                showAlert('Error deleting file', false);\n";
  html += "            });\n";
  html += "        }\n";
  html += "        \n";
  html += "        // Show alert\n";
  html += "        function showAlert(message, isSuccess) {\n";
  html += "            const alert = isSuccess ? alertSuccess : alertDanger;\n";
  html += "            alert.textContent = message;\n";
  html += "            alert.style.display = 'block';\n";
  html += "            \n";
  html += "            setTimeout(() => {\n";
  html += "                alert.style.display = 'none';\n";
  html += "            }, 3000);\n";
  html += "        }\n";
  html += "        \n";
  html += "        // Event listeners\n";
  html += "        refreshBtn.addEventListener('click', loadFileList);\n";
  html += "        \n";
  html += "        uploadForm.addEventListener('submit', function(e) {\n";
  html += "            e.preventDefault();\n";
  html += "            \n";
  html += "            if (!fileInput.files.length) {\n";
  html += "                showAlert('Please select a file', false);\n";
  html += "                return;\n";
  html += "            }\n";
  html += "            \n";
  html += "            const file = fileInput.files[0];\n";
  html += "            const formData = new FormData();\n";
  html += "            formData.append('file', file);\n";
  html += "            \n";
  html += "            // Show progress bar\n";
  html += "            progress.style.display = 'block';\n";
  html += "            progressBar.style.width = '0%';\n";
  html += "            \n";
  html += "            const xhr = new XMLHttpRequest();\n";
  html += "            xhr.open('POST', '/upload', true);\n";
  html += "            \n";
  html += "            xhr.upload.onprogress = function(e) {\n";
  html += "                if (e.lengthComputable) {\n";
  html += "                    const percentComplete = (e.loaded / e.total) * 100;\n";
  html += "                    progressBar.style.width = percentComplete + '%';\n";
  html += "                }\n";
  html += "            };\n";
  html += "            \n";
  html += "            xhr.onload = function() {\n";
  html += "                progress.style.display = 'none';\n";
  html += "                \n";
  html += "                if (xhr.status === 200) {\n";
  html += "                    showAlert('File uploaded successfully', true);\n";
  html += "                    fileInput.value = '';\n";
  html += "                    loadFileList();\n";
  html += "                } else {\n";
  html += "                    showAlert('Error uploading file: ' + xhr.responseText, false);\n";
  html += "                }\n";
  html += "            };\n";
  html += "            \n";
  html += "            xhr.onerror = function() {\n";
  html += "                progress.style.display = 'none';\n";
  html += "                showAlert('Error uploading file', false);\n";
  html += "            };\n";
  html += "            \n";
  html += "            xhr.send(formData);\n";
  html += "        });\n";
  html += "        \n";
  html += "        // Load file list on page load\n";
  html += "        loadFileList();\n";
  html += "    </script>\n";
  html += "</body>\n";
  html += "</html>";
  return html;
}

void FMS_FileManager::handleFileList() {
  String path = _directory;
  
  File root = FILESYSTEM.open(path);
  if (!root) {
    _server->send(500, "text/plain", "Failed to open directory");
    return;
  }
  
  if (!root.isDirectory()) {
    _server->send(500, "text/plain", "Not a directory");
    return;
  }
  
  String output = "{\"files\":[";
  File file = root.openNextFile();
  
  while (file) {
    if (output != "{\"files\":[") {
      output += ",";
    }
    
    output += "{\"name\":\"";
    // Remove the directory prefix if present
    String fileName = String(file.name());
    if (fileName.startsWith(_directory)) {
      fileName = fileName.substring(_directory.length());
    }
    output += fileName;
    output += "\",\"size\":\"";
    output += formatBytes(file.size());
    output += "\"}";
    
    file = root.openNextFile();
  }
  
  output += "],\"system\":{";
  output += "\"free\":\"" + formatBytes(FILESYSTEM.totalBytes() - FILESYSTEM.usedBytes()) + "\",";
  output += "\"used\":\"" + formatBytes(FILESYSTEM.usedBytes()) + "\",";
  output += "\"total\":\"" + formatBytes(FILESYSTEM.totalBytes()) + "\"";
  output += "}}";
  
  _server->send(200, "application/json", output);
}

void FMS_FileManager::handleFileUpload() {
  HTTPUpload& upload = _server->upload();
  static File uploadFile;
  static String uploadFilePath;
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = _directory + filename;
    }
    
    Serial.printf("Upload start: %s\n", filename.c_str());
    uploadFilePath = filename; // Store the path for later use
    
    // Check available space
    size_t freeSpace = FILESYSTEM.totalBytes() - FILESYSTEM.usedBytes();
    Serial.printf("Free space: %u bytes\n", freeSpace);
    
    // Create file
    uploadFile = FILESYSTEM.open(filename, "w");
    if (!uploadFile) {
      Serial.println("Failed to open file for writing");
      _server->send(500, "text/plain", "Failed to open file for writing");
      return;
    }
    
    Serial.printf("Upload file opened: %s\n", filename.c_str());
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    // Check if file is open
    if (uploadFile) {
      // Check if total size exceeds limit
      if (upload.totalSize > _maxUploadSize) {
        Serial.printf("File too large: %u bytes\n", upload.totalSize);
        uploadFile.close();
        FILESYSTEM.remove(uploadFilePath); // Clean up the partial file
        _server->send(413, "text/plain", "File too large");
        return;
      }
      
      // Check if we have enough space
      size_t freeSpace = FILESYSTEM.totalBytes() - FILESYSTEM.usedBytes() + uploadFile.size();
      if (upload.totalSize > freeSpace) {
        Serial.printf("Not enough space: %u required, %u available\n", upload.totalSize, freeSpace);
        uploadFile.close();
        FILESYSTEM.remove(uploadFilePath); // Clean up the partial file
        _server->send(507, "text/plain", "Not enough storage space");
        return;
      }
      
      // Write file data
      size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
      if (bytesWritten != upload.currentSize) {
        Serial.printf("Write error: %u/%u bytes written\n", bytesWritten, upload.currentSize);
      }
      Serial.printf("Upload write: %u bytes, total: %u bytes\n", upload.currentSize, upload.totalSize);
    } else {
      Serial.println("File not open for writing");
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    // Close file
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("Upload end: %u bytes\n", upload.totalSize);
      _server->send(200, "text/plain", "File uploaded successfully");
    } else {
      Serial.println("File not open for closing");
      _server->send(500, "text/plain", "Upload failed");
    }
  } 
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    Serial.println("Upload aborted");
    if (uploadFile) {
      uploadFile.close();
      // Clean up the partial file
      FILESYSTEM.remove(uploadFilePath);
    }
    _server->send(400, "text/plain", "Upload aborted");
  }
}

void FMS_FileManager::handleFileDelete() {
  if (!_server->hasArg("file")) {
    _server->send(400, "text/plain", "Missing file parameter");
    return;
  }
  
  String filename = _server->arg("file");
  if (!filename.startsWith("/")) {
    filename = _directory + filename;
  }
  
  if (!FILESYSTEM.exists(filename)) {
    _server->send(404, "text/plain", "File not found");
    return;
  }
  
  if (FILESYSTEM.remove(filename)) {
    _server->send(200, "text/plain", "File deleted");
    Serial.printf("File deleted: %s\n", filename.c_str());
  } else {
    _server->send(500, "text/plain", "Failed to delete file");
    Serial.printf("Failed to delete file: %s\n", filename.c_str());
  }
}

void FMS_FileManager::handleFileDownload() {
  if (!_server->hasArg("file")) {
    _server->send(400, "text/plain", "Missing file parameter");
    return;
  }
  
  String filename = _server->arg("file");
  if (!filename.startsWith("/")) {
    filename = _directory + filename;
  }
  
  if (!FILESYSTEM.exists(filename)) {
    _server->send(404, "text/plain", "File not found");
    return;
  }
  
  File file = FILESYSTEM.open(filename, "r");
  if (!file) {
    _server->send(500, "text/plain", "Failed to open file");
    return;
  }
  
  String baseName = filename;
  int lastSlash = filename.lastIndexOf('/');
  if (lastSlash >= 0) {
    baseName = filename.substring(lastSlash + 1);
  }
  
  _server->sendHeader("Content-Disposition", "attachment; filename=\"" + baseName + "\"");
  _server->sendHeader("Content-Type", getContentType(filename));
  _server->sendHeader("Content-Length", String(file.size()));
  _server->streamFile(file, getContentType(filename));
  
  file.close();
}

String FMS_FileManager::getContentType(const String& filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/pdf";
  else if (filename.endsWith(".zip")) return "application/zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".txt")) return "text/plain";
  return "application/octet-stream";
}

String FMS_FileManager::formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0, 2) + " KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0, 2) + " MB";
  }
  return String(bytes / 1024.0 / 1024.0 / 1024.0, 2) + " GB";
}