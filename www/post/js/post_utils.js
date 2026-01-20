/* =================================================================
   📝 POST TESTING - UTILITIES
   Utility functions for POST request testing functionality
   ================================================================= */

// Utility Functions
const PostUtils = {

    /**
     * Format JSON string with proper indentation
     * @param {string} jsonString - JSON string to format
     * @returns {string} - Formatted JSON string
     */
    formatJSON(jsonString) {
        try {
            const parsed = JSON.parse(jsonString);
            return JSON.stringify(parsed, null, POST_CONFIG.RESPONSE_FORMATTING.PRETTY_PRINT_INDENT);
        } catch (error) {
            throw new Error(POST_CONFIG.ERROR_MESSAGES.INVALID_JSON);
        }
    },

    /**
     * Validate JSON string
     * @param {string} jsonString - JSON string to validate
     * @returns {boolean} - True if valid, false otherwise
     */
    isValidJSON(jsonString) {
        try {
            JSON.parse(jsonString);
            return true;
        } catch (error) {
            return false;
        }
    },

    /**
     * Convert form data object to URL encoded string
     * @param {Object} formData - Form data object
     * @returns {string} - URL encoded string
     */
    objectToUrlEncoded(formData) {
        return Object.keys(formData)
            .map(key => `${encodeURIComponent(key)}=${encodeURIComponent(formData[key])}`)
            .join('&');
    },

    /**
     * Convert URL encoded string to object
     * @param {string} urlEncoded - URL encoded string
     * @returns {Object} - Form data object
     */
    urlEncodedToObject(urlEncoded) {
        const result = {};
        if (!urlEncoded) return result;

        urlEncoded.split('&').forEach(pair => {
            const [key, value] = pair.split('=');
            if (key) {
                result[decodeURIComponent(key)] = decodeURIComponent(value || '');
            }
        });
        return result;
    },

    /**
     * Format file size in human readable format
     * @param {number} bytes - Size in bytes
     * @returns {string} - Formatted size string
     */
    formatFileSize(bytes) {
        if (bytes === 0) return '0 B';

        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(1024));

        return parseFloat((bytes / Math.pow(1024, i)).toFixed(2)) + ' ' + sizes[i];
    },

    /**
     * Format time duration in human readable format
     * @param {number} milliseconds - Duration in milliseconds
     * @returns {string} - Formatted duration string
     */
    formatDuration(milliseconds) {
        if (milliseconds < 1000) {
            return `${milliseconds}ms`;
        } else if (milliseconds < 60000) {
            return `${(milliseconds / 1000).toFixed(2)}s`;
        } else {
            const minutes = Math.floor(milliseconds / 60000);
            const seconds = ((milliseconds % 60000) / 1000).toFixed(2);
            return `${minutes}m ${seconds}s`;
        }
    },

    /**
     * Get status category based on status code
     * @param {number} statusCode - HTTP status code
     * @returns {string} - Status category
     */
    getStatusCategory(statusCode) {
        if (POST_CONFIG.STATUS_CATEGORIES.SUCCESS.includes(statusCode)) {
            return 'success';
        } else if (POST_CONFIG.STATUS_CATEGORIES.REDIRECT.includes(statusCode)) {
            return 'info';
        } else if (POST_CONFIG.STATUS_CATEGORIES.CLIENT_ERROR.includes(statusCode)) {
            return 'error';
        } else if (POST_CONFIG.STATUS_CATEGORIES.SERVER_ERROR.includes(statusCode)) {
            return 'error';
        }
        return 'info';
    },

    /**
     * Truncate text if it exceeds maximum length
     * @param {string} text - Text to truncate
     * @param {number} maxLength - Maximum length
     * @returns {string} - Truncated text
     */
    truncateText(text, maxLength = POST_CONFIG.RESPONSE_FORMATTING.TRUNCATE_LENGTH) {
        if (text.length <= maxLength) {
            return text;
        }
        return text.substring(0, maxLength) + '\n\n... [Content truncated - too large to display]';
    },

    /**
     * Copy text to clipboard
     * @param {string} text - Text to copy
     * @returns {Promise<boolean>} - Success status
     */
    async copyToClipboard(text) {
        try {
            await navigator.clipboard.writeText(text);
            return true;
        } catch (error) {
            // Fallback for older browsers
            try {
                const textArea = document.createElement('textarea');
                textArea.value = text;
                textArea.style.position = 'fixed';
                textArea.style.opacity = '0';
                document.body.appendChild(textArea);
                textArea.focus();
                textArea.select();
                const successful = document.execCommand('copy');
                document.body.removeChild(textArea);
                return successful;
            } catch (fallbackError) {
                console.error('Failed to copy to clipboard:', fallbackError);
                return false;
            }
        }
    },

    /**
     * Download content as file
     * @param {string} content - Content to download
     * @param {string} filename - Name of the file
     * @param {string} contentType - MIME type of the content
     */
    downloadAsFile(content, filename, contentType = 'text/plain') {
        const blob = new Blob([content], { type: contentType });
        const url = URL.createObjectURL(blob);

        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        link.style.display = 'none';

        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);

        // Clean up the URL object
        setTimeout(() => URL.revokeObjectURL(url), 100);
    },

    /**
     * Generate timestamp string
     * @returns {string} - Formatted timestamp
     */
    getTimestamp() {
        return new Date().toLocaleString();
    },

    /**
     * Debounce function calls
     * @param {Function} func - Function to debounce
     * @param {number} delay - Delay in milliseconds
     * @returns {Function} - Debounced function
     */
    debounce(func, delay = POST_CONFIG.UI.DEBOUNCE_DELAY) {
        let timeoutId;
        return function (...args) {
            clearTimeout(timeoutId);
            timeoutId = setTimeout(() => func.apply(this, args), delay);
        };
    },

    /**
     * Validate URL format
     * @param {string} url - URL to validate
     * @returns {boolean} - True if valid, false otherwise
     */
    isValidUrl(url) {
        if (!url || url.trim() === '') {
            return false;
        }

        // Allow relative URLs and full URLs
        const relativeUrlPattern = /^\/.*$/;
        const fullUrlPattern = /^https?:\/\/.+$/;

        return relativeUrlPattern.test(url) || fullUrlPattern.test(url);
    },

    /**
     * Parse headers from string format
     * @param {string} headersString - Headers in string format
     * @returns {Object} - Headers object
     */
    parseHeaders(headersString) {
        const headers = {};
        if (!headersString) return headers;

        headersString.split('\n').forEach(line => {
            const [key, ...valueParts] = line.split(':');
            if (key && valueParts.length > 0) {
                headers[key.trim()] = valueParts.join(':').trim();
            }
        });

        return headers;
    },

    /**
     * Format headers object to string
     * @param {Object} headers - Headers object
     * @returns {string} - Headers in string format
     */
    formatHeaders(headers) {
        return Object.entries(headers)
            .map(([key, value]) => `${key}: ${value}`)
            .join('\n');
    },

    /**
     * Show notification message
     * @param {string} message - Message to show
     * @param {string} type - Notification type (success, error, info)
     */
    showNotification(message, type = 'info') {
        // Create notification element
        const notification = document.createElement('div');
        notification.className = `notification notification-${type}`;
        notification.textContent = message;

        // Style the notification
        Object.assign(notification.style, {
            position: 'fixed',
            top: '20px',
            right: '20px',
            padding: '15px 20px',
            borderRadius: '8px',
            color: 'white',
            fontWeight: 'bold',
            zIndex: '10000',
            opacity: '0',
            transform: 'translateX(100%)',
            transition: 'all 0.3s ease'
        });

        // Set background color based on type
        const colors = {
            success: '#2ed573',
            error: '#ff4757',
            info: '#667eea',
            warning: '#ffa502'
        };
        notification.style.backgroundColor = colors[type] || colors.info;

        // Add to DOM and animate in
        document.body.appendChild(notification);

        setTimeout(() => {
            notification.style.opacity = '1';
            notification.style.transform = 'translateX(0)';
        }, 10);

        // Remove after delay
        setTimeout(() => {
            notification.style.opacity = '0';
            notification.style.transform = 'translateX(100%)';
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.parentNode.removeChild(notification);
                }
            }, 300);
        }, 3000);
    },

    /**
     * Escape HTML special characters
     * @param {string} text - Text to escape
     * @returns {string} - Escaped text
     */
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    },

    /**
     * Generate unique ID
     * @returns {string} - Unique ID
     */
    generateId() {
        return Date.now().toString(36) + Math.random().toString(36).substr(2);
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = PostUtils;
}
