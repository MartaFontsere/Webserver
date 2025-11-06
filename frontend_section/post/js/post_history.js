/* =================================================================
   📝 POST TESTING - HISTORY
   Request history management for POST request testing
   ================================================================= */

// History Management
const PostHistory = {

    /**
     * Save request to history
     * @param {Object} request - Request configuration
     * @param {Object} response - Response object
     */
    saveToHistory(request, response) {
        try {
            const historyEntry = {
                id: PostUtils.generateId(),
                timestamp: PostUtils.getTimestamp(),
                request: {
                    method: request.method,
                    url: request.url,
                    contentType: request.contentType,
                    body: request.body,
                    headers: request.headers
                },
                response: {
                    status: response.status,
                    statusText: response.statusText,
                    headers: response.headers,
                    body: response.body,
                    duration: response.duration,
                    size: response.size,
                    error: response.error || false
                }
            };

            let history = this.getHistory();
            history.unshift(historyEntry);

            // Limit history size
            if (history.length > POST_CONFIG.HISTORY.MAX_ENTRIES) {
                history = history.slice(0, POST_CONFIG.HISTORY.MAX_ENTRIES);
            }

            // Save to localStorage
            localStorage.setItem(POST_CONFIG.HISTORY.STORAGE_KEY, JSON.stringify(history));

            // Update UI
            this.updateHistoryDisplay();

            console.log('💾 Request saved to history:', historyEntry.id);

        } catch (error) {
            console.error('Failed to save to history:', error);
        }
    },

    /**
     * Get history from localStorage
     * @returns {Array} - Array of history entries
     */
    getHistory() {
        try {
            const stored = localStorage.getItem(POST_CONFIG.HISTORY.STORAGE_KEY);
            return stored ? JSON.parse(stored) : [];
        } catch (error) {
            console.error('Failed to load history:', error);
            return [];
        }
    },

    /**
     * Clear all history
     */
    clearHistory() {
        try {
            localStorage.removeItem(POST_CONFIG.HISTORY.STORAGE_KEY);
            this.updateHistoryDisplay();
            PostUtils.showNotification('History cleared', 'success');
            console.log('🗑️ History cleared');
        } catch (error) {
            console.error('Failed to clear history:', error);
            PostUtils.showNotification('Failed to clear history', 'error');
        }
    },

    /**
     * Export history as JSON file
     */
    exportHistory() {
        try {
            const history = this.getHistory();
            if (history.length === 0) {
                PostUtils.showNotification('No history to export', 'warning');
                return;
            }

            const exportData = {
                exportDate: new Date().toISOString(),
                totalEntries: history.length,
                entries: history
            };

            const jsonContent = JSON.stringify(exportData, null, 2);
            const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
            const filename = `post-request-history-${timestamp}.json`;

            PostUtils.downloadAsFile(jsonContent, filename, 'application/json');
            PostUtils.showNotification('History exported successfully', 'success');

        } catch (error) {
            console.error('Failed to export history:', error);
            PostUtils.showNotification('Failed to export history', 'error');
        }
    },

    /**
     * Load request from history
     * @param {string} entryId - ID of the history entry
     */
    loadFromHistory(entryId) {
        try {
            const history = this.getHistory();
            const entry = history.find(item => item.id === entryId);

            if (!entry) {
                PostUtils.showNotification('History entry not found', 'error');
                return;
            }

            const request = entry.request;

            // Update UI with request data
            document.getElementById('method-select').value = request.method;
            document.getElementById('request-url').value = request.url;
            document.getElementById('content-type').value = request.contentType;

            // Handle custom content type
            if (!Object.values(POST_CONFIG.CONTENT_TYPES).includes(request.contentType)) {
                document.getElementById('content-type').value = 'custom';
                document.getElementById('custom-content-type').value = request.contentType;
                document.getElementById('custom-content-type').classList.remove('hidden');
            }

            // Set body content
            if (request.contentType === POST_CONFIG.CONTENT_TYPES.JSON) {
                PostHandlers.switchToTab('json');
                document.getElementById('json-body').value = request.body || '';
            } else if (request.contentType === POST_CONFIG.CONTENT_TYPES.FORM_URLENCODED) {
                PostHandlers.switchToTab('form');
                this.loadFormDataFromString(request.body || '');
            } else {
                PostHandlers.switchToTab('raw');
                document.getElementById('raw-body').value = request.body || '';
            }

            // Load custom headers
            this.loadCustomHeaders(request.headers || {});

            PostUtils.showNotification('Request loaded from history', 'success');
            console.log('📥 Request loaded from history:', entryId);

        } catch (error) {
            console.error('Failed to load from history:', error);
            PostUtils.showNotification('Failed to load from history', 'error');
        }
    },

    /**
     * Load form data from URL-encoded string
     * @param {string} formDataString - URL-encoded form data
     */
    loadFormDataFromString(formDataString) {
        // Clear existing form rows
        const container = document.querySelector('.form-data-container');
        container.innerHTML = '';

        if (!formDataString) {
            // Add empty row
            container.appendChild(PostHandlers.createFormRow());
            return;
        }

        // Parse form data and create rows
        const formData = PostUtils.urlEncodedToObject(formDataString);
        Object.entries(formData).forEach(([key, value]) => {
            const row = PostHandlers.createFormRow();
            row.querySelector('.form-key').value = key;
            row.querySelector('.form-value').value = value;
            container.appendChild(row);
        });

        // Add empty row for new entries
        container.appendChild(PostHandlers.createFormRow());
    },

    /**
     * Load custom headers
     * @param {Object} headers - Headers object
     */
    loadCustomHeaders(headers) {
        // Clear existing header rows
        const container = document.querySelector('.headers-container');
        container.innerHTML = '';

        // Filter out default headers
        const customHeaders = {};
        Object.entries(headers).forEach(([key, value]) => {
            if (!POST_CONFIG.DEFAULT_HEADERS.hasOwnProperty(key)) {
                customHeaders[key] = value;
            }
        });

        if (Object.keys(customHeaders).length === 0) {
            // Add empty row
            container.appendChild(PostHandlers.createHeaderRow());
            return;
        }

        // Create rows for custom headers
        Object.entries(customHeaders).forEach(([key, value]) => {
            const row = PostHandlers.createHeaderRow();
            row.querySelector('.header-key').value = key;
            row.querySelector('.header-value').value = value;
            container.appendChild(row);
        });

        // Add empty row for new entries
        container.appendChild(PostHandlers.createHeaderRow());
    },

    /**
     * Update history display in UI
     */
    updateHistoryDisplay() {
        const historyList = document.getElementById('history-list');
        const history = this.getHistory();

        if (history.length === 0) {
            historyList.innerHTML = '<p class="empty-history">No requests yet. Send a request to start building your history.</p>';
            return;
        }

        historyList.innerHTML = history.map(entry => this.createHistoryItemHTML(entry)).join('');

        // Add click handlers
        historyList.querySelectorAll('.history-item').forEach(item => {
            item.addEventListener('click', () => {
                const entryId = item.dataset.entryId;
                this.loadFromHistory(entryId);
            });
        });
    },

    /**
     * Create HTML for history item
     * @param {Object} entry - History entry
     * @returns {string} - HTML string
     */
    createHistoryItemHTML(entry) {
        const statusCategory = PostUtils.getStatusCategory(entry.response.status);
        const bodyPreview = entry.request.body
            ? (entry.request.body.length > 100
                ? entry.request.body.substring(0, 100) + '...'
                : entry.request.body)
            : 'No body';

        return `
            <div class="history-item" data-entry-id="${entry.id}">
                <div class="history-item-header">
                    <div class="history-left">
                        <span class="history-method">${entry.request.method}</span>
                        <span class="history-url">${PostUtils.escapeHtml(entry.request.url)}</span>
                    </div>
                    <div class="history-right">
                        <span class="history-status ${statusCategory}">${entry.response.status}</span>
                        <span class="history-time">${entry.timestamp}</span>
                    </div>
                </div>
                <div class="history-details">
                    <div class="history-meta">
                        <span>📄 ${entry.request.contentType}</span>
                        <span>⏱️ ${PostUtils.formatDuration(entry.response.duration)}</span>
                        <span>📦 ${PostUtils.formatFileSize(entry.response.size)}</span>
                    </div>
                    <div class="history-body-preview">
                        ${PostUtils.escapeHtml(bodyPreview)}
                    </div>
                </div>
            </div>
        `;
    },

    /**
     * Initialize history display
     */
    init() {
        this.updateHistoryDisplay();
        console.log('📚 History module initialized');
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = PostHistory;
}
