/* =================================================================
   📝 POST TESTING - MAIN
   Main initialization and event binding for POST request testing
   ================================================================= */

// Main Application
const PostApp = {

    /**
     * Initialize the application
     */
    init() {
        console.log('🚀 Initializing POST Testing Application...');

        // Initialize history
        PostHistory.init();

        // Bind event listeners
        this.bindEventListeners();

        // Initialize UI state
        this.initializeUI();

        console.log('✅ POST Testing Application initialized successfully');
    },

    /**
     * Bind all event listeners
     */
    bindEventListeners() {
        // Send request button
        document.getElementById('send-request').addEventListener('click', () => {
            this.handleSendRequest();
        });

        // Quick test buttons
        document.querySelectorAll('.quick-btn').forEach(button => {
            button.addEventListener('click', (e) => {
                const testKey = e.target.dataset.test;
                PostHandlers.runQuickTest(testKey);
            });
        });

        // Tab switching
        document.querySelectorAll('.tab-button').forEach(button => {
            button.addEventListener('click', (e) => {
                const tabName = e.target.dataset.tab;
                PostHandlers.switchToTab(tabName);
            });
        });

        // Response tab switching
        document.querySelectorAll('.response-tab-button').forEach(button => {
            button.addEventListener('click', (e) => {
                const tabName = e.target.dataset.tab;
                PostHandlers.switchToResponseTab(tabName);
            });
        });

        // Content type change
        document.getElementById('content-type').addEventListener('change', () => {
            PostHandlers.handleContentTypeChange();
        });

        // Form data management
        document.getElementById('add-form-row').addEventListener('click', () => {
            PostHandlers.addFormRow();
        });

        // Headers management
        document.getElementById('add-header-row').addEventListener('click', () => {
            PostHandlers.addHeaderRow();
        });

        // JSON controls
        document.getElementById('format-json').addEventListener('click', () => {
            PostHandlers.formatJSON();
        });

        document.getElementById('validate-json').addEventListener('click', () => {
            PostHandlers.validateJSON();
        });

        // Response controls
        document.getElementById('format-response').addEventListener('click', () => {
            PostHandlers.formatResponse();
        });

        document.getElementById('copy-response').addEventListener('click', () => {
            PostHandlers.copyResponse();
        });

        document.getElementById('download-response').addEventListener('click', () => {
            PostHandlers.downloadResponse();
        });

        // History controls
        document.getElementById('clear-history').addEventListener('click', () => {
            if (confirm('Are you sure you want to clear all history?')) {
                PostHistory.clearHistory();
            }
        });

        document.getElementById('export-history').addEventListener('click', () => {
            PostHistory.exportHistory();
        });

        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => {
            this.handleKeyboardShortcuts(e);
        });

        // Auto-format JSON on blur (debounced)
        const jsonTextarea = document.getElementById('json-body');
        const debouncedFormat = PostUtils.debounce(() => {
            if (POST_CONFIG.UI.AUTO_FORMAT_JSON && jsonTextarea.value.trim()) {
                try {
                    PostHandlers.formatJSON();
                } catch (error) {
                    // Ignore formatting errors during typing
                }
            }
        }, 1000);

        jsonTextarea.addEventListener('blur', debouncedFormat);

        console.log('🔗 Event listeners bound successfully');
    },

    /**
     * Initialize UI state
     */
    initializeUI() {
        // Add initial form and header rows
        if (document.querySelectorAll('.form-row').length === 0) {
            PostHandlers.addFormRow();
        }

        if (document.querySelectorAll('.header-row').length === 0) {
            PostHandlers.addHeaderRow();
        }

        // Set default values
        document.getElementById('request-url').value = POST_CONFIG.DEFAULT_ENDPOINTS.POST;
        document.getElementById('content-type').value = POST_CONFIG.CONTENT_TYPES.JSON;

        console.log('🎨 UI initialized with default values');
    },

    /**
     * Handle send request button click
     */
    async handleSendRequest() {
        try {
            // Get current request configuration
            const requestConfig = PostHandlers.getCurrentRequestConfig();

            // Validate request
            const validation = this.validateRequest(requestConfig);
            if (!validation.valid) {
                PostUtils.showNotification(validation.message, 'error');
                return;
            }

            // Show loading state
            this.setLoadingState(true);

            console.log('📤 Preparing to send request:', requestConfig);

            // Send request
            const response = await PostHandlers.sendRequest(requestConfig);

            // Display response
            PostHandlers.displayResponse(response);

            // Save to history
            if (POST_CONFIG.HISTORY.AUTO_SAVE) {
                PostHistory.saveToHistory(requestConfig, response);
            }

            // Show success notification
            const statusCategory = PostUtils.getStatusCategory(response.status);
            const notificationType = statusCategory === 'success' ? 'success' : 'info';
            PostUtils.showNotification(
                `Request completed: ${response.status} ${response.statusText}`,
                notificationType
            );

        } catch (error) {
            console.error('❌ Request failed:', error);
            PostUtils.showNotification('Request failed: ' + error.message, 'error');
        } finally {
            // Hide loading state
            this.setLoadingState(false);
        }
    },

    /**
     * Validate request configuration
     * @param {Object} requestConfig - Request configuration to validate
     * @returns {Object} - Validation result
     */
    validateRequest(requestConfig) {
        // Check URL
        if (!requestConfig.url) {
            return { valid: false, message: POST_CONFIG.ERROR_MESSAGES.EMPTY_URL };
        }

        if (!PostUtils.isValidUrl(requestConfig.url)) {
            return { valid: false, message: POST_CONFIG.ERROR_MESSAGES.INVALID_URL };
        }

        // Validate JSON if content type is JSON
        if (requestConfig.contentType === POST_CONFIG.CONTENT_TYPES.JSON && requestConfig.body) {
            if (!PostUtils.isValidJSON(requestConfig.body)) {
                return { valid: false, message: POST_CONFIG.ERROR_MESSAGES.INVALID_JSON };
            }
        }

        return { valid: true };
    },

    /**
     * Set loading state
     * @param {boolean} loading - Whether to show loading state
     */
    setLoadingState(loading) {
        const sendButton = document.getElementById('send-request');
        const loadingIndicator = document.getElementById('loading-indicator');

        if (loading) {
            sendButton.disabled = true;
            sendButton.textContent = '⏳ Sending...';
            loadingIndicator.classList.remove('hidden');
        } else {
            sendButton.disabled = false;
            sendButton.textContent = '📤 Send POST Request';
            loadingIndicator.classList.add('hidden');
        }
    },

    /**
     * Handle keyboard shortcuts
     * @param {KeyboardEvent} e - Keyboard event
     */
    handleKeyboardShortcuts(e) {
        // Ctrl/Cmd + Enter: Send request
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
            e.preventDefault();
            this.handleSendRequest();
        }

        // Ctrl/Cmd + Shift + F: Format JSON
        if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'F') {
            e.preventDefault();
            if (document.querySelector('.tab-content.active').id === 'json-tab') {
                PostHandlers.formatJSON();
            }
        }

        // Ctrl/Cmd + Shift + V: Validate JSON
        if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'V') {
            e.preventDefault();
            if (document.querySelector('.tab-content.active').id === 'json-tab') {
                PostHandlers.validateJSON();
            }
        }

        // Ctrl/Cmd + Shift + C: Copy response
        if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'C') {
            e.preventDefault();
            PostHandlers.copyResponse();
        }
    }
};

// Initialize when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    PostApp.init();
});

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = PostApp;
}
