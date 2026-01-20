# 📁 File Upload System - 42 Webserver

Complete file upload system wi└── js/                   # Modular JavaScript
    ├── README.md         # JavaScript documentation
    ├── upload_config.js  # Configuration and state (MODEL)
    ├── dom_manager.js    # DOM element management (VIEW)
    ├── utils.js          # Utility functions (VIEW)
    ├── file_manager.js   # File handling logic (CONTROLLER)
    ├── upload_handlers.js # Upload operations (CONTROLLER)
    ├── server_files.js   # Server communication (CONTROLLER)
    └── event_listeners.js # Event management (CONTROLLER) drop interface, modular JavaScript architecture, and MVC pattern implementation. Ready for C++ backend integration.

## 🎯 Features

### ✨ Upload Functionality
- **Drag & Drop Interface**: Modern drag-and-drop file selection
- **Multiple File Support**: Upload multiple files simultaneously
- **Individual Upload**: Upload files one by one with individual progress
- **Collective Upload**: Upload all selected files together
- **Progress Tracking**: Visual progress bars for both upload modes
- **File Management**: Remove individual files or clear all selections
- **Validation**: File size limits (10MB) and type checking
- **Responsive Design**: Works on all device sizes

### 🗄️ Server File Management
- **File Listing**: View all files currently stored on the server
- **File Information**: Display file names, sizes, types, and upload dates
- **Download Files**: Download files directly from the server
- **Delete Files**: Remove files from the server with confirmation
- **Auto-refresh**: Refresh file list after operations
- **File Icons**: Visual file type indicators

### 🔧 Development Features
- **Simulation Mode**: Test the interface without a backend server
- **Production Mode**: Ready for real backend integration
- **Error Handling**: Robust error handling and user feedback
- **Modular Architecture**: Clean, maintainable codebase

## 🏗️ MVC Architecture

The upload system implements a **Model-View-Controller (MVC)** pattern for clean separation of concerns:

### 📊 **MODEL (Data & State)**
```javascript
// upload_config.js - Application State
let selectedFiles = [];     // Main data model
const CONFIG = { ... };     // Configuration model
```
**Responsibilities:** Manage file selection state, configuration, validation rules

### 🖼️ **VIEW (User Interface)**
```html
<!-- upload.html - Template Structure -->
<div class="drop-zone" id="dropZone">...</div>
<div class="file-list" id="fileList">...</div>
```
**Responsibilities:** HTML structure, CSS styling, DOM element references

### 🎮 **CONTROLLER (Business Logic)**
```javascript
// event_listeners.js + file_manager.js + upload_handlers.js
function handleFiles(files) {
    // Process input → Update model → Refresh view
}
```
**Responsibilities:** Event handling, file operations, server communication

### 🔄 **Data Flow**
```
User Action → Controller → Model Update → View Refresh → User Feedback
```

## 📁 Directory Structure

```
upload/
├── README.md              # This documentation
├── upload.html           # Main upload interface
└── js/                   # Modular JavaScript
    ├── README.md         # JavaScript modules documentation
    ├── upload-config.js  # Configuration and state (MODEL)
    ├── dom-manager.js    # DOM element management (VIEW)
    ├── utils.js          # Utility functions (VIEW)
    ├── file-manager.js   # File handling logic (CONTROLLER)
    ├── upload-handlers.js # Upload operations (CONTROLLER)
    ├── server-files.js   # Server communication (CONTROLLER)
    └── event-listeners.js # Event management (CONTROLLER)
```

## 🌳 DOM Integration

The **DOM (Document Object Model)** is the bridge between HTML and JavaScript:

### **HTML → DOM Tree**
```html
<div class="drop-zone" id="dropZone">
    <h3>Drag files here</h3>
</div>
```
**Becomes:**
```
document → html → body → div#dropZone → h3
```

### **DOM Manipulation Flow**
1. **User drags file** → Browser event
2. **Event listener** → JavaScript function
3. **DOM update** → `dropZone.classList.add('drag-over')`
4. **CSS reaction** → Visual change
5. **User sees feedback** → Blue highlight

## 🔧 Backend Integration (C++ Ready)

### **Required Endpoints**
```cpp
// 1. File Upload
POST /upload
Content-Type: multipart/form-data
Response: {"success": true, "message": "File uploaded", "filename": "file.txt"}

// 2. List Files
GET /files  
Response: {"success": true, "files": [{"name": "file.txt", "size": 1024, ...}]}

// 3. Download File
GET /files/{filename}
Response: File content with proper headers

// 4. Delete File
DELETE /files/{filename}
Response: {"success": true, "message": "File deleted"}
```

### **Integration Steps**
1. **Set production mode**: `CONFIG.SIMULATION_MODE = false`
2. **Implement endpoints** in C++ server
3. **Test with real files** - system ready!

### **Recommended C++ Libraries**
- **cpp-httplib**: Simple HTTP server
- **crow**: Modern web framework  
- **pistache**: High performance
- **nlohmann/json**: JSON handling

## 🚀 Upload Modes

### **Individual Upload**
- **Button**: 📤 Upload button for each file
- **Progress**: Individual progress bar
- **Feedback**: Success/error per file
- **Auto-removal**: Uploaded files removed from list

### **Collective Upload**
- **Button**: 🚀 Upload Files button for all
- **Progress**: Combined progress bar
- **Feedback**: Batch operation results
- **Auto-clearing**: Form cleared after success

### **Validation & Security**
- **File size limit**: 10MB per file (configurable)
- **File type validation**: Configurable in JavaScript
- **Client-side validation**: Before upload
- **Server-side validation**: Backend implementation required

## 🎨 User Interface

### **Design Features**
- **Glass-morphism**: Modern backdrop blur effects
- **Purple gradient**: Professional color scheme (#667eea to #764ba2)
- **Yellow accents**: Highlight colors (#FFE135)
- **Responsive**: Mobile-friendly layout
- **Accessibility**: Keyboard navigation and screen reader support

### **Visual States**
- **Drag-over**: Blue highlight when dragging files
- **File selected**: Green checkmark and file info
- **Uploading**: Progress bars and loading indicators
- **Success**: Green confirmation messages
- **Error**: Red error messages with details

## 🛠️ Development

### **Getting Started**
1. Open `upload.html` in browser
2. Test with simulation mode (default)
3. Switch to production mode when backend ready

### **Configuration**
```javascript
// upload_config.js
const CONFIG = {
    SIMULATION_MODE: true,        // false for production
    MAX_FILE_SIZE: 10 * 1024 * 1024,  // 10MB
    UPLOAD_ENDPOINT: '/upload',   // Backend URL
    FILES_ENDPOINT: '/files'      // File management URL
};
```

### **Testing**
- **Simulation mode**: Test UI without backend
- **File validation**: Try different file sizes/types
- **Error handling**: Test network failures
- **Responsive**: Test on mobile devices

## 📝 Contributing

### **Code Style**
- **ES6+ JavaScript**: Modern syntax
- **Modular design**: One responsibility per file
- **Clear naming**: Self-documenting code
- **Comments**: Document integration points

### **Adding Features**
1. **New functionality** → Create new module in `/js/`
2. **Update model** → Modify `upload_config.js`
3. **Add UI elements** → Update `upload.html`
4. **Handle events** → Add to `event_listeners.js`

---

**🎯 Ready for production backend integration!**  
*Complete frontend system with professional architecture and C++ backend compatibility.*
