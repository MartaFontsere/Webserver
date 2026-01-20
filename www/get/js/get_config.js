/**
 * ================================================================
 * 📥 GET TESTING - CONFIGURATION
 * Configuration constants and settings for GET request testing
 * ================================================================
 */

// Configuration object
const GET_CONFIG = {
    // Default settings
    DEFAULT_URL: '/',
    REQUEST_TIMEOUT: 30000, // 30 seconds
    MAX_HISTORY_ITEMS: 50,

    // Server endpoints for testing
    ENDPOINTS: {
        STATUS: '/api/status',
        FILES: '/files/',
        UPLOAD: '/upload/',
        DIRECTORY: '/directory/',
        API: '/api/'
    },

    // Quick test configurations
    QUICK_TESTS: [
        { name: '🏠 Homepage', url: '/', description: 'Test main page' },
        { name: 'ℹ️ About Page', url: '/about.html', description: 'Test about page' },
        { name: '📁 Upload Page', url: '/upload/upload.html', description: 'Test upload interface' },
        { name: '📊 Server Status', url: '/api/status', description: 'Test status API' },
        { name: '📂 Files Directory', url: '/files/', description: 'Test directory listing' },
        { name: '❌ 404 Test', url: '/nonexistent', description: 'Test 404 error' }
    ],

    // HTTP status code configurations
    STATUS_CODES: {
        // Success
        200: { name: 'OK', class: 'success', color: '#28a745' },
        201: { name: 'Created', class: 'success', color: '#28a745' },
        204: { name: 'No Content', class: 'success', color: '#28a745' },

        // Redirection
        301: { name: 'Moved Permanently', class: 'redirect', color: '#ffc107' },
        302: { name: 'Found', class: 'redirect', color: '#ffc107' },
        304: { name: 'Not Modified', class: 'redirect', color: '#ffc107' },

        // Client Errors
        400: { name: 'Bad Request', class: 'error', color: '#dc3545' },
        401: { name: 'Unauthorized', class: 'error', color: '#dc3545' },
        403: { name: 'Forbidden', class: 'error', color: '#dc3545' },
        404: { name: 'Not Found', class: 'error', color: '#dc3545' },
        405: { name: 'Method Not Allowed', class: 'error', color: '#dc3545' },

        // Server Errors
        500: { name: 'Internal Server Error', class: 'error', color: '#dc3545' },
        502: { name: 'Bad Gateway', class: 'error', color: '#dc3545' },
        503: { name: 'Service Unavailable', class: 'error', color: '#dc3545' },
        505: { name: 'HTTP Version Not Supported', class: 'error', color: '#dc3545' }
    },

    // Content type configurations
    CONTENT_TYPES: {
        'text/html': { name: 'HTML', preview: true },
        'text/plain': { name: 'Text', preview: true },
        'text/css': { name: 'CSS', preview: true },
        'text/javascript': { name: 'JavaScript', preview: true },
        'application/json': { name: 'JSON', preview: true, format: true },
        'application/xml': { name: 'XML', preview: true, format: true },
        'image/jpeg': { name: 'JPEG Image', preview: false },
        'image/png': { name: 'PNG Image', preview: false },
        'image/gif': { name: 'GIF Image', preview: false },
        'application/pdf': { name: 'PDF', preview: false }
    },

    // Headers to highlight in response
    IMPORTANT_HEADERS: [
        'Content-Type',
        'Content-Length',
        'Server',
        'Date',
        'Last-Modified',
        'Cache-Control',
        'ETag',
        'Location',
        'Set-Cookie'
    ],

    // UI Messages
    MESSAGES: {
        NO_REQUESTS: 'No requests in history',
        NO_RESPONSE: 'No request sent yet',
        LOADING: 'Sending request...',
        SUCCESS: 'Request completed successfully',
        ERROR: 'Request failed',
        TIMEOUT: 'Request timed out',
        NETWORK_ERROR: 'Network error occurred',
        HISTORY_CLEARED: 'History cleared successfully',
        HISTORY_EXPORTED: 'History exported successfully'
    },

    // Local storage keys
    STORAGE_KEYS: {
        HISTORY: 'get_test_history',
        SETTINGS: 'get_test_settings'
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = GET_CONFIG;
}
