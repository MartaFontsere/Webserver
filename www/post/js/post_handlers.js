/* =================================================================
   📝 POST TESTING - HANDLERS
   Event handlers and API interaction for POST request testing
   ================================================================= */

// API Request Handlers
const PostHandlers = {

    /**
     * Send POST request with current configuration
     * @param {Object} requestData - Request configuration
     * @returns {Promise<Object>} - Response object
     */
    async sendRequest(requestData) {
        const startTime = Date.now();

        try {
            // Prepare request options
            const options = {
                method: requestData.method || 'POST',
                headers: {
                    ...POST_CONFIG.DEFAULT_HEADERS,
                    ...requestData.headers
                }
            };

            // Add body for methods that support it
            if (['POST', 'PUT', 'PATCH'].includes(options.method) && requestData.body) {
                options.body = requestData.body;
            }

            // Add Content-Type if not already set and body exists
            if (requestData.body && !options.headers['Content-Type']) {
                options.headers['Content-Type'] = requestData.contentType || POST_CONFIG.CONTENT_TYPES.JSON;
            }

            console.log('📤 Sending request:', {
                url: requestData.url,
                method: options.method,
                headers: options.headers,
                bodyLength: requestData.body ? requestData.body.length : 0
            });

            // Send the request
            const response = await fetch(requestData.url, options);
            const endTime = Date.now();
            const duration = endTime - startTime;

            // Get response text
            const responseText = await response.text();

            // Parse response headers
            const responseHeaders = {};
            response.headers.forEach((value, key) => {
                responseHeaders[key] = value;
            });

            console.log('📨 Response received:', {
                status: response.status,
                statusText: response.statusText,
                duration: duration + 'ms',
                size: responseText.length
            });

            return {
                ok: response.ok,
                status: response.status,
                statusText: response.statusText,
                headers: responseHeaders,
                body: responseText,
                duration: duration,
                size: responseText.length,
                url: requestData.url,
                method: options.method
            };

        } catch (error) {
            const endTime = Date.now();
            const duration = endTime - startTime;

            console.error('❌ Request failed:', error);

            let errorMessage = POST_CONFIG.ERROR_MESSAGES.UNKNOWN_ERROR;

            if (error.name === 'TypeError' && error.message.includes('fetch')) {
                errorMessage = POST_CONFIG.ERROR_MESSAGES.NETWORK_ERROR;
            } else if (error.name === 'AbortError') {
                errorMessage = POST_CONFIG.ERROR_MESSAGES.TIMEOUT_ERROR;
            }

            return {
                ok: false,
                status: 0,
                statusText: 'Network Error',
                headers: {},
                body: errorMessage,
                duration: duration,
                size: 0,
                url: requestData.url,
                method: requestData.method || 'POST',
                error: true
            };
        }
    },

    /**
     * Handle quick test button clicks
     * @param {string} testKey - Key of the quick test to run
     */
    async runQuickTest(testKey) {
        const testData = POST_CONFIG.QUICK_TESTS[testKey];
        if (!testData) {
            PostUtils.showNotification('Quick test not found', 'error');
            return;
        }

        console.log('⚡ Running quick test:', testData.name);

        // Update UI with test data
        document.getElementById('method-select').value = testData.method;
        document.getElementById('request-url').value = testData.url;
        document.getElementById('content-type').value = testData.contentType;

        // Set body content based on content type
        if (testData.contentType === POST_CONFIG.CONTENT_TYPES.JSON) {
            this.switchToTab('json');
            document.getElementById('json-body').value = testData.body;
        } else {
            this.switchToTab('raw');
            document.getElementById('raw-body').value = testData.body;
        }

        // Show notification
        PostUtils.showNotification(`Quick test "${testData.name}" loaded`, 'success');
    },

    /**
     * Switch between editor tabs
     * @param {string} tabName - Name of the tab to switch to
     */
    switchToTab(tabName) {
        // Update tab buttons
        document.querySelectorAll('.tab-button').forEach(btn => {
            btn.classList.remove('active');
        });
        document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');

        // Update tab content
        document.querySelectorAll('.tab-content').forEach(content => {
            content.classList.remove('active');
        });
        document.getElementById(`${tabName}-tab`).classList.add('active');
    },

    /**
     * Switch between response tabs
     * @param {string} tabName - Name of the response tab to switch to
     */
    switchToResponseTab(tabName) {
        // Update tab buttons
        document.querySelectorAll('.response-tab-button').forEach(btn => {
            btn.classList.remove('active');
        });
        document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');

        // Update tab content
        document.querySelectorAll('.response-tab-content').forEach(content => {
            content.classList.remove('active');
        });
        document.getElementById(tabName).classList.add('active');
    },

    /**
     * Get current request configuration from UI
     * @returns {Object} - Current request configuration
     */
    getCurrentRequestConfig() {
        const method = document.getElementById('method-select').value;
        const url = document.getElementById('request-url').value.trim();
        const contentTypeSelect = document.getElementById('content-type');
        let contentType = contentTypeSelect.value;

        // Handle custom content type
        if (contentType === 'custom') {
            contentType = document.getElementById('custom-content-type').value.trim();
        }

        // Get body based on active tab
        let body = '';
        const activeTab = document.querySelector('.tab-content.active').id;

        if (activeTab === 'raw-tab') {
            body = document.getElementById('raw-body').value;
        } else if (activeTab === 'form-tab') {
            body = this.getFormDataAsString();
        } else if (activeTab === 'json-tab') {
            body = document.getElementById('json-body').value;
        }

        // Get custom headers
        const headers = this.getCustomHeaders();

        return {
            method,
            url,
            contentType,
            body,
            headers
        };
    },

    /**
     * Get form data as URL-encoded string
     * @returns {string} - URL-encoded form data
     */
    getFormDataAsString() {
        const formData = {};
        const formRows = document.querySelectorAll('#form-tab .form-row');

        formRows.forEach(row => {
            const key = row.querySelector('.form-key').value.trim();
            const value = row.querySelector('.form-value').value.trim();
            if (key) {
                formData[key] = value;
            }
        });

        return PostUtils.objectToUrlEncoded(formData);
    },

    /**
     * Get custom headers from UI
     * @returns {Object} - Custom headers object
     */
    getCustomHeaders() {
        const headers = {};
        const headerRows = document.querySelectorAll('.headers-container .header-row');

        headerRows.forEach(row => {
            const key = row.querySelector('.header-key').value.trim();
            const value = row.querySelector('.header-value').value.trim();
            if (key) {
                headers[key] = value;
            }
        });

        return headers;
    },

    /**
     * Add new form data row
     */
    addFormRow() {
        const container = document.querySelector('.form-data-container');
        const newRow = this.createFormRow();
        container.appendChild(newRow);
    },

    /**
     * Create new form data row element
     * @returns {HTMLElement} - Form row element
     */
    createFormRow() {
        const row = document.createElement('div');
        row.className = 'form-row';
        row.innerHTML = `
            <input type="text" class="form-key" placeholder="Key">
            <input type="text" class="form-value" placeholder="Value">
            <button class="remove-row" type="button">❌</button>
        `;

        // Add remove functionality
        row.querySelector('.remove-row').addEventListener('click', () => {
            row.remove();
        });

        return row;
    },

    /**
     * Add new header row
     */
    addHeaderRow() {
        const container = document.querySelector('.headers-container');
        const newRow = this.createHeaderRow();
        container.appendChild(newRow);
    },

    /**
     * Create new header row element
     * @returns {HTMLElement} - Header row element
     */
    createHeaderRow() {
        const row = document.createElement('div');
        row.className = 'header-row';
        row.innerHTML = `
            <input type="text" class="header-key" placeholder="Header Name">
            <input type="text" class="header-value" placeholder="Header Value">
            <button class="remove-header" type="button">❌</button>
        `;

        // Add remove functionality
        row.querySelector('.remove-header').addEventListener('click', () => {
            row.remove();
        });

        return row;
    },

    /**
     * Format JSON in the JSON editor
     */
    formatJSON() {
        const jsonTextarea = document.getElementById('json-body');
        const jsonContent = jsonTextarea.value.trim();

        if (!jsonContent) {
            PostUtils.showNotification('No JSON content to format', 'warning');
            return;
        }

        try {
            const formatted = PostUtils.formatJSON(jsonContent);
            jsonTextarea.value = formatted;
            PostUtils.showNotification('JSON formatted successfully', 'success');
        } catch (error) {
            PostUtils.showNotification('Invalid JSON format', 'error');
        }
    },

    /**
     * Validate JSON in the JSON editor
     */
    validateJSON() {
        const jsonTextarea = document.getElementById('json-body');
        const jsonContent = jsonTextarea.value.trim();

        if (!jsonContent) {
            PostUtils.showNotification('No JSON content to validate', 'warning');
            return;
        }

        if (PostUtils.isValidJSON(jsonContent)) {
            PostUtils.showNotification('JSON is valid ✅', 'success');
        } else {
            PostUtils.showNotification('JSON is invalid ❌', 'error');
        }
    },

    /**
     * Display response in the UI
     * @param {Object} response - Response object
     */
    displayResponse(response) {
        // Update response body
        const responseBodyContent = document.getElementById('response-body-content');
        let displayBody = response.body;

        // Try to format JSON response
        if (response.headers['content-type'] &&
            response.headers['content-type'].includes('application/json') &&
            PostUtils.isValidJSON(response.body)) {
            try {
                displayBody = PostUtils.formatJSON(response.body);
            } catch (error) {
                // Keep original if formatting fails
            }
        }

        responseBodyContent.textContent = PostUtils.truncateText(displayBody);

        // Update response headers
        const responseHeadersContent = document.getElementById('response-headers-content');
        responseHeadersContent.textContent = PostUtils.formatHeaders(response.headers);

        // Update response info
        document.getElementById('response-status').textContent =
            `${response.status} ${response.statusText}`;
        document.getElementById('response-time').textContent =
            PostUtils.formatDuration(response.duration);
        document.getElementById('response-size').textContent =
            PostUtils.formatFileSize(response.size);
        document.getElementById('response-type').textContent =
            response.headers['content-type'] || 'Unknown';

        // Update status styling
        const statusElement = document.getElementById('response-status');
        const statusCategory = PostUtils.getStatusCategory(response.status);
        statusElement.className = `info-value status-${statusCategory}`;
    },

    /**
     * Handle content type change
     */
    handleContentTypeChange() {
        const contentTypeSelect = document.getElementById('content-type');
        const customInput = document.getElementById('custom-content-type');

        if (contentTypeSelect.value === 'custom') {
            customInput.classList.remove('hidden');
            customInput.focus();
        } else {
            customInput.classList.add('hidden');
        }
    },

    /**
     * Copy response content to clipboard
     */
    async copyResponse() {
        const responseContent = document.getElementById('response-body-content').textContent;
        const success = await PostUtils.copyToClipboard(responseContent);

        if (success) {
            PostUtils.showNotification('Response copied to clipboard', 'success');
        } else {
            PostUtils.showNotification('Failed to copy response', 'error');
        }
    },

    /**
     * Download response as file
     */
    downloadResponse() {
        const responseContent = document.getElementById('response-body-content').textContent;
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
        const filename = `post-response-${timestamp}.txt`;

        PostUtils.downloadAsFile(responseContent, filename);
        PostUtils.showNotification('Response downloaded', 'success');
    },

    /**
     * Format response content
     */
    formatResponse() {
        const responseContent = document.getElementById('response-body-content');
        const content = responseContent.textContent;

        if (PostUtils.isValidJSON(content)) {
            try {
                const formatted = PostUtils.formatJSON(content);
                responseContent.textContent = formatted;
                PostUtils.showNotification('Response formatted', 'success');
            } catch (error) {
                PostUtils.showNotification('Failed to format response', 'error');
            }
        } else {
            PostUtils.showNotification('Response is not valid JSON', 'warning');
        }
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = PostHandlers;
}
