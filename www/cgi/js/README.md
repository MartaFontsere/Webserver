# 📁 CGI Testing - JavaScript Modules

## 🎯 Overview

This directory contains the modular JavaScript architecture for the CGI Testing interface. Each module has a specific responsibility following the separation of concerns principle.

## 📂 Module Structure

### 🔧 `cgi_config.js` - Configuration & Constants
**Purpose**: Central configuration management and constants
**Exports**: `CGI_CONFIG` object

#### Key Features:
- Default settings and timeouts
- CGI script paths and endpoints
- Quick script configurations
- HTTP methods and content types
- Common environment variables
- Status code mappings
- Simulation responses for development
- UI messages and storage keys

#### Configuration Sections:
```javascript
CGI_CONFIG = {
    DEFAULT_SCRIPT: '/cgi-bin/mini.py',
    REQUEST_TIMEOUT: 30000,
    MAX_HISTORY_ITEMS: 50,
    SCRIPTS: { 
        // Python scripts
        MINI: '/cgi-bin/mini.py',
        ENV_PY: '/cgi-bin/env.py',
        FORM_PY: '/cgi-bin/form.py',
        // Shell scripts
        HELLO_SH: '/cgi-bin/hello.sh',
        // C++ executables
        HELLO_CGI: '/cgi-bin/hello.cgi'
    },
    QUICK_SCRIPTS: [
        '🚀 Mini Test (Python)',
        '� Hello (Shell)', 
        '⚡ Hello (C++)',
        '🌍 Environment (Python)',
        '📝 Form (Python)'
    ],
    HTTP_METHODS: ['GET', 'POST'],
    CONTENT_TYPES: { ... },
    SIMULATION_RESPONSES: { ... }
}
```

### 🎛️ `cgi_handlers.js` - Event Handlers & UI Logic
**Purpose**: User interaction and event management
**Exports**: `CGI_HANDLERS` object

#### Key Features:
- Script execution orchestration
- Parameter management (query, POST, environment)
- Quick script loading
- Output view switching
- History interaction
- Keyboard shortcuts
- Real and simulated script execution
- Result display and formatting

#### Main Methods:
- `init()`: Initialize all event handlers
- `executeScript()`: Execute CGI script with current configuration
- `performExecution()`: Handle real vs simulated execution
- `simulateExecution()`: Generate realistic test responses
- `displayResults()`: Show execution results in UI
- `switchView()`: Change between output views
- `addParameter()`: Add parameter rows dynamically
- `loadQuickScript()`: Load predefined script configuration

#### Simulation Types:
- **Mini Script (Python)**: Basic testing script with dynamic parameters
- **Environment Script (Python)**: System environment variables display
- **Form Script (Python)**: POST data processing simulation
- **Shell Script (Bash)**: Shell script execution simulation
- **C++ Executable**: Compiled binary execution simulation
- **Error Script**: Error condition testing (405, 500 errors)
- **Generic Script**: Fallback for undefined scripts

### 🛠️ `cgi_utils.js` - Utility Functions
**Purpose**: Helper functions and common operations
**Exports**: `CGI_UTILS` object

#### Key Features:
- Status indicator management
- Message display system
- Data formatting (bytes, time, timestamps)
- HTML escaping and syntax highlighting
- Clipboard operations
- File download functionality
- Validation helpers
- Debugging utilities

#### Utility Categories:

##### UI Helpers:
- `showIndicator()`: Display loading/success/error indicators
- `showMessage()`: Show temporary notifications
- `updateExecutionStatus()`: Update status display

##### Data Formatting:
- `formatBytes()`: Human-readable byte sizes
- `formatTime()`: Milliseconds to readable time
- `formatTimestamp()`: Date formatting for display
- `escapeHtml()`: Safe HTML escaping

##### Validation:
- `validateScriptPath()`: CGI script path validation
- `validateEnvVarName()`: Environment variable name validation
- `isValidJson()`: JSON format validation

##### Browser Operations:
- `copyToClipboard()`: Copy text to clipboard
- `downloadAsFile()`: Download content as file
- `getBrowserInfo()`: Browser environment information

##### Development:
- `debug()`: Conditional debug logging
- `delay()`: Promise-based delays
- `randomBetween()`: Random number generation

### 📚 `cgi_history.js` - History Management
**Purpose**: Execution history tracking and persistence
**Exports**: `CGI_HISTORY` object

#### Key Features:
- Persistent history storage (localStorage)
- History item rendering and interaction
- Search and filtering capabilities
- Import/export functionality
- Execution loading and replay
- Statistics and analytics

#### Main Operations:
- `addExecution()`: Add new execution to history
- `loadExecution()`: Restore previous execution configuration
- `clear()`: Clear all history
- `export()`: Download history as JSON
- `import()`: Load history from file
- `search()`: Filter history by criteria

#### History Item Structure:
```javascript
{
    id: 'exec_timestamp_random',
    timestamp: '2025-01-XX...',
    script: '/cgi-bin/script.py',
    method: 'GET|POST',
    queryParams: { ... },
    postData: { ... },
    envVars: { ... },
    result: { ... },
    success: true|false
}
```

#### Features:
- **Persistent Storage**: Survives browser sessions
- **Size Limiting**: Automatic cleanup of old entries
- **Rich Display**: Detailed execution information
- **Interactive Loading**: Click to restore configuration
- **Export/Import**: JSON-based data exchange
- **Search**: Filter by script, method, status

### 🚀 `cgi_main.js` - Main Application Controller
**Purpose**: Application initialization and coordination
**Exports**: `CGI_APP` object

#### Key Features:
- Module initialization orchestration
- Backend connectivity detection
- Mode switching (simulation/production)
- Error handling and reporting
- User preference management
- Application lifecycle management

#### Initialization Flow:
1. **Module Loading**: Verify all modules are available
2. **UI Setup**: Initialize interface components
3. **Backend Check**: Test connectivity and set mode
4. **Configuration**: Load settings and preferences
5. **Ready State**: Mark application as initialized

#### Mode Detection:
- **Production Mode**: When backend endpoint responds to health check
- **Simulation Mode**: When backend is unavailable (fallback)
- **Automatic Switching**: Seamless mode detection and UI updates

#### Error Handling:
- **Global Error Capture**: Window error and promise rejection handlers
- **Contextual Logging**: Detailed error information with context
- **User Feedback**: Friendly error messages in UI
- **Backend Reporting**: Optional error reporting in production mode

## 🔄 Module Interactions

### Initialization Chain:
1. **DOM Ready** → `CGI_APP.init()`
2. **App Init** → `CGI_HISTORY.init()` → `CGI_HANDLERS.init()`
3. **Handlers Init** → Setup event listeners and UI bindings
4. **History Init** → Load saved history and render UI
5. **Backend Check** → Test connectivity and set mode
6. **Ready State** → Application ready for use

### Data Flow:
```
User Input → CGI_HANDLERS → CGI_UTILS (validation)
          ↓
CGI_CONFIG (settings) → Execution Logic → Backend/Simulation
          ↓
Results → CGI_HANDLERS (display) → CGI_HISTORY (storage)
```

### Event Flow:
- **UI Events** → `CGI_HANDLERS` → Business logic
- **Execution** → `CGI_HANDLERS.executeScript()` → Backend/Simulation
- **Results** → `CGI_HANDLERS.displayResults()` → UI update
- **History** → `CGI_HISTORY.addExecution()` → Persistence

## 🎨 Architecture Principles

### Separation of Concerns:
- **Config**: Pure data and settings
- **Handlers**: UI logic and user interaction
- **Utils**: Pure functions and helpers
- **History**: Data persistence and management
- **Main**: Application coordination and lifecycle

### Modularity:
- Each module has a single responsibility
- Clear interfaces between modules
- No circular dependencies
- Easy to test and maintain

### Error Handling:
- Graceful degradation
- User-friendly error messages
- Detailed logging for debugging
- Fallback behaviors

### Performance:
- Lazy loading where possible
- Efficient DOM manipulation
- Debounced input handling
- Optimized storage operations

## 🐛 Debugging

### Debug Mode:
Enable debugging by setting `CGI_CONFIG.DEBUG_MODE = true`

### Console Access:
All modules are accessible via browser console:
- `CGI_APP`: Main application controller
- `CGI_CONFIG`: Configuration object
- `CGI_HANDLERS`: Event handlers
- `CGI_UTILS`: Utility functions
- `CGI_HISTORY`: History management

### Logging Levels:
- `CGI_UTILS.debug()`: Debug information (when enabled)
- `CGI_UTILS.error()`: Error messages (always shown)
- `console.log/warn/error`: Standard logging

## 📊 Performance Considerations

### Memory Management:
- History size limits to prevent memory bloat
- Cleanup on page unload
- Efficient DOM updates

### Network Optimization:
- Request timeouts to prevent hanging
- Graceful fallback to simulation mode
- Minimal payload sizes

### Storage Efficiency:
- Compressed history storage
- Automatic cleanup of old entries
- Efficient serialization

## 🔧 Customization

### Adding New Scripts:
1. Add to `CGI_CONFIG.QUICK_SCRIPTS`
2. Add simulation response to `CGI_CONFIG.SIMULATION_RESPONSES`
3. Add button to HTML with appropriate data attributes

### Extending Functionality:
1. Add new methods to appropriate module
2. Update initialization in `CGI_APP.init()`
3. Add configuration options to `CGI_CONFIG`

### Styling Customization:
- Modify CSS classes used in JavaScript
- Update UI messages in `CGI_CONFIG.MESSAGES`
- Customize status indicators and themes

## 📄 Dependencies

### External Dependencies:
- **None**: Pure vanilla JavaScript implementation
- **Browser APIs**: localStorage, fetch, clipboard
- **ES6+ Features**: Promises, async/await, destructuring

### Internal Dependencies:
- **Config** → Used by all modules
- **Utils** → Used by handlers and history
- **Handlers** ↔ **History**: Bidirectional communication
- **Main** → Coordinates all modules

## 🔒 Security Considerations

### Input Validation:
- Script path validation
- Parameter sanitization
- Environment variable validation

### XSS Prevention:
- HTML escaping for all user input
- Safe DOM manipulation
- Content Security Policy compliance

### Data Privacy:
- Local storage only (no external tracking)
- Optional error reporting
- User consent for data operations

## 📋 Active Scripts & Simulations

### Current Quick Scripts:
1. **🚀 Mini Test (Python)** - `/cgi-bin/mini.py`
   - **Method**: GET
   - **Purpose**: Basic testing script with parameter processing
   - **Simulation**: Dynamic parameter parsing and output

2. **🐚 Hello Shell** - `/cgi-bin/hello.sh`
   - **Method**: GET  
   - **Purpose**: Bash shell script execution
   - **Simulation**: Shell script output with environment info

3. **⚡ Hello C++** - `/cgi-bin/hello.cgi`
   - **Method**: GET
   - **Purpose**: Compiled C++ executable
   - **Simulation**: Binary execution with source code display

4. **🌍 Environment (Python)** - `/cgi-bin/env.py`
   - **Method**: GET
   - **Purpose**: Display CGI environment variables
   - **Simulation**: Standard CGI environment output

5. **📝 Form Handler (Python)** - `/cgi-bin/form.py`
   - **Method**: POST
   - **Purpose**: Process form data and POST requests
   - **Simulation**: HTML response with form data confirmation

### Physical Scripts Present:
- **`mini.py`**: Only actual script file in the repository
- **Others**: Simulated for testing purposes (will be replaced with real scripts when backend is ready)

### Error Simulations:
- **405 Method Not Allowed**: Triggered for incorrect method usage
- **500 Internal Server Error**: Simulated server errors
- **Timeout**: Network timeout simulation
- **Network Error**: Connection failure simulation
