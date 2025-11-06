# 🚀 WebServer Development Plan - 42 School

**Project Duration**: 6 weeks (November 6 - December 17, 2025)  
**Team Size**: 3 developers  
**Architecture**: HTTP/1.1 compliant web server with CGI support

---

## 👥 Team Roles & Responsibilities

| Developer | Focus Area | Primary Components |
|-----------|------------|-------------------|
| **🔵 Eina** | HTTP Core & Request Processing | Request Parser, Request Processor, HTTP validation |
| **🟢 Marta** | Configuration & Response System | Config Parser, HTTP Response, Server integration |
| **🟣 Patri** | File Serving & CGI System | Static files, CGI execution, Utils, DELETE method |

---

## 📊 Current Project Status

### ✅ **Completed (by Marta)**
- Basic Server architecture with non-blocking sockets
- Multi-client connection handling using `poll()`
- Basic Client class with socket management
- "Hello World" response functionality
- Project structure and initial codebase

### ❌ **To Be Implemented**
- HTTP request parsing (headers, body, methods)
- Configuration file parser
- HTTP response generation
- Static file serving
- CGI script execution
- Error handling and logging
- Makefile and build system

---

## 🗓️ Development Timeline

### **📅 Week 1-2: Foundation Phase** *(Nov 6-19)*
**Goal**: Implement core independent components

#### **🔵 Eina - HTTP Request Foundation**
- **Branch**: `feature/request-parser`
- **Files**: `includes/Request.hpp`, `src/Request.cpp`
- **Tasks**:
  - [ ] Design Request class structure
  - [ ] Implement HTTP request line parsing (`GET /path HTTP/1.1`)
  - [ ] Parse HTTP headers into `std::map<string, string>`
  - [ ] Parse query parameters (`?name=value&foo=bar`)
  - [ ] Parse request body for POST requests
  - [ ] Add basic HTTP syntax validation
  - [ ] Handle chunked encoding

#### **🟢 Marta - Configuration System**
- **Branch**: `feature/config-parser`
- **Files**: `includes/ConfigParser.hpp`, `src/ConfigParser.cpp`, `config/default.conf`
- **Tasks**:
  - [ ] Design `ServerConfig` struct and `ConfigParser` class
  - [ ] Create nginx-style configuration file format
  - [ ] Implement config file lexer/tokenizer
  - [ ] Parse server blocks and directives
  - [ ] Add configuration validation
  - [ ] Handle syntax errors gracefully

#### **🟣 Patri - File Operations & Utils**
- **Branch**: `feature/utils-implementation`
- **Files**: `includes/utils.hpp`, `src/utils.cpp`
- **Tasks**:
  - [ ] Implement file operation utilities (`fileExists`, `readFile`, etc.)
  - [ ] Add MIME type detection based on file extensions
  - [ ] Create path security validation (prevent path traversal)
  - [ ] Implement URL decoding functionality
  - [ ] Add directory listing utilities
  - [ ] Create comprehensive unit tests

### **📅 Week 3-4: Integration Phase** *(Nov 20 - Dec 3)*
**Goal**: Connect components and implement core HTTP functionality

#### **🔵 Eina - Request Processing Logic**
- **Branch**: `feature/request-processor`
- **Files**: `includes/RequestProcessor.hpp`, `src/RequestProcessor.cpp`
- **Tasks**:
  - [ ] Design RequestProcessor class architecture
  - [ ] Implement request routing based on configuration
  - [ ] Add HTTP method validation (GET, POST, DELETE)
  - [ ] Implement path validation and permission checks
  - [ ] Integrate with FileSender for static content
  - [ ] Integrate with CgiHandler for script execution
  - [ ] Implement comprehensive HTTP error handling (400, 404, 405, 500)
  - [ ] Enhance Client.cpp integration

#### **🟢 Marta - HTTP Response System**
- **Branch**: `feature/response-system`
- **Files**: `includes/Response.hpp`, `src/Response.cpp`
- **Tasks**:
  - [ ] Design Response class with status codes and headers
  - [ ] Implement HTTP status line generation
  - [ ] Add automatic Content-Length calculation
  - [ ] Include security headers (X-Content-Type-Options, etc.)
  - [ ] Support multiple content-types
  - [ ] Create response templates for error pages

#### **🟣 Patri - Static File Serving**
- **Branch**: `feature/file-sender`
- **Files**: `includes/FileSender.hpp`, `src/FileSender.cpp`
- **Tasks**:
  - [ ] Design FileSender class architecture
  - [ ] Implement secure static file serving
  - [ ] Generate directory listings with HTML formatting
  - [ ] Handle large files efficiently (chunked transfer)
  - [ ] Integrate with utils for MIME type detection
  - [ ] Add comprehensive error handling

### **📅 Week 5-6: Advanced Features & Polish** *(Dec 4-17)*
**Goal**: Implement advanced features and optimize the system

#### **🔵 Eina - Testing & Optimization**
- **Branch**: `feature/http-testing`
- **Tasks**:
  - [ ] Create comprehensive HTTP parsing test suite
  - [ ] Test edge cases (malformed requests, large headers)
  - [ ] Performance optimization for request processing
  - [ ] Memory leak detection and fixing
  - [ ] Integration testing with other components

#### **🟢 Marta - Build System & Server Integration**
- **Branch**: `feature/build-system`
- **Files**: `Makefile`, Server.cpp improvements
- **Tasks**:
  - [ ] Create complete Makefile with proper dependencies
  - [ ] Integrate ConfigParser with Server.cpp
  - [ ] Add support for multiple server configurations
  - [ ] Implement connection timeouts
  - [ ] Add graceful shutdown handling

#### **🟣 Patri - CGI & Advanced Features**
- **Branch**: `feature/cgi-handler`
- **Files**: `includes/CgiHandler.hpp`, `src/CgiHandler.cpp`
- **Tasks**:
  - [ ] Design CGI execution architecture
  - [ ] Implement CGI environment variable setup
  - [ ] Add Python script execution using `fork()`/`execve()`
  - [ ] Handle CGI input/output with pipes
  - [ ] Implement script timeout mechanism
  - [ ] Parse CGI output (headers + body separation)

---

## 🔧 Technical Architecture

### **Core Classes Overview**

```cpp
// HTTP Processing Chain
Client -> Request -> RequestProcessor -> Response -> Client

// Supporting Systems
ConfigParser -> ServerConfig
FileSender -> Static Content
CgiHandler -> Dynamic Content
Logger -> System Monitoring
```

### **File Structure**
```
WebServer_oficial/
├── includes/           # Header files (.hpp)
├── src/               # Implementation files (.cpp)
├── config/            # Configuration files
├── frontend_section/  # Static web content (by Patri)
├── Makefile          # Build configuration
└── README.md         # Project documentation
```

---

## 🌊 Development Workflow

### **Branch Strategy**
- **main**: Stable, deployable code only
- **feature/**: One branch per feature/component
- **hotfix/**: Critical bug fixes
- **docs/**: Documentation updates

### **Branch Naming Convention**
```
feature/request-parser
feature/config-parser
feature/file-sender
feature/cgi-handler
feature/response-system
feature/utils-implementation
hotfix/memory-leak-client
docs/api-documentation
```

### **Daily Workflow**
```bash
# 1. Start of day - sync with main
git checkout main
git pull origin main
git checkout feature/your-feature
git merge main

# 2. Development work
git add .
git commit -m "feat: implement HTTP header parsing"
git push origin feature/your-feature

# 3. Create PR when feature is complete
# 4. Code review by team member
# 5. Merge to main after approval
```

### **Commit Message Convention**
```
feat: add new functionality
fix: bug correction
docs: documentation changes
test: add or modify tests
refactor: code restructuring
style: formatting changes
```

---

## 🧪 Testing Strategy

### **Unit Testing**
- Each developer tests their own components
- Mock dependencies for isolated testing
- Test edge cases and error conditions

### **Integration Testing**
- Test component interactions
- End-to-end HTTP request/response flow
- Configuration loading and validation

### **Acceptance Testing**
- Compatibility with provided frontend
- CGI script execution with existing Python scripts
- HTTP/1.1 compliance testing

---

## 📋 Dependencies & Integration Points

### **Week 1-2 Dependencies**
- ✅ **All components independent** - can develop in parallel

### **Week 3-4 Dependencies**
- **RequestProcessor** needs: Request class, basic ConfigParser
- **FileSender** needs: Utils, basic Response
- **Response** needs: Basic ConfigParser

### **Week 5-6 Dependencies**
- **CgiHandler** needs: Request, Response, FileSender
- **Final integration** needs: All components functional

---

## 🎯 Success Criteria

### **Minimum Viable Product (MVP)**
- [ ] Serve static files from `frontend_section/`
- [ ] Handle GET and POST requests
- [ ] Execute Python CGI scripts
- [ ] Return proper HTTP error codes
- [ ] Parse configuration file
- [ ] Handle multiple simultaneous connections

### **Advanced Features**
- [ ] DELETE method implementation
- [ ] Directory browsing
- [ ] Request/response logging
- [ ] Chunked transfer encoding
- [ ] Connection keep-alive
- [ ] Security features (path traversal protection)

---

## 🚨 Risk Mitigation

### **Technical Risks**
- **CGI complexity**: Start with simple Python execution, add features incrementally
- **HTTP compliance**: Test with multiple browsers and tools
- **Memory management**: Regular testing with valgrind
- **Concurrency issues**: Careful testing of non-blocking operations

### **Team Coordination Risks**
- **Merge conflicts**: Regular communication and small, frequent commits
- **Integration delays**: Define clear APIs early
- **Testing gaps**: Each developer responsible for their component tests

---

## 📞 Communication Plan

### **Daily Coordination**
- Brief status updates in team chat
- Blocked/dependency issues communicated immediately
- Code review requests within 24 hours

### **Weekly Milestones**
- End of week demo/integration session
- Planning for following week
- Risk assessment and mitigation

---

## 📚 Resources & References

- **HTTP/1.1 Specification**: RFC 7230-7237
- **CGI Specification**: RFC 3875
- **42 WebServer Subject**: Project requirements and constraints
- **Testing Tools**: curl, Postman, browser developer tools

---

*Last Updated: November 6, 2025*  
*Next Review: November 13, 2025*
