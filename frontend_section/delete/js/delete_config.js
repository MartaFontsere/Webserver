/* =================================================================
   🗑️ DELETE TESTING - CONFIGURATION
   Configuration constants and settings for DELETE request testing
   ================================================================= */

// API Configuration
const DELETE_CONFIG = {
    // Default endpoints for testing
    DEFAULT_ENDPOINTS: {
        FILES: '/files',
        USERS: '/api/users',
        PRODUCTS: '/api/products',
        TEST_DELETE: '/test-delete'
    },

    // Resource discovery endpoints
    DISCOVERY_ENDPOINTS: {
        files: '/files',
        users: '/api/users',
        products: '/api/products',
        posts: '/api/posts',
        comments: '/api/comments'
    },

    // Quick test templates
    QUICK_TESTS: {
        'delete-file': {
            name: '📄 Delete Test File',
            method: 'DELETE',
            url: '/files/test-file.txt',
            description: 'Delete a test file from the server',
            expectedStatus: [200, 204, 404]
        },
        'delete-user': {
            name: '👤 Delete Test User',
            method: 'DELETE',
            url: '/api/users/test-user-123',
            description: 'Delete a test user account',
            expectedStatus: [200, 204, 404, 403]
        },
        'delete-product': {
            name: '📦 Delete Test Product',
            method: 'DELETE',
            url: '/api/products/test-product-456',
            description: 'Delete a test product from catalog',
            expectedStatus: [200, 204, 404, 409]
        },
        'delete-nonexistent': {
            name: '❌ Delete Non-existent',
            method: 'DELETE',
            url: '/files/nonexistent-file-xyz.txt',
            description: 'Try to delete a resource that does not exist',
            expectedStatus: [404]
        },
        'delete-protected': {
            name: '🔒 Delete Protected Resource',
            method: 'DELETE',
            url: '/api/admin/protected-resource',
            description: 'Try to delete a protected/restricted resource',
            expectedStatus: [403, 401]
        },
        'delete-bulk': {
            name: '📋 Bulk Delete Test',
            method: 'POST',
            url: '/api/bulk-delete',
            description: 'Delete multiple resources in bulk',
            body: JSON.stringify({
                resources: [
                    '/files/bulk-test-1.txt',
                    '/files/bulk-test-2.txt',
                    '/api/users/bulk-user-1'
                ]
            }),
            headers: {
                'Content-Type': 'application/json'
            },
            expectedStatus: [200, 207] // 207 Multi-Status for partial success
        }
    },

    // Resource types configuration
    RESOURCE_TYPES: {
        files: {
            name: 'Files',
            endpoint: '/files',
            urlPattern: '/files/{filename}',
            icon: '📄',
            preview: true
        },
        users: {
            name: 'Users',
            endpoint: '/api/users',
            urlPattern: '/api/users/{id}',
            icon: '👤',
            preview: false
        },
        products: {
            name: 'Products',
            endpoint: '/api/products',
            urlPattern: '/api/products/{id}',
            icon: '📦',
            preview: false
        },
        posts: {
            name: 'Posts',
            endpoint: '/api/posts',
            urlPattern: '/api/posts/{id}',
            icon: '📝',
            preview: true
        },
        comments: {
            name: 'Comments',
            endpoint: '/api/comments',
            urlPattern: '/api/comments/{id}',
            icon: '💬',
            preview: true
        }
    },

    // Default headers for DELETE requests
    DEFAULT_HEADERS: {
        'User-Agent': 'WebServer-DELETE-Tester/1.0',
        'Accept': '*/*',
        'Cache-Control': 'no-cache'
    },

    // Confirmation settings
    CONFIRMATION: {
        REQUIRE_CONFIRMATION: true,
        SHOW_PREVIEW: false,
        SAFE_MODE: true,
        CONFIRMATION_DELAY: 3000, // 3 seconds delay for dangerous operations
        DOUBLE_CONFIRMATION: ['bulk-delete', 'delete-all']
    },

    // Response handling
    RESPONSE_HANDLING: {
        PRETTY_PRINT_INDENT: 2,
        MAX_CONTENT_LENGTH: 1048576, // 1MB
        TRUNCATE_LENGTH: 10000,
        SHOW_TIMING: true
    },

    // History settings
    HISTORY: {
        MAX_ENTRIES: 100,
        STORAGE_KEY: 'delete_request_history',
        AUTO_SAVE: true,
        ENABLE_UNDO: true,
        UNDO_TIMEOUT: 30000 // 30 seconds to undo
    },

    // UI settings
    UI: {
        ANIMATION_DURATION: 300,
        DEBOUNCE_DELAY: 500,
        AUTO_REFRESH_RESOURCES: false,
        REFRESH_INTERVAL: 30000, // 30 seconds
        BULK_SELECT_LIMIT: 50
    },

    // Status code handling
    STATUS_CODES: {
        SUCCESS: [200, 204], // OK, No Content
        CLIENT_ERROR: [400, 401, 403, 404, 405, 409, 422],
        SERVER_ERROR: [500, 501, 502, 503, 504, 505],
        DELETE_SUCCESS: [200, 204],
        RESOURCE_NOT_FOUND: [404],
        FORBIDDEN: [401, 403],
        CONFLICT: [409] // Resource has dependencies
    },

    // Error messages
    ERROR_MESSAGES: {
        NETWORK_ERROR: 'Network error. Please check your connection.',
        TIMEOUT_ERROR: 'Request timeout. The server did not respond in time.',
        UNKNOWN_ERROR: 'An unexpected error occurred.',
        EMPTY_URL: 'Please enter a resource URL.',
        INVALID_URL: 'Please enter a valid resource URL.',
        NO_RESOURCES_SELECTED: 'Please select at least one resource to delete.',
        DELETION_FAILED: 'Failed to delete resource. Please try again.',
        BULK_DELETION_FAILED: 'Some resources could not be deleted.',
        CONFIRMATION_REQUIRED: 'Please confirm the deletion.',
        SAFE_MODE_VIOLATION: 'Operation blocked by safe mode.',
        UNDO_EXPIRED: 'Undo period has expired.',
        DISCOVERY_FAILED: 'Failed to discover resources.'
    },

    // Success messages
    SUCCESS_MESSAGES: {
        DELETION_SUCCESS: 'Resource deleted successfully.',
        BULK_DELETION_SUCCESS: 'All selected resources deleted successfully.',
        BULK_DELETION_PARTIAL: 'Some resources were deleted successfully.',
        UNDO_SUCCESS: 'Deletion undone successfully.',
        HISTORY_CLEARED: 'Deletion history cleared.',
        HISTORY_EXPORTED: 'History exported successfully.'
    },

    // Bulk operations
    BULK_OPERATIONS: {
        MAX_CONCURRENT: 5, // Maximum concurrent delete requests
        BATCH_SIZE: 10,    // Resources per batch
        RETRY_COUNT: 3,    // Retry failed deletions
        RETRY_DELAY: 1000  // Delay between retries
    },

    // Safety features
    SAFETY: {
        DANGEROUS_PATTERNS: [
            '/system/',
            '/admin/',
            '/config/',
            '/root/',
            '/.env',
            '/database/'
        ],
        PROTECTED_EXTENSIONS: [
            '.config',
            '.env',
            '.key',
            '.pem',
            '.cert'
        ],
        CONFIRMATION_WORDS: ['DELETE', 'REMOVE', 'DESTROY'],
        REQUIRE_TYPED_CONFIRMATION: false
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = DELETE_CONFIG;
}
