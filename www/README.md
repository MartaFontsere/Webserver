# 🌐 HTML Frontend - 42 Webserver Project

Modern, responsive frontend for a webserver project with file upload functionality, error handling, and clean architecture ready for C++ backend integration.

## 📁 Project Structure

```
Html/
├── README.md                 # This documentation
├── index.html               # Main homepage
├── about.html               # About page with project info
├── upload/                  # 📁 File Upload System (✅ COMPLETE)
│   ├── README.md            # Complete upload documentation
│   ├── upload.html          # Upload interface
│   └── js/                  # Modular JavaScript (MVC architecture)
├── get/                     # 📥 GET Request Testing (✅ COMPLETE)
│   ├── README.md            # Complete GET testing documentation
│   ├── test_get.html        # GET testing interface
│   └── js/                  # Modular JavaScript (Request handling)
├── cgi/                     # ⚙️ CGI Script Testing (✅ COMPLETE)
│   ├── README.md            # Complete CGI testing documentation
│   ├── test_cgi.html        # CGI testing interface
│   └── js/                  # Modular JavaScript (CGI execution)
├── post/                    # 📝 POST Request Testing (✅ COMPLETE)
│   ├── README.md            # Complete POST testing documentation
│   ├── test_post.html       # POST testing interface
│   └── js/                  # Modular JavaScript (Request handling)
├── delete/                  # 🗑️ DELETE Request Testing (✅ COMPLETE)
│   ├── README.md            # Complete DELETE testing documentation
│   ├── test_delete.html     # DELETE testing interface
│   └── js/                  # Modular JavaScript (Request handling)
├── directory/               # 📂 Directory Explorer (✅ COMPLETE)
│   ├── README.md            # Complete directory explorer documentation
│   ├── explorer.html        # Directory explorer interface
│   └── js/                  # Modular JavaScript (Navigation and file management)
├── css/                     # Stylesheets
│   ├── index_styles.css     # Homepage styles
│   ├── about_styles.css     # About page styles
│   ├── upload_styles.css    # Upload system styles
│   ├── get_styles.css       # GET testing styles
│   ├── cgi_styles.css       # CGI testing styles
│   ├── post_styles.css      # POST testing styles
│   ├── delete_styles.css    # DELETE testing styles
│   └── directory_styles.css # Directory explorer styles
├── error_files/             # HTTP error pages (403, 404, 405, 500, 505)
└── images/                  # Image assets
```

## 🚀 Features

### 🏠 **Homepage** (`index.html`)
- Modern responsive design with gradient background
- Navigation cards to different sections
- Technical webserver information
- Professional layout with clean typography

### 📁 **File Upload System** (`upload/`) ✅
- **Complete upload solution** with drag & drop interface
- **MVC architecture** with modular JavaScript
- **C++ backend ready** with documented API endpoints
- **Production/simulation modes** for development and deployment
- 📖 **[Full documentation](upload/README.md)**

### 📥 **GET Request Testing** (`get/`) ✅
- **Comprehensive HTTP GET testing** interface
- **Real-time response analysis** with multiple view modes
- **Request history** with persistent storage and export
- **Simulation mode** for development, production-ready for C++ backend
- 📖 **[Full documentation](get/README.md)**

### ⚙️ **CGI Script Testing** (`cgi/`) ✅
- **Comprehensive CGI script execution** with GET and POST methods
- **Parameter management** for query strings, POST data, and environment variables
- **Real-time output analysis** with headers, errors, and environment views
- **Quick scripts** for multiple languages: Python (mini, env, form), Shell (hello.sh), C++ (hello.cgi)
- **Execution history** with persistent storage and export capabilities
- **Simulation/Production modes** for development and backend integration
- 📖 **[Full documentation](cgi/README.md)**

### 📝 **POST Request Testing** (`post/`) ✅
- **Comprehensive HTTP POST testing** with support for POST, PUT, PATCH methods
- **Multiple content types**: JSON, form-urlencoded, text/plain, multipart/form-data
- **Advanced body editors**: Raw text, form data builder, JSON editor with validation
- **Quick test scenarios**: User data, login forms, products, contact forms, text messages
- **Custom headers management** and real-time request/response analysis
- **Request history** with persistent storage, replay functionality, and export
- **Keyboard shortcuts** for power users and professional workflow
- 📖 **[Full documentation](post/README.md)**

### 🗑️ **DELETE Request Testing** (`delete/`) ✅
- **Comprehensive HTTP DELETE testing** with resource management and bulk operations
- **Resource discovery**: Automatic detection of deletable resources on the server
- **Bulk operations**: Multi-select and batch deletion with progress tracking
- **Quick test scenarios**: File deletion, user removal, product catalog cleanup, error conditions
- **Advanced safety features**: Multiple confirmation layers, promise-based confirmations, safe mode, and protected file detection
- **Smart confirmation system**: Async/await pattern with custom modal dialogs and keyboard support
- **Custom headers management** and advanced request configuration
- **Operation history** with undo capabilities, replay functionality, and export
- **Professional UI** with confirmation dialogs and real-time response analysis
- 📖 **[Full documentation](delete/README.md)**

### 📂 **Directory Explorer** (`directory/`) ✅
- **Comprehensive file system navigation** with modern, professional interface
- **Multiple view modes**: Grid, List, and Details views with integrated controls
- **Advanced navigation**: Breadcrumb navigation, address bar with auto-completion, and history tracking
- **File management operations**: Preview, download, properties, and context menu actions
- **Smart filtering and sorting**: File type filters, name/size/date sorting, and real-time search
- **Responsive design**: Optimized for desktop, tablet, and mobile devices
- **Backend integration ready**: RESTful API endpoints for full C++ backend communication
- 📖 **[Full documentation](directory/README.md)**

### ℹ️ **About Page** (`about.html`)
- Project information and technical specifications
- Team details and development notes
- Consistent styling with main theme

### 🚨 **Error Pages** (`error_files/`)
- Custom error pages for HTTP status codes (403, 404, 405, 500, 505)
- Professional styling matching main theme
- User-friendly error messages with navigation

## 🎨 Design System

### **Color Palette**
- **Primary**: Purple gradient (#667eea → #764ba2)
- **Accent**: Yellow (#FFE135)
- **Success**: Green (#4CAF50)
- **Error**: Red (#FF6B6B)

### **Features**
- **Glass-morphism effects** for modern appearance
- **Responsive design** for all devices
- **Consistent typography** (Arial, sans-serif)
- **Accessibility support** (keyboard navigation, screen readers)

## 🔧 Backend Integration

### **Ready for C++ Server**
The frontend is designed to work with any HTTP server implementing standard REST APIs:

```cpp
// Required endpoints for full functionality
GET /                     // Homepage
GET /about.html          // About page  
GET /api/status          // Server status
POST /upload             // File upload
GET /files               // List files
GET /files/{filename}    // Download file
DELETE /files/{filename} // Delete file
POST /cgi-execute        // Execute CGI scripts
GET /health              // Health check for CGI
POST /test-post          // POST testing endpoint
POST /api/users          // User data endpoint
POST /auth/login         // Login endpoint
POST /api/products       // Products endpoint
POST /contact/submit     // Contact form endpoint
PUT /test-put            // PUT testing endpoint
PATCH /test-patch        // PATCH testing endpoint
DELETE /files/{filename} // Delete file endpoint
DELETE /api/users/{id}   // Delete user endpoint
DELETE /api/products/{id}// Delete product endpoint
DELETE /api/bulk-delete  // Bulk deletion endpoint
```

### **Integration Steps**
1. **Implement HTTP endpoints** in C++ server
2. **Configure simulation modes**: Set `SIMULATION_MODE = false` in respective configs
3. **Deploy frontend files** to server static directory
4. **Configure error pages** in server config
5. **Test with real operations** using the testing interfaces

**📖 Detailed integration guides:**
- **[Upload Integration](upload/README.md#backend-integration)**
- **[GET Testing Integration](get/README.md#integration-notes)**
- **[CGI Testing Integration](cgi/README.md#integration)**
- **[POST Testing Integration](post/README.md#backend-integration-points)**
- **[DELETE Testing Integration](delete/README.md#integration-notes)**

## 🛠️ Development Status

### **✅ Completed Modules**
- ✅ **Homepage** (`index.html`) - Navigation and information hub
- ✅ **About Page** (`about.html`) - Project and team information
- ✅ **File Upload System** (`upload/`) - Complete with drag & drop
- ✅ **GET Request Testing** (`get/`) - Comprehensive HTTP testing
- ✅ **CGI Script Testing** (`cgi/`) - Complete script execution testing
- ✅ **POST Request Testing** (`post/`) - Form and data submission testing
- ✅ **DELETE Request Testing** (`delete/`) - Resource deletion with bulk operations
- ✅ **Error Pages** (`error_files/`) - Custom 403, 404, 405, 500, 505
- ✅ **Responsive Design** - Mobile, tablet, desktop support

### **🚧 Pending Modules**
- 🔄 **Directory Explorer** (`directory/`) - File browser interface

### **📈 Project Progress**
**Overall Completion**: ~95% (7/8 major modules complete)

### **Quick Start**
1. **Open** `index.html` in browser for homepage navigation
2. **Test upload system** via "📁 File Upload" button  
3. **Test GET requests** via "📥 Test GET" button
4. **Test POST requests** via "📝 Test POST" button
5. **Test DELETE operations** via "🗑️ Test DELETE" button
6. **Test CGI scripts** via "⚙️ Test CGI" button
7. **View error pages** via bottom section links
8. **Integrate with C++ backend** when ready for production

### **Technologies Used**
- **HTML5**: Semantic structure and modern APIs
- **CSS3**: Grid, Flexbox, custom properties, animations  
- **JavaScript ES6+**: Modular architecture, Fetch API, DOM manipulation
- **MVC Pattern**: Clean separation of concerns
- **Progressive Enhancement**: Works without JavaScript for basic functionality

## 📝 Contributing

### **Code Standards**
- English language throughout
- Consistent naming conventions
- Modular, maintainable code
- Clear documentation of integration points

### **File Organization**
- **General frontend**: Root level (`index.html`, `about.html`, `css/`)
- **Upload system**: Dedicated directory (`upload/`) with own documentation
- **Error handling**: Separate directory (`error_files/`)

---

**🎯 Professional frontend ready for 42 webserver project**  
*Modern UI/UX with clean architecture and C++ backend integration support*
