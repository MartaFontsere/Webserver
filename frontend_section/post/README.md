# 📝 POST Testing Module

## Overview
This module provides a comprehensive interface for testing HTTP POST requests in the webserver. It allows users to send POST, PUT, and PATCH requests with different content types and analyze the responses.

## Files Structure

### HTML
- `test_post.html` - Main interface for POST request testing

### CSS
- `../css/post_styles.css` - Styling for the POST testing interface

### JavaScript
- `post_config.js` - Configuration constants and settings
- `post_utils.js` - Utility functions for data manipulation and formatting
- `post_handlers.js` - Event handlers and API interaction logic
- `post_history.js` - Request history management
- `post_main.js` - Main application initialization and event binding

## Features

### 🌐 Request Configuration
- HTTP Method selection (POST, PUT, PATCH)
- URL configuration with validation
- Content-Type selection with custom options
- Custom headers management

### 📝 Request Body Editors
- **Raw Editor**: Plain text input for any content type
- **Form Data Editor**: Key-value pair interface for form data
- **JSON Editor**: Specialized JSON editor with formatting and validation

### ⚡ Quick Tests
Pre-configured test scenarios:
- 👤 JSON User Data
- 🔐 Form Login
- 📦 JSON Product
- 📧 Form Contact
- 💬 Text Message
- 📋 JSON Array

### 📨 Response Analysis
- **Body Tab**: Formatted response content
- **Headers Tab**: Response headers display
- **Info Tab**: Status, timing, and size information

### 📚 Request History
- Automatic saving of requests and responses
- History browsing and replay
- Export functionality
- Clear history option

### 🛠️ Advanced Features
- JSON validation and formatting
- Response content formatting
- Copy to clipboard functionality
- Download responses as files
- Keyboard shortcuts support
- Real-time request timing

## Usage

### Basic POST Request
1. Enter the target URL
2. Select POST method
3. Choose content type
4. Enter request body
5. Click "Send POST Request"

### Quick Testing
Click any of the Quick Test buttons to load pre-configured test scenarios.

### Form Data
1. Switch to "Form Data" tab
2. Add key-value pairs
3. Content-Type automatically set to `application/x-www-form-urlencoded`

### JSON Testing
1. Switch to "JSON" tab
2. Enter JSON data
3. Use "Format JSON" to prettify
4. Use "Validate JSON" to check syntax

### Custom Headers
1. Add custom headers in the Headers section
2. Headers will be included in the request

## Backend Integration Points

### Required Endpoints
The backend should implement these endpoints for testing:

```
POST /test-post
POST /api/users
POST /auth/login
POST /api/products
POST /contact/submit
POST /api/messages
POST /api/batch
PUT /test-put
PATCH /test-patch
```

### Integration Comments in HTML
The `test_post.html` file contains detailed backend integration comments:
- **BACKEND INTEGRATION POINT #1**: Main POST request testing interface
- **BACKEND INTEGRATION POINT #2**: Quick test endpoints with specific data formats

### Expected Response Format
The backend should return responses in standard HTTP format:
- Status codes (200, 201, 400, 500, etc.)
- Content-Type headers
- Response body (JSON, text, etc.)

### Error Handling
The interface handles various error scenarios:
- Network errors
- Invalid JSON
- Server errors
- Timeout errors

## Configuration

### POST_CONFIG Object
Main configuration in `post_config.js`:
- Default endpoints
- Content types
- Quick test templates
- Response formatting options
- History settings

### Customization
To add new quick tests, modify the `QUICK_TESTS` object in `post_config.js`.

## Keyboard Shortcuts
- `Ctrl/Cmd + Enter`: Send request
- `Ctrl/Cmd + Shift + F`: Format JSON
- `Ctrl/Cmd + Shift + V`: Validate JSON
- `Ctrl/Cmd + Shift + C`: Copy response

## Browser Compatibility
- Modern browsers with ES6+ support
- Fetch API support required
- Local storage for history persistence

## Development Notes
- Follows the same architecture as GET and CGI modules
- Modular JavaScript structure for maintainability
- Responsive design for mobile compatibility
- Error handling and user feedback throughout
