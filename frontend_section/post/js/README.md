# 📝 POST Testing - JavaScript Modules

## Module Overview
This directory contains the JavaScript modules for the POST request testing functionality. The code is organized into separate modules for better maintainability and separation of concerns.

## File Structure

### 🔧 post_config.js
**Purpose**: Configuration and constants
- API endpoints configuration
- Content type definitions
- Quick test templates
- Default headers and settings
- Error messages
- UI configuration

**Key Features**:
- Centralized configuration management
- Pre-defined test scenarios
- Customizable settings
- Status code categorization

### 🛠️ post_utils.js
**Purpose**: Utility functions and helpers
- JSON formatting and validation
- Data conversion utilities
- File size and duration formatting
- Clipboard and download operations
- Notification system
- URL validation

**Key Functions**:
- `formatJSON()` - Format JSON with proper indentation
- `isValidJSON()` - Validate JSON syntax
- `objectToUrlEncoded()` - Convert objects to URL-encoded strings
- `formatFileSize()` - Human-readable file sizes
- `copyToClipboard()` - Cross-browser clipboard operations
- `downloadAsFile()` - File download functionality

### 🎯 post_handlers.js
**Purpose**: Event handlers and API interactions
- HTTP request handling
- UI state management
- Form data processing
- Response display logic
- Tab switching functionality
- Content type handling

**Key Functions**:
- `sendRequest()` - Main API request function
- `runQuickTest()` - Execute predefined tests
- `getCurrentRequestConfig()` - Extract UI state
- `displayResponse()` - Update response display
- `formatJSON()` - JSON editor formatting
- `addFormRow()` - Dynamic form management

### 📚 post_history.js
**Purpose**: Request history management
- localStorage integration
- History display and navigation
- Request replay functionality
- History export/import
- Entry management

**Key Functions**:
- `saveToHistory()` - Save request/response pairs
- `loadFromHistory()` - Restore previous requests
- `exportHistory()` - Export history as JSON
- `clearHistory()` - Remove all history entries
- `updateHistoryDisplay()` - Refresh UI display

### 🚀 post_main.js
**Purpose**: Application initialization and coordination
- DOM ready handling
- Event listener binding
- Keyboard shortcuts
- Application lifecycle
- Error handling coordination

**Key Functions**:
- `init()` - Initialize the application
- `bindEventListeners()` - Set up all event handlers
- `handleSendRequest()` - Main request workflow
- `validateRequest()` - Request validation
- `handleKeyboardShortcuts()` - Keyboard interaction

## Architecture

### Module Dependencies
```
post_main.js
├── post_config.js (configuration)
├── post_utils.js (utilities)
├── post_handlers.js (API & UI)
└── post_history.js (persistence)
```

### Data Flow
1. **User Input** → UI elements capture user configuration
2. **Validation** → Request data is validated
3. **API Call** → HTTP request is sent via fetch()
4. **Response** → Response is processed and displayed
5. **History** → Request/response is saved to localStorage
6. **UI Update** → Interface reflects current state

### Error Handling
- Network errors (connection issues)
- JSON parsing errors (invalid syntax)
- Validation errors (empty URLs, invalid data)
- Server errors (HTTP error codes)
- Browser compatibility issues

## Event System

### UI Events
- Button clicks (send, format, validate)
- Tab switching (raw, form, json)
- Form management (add/remove rows)
- Content type changes
- History interactions

### Keyboard Events
- `Ctrl+Enter` - Send request
- `Ctrl+Shift+F` - Format JSON
- `Ctrl+Shift+V` - Validate JSON
- `Ctrl+Shift+C` - Copy response

### Custom Events
- Request completion
- History updates
- Error notifications
- State changes

## Configuration Options

### Request Settings
```javascript
POST_CONFIG.DEFAULT_ENDPOINTS = {
    POST: '/test-post',
    PUT: '/test-put',
    PATCH: '/test-patch'
};
```

### UI Behavior
```javascript
POST_CONFIG.UI = {
    ANIMATION_DURATION: 300,
    DEBOUNCE_DELAY: 500,
    AUTO_FORMAT_JSON: true,
    SHOW_REQUEST_TIME: true
};
```

### History Management
```javascript
POST_CONFIG.HISTORY = {
    MAX_ENTRIES: 50,
    STORAGE_KEY: 'post_request_history',
    AUTO_SAVE: true
};
```

## Extending the Module

### Adding New Quick Tests
Modify `POST_CONFIG.QUICK_TESTS` in `post_config.js`:
```javascript
'new-test': {
    name: '🆕 New Test',
    method: 'POST',
    url: '/new-endpoint',
    contentType: 'application/json',
    body: JSON.stringify({ data: 'example' }, null, 2)
}
```

### Adding New Content Types
Extend `POST_CONFIG.CONTENT_TYPES`:
```javascript
CONTENT_TYPES: {
    XML: 'application/xml',
    YAML: 'application/x-yaml'
}
```

### Custom Validation
Add validation logic in `post_main.js`:
```javascript
validateRequest(requestConfig) {
    // Custom validation logic here
    return { valid: true/false, message: 'Error message' };
}
```

## Browser Compatibility
- **ES6+** features used (arrow functions, destructuring, etc.)
- **Fetch API** required for HTTP requests
- **LocalStorage** for history persistence
- **Clipboard API** with fallback for older browsers

## Performance Considerations
- Debounced input handlers to prevent excessive calls
- Lazy loading of large responses
- History size limits to prevent memory issues
- Request timeout handling
- Response truncation for large payloads

## Security Notes
- XSS protection via HTML escaping
- CSRF tokens should be added via custom headers
- Content validation before sending
- Safe JSON parsing with error handling
