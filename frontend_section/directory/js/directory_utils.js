/* =================================================================
   📂 DIRECTORY EXPLORER - UTILITIES
   Utility functions for directory navigation and file management
   ================================================================= */

// Directory utility functions
const DirectoryUtils = {

    /**
     * Format file size in human readable format
     * @param {number} bytes - Size in bytes
     * @returns {string} - Formatted size string
     */
    formatFileSize(bytes) {
        if (bytes === 0 || bytes === null || bytes === undefined) return '0 B';

        const k = 1024;
        const sizes = DIRECTORY_CONFIG.SIZE_UNITS;
        const i = Math.floor(Math.log(bytes) / Math.log(k));

        if (i === 0) return bytes + ' B';

        return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
    },

    /**
     * Format date in human readable format
     * @param {string|Date} date - Date string or Date object
     * @returns {string} - Formatted date string
     */
    formatDate(date) {
        if (!date) return 'Unknown';

        const dateObj = typeof date === 'string' ? new Date(date) : date;
        if (isNaN(dateObj.getTime())) return 'Invalid Date';

        return dateObj.toLocaleDateString('en-US', DIRECTORY_CONFIG.DATE_OPTIONS);
    },

    /**
     * Get file extension from filename
     * @param {string} filename - File name
     * @returns {string} - File extension (lowercase)
     */
    getFileExtension(filename) {
        if (!filename || typeof filename !== 'string') return '';

        const lastDot = filename.lastIndexOf('.');
        if (lastDot === -1 || lastDot === filename.length - 1) return '';

        return filename.substring(lastDot + 1).toLowerCase();
    },

    /**
     * Get file type information
     * @param {string} filename - File name
     * @param {string} type - File type ('file' or 'directory')
     * @returns {Object} - File type info with icon and category
     */
    getFileTypeInfo(filename, type) {
        if (type === 'directory') {
            return DIRECTORY_CONFIG.FILE_TYPES.directory;
        }

        const extension = this.getFileExtension(filename);
        return DIRECTORY_CONFIG.FILE_TYPES[extension] || DIRECTORY_CONFIG.FILE_TYPES.unknown;
    },

    /**
     * Validate directory path
     * @param {string} path - Directory path
     * @returns {boolean} - True if valid path
     */
    isValidPath(path) {
        if (!path || typeof path !== 'string') return false;

        // Check length
        if (path.length > DIRECTORY_CONFIG.SECURITY.MAX_PATH_LENGTH) return false;

        // Check for forbidden characters
        const forbidden = DIRECTORY_CONFIG.SECURITY.FORBIDDEN_CHARS;
        for (const char of forbidden) {
            if (path.includes(char)) return false;
        }

        // Check for relative path traversal
        if (path.includes('..') || path.includes('./')) return false;

        // Must start with /
        if (!path.startsWith('/')) return false;

        return true;
    },

    /**
     * Normalize directory path
     * @param {string} path - Directory path
     * @returns {string} - Normalized path
     */
    normalizePath(path) {
        if (!path || typeof path !== 'string') return '/';

        // Remove trailing slash except for root
        path = path.replace(/\/+$/, '') || '/';

        // Ensure starts with /
        if (!path.startsWith('/')) path = '/' + path;

        // Remove double slashes
        path = path.replace(/\/+/g, '/');

        return path;
    },

    /**
     * Get parent directory path
     * @param {string} path - Current path
     * @returns {string} - Parent directory path
     */
    getParentPath(path) {
        if (!path || path === '/') return '/';

        const normalizedPath = this.normalizePath(path);
        const lastSlash = normalizedPath.lastIndexOf('/');

        if (lastSlash === 0) return '/';
        return normalizedPath.substring(0, lastSlash);
    },

    /**
     * Get directory name from path
     * @param {string} path - Directory path
     * @returns {string} - Directory name
     */
    getDirectoryName(path) {
        if (!path || path === '/') return 'Root';

        const normalizedPath = this.normalizePath(path);
        const parts = normalizedPath.split('/').filter(part => part.length > 0);

        return parts[parts.length - 1] || 'Root';
    },

    /**
     * Split path into breadcrumb parts
     * @param {string} path - Directory path
     * @returns {Array} - Array of breadcrumb objects
     */
    getBreadcrumbs(path) {
        const breadcrumbs = [{ name: 'Root', path: '/' }];

        if (!path || path === '/') return breadcrumbs;

        const normalizedPath = this.normalizePath(path);
        const parts = normalizedPath.split('/').filter(part => part.length > 0);

        let currentPath = '';
        for (const part of parts) {
            currentPath += '/' + part;
            breadcrumbs.push({
                name: part,
                path: currentPath
            });
        }

        return breadcrumbs;
    },

    /**
     * Check if file can be previewed
     * @param {string} filename - File name
     * @param {number} size - File size in bytes
     * @returns {boolean} - True if file can be previewed
     */
    canPreviewFile(filename, size) {
        const typeInfo = this.getFileTypeInfo(filename, 'file');

        if (!typeInfo.preview) return false;

        if (size > DIRECTORY_CONFIG.DEFAULTS.MAX_FILE_SIZE_PREVIEW) return false;

        return true;
    },

    /**
     * Get preview type for file
     * @param {string} filename - File name
     * @returns {string} - Preview type ('text', 'image', 'none')
     */
    getPreviewType(filename) {
        const extension = this.getFileExtension(filename);

        if (DIRECTORY_CONFIG.PREVIEW.SUPPORTED_IMAGES.includes(extension)) {
            return 'image';
        }

        if (DIRECTORY_CONFIG.PREVIEW.SUPPORTED_TEXT.includes(extension)) {
            return 'text';
        }

        return 'none';
    },

    /**
     * Sanitize filename
     * @param {string} filename - Original filename
     * @returns {string} - Sanitized filename
     */
    sanitizeFilename(filename) {
        if (!filename || typeof filename !== 'string') return 'untitled';

        let sanitized = filename;

        // Remove forbidden characters
        const forbidden = DIRECTORY_CONFIG.SECURITY.FORBIDDEN_CHARS;
        for (const char of forbidden) {
            sanitized = sanitized.replace(new RegExp('\\' + char, 'g'), '_');
        }

        // Remove leading/trailing whitespace and dots
        sanitized = sanitized.trim().replace(/^\.+/, '').replace(/\.+$/, '');

        // Limit length
        if (sanitized.length > 255) {
            sanitized = sanitized.substring(0, 255);
        }

        return sanitized || 'untitled';
    },

    /**
     * Check if file extension is dangerous
     * @param {string} filename - File name
     * @returns {boolean} - True if potentially dangerous
     */
    isDangerousFile(filename) {
        const extension = this.getFileExtension(filename);
        return DIRECTORY_CONFIG.SECURITY.DANGEROUS_EXTENSIONS.includes(extension);
    },

    /**
     * Sort directory items
     * @param {Array} items - Array of directory items
     * @param {string} sortBy - Sort field ('name', 'type', 'size', 'modified')
     * @param {string} order - Sort order ('asc', 'desc')
     * @returns {Array} - Sorted array
     */
    sortItems(items, sortBy = 'name', order = 'asc') {
        if (!Array.isArray(items)) return [];

        const sorted = [...items].sort((a, b) => {
            // Directories first
            if (a.type === 'directory' && b.type !== 'directory') return -1;
            if (b.type === 'directory' && a.type !== 'directory') return 1;

            let compareValue = 0;

            switch (sortBy) {
                case 'name':
                    compareValue = a.name.toLowerCase().localeCompare(b.name.toLowerCase());
                    break;
                case 'type':
                    const aExt = this.getFileExtension(a.name);
                    const bExt = this.getFileExtension(b.name);
                    compareValue = aExt.localeCompare(bExt);
                    break;
                case 'size':
                    compareValue = (a.size || 0) - (b.size || 0);
                    break;
                case 'modified':
                    const aDate = new Date(a.modified || 0);
                    const bDate = new Date(b.modified || 0);
                    compareValue = aDate.getTime() - bDate.getTime();
                    break;
                default:
                    compareValue = a.name.toLowerCase().localeCompare(b.name.toLowerCase());
            }

            return order === 'asc' ? compareValue : -compareValue;
        });

        return sorted;
    },

    /**
     * Filter directory items
     * @param {Array} items - Array of directory items
     * @param {string} searchTerm - Search term
     * @param {string} typeFilter - Type filter ('all', 'file', 'directory', etc.)
     * @returns {Array} - Filtered array
     */
    filterItems(items, searchTerm = '', typeFilter = 'all') {
        if (!Array.isArray(items)) return [];

        return items.filter(item => {
            // Search filter
            if (searchTerm) {
                const term = searchTerm.toLowerCase();
                if (!item.name.toLowerCase().includes(term)) {
                    return false;
                }
            }

            // Type filter
            if (typeFilter !== 'all') {
                if (typeFilter === 'directory' && item.type !== 'directory') {
                    return false;
                }
                if (typeFilter === 'file' && item.type !== 'file') {
                    return false;
                }
                if (typeFilter === 'image') {
                    const typeInfo = this.getFileTypeInfo(item.name, item.type);
                    if (typeInfo.category !== 'image') {
                        return false;
                    }
                }
                if (typeFilter === 'document') {
                    const typeInfo = this.getFileTypeInfo(item.name, item.type);
                    if (typeInfo.category !== 'document') {
                        return false;
                    }
                }
                if (typeFilter === 'archive') {
                    const typeInfo = this.getFileTypeInfo(item.name, item.type);
                    if (typeInfo.category !== 'archive') {
                        return false;
                    }
                }
            }

            return true;
        });
    },

    /**
     * Calculate total size of items
     * @param {Array} items - Array of directory items
     * @returns {number} - Total size in bytes
     */
    calculateTotalSize(items) {
        if (!Array.isArray(items)) return 0;

        return items.reduce((total, item) => {
            return total + (item.size || 0);
        }, 0);
    },

    /**
     * Get items count by type
     * @param {Array} items - Array of directory items
     * @returns {Object} - Object with count by type
     */
    getItemsCount(items) {
        if (!Array.isArray(items)) return { total: 0, files: 0, directories: 0 };

        const count = {
            total: items.length,
            files: 0,
            directories: 0
        };

        items.forEach(item => {
            if (item.type === 'directory') {
                count.directories++;
            } else {
                count.files++;
            }
        });

        return count;
    },

    /**
     * Generate unique ID
     * @returns {string} - Unique identifier
     */
    generateId() {
        return 'dir_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
    },

    /**
     * Debounce function calls
     * @param {Function} func - Function to debounce
     * @param {number} delay - Delay in milliseconds
     * @returns {Function} - Debounced function
     */
    debounce(func, delay) {
        let timeoutId;
        return function (...args) {
            clearTimeout(timeoutId);
            timeoutId = setTimeout(() => func.apply(this, args), delay);
        };
    },

    /**
     * Throttle function calls
     * @param {Function} func - Function to throttle
     * @param {number} delay - Delay in milliseconds
     * @returns {Function} - Throttled function
     */
    throttle(func, delay) {
        let inThrottle;
        return function (...args) {
            if (!inThrottle) {
                func.apply(this, args);
                inThrottle = true;
                setTimeout(() => inThrottle = false, delay);
            }
        };
    },

    /**
     * Show notification message
     * @param {string} message - Notification message
     * @param {string} type - Notification type ('success', 'error', 'warning', 'info')
     * @param {number} duration - Duration in milliseconds
     */
    showNotification(message, type = 'info', duration = 3000) {
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
            success: '#4CAF50',
            error: '#FF6B6B',
            warning: '#FFA726',
            info: '#42A5F5'
        };
        notification.style.backgroundColor = colors[type] || colors.info;

        // Add to DOM and animate in
        document.body.appendChild(notification);

        // Trigger animation
        setTimeout(() => {
            notification.style.opacity = '1';
            notification.style.transform = 'translateX(0)';
        }, 10);

        // Auto remove
        setTimeout(() => {
            notification.style.opacity = '0';
            notification.style.transform = 'translateX(100%)';
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.parentNode.removeChild(notification);
                }
            }, 300);
        }, duration);
    },

    /**
     * Copy text to clipboard
     * @param {string} text - Text to copy
     * @returns {Promise<boolean>} - Success status
     */
    async copyToClipboard(text) {
        try {
            if (navigator.clipboard && window.isSecureContext) {
                await navigator.clipboard.writeText(text);
                return true;
            } else {
                // Fallback for older browsers
                const textArea = document.createElement('textarea');
                textArea.value = text;
                textArea.style.position = 'fixed';
                textArea.style.left = '-999999px';
                textArea.style.top = '-999999px';
                document.body.appendChild(textArea);
                textArea.focus();
                textArea.select();
                const result = document.execCommand('copy');
                document.body.removeChild(textArea);
                return result;
            }
        } catch (error) {
            console.error('Failed to copy to clipboard:', error);
            return false;
        }
    },

    /**
     * Save preferences to localStorage
     * @param {Object} preferences - User preferences
     */
    savePreferences(preferences) {
        try {
            localStorage.setItem(
                DIRECTORY_CONFIG.STORAGE_KEYS.PREFERENCES,
                JSON.stringify(preferences)
            );
        } catch (error) {
            console.warn('Failed to save preferences:', error);
        }
    },

    /**
     * Load preferences from localStorage
     * @returns {Object} - User preferences
     */
    loadPreferences() {
        try {
            const stored = localStorage.getItem(DIRECTORY_CONFIG.STORAGE_KEYS.PREFERENCES);
            return stored ? JSON.parse(stored) : {};
        } catch (error) {
            console.warn('Failed to load preferences:', error);
            return {};
        }
    },

    /**
     * Get current timestamp
     * @returns {string} - ISO timestamp
     */
    getTimestamp() {
        return new Date().toISOString();
    },

    /**
     * Sleep for specified milliseconds
     * @param {number} ms - Milliseconds to sleep
     * @returns {Promise} - Promise that resolves after delay
     */
    sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = DirectoryUtils;
}

// Exportar para uso global en el navegador
window.DirectoryUtils = DirectoryUtils;
