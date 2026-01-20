# 📂 JavaScript Modules - Directory Explorer

This directory contains the modular JavaScript architecture for the directory explorer system. The code follows a clean separation of concerns for better maintainability and scalability.

## � Module Structure

### ⚙️ `directory_config.js`
**Configuration & Constants**
- Global configuration object `DirectoryConfig`
- API endpoint definitions
- UI settings and file type mappings
- Simulation/production mode settings

### 🛠️ `directory_utils.js`
**Utility Functions & Helpers**
- `formatFileSize()` - Human-readable file sizes
- `formatDate()` - Date formatting for timestamps
- `getFileIcon()` - File type icon selection
- `sanitizePath()` - Path validation and cleaning
- `debounce()` - Function debouncing for search
- `showMessage()` - User notification system

### 🎯 `directory_handlers.js`
**Event Handling & API Communication**
- File and folder interaction handlers
- Context menu management
- API request/response handling
- File operations (download, delete, properties)
- Error handling and user feedback
- Progress tracking for operations

### 🧭 `directory_navigation.js`
**Navigation Logic & History**
- Path navigation and routing
- Breadcrumb generation and management
- History tracking (back/forward)
- Bookmark system
- Address bar functionality
- Search and filtering logic

### 🚀 `directory_main.js`
**Main Controller & Initialization**
- Primary application controller
- Module initialization and coordination
- View switching (grid/list/details)
- Event listener setup
- Integration with other modules
- State management

## 🔄 Module Dependencies

```
directory_main.js
├── directory_handlers.js
├── directory_navigation.js
├── directory_utils.js
└── directory_config.js
```

## 🎯 Integration Points

- **Global Variables**: `DirectoryConfig`, `DirectoryUtils`, etc.
- **Events**: Custom events for inter-module communication
- **DOM**: Centralized element management
- **APIs**: RESTful backend communication ready

