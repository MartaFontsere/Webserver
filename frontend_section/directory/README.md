# 📂 Directory Explorer Module

Modern file system explorer interface for the 42 webserver project. Provides comprehensive directory browsing, file management, and system navigation capabilities with a professional, integrated design.

## ✅ **STATUS: COMPLETED & PRODUCTION READY**

Full-featured directory explorer with unified design, modular architecture, and backend integration readiness.

## 🎯 Features

### 🧭 **Navigation & Browsing**
- **Breadcrumb navigation** with clickable path segments
- **Address bar** with direct path input and auto-completion
- **Back/Forward** navigation history
- **Bookmark system** for favorite directories
- **Real-time search** across files and folders

### 👁️ **View Controls**
- **Multiple view modes**: Grid, List, and Details view
- **Advanced filtering** by type, name, date, size
- **Smart sorting** by name, size, date, type (ascending/descending)
- **Hidden files toggle** for system file visibility
- **File type icons** with extension-specific indicators

### 🔧 **File Operations**
- **File preview** for images, videos, audio, documents, and code
- **Direct download** functionality for individual files
- **File management** operations (rename, copy, move, delete)
- **Context menus** with right-click actions
- **Drag & drop** support for file operations
- **Properties panel** with detailed file information

### 🎨 **User Interface**
- **Responsive design** optimized for all screen sizes
- **Modern glassmorphism** effects and animations
- **Smooth transitions** and hover effects
- **Keyboard navigation** support
- **Accessibility features** for screen readers
- **Theme integration** with site-wide styling

## � File Structure

```
directory/
├── explorer.html               # Main directory explorer interface
├── README.md                   # This documentation
└── js/
    ├── directory_config.js     # Configuration and constants
    ├── directory_utils.js      # Utility functions and helpers
    ├── directory_handlers.js   # Event handlers and API communication
    ├── directory_navigation.js # Navigation logic and history
    ├── directory_main.js       # Main initialization and coordination
    └── README.md               # JavaScript documentation
```

## �️ Backend Integration

### Required API Endpoints

The directory explorer expects the following REST API endpoints:

```javascript
// GET /api/directory/list?path=/path/to/directory
// Response: { files: [...], directories: [...] }

// GET /api/files/download?path=/path/to/file
// Response: File download stream

// DELETE /api/files/delete
// Body: { path: "/path/to/file" }

// GET /api/files/info?path=/path/to/file
// Response: { name, size, type, modified, permissions }
```

## 🚀 Quick Start

1. **Include Required Files**:
```html
<link rel="stylesheet" href="css/directory_styles.css">
<script src="directory/js/directory_config.js"></script>
<script src="directory/js/directory_utils.js"></script>
<script src="directory/js/directory_handlers.js"></script>
<script src="directory/js/directory_navigation.js"></script>
<script src="directory/js/directory_main.js"></script>
```

2. **Initialize**: The module auto-initializes when the page loads

3. **Configure Endpoints**: Update `DirectoryConfig.endpoints` in `directory_config.js` for your backend

## ⚙️ Configuration

### Simulation Mode
For development without backend:
```javascript
DirectoryConfig.simulation.enabled = true;
```

### Production Mode
For backend integration:
```javascript
DirectoryConfig.simulation.enabled = false;
DirectoryConfig.endpoints.list = '/api/directory/list';
// ... configure other endpoints
```

## 🎹 Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+A` | Select all files |
| `Delete` | Delete selected |
| `F5` | Refresh directory |
| `Ctrl+F` | Focus search |
| `Backspace` | Navigate back |
| `Enter` | Open file/folder |
| `Space` | Toggle selection |

## 📱 Responsive Design

- **Desktop**: Full feature set with advanced controls
- **Tablet**: Optimized touch interface
- **Mobile**: Simplified view with essential functions

## � Module Integration

Works seamlessly with other webserver modules:
- **Upload Module**: Auto-refresh after file uploads
- **GET Module**: Direct API endpoint testing
- **Error Pages**: Integrated error handling

---

**Part of the 42 Webserver Project** | [Main Documentation](../README.md)

**Desarrollado para el proyecto 42_webserv_html** 🚀
