/* =================================================================
   📝 POST TESTING - CONFIGURATION
   Configuration constants and settings for POST request testing
   ================================================================= */

// API Configuration
const POST_CONFIG = {
    // Default endpoints for testing
    DEFAULT_ENDPOINTS: {
        POST: '/test-post',
        PUT: '/test-put',
        PATCH: '/test-patch'
    },

    // Content types
    CONTENT_TYPES: {
        JSON: 'application/json',
        FORM_URLENCODED: 'application/x-www-form-urlencoded',
        TEXT_PLAIN: 'text/plain',
        MULTIPART: 'multipart/form-data'
    },

    // Quick test templates
    QUICK_TESTS: {
        'json-user': {
            name: '👤 JSON User Data',
            method: 'POST',
            url: '/api/users',
            contentType: 'application/json',
            body: JSON.stringify({
                name: 'John Doe',
                email: 'john.doe@example.com',
                age: 30,
                city: 'Barcelona',
                active: true
            }, null, 2)
        },
        'form-login': {
            name: '🔐 Form Login',
            method: 'POST',
            url: '/auth/login',
            contentType: 'application/x-www-form-urlencoded',
            body: 'username=testuser&password=secretpass&remember=true'
        },
        'json-product': {
            name: '📦 JSON Product',
            method: 'POST',
            url: '/api/products',
            contentType: 'application/json',
            body: JSON.stringify({
                name: 'Awesome Product',
                description: 'This is an amazing product that everyone needs',
                price: 99.99,
                category: 'electronics',
                inStock: true,
                tags: ['popular', 'new', 'featured'],
                specifications: {
                    weight: '1.5kg',
                    dimensions: '20x15x5cm',
                    color: 'black'
                }
            }, null, 2)
        },
        'form-contact': {
            name: '📧 Form Contact',
            method: 'POST',
            url: '/contact/submit',
            contentType: 'application/x-www-form-urlencoded',
            body: 'name=Jane+Smith&email=jane%40example.com&subject=Website+Inquiry&message=Hello%2C+I+am+interested+in+your+services.'
        },
        'text-message': {
            name: '💬 Text Message',
            method: 'POST',
            url: '/api/messages',
            contentType: 'text/plain',
            body: 'This is a plain text message that will be sent to the server. It can contain any text content without formatting.'
        },
        'json-array': {
            name: '📋 JSON Array',
            method: 'POST',
            url: '/api/batch',
            contentType: 'application/json',
            body: JSON.stringify([
                { id: 1, action: 'create', data: { name: 'Item 1' } },
                { id: 2, action: 'update', data: { name: 'Item 2', status: 'active' } },
                { id: 3, action: 'delete', data: { id: 123 } }
            ], null, 2)
        }
    },

    // Default headers
    DEFAULT_HEADERS: {
        'User-Agent': 'WebServer-POST-Tester/1.0',
        'Accept': '*/*',
        'Cache-Control': 'no-cache'
    },

    // Response formatting
    RESPONSE_FORMATTING: {
        PRETTY_PRINT_INDENT: 2,
        MAX_CONTENT_LENGTH: 1048576, // 1MB
        TRUNCATE_LENGTH: 10000
    },

    // History settings
    HISTORY: {
        MAX_ENTRIES: 50,
        STORAGE_KEY: 'post_request_history',
        AUTO_SAVE: true
    },

    // UI settings
    UI: {
        ANIMATION_DURATION: 300,
        DEBOUNCE_DELAY: 500,
        AUTO_FORMAT_JSON: true,
        SHOW_REQUEST_TIME: true
    },

    // Status code categories
    STATUS_CATEGORIES: {
        SUCCESS: [200, 201, 202, 204],
        REDIRECT: [301, 302, 304, 307, 308],
        CLIENT_ERROR: [400, 401, 403, 404, 405, 409, 422],
        SERVER_ERROR: [500, 501, 502, 503, 504, 505]
    },

    // Error messages
    ERROR_MESSAGES: {
        INVALID_JSON: 'Invalid JSON format. Please check your syntax.',
        NETWORK_ERROR: 'Network error. Please check your connection.',
        TIMEOUT_ERROR: 'Request timeout. The server did not respond in time.',
        UNKNOWN_ERROR: 'An unexpected error occurred.',
        EMPTY_URL: 'Please enter a URL.',
        INVALID_URL: 'Please enter a valid URL.'
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = POST_CONFIG;
}
