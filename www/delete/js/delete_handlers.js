/* =================================================================
   🗑️ DELETE TESTING - HANDLERS
   Event handlers and API interaction for DELETE request testing
   ================================================================= */

// API Request Handlers
const DeleteHandlers = {

    /**
     * Send DELETE request to specified URL
     * @param {string} url - URL to send DELETE request to
     * @param {Object} headers - Custom headers
     * @returns {Promise<Object>} - Response object
     */
    async sendDeleteRequest(url, headers = {}) {
        const startTime = Date.now();

        try {
            // Prepare request options
            const options = {
                method: 'DELETE',
                headers: {
                    ...DELETE_CONFIG.DEFAULT_HEADERS,
                    ...headers
                }
            };

            console.log('🗑️ Sending DELETE request:', {
                url: url,
                headers: options.headers
            });

            // Send the request
            const response = await fetch(url, options);
            const endTime = Date.now();
            const duration = endTime - startTime;

            // Get response text
            const responseText = await response.text();

            // Parse response headers
            const responseHeaders = {};
            response.headers.forEach((value, key) => {
                responseHeaders[key] = value;
            });

            console.log('📨 DELETE response received:', {
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
                url: url,
                method: 'DELETE'
            };

        } catch (error) {
            const endTime = Date.now();
            const duration = endTime - startTime;

            console.error('❌ DELETE request failed:', error);

            let errorMessage = DELETE_CONFIG.ERROR_MESSAGES.UNKNOWN_ERROR;

            if (error.name === 'TypeError' && error.message.includes('fetch')) {
                errorMessage = DELETE_CONFIG.ERROR_MESSAGES.NETWORK_ERROR;
            } else if (error.name === 'AbortError') {
                errorMessage = DELETE_CONFIG.ERROR_MESSAGES.TIMEOUT_ERROR;
            }

            return {
                ok: false,
                status: 0,
                statusText: 'Network Error',
                headers: {},
                body: errorMessage,
                duration: duration,
                size: 0,
                url: url,
                method: 'DELETE',
                error: true
            };
        }
    },

    /**
     * Discover resources from endpoint
     * @param {string} endpoint - Endpoint to discover resources from
     * @returns {Promise<Array>} - Array of discovered resources
     */
    async discoverResources(endpoint) {
        try {
            console.log('🔍 Discovering resources from:', endpoint);

            const response = await fetch(endpoint);

            if (!response.ok) {
                throw new Error(`Failed to fetch resources: ${response.status} ${response.statusText}`);
            }

            const contentType = response.headers.get('content-type');
            let data;

            if (contentType && contentType.includes('application/json')) {
                data = await response.json();
            } else {
                // Try to parse as text and look for file listings
                const text = await response.text();
                data = this.parseTextListing(text, endpoint);
            }

            // Normalize the response format
            const resources = this.normalizeResourceList(data, endpoint);

            console.log('📋 Discovered resources:', resources);
            return resources;

        } catch (error) {
            console.error('❌ Resource discovery failed:', error);
            throw new Error(DELETE_CONFIG.ERROR_MESSAGES.DISCOVERY_FAILED);
        }
    },

    /**
     * Parse text listing (for autoindex pages)
     * @param {string} text - HTML or text content
     * @param {string} endpoint - Base endpoint
     * @returns {Object} - Parsed resource data
     */
    parseTextListing(text, endpoint) {
        const resources = [];

        // Try to parse HTML directory listing
        if (text.includes('<a href=')) {
            const regex = /<a href="([^"]+)"[^>]*>([^<]+)<\/a>/gi;
            let match;

            while ((match = regex.exec(text)) !== null) {
                const href = match[1];
                const name = match[2].trim();

                // Skip parent directory and current directory
                if (href === '../' || href === './' || name === '..') continue;

                resources.push({
                    id: href,
                    name: name,
                    url: endpoint + (endpoint.endsWith('/') ? '' : '/') + href,
                    type: href.endsWith('/') ? 'directory' : 'file'
                });
            }
        }

        return { resources };
    },

    /**
     * Normalize resource list to standard format
     * @param {Object|Array} data - Raw resource data
     * @param {string} endpoint - Base endpoint
     * @returns {Array} - Normalized resource array
     */
    normalizeResourceList(data, endpoint) {
        let resources = [];

        // Handle different response formats
        if (Array.isArray(data)) {
            resources = data;
        } else if (data.resources && Array.isArray(data.resources)) {
            resources = data.resources;
        } else if (data.files && Array.isArray(data.files)) {
            resources = data.files;
        } else if (data.items && Array.isArray(data.items)) {
            resources = data.items;
        }

        // Normalize each resource
        return resources.map(resource => {
            if (typeof resource === 'string') {
                // Simple string resource
                return {
                    id: resource,
                    name: resource,
                    url: endpoint + (endpoint.endsWith('/') ? '' : '/') + resource,
                    type: DeleteUtils.getResourceType(endpoint)
                };
            } else {
                // Object resource - ensure required fields
                return {
                    id: resource.id || resource.name || resource.filename,
                    name: resource.name || resource.filename || resource.id,
                    url: resource.url || (endpoint + (endpoint.endsWith('/') ? '' : '/') + (resource.id || resource.name)),
                    type: resource.type || DeleteUtils.getResourceType(endpoint),
                    size: resource.size || null,
                    modified: resource.modified || resource.lastModified || null
                };
            }
        });
    },

    /**
     * Handle quick test execution
     * @param {string} testKey - Key of the quick test to run
     */
    async runQuickTest(testKey) {
        console.log('[DELETE] Running quick test:', testKey);

        const testData = DELETE_CONFIG.QUICK_TESTS[testKey];
        if (!testData) {
            console.error('[DELETE] Quick test not found:', testKey);
            console.log('[DELETE] Available tests:', Object.keys(DELETE_CONFIG.QUICK_TESTS));
            DeleteUtils.showNotification('Quick test not found', 'error');
            return;
        }

        console.log('[DELETE] Test data:', testData);
        console.log('⚡ Running quick test:', testData.name);

        // Special handling for bulk delete test
        if (testKey === 'delete-bulk') {
            await this.executeBulkDeleteTest(testData);
        } else {
            // Update UI with test data
            const resourceUrlInput = document.getElementById('resource-url');
            if (resourceUrlInput) {
                resourceUrlInput.value = testData.url;
                console.log('[DELETE] Updated resource URL input:', testData.url);
            } else {
                console.error('[DELETE] Resource URL input not found');
            }

            // Show notification about the test
            DeleteUtils.showNotification(`Quick test "${testData.name}" loaded`, 'info');

            // Execute the test
            await this.executeDeleteWithConfirmation(testData.url, {}, testData.description);
        }
    },

    /**
     * Execute bulk delete test
     * @param {Object} testData - Test configuration
     */
    async executeBulkDeleteTest(testData) {
        try {
            const response = await fetch(testData.url, {
                method: testData.method,
                headers: {
                    ...DELETE_CONFIG.DEFAULT_HEADERS,
                    ...testData.headers
                },
                body: testData.body
            });

            const responseData = await response.text();

            // Display the result
            this.displayResponse({
                ok: response.ok,
                status: response.status,
                statusText: response.statusText,
                headers: Object.fromEntries(response.headers.entries()),
                body: responseData,
                duration: 0,
                size: responseData.length,
                url: testData.url,
                method: testData.method
            });

            const message = response.ok ?
                DELETE_CONFIG.SUCCESS_MESSAGES.BULK_DELETION_SUCCESS :
                DELETE_CONFIG.ERROR_MESSAGES.BULK_DELETION_FAILED;

            DeleteUtils.showNotification(message, response.ok ? 'success' : 'error');

        } catch (error) {
            console.error('Bulk delete test failed:', error);
            DeleteUtils.showNotification('Bulk delete test failed', 'error');
        }
    },

    /**
     * Execute delete with confirmation if required
     * @param {string} url - URL to delete
     * @param {Object} headers - Custom headers
     * @param {string} description - Description for confirmation
     */
    async executeDeleteWithConfirmation(url, headers = {}, description = '') {
        const requireConfirmation = document.getElementById('require-confirmation').checked;
        const safeMode = document.getElementById('safe-mode').checked;

        // Safety checks
        if (safeMode && DeleteUtils.isDangerousUrl(url)) {
            DeleteUtils.showNotification('Operation blocked by safe mode: Dangerous URL detected', 'warning');
            return;
        }

        const resourceId = DeleteUtils.extractResourceId(url);
        if (safeMode && DeleteUtils.isProtectedFile(resourceId)) {
            DeleteUtils.showNotification('Operation blocked by safe mode: Protected file type', 'warning');
            return;
        }

        if (requireConfirmation) {
            const message = description ?
                `${description}\n\nAre you sure you want to delete: ${url}?` :
                `Are you sure you want to delete: ${url}?`;

            DeleteUtils.showConfirmation(message, async () => {
                await this.executeSingleDelete(url, headers);
            });
        } else {
            await this.executeSingleDelete(url, headers);
        }
    },

    /**
     * Execute single delete operation
     * @param {string} url - URL to delete
     * @param {Object} headers - Custom headers
     */
    async executeSingleDelete(url, headers = {}) {
        try {
            // Show loading state
            this.setLoadingState(true);

            // Send delete request
            const response = await this.sendDeleteRequest(url, headers);

            // Display response
            this.displayResponse(response);

            // Save to history
            if (DELETE_CONFIG.HISTORY.AUTO_SAVE) {
                DeleteHistory.saveToHistory({ url, headers, method: 'DELETE' }, response);
            }

            // Show appropriate notification
            const statusCategory = DeleteUtils.getStatusCategory(response.status);
            if (statusCategory === 'success') {
                DeleteUtils.showNotification(DeleteUtils.getSuccessMessage(response.status), 'success');
            } else {
                DeleteUtils.showNotification(DeleteUtils.getErrorMessage(response.status), 'error');
            }

        } catch (error) {
            console.error('❌ Delete operation failed:', error);
            DeleteUtils.showNotification(DELETE_CONFIG.ERROR_MESSAGES.DELETION_FAILED, 'error');
        } finally {
            this.setLoadingState(false);
        }
    },

    /**
     * Execute bulk delete operation
     * @param {Array} selectedResources - Array of selected resource URLs
     */
    async executeBulkDelete(selectedResources) {
        if (selectedResources.length === 0) {
            DeleteUtils.showNotification(DELETE_CONFIG.ERROR_MESSAGES.NO_RESOURCES_SELECTED, 'warning');
            return;
        }

        const confirmMessage = `Are you sure you want to delete ${selectedResources.length} resources?\n\nThis action cannot be undone.`;

        DeleteUtils.showConfirmation(confirmMessage, async () => {
            await this.performBulkDelete(selectedResources);
        });
    },

    /**
     * Perform bulk delete operation
     * @param {Array} resourceUrls - Array of resource URLs to delete
     */
    async performBulkDelete(resourceUrls) {
        try {
            this.setLoadingState(true);

            let successCount = 0;
            let errorCount = 0;
            const results = [];

            // Process deletions in batches
            const processor = async (url) => {
                const result = await this.sendDeleteRequest(url, this.getCustomHeaders());
                results.push({ url, result });

                if (result.ok) {
                    successCount++;
                } else {
                    errorCount++;
                }

                return result;
            };

            await DeleteUtils.batchProcess(resourceUrls, processor, DELETE_CONFIG.BULK_OPERATIONS.BATCH_SIZE);

            // Display summary
            const summaryResponse = {
                ok: errorCount === 0,
                status: errorCount === 0 ? 200 : 207, // Multi-Status
                statusText: `Bulk Delete: ${successCount} succeeded, ${errorCount} failed`,
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    summary: {
                        total: resourceUrls.length,
                        succeeded: successCount,
                        failed: errorCount
                    },
                    results: results
                }, null, 2),
                duration: 0,
                size: 0,
                url: 'bulk-delete',
                method: 'BULK DELETE'
            };

            this.displayResponse(summaryResponse);

            // Save to history
            if (DELETE_CONFIG.HISTORY.AUTO_SAVE) {
                DeleteHistory.saveToHistory({
                    urls: resourceUrls,
                    method: 'BULK DELETE'
                }, summaryResponse);
            }

            // Show notification
            if (errorCount === 0) {
                DeleteUtils.showNotification(DELETE_CONFIG.SUCCESS_MESSAGES.BULK_DELETION_SUCCESS, 'success');
            } else if (successCount > 0) {
                DeleteUtils.showNotification(DELETE_CONFIG.SUCCESS_MESSAGES.BULK_DELETION_PARTIAL, 'warning');
            } else {
                DeleteUtils.showNotification(DELETE_CONFIG.ERROR_MESSAGES.BULK_DELETION_FAILED, 'error');
            }

            // Refresh resource list
            this.refreshResourceList();

        } catch (error) {
            console.error('❌ Bulk delete failed:', error);
            DeleteUtils.showNotification(DELETE_CONFIG.ERROR_MESSAGES.BULK_DELETION_FAILED, 'error');
        } finally {
            this.setLoadingState(false);
        }
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
            DeleteUtils.isValidJSON(response.body)) {
            try {
                displayBody = DeleteUtils.formatJSON(response.body);
            } catch (error) {
                // Keep original if formatting fails
            }
        }

        responseBodyContent.textContent = DeleteUtils.truncateText(displayBody);

        // Update response headers
        const responseHeadersContent = document.getElementById('response-headers-content');
        responseHeadersContent.textContent = DeleteUtils.formatHeaders(response.headers);

        // Update response info
        document.getElementById('response-status').textContent =
            `${response.status} ${response.statusText}`;
        document.getElementById('response-time').textContent =
            DeleteUtils.formatDuration(response.duration);
        document.getElementById('response-size').textContent =
            DeleteUtils.formatFileSize(response.size);
        document.getElementById('response-method').textContent = response.method;

        // Update status styling
        const statusElement = document.getElementById('response-status');
        const statusCategory = DeleteUtils.getStatusCategory(response.status);
        statusElement.className = `info-value status-${statusCategory}`;
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
     * Set loading state
     * @param {boolean} loading - Whether to show loading state
     */
    setLoadingState(loading) {
        const deleteButton = document.getElementById('delete-single');
        const deleteSelectedButton = document.getElementById('delete-selected');

        if (loading) {
            deleteButton.disabled = true;
            deleteButton.textContent = '⏳ Deleting...';
            deleteSelectedButton.disabled = true;
            deleteSelectedButton.textContent = '⏳ Deleting...';
        } else {
            deleteButton.disabled = false;
            deleteButton.textContent = '🗑️ Delete Resource';
            deleteSelectedButton.disabled = false;
            deleteSelectedButton.textContent = '🗑️ Delete Selected';
        }
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
     * Refresh resource list
     */
    async refreshResourceList() {
        const resourceType = document.getElementById('resource-type').value;
        if (resourceType && resourceType !== 'custom') {
            await this.discoverAndDisplayResources();
        }
    },

    /**
     * Discover and display resources
     */
    async discoverAndDisplayResources() {
        try {
            const resourceType = document.getElementById('resource-type').value;
            const customEndpoint = document.getElementById('custom-endpoint').value;

            let endpoint;
            if (resourceType === 'custom') {
                endpoint = customEndpoint.trim();
                if (!endpoint) {
                    DeleteUtils.showNotification('Please enter a custom endpoint', 'warning');
                    return;
                }
            } else {
                endpoint = DELETE_CONFIG.DISCOVERY_ENDPOINTS[resourceType];
            }

            const resources = await this.discoverResources(endpoint);
            this.displayResourceList(resources);

        } catch (error) {
            console.error('Resource discovery failed:', error);
            DeleteUtils.showNotification(error.message, 'error');
        }
    },

    /**
     * Display resource list in UI
     * @param {Array} resources - Array of resources
     */
    displayResourceList(resources) {
        const resourceList = document.getElementById('resource-list');

        if (resources.length === 0) {
            resourceList.innerHTML = '<p class="empty-resources">No resources found.</p>';
            return;
        }

        resourceList.innerHTML = resources.map(resource => `
            <div class="resource-item" data-url="${resource.url}">
                <input type="checkbox" class="resource-checkbox" value="${resource.url}">
                <div class="resource-info">
                    <div class="resource-name">${DeleteUtils.escapeHtml(resource.name)}</div>
                    <div class="resource-url">${DeleteUtils.escapeHtml(resource.url)}</div>
                </div>
                <span class="resource-type">${resource.type}</span>
            </div>
        `).join('');

        // Add click handlers for resource items
        resourceList.querySelectorAll('.resource-item').forEach(item => {
            item.addEventListener('click', (e) => {
                if (e.target.type !== 'checkbox') {
                    const checkbox = item.querySelector('.resource-checkbox');
                    checkbox.checked = !checkbox.checked;
                }
                this.updateBulkActionButtons();
            });
        });

        // Add change handlers for checkboxes
        resourceList.querySelectorAll('.resource-checkbox').forEach(checkbox => {
            checkbox.addEventListener('change', () => {
                this.updateBulkActionButtons();
            });
        });

        this.updateBulkActionButtons();
    },

    /**
     * Update bulk action buttons state
     */
    updateBulkActionButtons() {
        const checkboxes = document.querySelectorAll('.resource-checkbox');
        const checkedBoxes = document.querySelectorAll('.resource-checkbox:checked');
        const deleteSelectedButton = document.getElementById('delete-selected');

        deleteSelectedButton.disabled = checkedBoxes.length === 0;

        // Update button text with count
        if (checkedBoxes.length > 0) {
            deleteSelectedButton.textContent = `🗑️ Delete Selected (${checkedBoxes.length})`;
        } else {
            deleteSelectedButton.textContent = '🗑️ Delete Selected';
        }
    },

    /**
     * Select all resources
     */
    selectAllResources() {
        const checkboxes = document.querySelectorAll('.resource-checkbox');
        checkboxes.forEach(checkbox => {
            checkbox.checked = true;
        });
        this.updateBulkActionButtons();
    },

    /**
     * Deselect all resources
     */
    selectNoneResources() {
        const checkboxes = document.querySelectorAll('.resource-checkbox');
        checkboxes.forEach(checkbox => {
            checkbox.checked = false;
        });
        this.updateBulkActionButtons();
    },

    /**
     * Get selected resource URLs
     * @returns {Array} - Array of selected resource URLs
     */
    getSelectedResources() {
        const checkedBoxes = document.querySelectorAll('.resource-checkbox:checked');
        return Array.from(checkedBoxes).map(checkbox => checkbox.value);
    },

    /**
     * Copy response content to clipboard
     */
    async copyResponse() {
        const responseContent = document.getElementById('response-body-content').textContent;
        const success = await DeleteUtils.copyToClipboard(responseContent);

        if (success) {
            DeleteUtils.showNotification('Response copied to clipboard', 'success');
        } else {
            DeleteUtils.showNotification('Failed to copy response', 'error');
        }
    },

    /**
     * Download response as file
     */
    downloadResponse() {
        const responseContent = document.getElementById('response-body-content').textContent;
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
        const filename = `delete-response-${timestamp}.txt`;

        DeleteUtils.downloadAsFile(responseContent, filename);
        DeleteUtils.showNotification('Response downloaded', 'success');
    },

    /**
     * Format response content
     */
    formatResponse() {
        const responseContent = document.getElementById('response-body-content');
        const content = responseContent.textContent;

        if (DeleteUtils.isValidJSON(content)) {
            try {
                const formatted = DeleteUtils.formatJSON(content);
                responseContent.textContent = formatted;
                DeleteUtils.showNotification('Response formatted', 'success');
            } catch (error) {
                DeleteUtils.showNotification('Failed to format response', 'error');
            }
        } else {
            DeleteUtils.showNotification('Response is not valid JSON', 'warning');
        }
    },

    /**
     * Handle resource type change
     */
    handleResourceTypeChange() {
        const resourceTypeSelect = document.getElementById('resource-type');
        const customEndpointInput = document.getElementById('custom-endpoint');

        if (resourceTypeSelect.value === 'custom') {
            customEndpointInput.classList.remove('hidden');
            customEndpointInput.focus();
        } else {
            customEndpointInput.classList.add('hidden');
        }
    },

    /**
     * Initialize response display elements
     */
    initializeResponseDisplay() {
        console.log('[DELETE] Initializing response display...');

        // Initialize response format selector
        const formatBtns = document.querySelectorAll('.format-btn');
        if (formatBtns.length > 0) {
            formatBtns[0].classList.add('active'); // Default to first format
        }

        // Clear any existing response content
        const responseContent = document.getElementById('response-content');
        if (responseContent) {
            responseContent.textContent = 'No response yet...';
        }

        const responseHeaders = document.getElementById('response-headers');
        if (responseHeaders) {
            responseHeaders.textContent = 'No headers yet...';
        }

        const responseStatus = document.getElementById('response-status');
        if (responseStatus) {
            responseStatus.textContent = '';
            responseStatus.className = 'response-status';
        }

        console.log('[DELETE] Response display initialized');
    },

    /**
     * Add a new custom header row
     * @param {string} name - Header name (optional)
     * @param {string} value - Header value (optional)
     */
    addCustomHeader(name = '', value = '') {
        console.log('[DELETE] Adding custom header row');

        const headersContainer = document.querySelector('.headers-container');
        if (!headersContainer) {
            console.warn('[DELETE] Headers container not found');
            return;
        }

        const headerRow = document.createElement('div');
        headerRow.className = 'header-row';
        headerRow.innerHTML = `
            <input type="text" class="header-key" placeholder="Header Name" value="${name}">
            <input type="text" class="header-value" placeholder="Header Value" value="${value}">
            <button class="remove-header">❌</button>
        `;

        headersContainer.appendChild(headerRow);

        // Focus on the header name input if it's empty
        if (!name) {
            const headerKeyInput = headerRow.querySelector('.header-key');
            if (headerKeyInput) {
                headerKeyInput.focus();
            }
        }

        console.log('[DELETE] Custom header row added');
    },

    /**
     * Remove a custom header row
     * @param {HTMLElement} button - The remove button element
     */
    removeCustomHeader(button) {
        console.log('[DELETE] Removing custom header row');

        const headerRow = button.closest('.header-row');
        if (headerRow) {
            // Check if this is the last header row
            const headersContainer = document.querySelector('.headers-container');
            const headerRows = headersContainer.querySelectorAll('.header-row');

            if (headerRows.length > 1) {
                headerRow.remove();
                console.log('[DELETE] Custom header row removed');
            } else {
                // Clear the inputs instead of removing the last row
                const keyInput = headerRow.querySelector('.header-key');
                const valueInput = headerRow.querySelector('.header-value');
                if (keyInput) keyInput.value = '';
                if (valueInput) valueInput.value = '';
                console.log('[DELETE] Last header row cleared (not removed)');
            }
        } else {
            console.warn('[DELETE] Header row not found');
        }
    },

    /**
     * Get all custom headers as an object
     * @returns {Object} - Headers object
     */
    getCustomHeaders() {
        const headers = {};
        const headerRows = document.querySelectorAll('.header-row');

        headerRows.forEach(row => {
            const keyInput = row.querySelector('.header-key');
            const valueInput = row.querySelector('.header-value');

            if (keyInput && valueInput && keyInput.value.trim() && valueInput.value.trim()) {
                headers[keyInput.value.trim()] = valueInput.value.trim();
            }
        });

        return headers;
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = DeleteHandlers;
}
