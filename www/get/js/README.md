# 📜 GET Testing - JavaScript Modules

Modular JavaScript architecture for the GET request testing interface. This directory contains all the client-side logic organized following MVC patterns and separation of concerns.

## 📁 Module Structure

```
js/
├── get_config.js       # 🔧 Configuration and constants
├── get_handlers.js     # 🌐 HTTP request handling logic  
├── get_utils.js        # 🛠️ Utility functions and helpers
├── get_history.js      # 📚 History management and persistence
├── get_main.js         # 🚀 Main application and event coordination
└── README.md           # This documentation
```

## 🔧 **get_config.js**

**Purpose**: Central configuration and constants  
**Size**: ~150 lines  
**Dependencies**: None (standalone)

### **Key Components**
- ⚙️ **Default settings** (URLs, timeouts, limits)
- 🌐 **Endpoint definitions** for backend integration
- ⚡ **Quick test configurations** with predefined URLs
- 📊 **HTTP status code mappings** with UI styling
- 🎨 **Content-type configurations** for response handling
- 📋 **UI messages** and text constants
- 💾 **Storage keys** for localStorage

### **Usage**
```javascript
// Access configuration anywhere
const timeout = GET_CONFIG.REQUEST_TIMEOUT;
const endpoints = GET_CONFIG.ENDPOINTS;
const statusInfo = GET_CONFIG.STATUS_CODES[404];
```

## 🌐 **get_handlers.js**

**Purpose**: Core HTTP request handling and simulation  
**Size**: ~320 lines  
**Dependencies**: get_config.js

### **Key Components**
- 🎯 **GetRequestHandler class** - Main request processor
- 🔄 **Simulation mode** with realistic mock responses
- 🌍 **Production mode** using native fetch API
- ⏱️ **Timeout handling** and error management
- 📊 **Response processing** and standardization
- 🎭 **Mock data generators** for different content types

### **Key Methods**
```javascript
// Send request (auto-detects mode)
await getRequestHandler.sendRequest(url)

// Switch between modes
getRequestHandler.isSimulationMode = false; // Production mode
```

### **Simulation Triggers**
- `/api/status` → JSON server information
- `/files/` → JSON file listing
- `"404"/"nonexistent"` → 404 error page
- `"403"/"forbidden"` → 403 error page
- `"500"/"error"` → 500 error page

## 🛠️ **get_utils.js**

**Purpose**: Utility functions and data formatting  
**Size**: ~360 lines  
**Dependencies**: get_config.js

### **Key Utilities**
- 📏 **Data formatting**: bytes, duration, timestamps
- 🔍 **URL validation** and normalization
- 📋 **Status code** and content-type information
- 🎨 **JSON/XML formatting** with error handling
- 🔒 **HTML escaping** for security
- 📋 **Clipboard operations** and file downloads
- ⏱️ **Debouncing** and performance helpers
- 📢 **Notification system** for user feedback

### **Usage Examples**
```javascript
// Format data
GetUtils.formatBytes(1024);        // "1.00 KB"
GetUtils.formatDuration(1500);     // "1.50s"

// Validate and normalize
GetUtils.isValidUrl('/api/test');  // true
GetUtils.normalizeUrl('api/test'); // "/api/test"

// Format content
GetUtils.tryFormatJson('{"test":1}'); // Indented JSON
GetUtils.escapeHtml('<script>');      // Safe HTML
```

## 📚 **get_history.js**

**Purpose**: Request history management and persistence  
**Size**: ~180 lines  
**Dependencies**: get_config.js, get_utils.js

### **Key Components**
- 🗄️ **GetHistory class** - History management
- 💾 **localStorage integration** with error handling
- 📊 **Request tracking** with metadata
- 🔄 **Replay functionality** for repeated testing
- 📤 **Export/Import** as structured JSON
- 🧮 **Statistics calculation** and analytics
- 🔍 **Filtering and search** capabilities

### **Features**
- **Auto-cleanup**: Max 50 items with oldest-first removal
- **Rich metadata**: Timestamps, sizes, success rates
- **Crash-safe**: Graceful localStorage error handling
- **Export format**: Structured JSON with metadata

### **Usage**
```javascript
// Add request to history
getHistory.addRequest(responseData);

// Replay from history  
getHistory.replayRequest(historyItemId);

// Export/clear
getHistory.exportHistory();
getHistory.clearHistory();
```

## 🚀 **get_main.js**

**Purpose**: Application initialization and event coordination  
**Size**: ~340 lines  
**Dependencies**: All other modules

### **Key Responsibilities**
- 🎬 **Application initialization** on DOM ready
- 🔧 **Event listener setup** for all UI interactions
- 🎛️ **Response view controls** (Raw/Formatted/Preview)
- 📤 **Request orchestration** and UI updates
- 🎨 **Dynamic content rendering** based on response type
- ⌨️ **Keyboard shortcuts** handling
- 🔗 **Module coordination** and state management

### **Global Functions**
```javascript
// Main request function
sendGetRequest()           // Send request from current URL input

// UI update functions  
updateResponseDisplay(response)    // Update all response sections
updateStatusInfo(response)         // Update status/timing info
updateResponseBodyView(body, type) // Handle view mode switching
```

### **Event Handling**
- **Button clicks**: Send request, quick tests, view switching
- **Keyboard shortcuts**: Enter (send), Ctrl+Enter (send), Escape (hide)
- **History interactions**: Item clicks, export, clear
- **View mode switching**: Raw/Formatted/Preview buttons

## 🔄 **Module Interactions**

```
get_main.js (coordinator)
    ├── get_config.js (constants)
    ├── get_handlers.js (HTTP requests)
    │   └── get_config.js
    ├── get_utils.js (formatting)
    │   └── get_config.js  
    └── get_history.js (persistence)
        ├── get_config.js
        └── get_utils.js
```

## 🏗️ **Architecture Patterns**

### **🎯 Single Responsibility**
- Each module has one clear purpose
- No overlap in functionality
- Clean separation of concerns

### **📦 Module Pattern**
- Self-contained functionality
- Minimal global scope pollution
- Clear dependency chains

### **🔧 Configuration-Driven**
- Centralized settings in get-config.js
- Easy to modify behavior
- Environment-specific adjustments

### **🛡️ Error Boundary**
- Graceful degradation on failures
- No single point of failure
- Comprehensive error handling

## 🔧 **Development Notes**

### **Adding New Features**
1. **Configuration**: Add constants to `get_config.js`
2. **Logic**: Implement in appropriate module
3. **UI**: Update `get_main.js` event handlers
4. **Persistence**: Extend `get_history.js` if needed

### **Debugging**
- **Console logs**: Each module logs initialization
- **Error boundaries**: Failures are caught and logged
- **State inspection**: Global instances accessible in console

### **Performance**
- **Lazy loading**: Modules load only when needed
- **Debounced events**: Prevents excessive API calls
- **Efficient DOM**: Minimal manipulations
- **Memory management**: Cleanup of event listeners

## 🧪 **Testing Strategy**

### **Unit Testing Ready**
- **Pure functions**: Most utilities are testable
- **Mocked dependencies**: Simulation mode for testing
- **State isolation**: No shared mutable state
- **Error scenarios**: Comprehensive error handling

### **Integration Points**
- **localStorage**: Graceful fallback on failure
- **Fetch API**: Timeout and error handling
- **DOM**: Event delegation and cleanup
- **Browser APIs**: Feature detection

---

## 📈 **Module Status**

- **get_config.js**: ✅ **Stable** - No changes needed
- **get_handlers.js**: ✅ **Stable** - Production ready
- **get_utils.js**: ✅ **Stable** - Comprehensive utilities
- **get_history.js**: ✅ **Stable** - Full persistence
- **get_main.js**: ✅ **Stable** - Complete event handling

**Last Updated**: November 4, 2025  
**Architecture**: Modular ES6+  
**Status**: Production ready for C++ backend integration

---

**Built with modern JavaScript patterns for maintainability and extensibility.** 🚀
