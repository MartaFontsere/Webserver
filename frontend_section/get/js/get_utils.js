/**
 * ================================================================
 * 📥 GET TESTING - UTILITY FUNCTIONS
 * Helper functions for formatting, validation, and UI updates
 * ================================================================
 */

class GetUtils {
    /**
     * Format byte size to human readable format
     * @param {number} bytes - Size in bytes
     * @returns {string} Formatted size
     */
    static formatBytes(bytes) {
        if (bytes === 0) return '0 B';

        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));

        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    /**
     * Format duration to human readable format
     * @param {number} ms - Duration in milliseconds
     * @returns {string} Formatted duration
     */
    static formatDuration(ms) {
        if (ms < 1000) {
            return `${ms}ms`;
        } else if (ms < 60000) {
            return `${(ms / 1000).toFixed(2)}s`;
        } else {
            const minutes = Math.floor(ms / 60000);
            const seconds = ((ms % 60000) / 1000).toFixed(0);
            return `${minutes}m ${seconds}s`;
        }
    }

    /**
     * Format timestamp to readable date
     * @param {string} timestamp - ISO timestamp
     * @returns {string} Formatted date and time
     */
    static formatTimestamp(timestamp) {
        const date = new Date(timestamp);
        return date.toLocaleString();
    }

    /**
     * Get status code information
     * @param {number} status - HTTP status code
     * @returns {Object} Status information
     */
    static getStatusInfo(status) {
        return GET_CONFIG.STATUS_CODES[status] || {
            name: 'Unknown',
            class: 'default',
            color: '#6c757d'
        };
    }

    /**
     * Get content type information
     * @param {string} contentType - Content-Type header value
     * @returns {Object} Content type information
     */
    static getContentTypeInfo(contentType) {
        // Extract main type (remove charset, etc.)
        const mainType = contentType ? contentType.split(';')[0].trim().toLowerCase() : 'unknown';

        return GET_CONFIG.CONTENT_TYPES[mainType] || {
            name: 'Unknown',
            preview: false,
            format: false
        };
    }

    /**
     * Validate URL format
     * @param {string} url - URL to validate
     * @returns {boolean} True if valid
     */
    static isValidUrl(url) {
        if (!url || typeof url !== 'string') return false;

        // Allow relative URLs (starting with /)
        if (url.startsWith('/')) return true;

        // Allow absolute URLs
        try {
            new URL(url);
            return true;
        } catch {
            return false;
        }
    }

    /**
     * Normalize URL for requests
     * @param {string} url - URL to normalize
     * @returns {string} Normalized URL
     */
    static normalizeUrl(url) {
        if (!url) return '/';

        // Trim whitespace
        url = url.trim();

        // Ensure starts with /
        if (!url.startsWith('/') && !url.startsWith('http')) {
            url = '/' + url;
        }

        return url;
    }

    /**
     * Extract filename from URL
     * @param {string} url - URL to extract filename from
     * @returns {string} Filename or 'index'
     */
    static getFilenameFromUrl(url) {
        if (!url) return 'index';

        const parts = url.split('/');
        const filename = parts[parts.length - 1];

        return filename || 'index';
    }

    /**
     * Try to format JSON string
     * @param {string} text - Text that might be JSON
     * @returns {string} Formatted JSON or original text
     */
    static tryFormatJson(text) {
        try {
            const parsed = JSON.parse(text);
            return JSON.stringify(parsed, null, 2);
        } catch {
            return text;
        }
    }

    /**
     * Try to format XML string
     * @param {string} text - Text that might be XML
     * @returns {string} Formatted XML or original text
     */
    static tryFormatXml(text) {
        try {
            const parser = new DOMParser();
            const xmlDoc = parser.parseFromString(text, 'text/xml');

            if (xmlDoc.querySelector('parsererror')) {
                return text;
            }

            const serializer = new XMLSerializer();
            const formatted = serializer.serializeToString(xmlDoc);

            // Basic indentation (simple approach)
            return formatted.replace(/></g, '>\n<');
        } catch {
            return text;
        }
    }

    /**
     * Escape HTML for safe display
     * @param {string} text - Text to escape
     * @returns {string} Escaped text
     */
    static escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    /**
     * Truncate text to specified length
     * @param {string} text - Text to truncate
     * @param {number} maxLength - Maximum length
     * @returns {string} Truncated text
     */
    static truncateText(text, maxLength = 100) {
        if (!text || text.length <= maxLength) return text;

        return text.substring(0, maxLength) + '...';
    }

    /**
     * Generate unique ID
     * @returns {string} Unique ID
     */
    static generateId() {
        return Date.now().toString(36) + Math.random().toString(36).substr(2);
    }

    /**
     * Copy text to clipboard
     * @param {string} text - Text to copy
     * @returns {Promise<boolean>} Success status
     */
    static async copyToClipboard(text) {
        try {
            await navigator.clipboard.writeText(text);
            return true;
        } catch {
            // Fallback for older browsers
            const textArea = document.createElement('textarea');
            textArea.value = text;
            document.body.appendChild(textArea);
            textArea.focus();
            textArea.select();

            try {
                document.execCommand('copy');
                return true;
            } catch {
                return false;
            } finally {
                document.body.removeChild(textArea);
            }
        }
    }

    /**
     * Download text as file
     * @param {string} text - Text content
     * @param {string} filename - Download filename
     * @param {string} mimeType - MIME type
     */
    static downloadAsFile(text, filename, mimeType = 'text/plain') {
        const blob = new Blob([text], { type: mimeType });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();

        setTimeout(() => {
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
        }, 100);
    }

    /**
     * Debounce function calls
     * @param {Function} func - Function to debounce
     * @param {number} wait - Wait time in ms
     * @returns {Function} Debounced function
     */
    static debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    }

    /**
     * Show temporary notification
     * @param {string} message - Message to show
     * @param {string} type - Type: success, error, info
     * @param {number} duration - Duration in ms
     */
    static showNotification(message, type = 'info', duration = 3000) {
        // Create notification element
        const notification = document.createElement('div');
        notification.className = `notification notification-${type}`;
        notification.textContent = message;

        // Style the notification
        Object.assign(notification.style, {
            position: 'fixed',
            top: '20px',
            left: '50%',
            transform: 'translateX(-50%)',
            padding: '12px 20px',
            borderRadius: '8px',
            color: 'white',
            fontWeight: '600',
            zIndex: '10000',
            boxShadow: '0 4px 15px rgba(0, 0, 0, 0.2)',
            transition: 'all 0.3s ease'
        });

        // Set background color based on type
        const colors = {
            success: '#28a745',
            error: '#dc3545',
            info: '#007bff',
            warning: '#ffc107'
        };
        notification.style.backgroundColor = colors[type] || colors.info;

        // Add to page
        document.body.appendChild(notification);

        // Remove after duration
        setTimeout(() => {
            notification.style.opacity = '0';
            notification.style.transform = 'translateX(-50%) translateY(-20px)';

            setTimeout(() => {
                if (notification.parentNode) {
                    notification.parentNode.removeChild(notification);
                }
            }, 300);
        }, duration);
    }

    /**
     * Parse query parameters from URL
     * @param {string} url - URL to parse
     * @returns {Object} Query parameters
     */
    static parseQueryParams(url) {
        const params = {};
        const queryString = url.split('?')[1];

        if (queryString) {
            queryString.split('&').forEach(param => {
                const [key, value] = param.split('=');
                if (key) {
                    params[decodeURIComponent(key)] = value ? decodeURIComponent(value) : '';
                }
            });
        }

        return params;
    }

    /**
     * Build query string from parameters
     * @param {Object} params - Parameters object
     * @returns {string} Query string
     */
    static buildQueryString(params) {
        const entries = Object.entries(params)
            .filter(([key, value]) => key && value !== undefined && value !== null)
            .map(([key, value]) => `${encodeURIComponent(key)}=${encodeURIComponent(value)}`);

        return entries.length > 0 ? '?' + entries.join('&') : '';
    }
}
