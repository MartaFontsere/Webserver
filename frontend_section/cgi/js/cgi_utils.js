/**
 * ================================================================
 * ⚙️ CGI TESTING - UTILITIES
 * Utility functions and helpers for CGI script testing
 * ================================================================
 */

// Utilities object
const CGI_UTILS = {

    /**
     * Show status indicators
     */
    showIndicator(type) {
        // Hide all indicators first
        const indicators = document.querySelectorAll('.indicator');
        indicators.forEach(indicator => indicator.classList.add('hidden'));

        // Show specific indicator
        const indicator = document.getElementById(`${type}-indicator`);
        if (indicator) {
            indicator.classList.remove('hidden');

            // Auto-hide success and error indicators
            if (type === 'success' || type === 'error') {
                setTimeout(() => {
                    indicator.classList.add('hidden');
                }, 3000);
            }
        }
    },

    /**
     * Update execution status
     */
    updateExecutionStatus(status, type = 'default') {
        const statusElement = document.getElementById('execution-status');
        if (statusElement) {
            statusElement.textContent = status;
            statusElement.className = `status-code ${type}`;
        }
    },

    /**
     * Show temporary message
     */
    showMessage(message, type = 'info') {
        // Create message element if it doesn't exist
        let messageContainer = document.getElementById('message-container');
        if (!messageContainer) {
            messageContainer = document.createElement('div');
            messageContainer.id = 'message-container';
            messageContainer.className = 'message-container';
            document.body.appendChild(messageContainer);
        }

        // Create message element
        const messageElement = document.createElement('div');
        messageElement.className = `message message-${type}`;
        messageElement.innerHTML = `
            <span class="message-icon">${this.getMessageIcon(type)}</span>
            <span class="message-text">${message}</span>
        `;

        // Add to container
        messageContainer.appendChild(messageElement);

        // Animate in
        setTimeout(() => messageElement.classList.add('show'), 10);

        // Auto remove after delay
        setTimeout(() => {
            messageElement.classList.add('hide');
            setTimeout(() => {
                if (messageElement.parentNode) {
                    messageElement.parentNode.removeChild(messageElement);
                }
            }, 300);
        }, 3000);
    },

    /**
     * Get icon for message type
     */
    getMessageIcon(type) {
        const icons = {
            'success': '✅',
            'error': '❌',
            'warning': '⚠️',
            'info': 'ℹ️'
        };
        return icons[type] || 'ℹ️';
    },

    /**
     * Format bytes to human readable format
     */
    formatBytes(bytes) {
        if (bytes === 0) return '0 Bytes';

        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));

        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    },

    /**
     * Format execution time
     */
    formatTime(milliseconds) {
        if (milliseconds < 1000) {
            return `${milliseconds}ms`;
        } else if (milliseconds < 60000) {
            return `${(milliseconds / 1000).toFixed(2)}s`;
        } else {
            const minutes = Math.floor(milliseconds / 60000);
            const seconds = ((milliseconds % 60000) / 1000).toFixed(0);
            return `${minutes}m ${seconds}s`;
        }
    },

    /**
     * Escape HTML for safe display
     */
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    },

    /**
     * Generate random number between min and max
     */
    randomBetween(min, max) {
        return Math.floor(Math.random() * (max - min + 1)) + min;
    },

    /**
     * Create delay promise
     */
    delay(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    },

    /**
     * Validate CGI script path
     */
    validateScriptPath(path) {
        if (!path || typeof path !== 'string') {
            return { valid: false, error: 'Script path is required' };
        }

        const trimmedPath = path.trim();

        if (trimmedPath.length === 0) {
            return { valid: false, error: 'Script path cannot be empty' };
        }

        if (!trimmedPath.startsWith('/')) {
            return { valid: false, error: 'Script path must start with /' };
        }

        if (trimmedPath.includes('..')) {
            return { valid: false, error: 'Script path cannot contain ..' };
        }

        return { valid: true, path: trimmedPath };
    },

    /**
     * Parse query string
     */
    parseQueryString(queryString) {
        const params = {};
        if (!queryString) return params;

        const pairs = queryString.split('&');
        pairs.forEach(pair => {
            const [key, value] = pair.split('=');
            if (key) {
                params[decodeURIComponent(key)] = decodeURIComponent(value || '');
            }
        });

        return params;
    },

    /**
     * Build query string from parameters
     */
    buildQueryString(params) {
        if (!params || Object.keys(params).length === 0) {
            return '';
        }

        return Object.entries(params)
            .filter(([key, value]) => key.trim() !== '')
            .map(([key, value]) => `${encodeURIComponent(key)}=${encodeURIComponent(value)}`)
            .join('&');
    },

    /**
     * Validate environment variable name
     */
    validateEnvVarName(name) {
        if (!name || typeof name !== 'string') {
            return false;
        }

        // Environment variable names should be valid identifiers
        const envVarPattern = /^[A-Z_][A-Z0-9_]*$/i;
        return envVarPattern.test(name);
    },

    /**
     * Copy text to clipboard
     */
    async copyToClipboard(text) {
        try {
            await navigator.clipboard.writeText(text);
            this.showMessage('Copied to clipboard', 'success');
            return true;
        } catch (error) {
            console.error('Failed to copy to clipboard:', error);
            this.showMessage('Failed to copy to clipboard', 'error');
            return false;
        }
    },

    /**
     * Download content as file
     */
    downloadAsFile(content, filename, contentType = 'text/plain') {
        const blob = new Blob([content], { type: contentType });
        const url = URL.createObjectURL(blob);

        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        document.body.appendChild(link);
        link.click();

        document.body.removeChild(link);
        URL.revokeObjectURL(url);
    },

    /**
     * Get current timestamp
     */
    getCurrentTimestamp() {
        return new Date().toISOString();
    },

    /**
     * Format timestamp for display
     */
    formatTimestamp(timestamp) {
        const date = new Date(timestamp);
        return date.toLocaleString();
    },

    /**
     * Debounce function calls
     */
    debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    },

    /**
     * Throttle function calls
     */
    throttle(func, limit) {
        let inThrottle;
        return function () {
            const args = arguments;
            const context = this;
            if (!inThrottle) {
                func.apply(context, args);
                inThrottle = true;
                setTimeout(() => inThrottle = false, limit);
            }
        };
    },

    /**
     * Deep clone object
     */
    deepClone(obj) {
        if (obj === null || typeof obj !== 'object') {
            return obj;
        }

        if (obj instanceof Date) {
            return new Date(obj.getTime());
        }

        if (obj instanceof Array) {
            return obj.map(item => this.deepClone(item));
        }

        if (typeof obj === 'object') {
            const clonedObj = {};
            for (const key in obj) {
                if (obj.hasOwnProperty(key)) {
                    clonedObj[key] = this.deepClone(obj[key]);
                }
            }
            return clonedObj;
        }
    },

    /**
     * Check if object is empty
     */
    isEmpty(obj) {
        if (obj === null || obj === undefined) {
            return true;
        }

        if (Array.isArray(obj) || typeof obj === 'string') {
            return obj.length === 0;
        }

        if (typeof obj === 'object') {
            return Object.keys(obj).length === 0;
        }

        return false;
    },

    /**
     * Truncate text with ellipsis
     */
    truncateText(text, maxLength = 100) {
        if (!text || text.length <= maxLength) {
            return text;
        }
        return text.substring(0, maxLength - 3) + '...';
    },

    /**
     * Highlight syntax in code
     */
    highlightSyntax(code, language = 'text') {
        // Basic syntax highlighting for common formats
        if (language === 'json') {
            return this.highlightJson(code);
        } else if (language === 'html') {
            return this.highlightHtml(code);
        } else {
            return this.escapeHtml(code);
        }
    },

    /**
     * Highlight JSON syntax
     */
    highlightJson(json) {
        try {
            const parsed = JSON.parse(json);
            const formatted = JSON.stringify(parsed, null, 2);

            return formatted
                .replace(/(".*?")\s*:/g, '<span class="json-key">$1</span>:')
                .replace(/:\s*(".*?")/g, ': <span class="json-string">$1</span>')
                .replace(/:\s*(\d+)/g, ': <span class="json-number">$1</span>')
                .replace(/:\s*(true|false|null)/g, ': <span class="json-boolean">$1</span>');
        } catch (error) {
            return this.escapeHtml(json);
        }
    },

    /**
     * Highlight HTML syntax
     */
    highlightHtml(html) {
        return this.escapeHtml(html)
            .replace(/(&lt;\/?[^&gt;]+&gt;)/g, '<span class="html-tag">$1</span>')
            .replace(/(&lt;!--.*?--&gt;)/g, '<span class="html-comment">$1</span>');
    },

    /**
     * Check if running in production mode
     */
    isProduction() {
        return CGI_CONFIG.PRODUCTION_MODE || false;
    },

    /**
     * Log debug information
     */
    debug(...args) {
        if (CGI_CONFIG.DEBUG_MODE) {
            console.log('[CGI DEBUG]', ...args);
        }
    },

    /**
     * Log error information
     */
    error(...args) {
        console.error('[CGI ERROR]', ...args);
    },

    /**
     * Get browser information
     */
    getBrowserInfo() {
        return {
            userAgent: navigator.userAgent,
            platform: navigator.platform,
            language: navigator.language,
            cookieEnabled: navigator.cookieEnabled,
            onLine: navigator.onLine
        };
    },

    /**
     * Format HTTP status code
     */
    formatStatusCode(code) {
        const statusCodes = {
            200: { text: 'OK', class: 'success' },
            201: { text: 'Created', class: 'success' },
            204: { text: 'No Content', class: 'success' },
            400: { text: 'Bad Request', class: 'error' },
            401: { text: 'Unauthorized', class: 'error' },
            403: { text: 'Forbidden', class: 'error' },
            404: { text: 'Not Found', class: 'error' },
            405: { text: 'Method Not Allowed', class: 'error' },
            500: { text: 'Internal Server Error', class: 'error' },
            502: { text: 'Bad Gateway', class: 'error' },
            503: { text: 'Service Unavailable', class: 'error' },
            504: { text: 'Gateway Timeout', class: 'error' }
        };

        const statusCode = parseInt(code);
        const status = statusCodes[statusCode] || { text: 'Unknown', class: 'default' };

        return {
            code: statusCode,
            text: status.text,
            class: status.class,
            full: `${statusCode} ${status.text}`
        };
    },

    /**
     * Generate execution ID
     */
    generateExecutionId() {
        return `exec_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    },

    /**
     * Sanitize filename
     */
    sanitizeFilename(filename) {
        return filename
            .replace(/[^a-z0-9.-]/gi, '_')
            .replace(/_{2,}/g, '_')
            .replace(/^_|_$/g, '');
    },

    /**
     * Check if string is valid JSON
     */
    isValidJson(str) {
        try {
            JSON.parse(str);
            return true;
        } catch (error) {
            return false;
        }
    },

    /**
     * Format duration
     */
    formatDuration(start, end) {
        const duration = end - start;
        return this.formatTime(duration);
    },

    /**
     * Get file extension from path
     */
    getFileExtension(path) {
        const lastDot = path.lastIndexOf('.');
        return lastDot > 0 ? path.substring(lastDot + 1).toLowerCase() : '';
    },

    /**
     * Check if URL is valid
     */
    isValidUrl(string) {
        try {
            new URL(string);
            return true;
        } catch (_) {
            return false;
        }
    }
};
