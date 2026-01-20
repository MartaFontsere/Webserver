/* =================================================================
   🗑️ DELETE TESTING - UTILITIES
   Utility functions for DELETE request testing functionality
   ================================================================= */

// Utility Functions
const DeleteUtils = {

    /**
     * Format JSON string with proper indentation
     * @param {string} jsonString - JSON string to format
     * @returns {string} - Formatted JSON string
     */
    formatJSON(jsonString) {
        try {
            const parsed = JSON.parse(jsonString);
            return JSON.stringify(parsed, null, DELETE_CONFIG.RESPONSE_HANDLING.PRETTY_PRINT_INDENT);
        } catch (error) {
            throw new Error('Invalid JSON format');
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
     * Extract resource ID from URL
     * @param {string} url - Resource URL
     * @returns {string} - Resource ID or filename
     */
    extractResourceId(url) {
        const parts = url.split('/');
        return parts[parts.length - 1] || 'unknown';
    },

    /**
     * Get resource type from URL
     * @param {string} url - Resource URL
     * @returns {string} - Resource type
     */
    getResourceType(url) {
        if (url.includes('/files/')) return 'file';
        if (url.includes('/users/')) return 'user';
        if (url.includes('/products/')) return 'product';
        if (url.includes('/posts/')) return 'post';
        if (url.includes('/comments/')) return 'comment';
        return 'resource';
    },

    /**
     * Check if URL matches dangerous patterns
     * @param {string} url - URL to check
     * @returns {boolean} - True if dangerous, false otherwise
     */
    isDangerousUrl(url) {
        return DELETE_CONFIG.SAFETY.DANGEROUS_PATTERNS.some(pattern =>
            url.toLowerCase().includes(pattern.toLowerCase())
        );
    },

    /**
     * Check if file has protected extension
     * @param {string} filename - Filename to check
     * @returns {boolean} - True if protected, false otherwise
     */
    isProtectedFile(filename) {
        const extension = '.' + filename.split('.').pop().toLowerCase();
        return DELETE_CONFIG.SAFETY.PROTECTED_EXTENSIONS.includes(extension);
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
        if (DELETE_CONFIG.STATUS_CODES.SUCCESS.includes(statusCode)) {
            return 'success';
        } else if (DELETE_CONFIG.STATUS_CODES.CLIENT_ERROR.includes(statusCode)) {
            return 'error';
        } else if (DELETE_CONFIG.STATUS_CODES.SERVER_ERROR.includes(statusCode)) {
            return 'error';
        }
        return 'info';
    },

    /**
     * Get appropriate success message for status code
     * @param {number} statusCode - HTTP status code
     * @returns {string} - Success message
     */
    getSuccessMessage(statusCode) {
        switch (statusCode) {
            case 200:
                return 'Resource deleted successfully with response.';
            case 204:
                return 'Resource deleted successfully.';
            default:
                return 'Operation completed.';
        }
    },

    /**
     * Get appropriate error message for status code
     * @param {number} statusCode - HTTP status code
     * @returns {string} - Error message
     */
    getErrorMessage(statusCode) {
        switch (statusCode) {
            case 404:
                return 'Resource not found. It may have already been deleted.';
            case 403:
                return 'Access forbidden. You do not have permission to delete this resource.';
            case 401:
                return 'Authentication required. Please log in and try again.';
            case 409:
                return 'Conflict. Resource cannot be deleted due to dependencies.';
            case 405:
                return 'Method not allowed. DELETE is not supported for this resource.';
            case 500:
                return 'Internal server error. Please try again later.';
            default:
                return `Deletion failed with status ${statusCode}.`;
        }
    },

    /**
     * Truncate text if it exceeds maximum length
     * @param {string} text - Text to truncate
     * @param {number} maxLength - Maximum length
     * @returns {string} - Truncated text
     */
    truncateText(text, maxLength = DELETE_CONFIG.RESPONSE_HANDLING.TRUNCATE_LENGTH) {
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
    debounce(func, delay = DELETE_CONFIG.UI.DEBOUNCE_DELAY) {
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
     * @param {string} type - Notification type (success, error, info, warning)
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
            transition: 'all 0.3s ease',
            maxWidth: '400px',
            wordWrap: 'break-word'
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
        const displayTime = type === 'error' ? 5000 : 3000;
        setTimeout(() => {
            notification.style.opacity = '0';
            notification.style.transform = 'translateX(100%)';
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.parentNode.removeChild(notification);
                }
            }, 300);
        }, displayTime);
    },

    /**
     * Show confirmation dialog
     * @param {string} message - Confirmation message
     * @param {Function} onConfirm - Callback for confirmation
     * @param {Function} onCancel - Callback for cancellation
     */
    showConfirmation(message, onConfirm, onCancel = null) {
        const modal = document.getElementById('confirmation-modal');
        const messageElement = document.getElementById('confirmation-message');
        const confirmButton = document.getElementById('confirm-delete');
        const cancelButton = document.getElementById('cancel-delete');
        const closeButton = document.querySelector('.close-modal');

        messageElement.textContent = message;
        modal.classList.remove('hidden');

        // Clear previous event listeners
        const newConfirmButton = confirmButton.cloneNode(true);
        const newCancelButton = cancelButton.cloneNode(true);
        const newCloseButton = closeButton.cloneNode(true);

        confirmButton.parentNode.replaceChild(newConfirmButton, confirmButton);
        cancelButton.parentNode.replaceChild(newCancelButton, cancelButton);
        closeButton.parentNode.replaceChild(newCloseButton, closeButton);

        // Add new event listeners
        newConfirmButton.addEventListener('click', () => {
            modal.classList.add('hidden');
            if (onConfirm) onConfirm();
        });

        const closeModal = () => {
            modal.classList.add('hidden');
            if (onCancel) onCancel();
        };

        newCancelButton.addEventListener('click', closeModal);
        newCloseButton.addEventListener('click', closeModal);

        // Close on escape key
        const handleEscape = (e) => {
            if (e.key === 'Escape') {
                closeModal();
                document.removeEventListener('keydown', handleEscape);
            }
        };
        document.addEventListener('keydown', handleEscape);
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
    },

    /**
     * Batch process array with delay
     * @param {Array} items - Items to process
     * @param {Function} processor - Processing function
     * @param {number} batchSize - Batch size
     * @param {number} delay - Delay between batches
     * @returns {Promise} - Promise resolving when all batches complete
     */
    async batchProcess(items, processor, batchSize = DELETE_CONFIG.BULK_OPERATIONS.BATCH_SIZE, delay = 100) {
        const results = [];

        for (let i = 0; i < items.length; i += batchSize) {
            const batch = items.slice(i, i + batchSize);
            const batchResults = await Promise.allSettled(
                batch.map(item => processor(item))
            );
            results.push(...batchResults);

            // Add delay between batches to avoid overwhelming the server
            if (i + batchSize < items.length) {
                await new Promise(resolve => setTimeout(resolve, delay));
            }
        }

        return results;
    },

    /**
     * Sleep for specified duration
     * @param {number} ms - Milliseconds to sleep
     * @returns {Promise} - Promise resolving after delay
     */
    sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    },

    /**
     * Show confirmation dialog and return a promise
     * @param {string} message - Confirmation message
     * @param {string} details - Additional details (optional)
     * @returns {Promise<boolean>} - Promise that resolves with true/false
     */
    confirmAction(message, details = '') {
        return new Promise((resolve) => {
            const fullMessage = details ? `${message}\n\n${details}` : message;

            this.showConfirmation(fullMessage, () => {
                resolve(true);
            }, () => {
                resolve(false);
            });
        });
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = DeleteUtils;
}
