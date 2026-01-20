# 📁 CGI Testing Module

## 🎯 Overview

The CGI Testing module provides a comprehensive interface for testing and executing CGI scripts on the webserver. It offers both simulation and production modes, allowing developers to test CGI functionality locally and against real backend implementations.

## 🚀 Features

### Core Functionality
- **Script Execution**: Execute CGI scripts with GET and POST methods
- **Parameter Management**: Dynamic query parameters and environment variables
- **Multiple Content Types**: Support for form data, JSON, plain text, and multipart data
- **Real-time Output**: View script output, headers, errors, and environment variables
- **Execution History**: Persistent history with search and export capabilities

### Quick Scripts
- 🚀 **Mini Test (Python)**: Basic CGI script testing with parameter processing
- 🐚 **Hello Shell**: Bash shell script execution testing  
- ⚡ **Hello C++**: Compiled C++ executable testing
- 🌍 **Environment (Python)**: Display CGI environment variables
- 📝 **Form Handler (Python)**: Process POST form data

### Execution Modes
- **🧪 Simulation Mode**: Local testing with simulated responses
- **🌐 Production Mode**: Real backend communication (when available)

## 📂 File Structure

```
/cgi/
├── test_cgi.html          # Main CGI testing interface
├── README.md              # This documentation
└── js/                    # JavaScript modules
    ├── cgi_config.js      # Configuration and constants
    ├── cgi_handlers.js    # Event handlers and UI logic
    ├── cgi_utils.js       # Utility functions and helpers
    ├── cgi_history.js     # History management
    ├── cgi_main.js        # Main application initialization
    └── README.md          # JavaScript modules documentation
```

## 🛠️ Usage

### Basic Script Execution
1. **Select Script Path**: Enter or select a CGI script path (e.g., `/cgi-bin/hello.py`)
2. **Choose Method**: Select GET or POST method
3. **Configure Parameters**: Add query parameters or POST data as needed
4. **Set Environment**: Configure custom environment variables if required
5. **Execute**: Click the "⚙️ Execute Script" button

### Quick Scripts
Use the predefined quick script buttons for common testing scenarios:
- Click any quick script button to auto-configure the interface
- Scripts are automatically loaded with appropriate method and parameters

### Parameter Configuration

#### Query Parameters (GET)
- Add key-value pairs for URL query parameters
- Parameters are automatically URL-encoded
- Remove parameters using the 🗑️ button

#### POST Data
- Select appropriate Content-Type from dropdown
- Enter data in the textarea (format depends on content type)
- Supports various formats: form data, JSON, plain text, multipart

#### Environment Variables
- Add custom environment variables for script execution
- Common CGI environment variables are automatically set
- Variables are passed to the CGI script during execution

### Output Analysis

#### Output Views
- **📄 Output**: Raw script output including headers and content
- **📋 Headers**: HTTP headers returned by the script
- **❌ Errors**: Error messages and debugging information
- **🌍 Environment**: Environment variables available to the script

#### Execution Information
- **Status**: HTTP status code and message
- **Execution Time**: Time taken to execute the script
- **Output Size**: Size of the generated output

## 📚 History Management

### Features
- **Persistent Storage**: History is saved locally and persists between sessions
- **Search**: Find specific executions by script path, method, or status
- **Export**: Download history as JSON file for analysis
- **Load Previous**: Restore previous execution configuration
- **Copy Details**: Copy execution details to clipboard

### History Operations
- **🔄 Load**: Restore script configuration and results
- **📋 Copy**: Copy execution details to clipboard
- **🗑️ Remove**: Delete specific execution from history
- **💾 Export**: Download complete history as JSON

## ⌨️ Keyboard Shortcuts

- **Ctrl/Cmd + Enter**: Execute current script
- **Ctrl/Cmd + K**: Clear execution history
- **Ctrl/Cmd + E**: Export history to file

## 🔧 Configuration

### Backend Integration
The module automatically detects backend availability:
- **Production Mode**: When backend is available at configured endpoint
- **Simulation Mode**: When backend is unavailable (default for development)

#### Automatic Mode Detection
The application automatically determines the operating mode during initialization:

1. **Backend Endpoint Check**: Verifies if `CGI_CONFIG.CGI_ENDPOINT` is configured
2. **Health Check**: Attempts to ping the backend at `/health` endpoint  
3. **Mode Selection**:
   - ✅ **Production Mode**: Backend responds successfully to health check
   - 🧪 **Simulation Mode**: Backend not configured or not responding

#### Visual Mode Indicator
A mode indicator appears below the navigation showing:
- **🌐 Production**: Connected to real backend server
- **🧪 Simulation**: Using simulated responses for testing

### Customization
Configure behavior by modifying `cgi_config.js`:
- Script paths and endpoints
- Timeout values and limits
- Simulation responses
- UI messages and labels

## 📊 Simulation Mode

When the backend is not available, the module provides realistic simulated responses:

### Simulated Scripts
- **mini.py**: Basic testing script with dynamic parameter processing (Python)
- **env.py**: Displays CGI environment variables and server settings (Python)
- **form.py**: Processes and echoes POST form data (Python)
- **hello.sh**: Basic shell script execution with environment info (Bash)
- **hello.cgi**: Compiled C++ executable with source code display (C++)

### Response Characteristics
- Realistic execution times (50-300ms)
- Proper HTTP headers and status codes
- Environment variable simulation
- Error condition handling

## 🎨 User Interface

### Design Features
- **Responsive Layout**: Works on desktop and mobile devices
- **Visual Feedback**: Clear status indicators and progress feedback
- **Intuitive Controls**: Easy-to-use parameter management
- **Professional Styling**: Consistent with overall webserver theme

### Status Indicators
- **🔄 Loading**: Script execution in progress
- **✅ Success**: Script executed successfully
- **❌ Error**: Execution failed or error occurred

## 🔗 Integration

### Backend Requirements
For production mode, the backend should provide:
- **POST /cgi-execute**: Execute CGI scripts with parameters
- **GET /health**: Health check endpoint
- **POST /error-report**: Error reporting (optional)

### Expected Request Format
```json
{
  "script": "/cgi-bin/script.py",
  "method": "GET|POST",
  "queryParams": {"key": "value"},
  "postData": {"content": "data", "contentType": "type"},
  "envVars": {"VAR": "value"},
  "timestamp": "2025-01-XX..."
}
```

### Expected Response Format
```json
{
  "status": "200 OK",
  "success": true,
  "headers": {"Content-Type": "text/html"},
  "output": "script output",
  "errors": "error messages",
  "environment": {"VAR": "value"},
  "executionTime": 150,
  "outputSize": 1024
}
```

## 🐛 Troubleshooting

### Common Issues
1. **Script Path Validation**: Ensure paths start with `/` and don't contain `..`
2. **Backend Connectivity**: Check network connection for production mode
3. **Parameter Format**: Verify parameter names and values are properly formatted
4. **Browser Storage**: Ensure localStorage is available for history persistence

### Debug Mode
Enable debug logging by setting `CGI_CONFIG.DEBUG_MODE = true` in the browser console.

## 📄 License

Part of the 42 Barcelona Webserver Project. Developed with ❤️ for comprehensive CGI testing and validation.
