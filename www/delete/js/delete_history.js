/* =================================================================
   🗑️ DELETE TESTING - HISTORY
   Request history management for DELETE request testing
   ================================================================= */

// History Management
const DeleteHistory = {

    /**
     * Save deletion request to history
     * @param {Object} request - Request configuration
     * @param {Object} response - Response object
     */
    saveToHistory(request, response) {
        try {
            const historyEntry = {
                id: DeleteUtils.generateId(),
                timestamp: DeleteUtils.getTimestamp(),
                request: {
                    method: request.method || 'DELETE',
                    url: request.url || request.urls || 'Unknown',
                    headers: request.headers || {},
                    resourceCount: Array.isArray(request.urls) ? request.urls.length : 1
                },
                response: {
                    status: response.status,
                    statusText: response.statusText,
                    headers: response.headers,
                    body: response.body,
                    duration: response.duration,
                    size: response.size,
                    error: response.error || false
                },
                canUndo: this.canUndoOperation(response.status),
                undoExpires: Date.now() + DELETE_CONFIG.HISTORY.UNDO_TIMEOUT
            };

            let history = this.getHistory();
            history.unshift(historyEntry);

            // Limit history size
            if (history.length > DELETE_CONFIG.HISTORY.MAX_ENTRIES) {
                history = history.slice(0, DELETE_CONFIG.HISTORY.MAX_ENTRIES);
            }

            // Save to localStorage
            localStorage.setItem(DELETE_CONFIG.HISTORY.STORAGE_KEY, JSON.stringify(history));

            // Update UI
            this.updateHistoryDisplay();

            // Update undo button
            this.updateUndoButton();

            console.log('💾 Deletion saved to history:', historyEntry.id);

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
            const stored = localStorage.getItem(DELETE_CONFIG.HISTORY.STORAGE_KEY);
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
            localStorage.removeItem(DELETE_CONFIG.HISTORY.STORAGE_KEY);
            this.updateHistoryDisplay();
            this.updateUndoButton();
            DeleteUtils.showNotification(DELETE_CONFIG.SUCCESS_MESSAGES.HISTORY_CLEARED, 'success');
            console.log('🗑️ Delete history cleared');
        } catch (error) {
            console.error('Failed to clear history:', error);
            DeleteUtils.showNotification('Failed to clear history', 'error');
        }
    },

    /**
     * Export history as JSON file
     */
    exportHistory() {
        try {
            const history = this.getHistory();
            if (history.length === 0) {
                DeleteUtils.showNotification('No history to export', 'warning');
                return;
            }

            const exportData = {
                exportDate: new Date().toISOString(),
                exportType: 'DELETE_REQUEST_HISTORY',
                totalEntries: history.length,
                entries: history.map(entry => ({
                    ...entry,
                    // Remove sensitive data from export
                    response: {
                        ...entry.response,
                        body: entry.response.body.length > 1000 ?
                            entry.response.body.substring(0, 1000) + '... [truncated]' :
                            entry.response.body
                    }
                }))
            };

            const jsonContent = JSON.stringify(exportData, null, 2);
            const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
            const filename = `delete-request-history-${timestamp}.json`;

            DeleteUtils.downloadAsFile(jsonContent, filename, 'application/json');
            DeleteUtils.showNotification(DELETE_CONFIG.SUCCESS_MESSAGES.HISTORY_EXPORTED, 'success');

        } catch (error) {
            console.error('Failed to export history:', error);
            DeleteUtils.showNotification('Failed to export history', 'error');
        }
    },

    /**
     * Attempt to undo the last deletion
     */
    async undoLastDeletion() {
        try {
            const history = this.getHistory();
            const lastEntry = history.find(entry =>
                entry.canUndo &&
                entry.undoExpires > Date.now() &&
                entry.response.status >= 200 && entry.response.status < 300
            );

            if (!lastEntry) {
                DeleteUtils.showNotification(DELETE_CONFIG.ERROR_MESSAGES.UNDO_EXPIRED, 'warning');
                return;
            }

            // Attempt to restore the resource
            const success = await this.attemptRestore(lastEntry);

            if (success) {
                // Mark as undone
                lastEntry.undone = true;
                lastEntry.undoTimestamp = DeleteUtils.getTimestamp();

                // Save updated history
                localStorage.setItem(DELETE_CONFIG.HISTORY.STORAGE_KEY, JSON.stringify(history));

                this.updateHistoryDisplay();
                this.updateUndoButton();

                DeleteUtils.showNotification(DELETE_CONFIG.SUCCESS_MESSAGES.UNDO_SUCCESS, 'success');
            } else {
                DeleteUtils.showNotification('Undo operation failed', 'error');
            }

        } catch (error) {
            console.error('Undo operation failed:', error);
            DeleteUtils.showNotification('Undo operation failed', 'error');
        }
    },

    /**
     * Attempt to restore a deleted resource
     * @param {Object} historyEntry - History entry to restore
     * @returns {Promise<boolean>} - Success status
     */
    async attemptRestore(historyEntry) {
        // This is a placeholder for undo functionality
        // In a real implementation, this would depend on server support
        // for restoration or backup mechanisms

        console.log('Attempting to restore:', historyEntry.request.url);

        // For demonstration, we'll just log the attempt
        // In practice, you might:
        // 1. Check if the resource has a backup
        // 2. Call a restore endpoint
        // 3. Re-upload the resource if it was a file

        return new Promise((resolve) => {
            setTimeout(() => {
                // Simulate restoration attempt
                const success = Math.random() > 0.5; // 50% success rate for demo
                resolve(success);
            }, 1000);
        });
    },

    /**
     * Load request from history (replay)
     * @param {string} entryId - ID of the history entry
     */
    loadFromHistory(entryId) {
        try {
            const history = this.getHistory();
            const entry = history.find(item => item.id === entryId);

            if (!entry) {
                DeleteUtils.showNotification('History entry not found', 'error');
                return;
            }

            const request = entry.request;

            // Update UI with request data
            if (Array.isArray(request.url)) {
                // Bulk deletion - show in bulk section
                DeleteUtils.showNotification('Bulk deletion loaded - use bulk delete section to retry', 'info');
            } else {
                // Single deletion
                document.getElementById('resource-url').value = request.url;

                // Load custom headers
                this.loadCustomHeaders(request.headers || {});

                DeleteUtils.showNotification('Deletion request loaded from history', 'success');
            }

            console.log('📥 Request loaded from history:', entryId);

        } catch (error) {
            console.error('Failed to load from history:', error);
            DeleteUtils.showNotification('Failed to load from history', 'error');
        }
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
            if (!DELETE_CONFIG.DEFAULT_HEADERS.hasOwnProperty(key)) {
                customHeaders[key] = value;
            }
        });

        if (Object.keys(customHeaders).length === 0) {
            // Add empty row
            container.appendChild(DeleteHandlers.createHeaderRow());
            return;
        }

        // Create rows for custom headers
        Object.entries(customHeaders).forEach(([key, value]) => {
            const row = DeleteHandlers.createHeaderRow();
            row.querySelector('.header-key').value = key;
            row.querySelector('.header-value').value = value;
            container.appendChild(row);
        });

        // Add empty row for new entries
        container.appendChild(DeleteHandlers.createHeaderRow());
    },

    /**
     * Check if operation can be undone
     * @param {number} statusCode - Response status code
     * @returns {boolean} - Whether operation can be undone
     */
    canUndoOperation(statusCode) {
        return DELETE_CONFIG.HISTORY.ENABLE_UNDO &&
            DELETE_CONFIG.STATUS_CODES.DELETE_SUCCESS.includes(statusCode);
    },

    /**
     * Update undo button state
     */
    updateUndoButton() {
        const undoButton = document.getElementById('undo-last');
        const history = this.getHistory();

        const canUndo = history.some(entry =>
            entry.canUndo &&
            !entry.undone &&
            entry.undoExpires > Date.now() &&
            entry.response.status >= 200 && entry.response.status < 300
        );

        undoButton.disabled = !canUndo;

        if (canUndo) {
            const lastUndoable = history.find(entry =>
                entry.canUndo &&
                !entry.undone &&
                entry.undoExpires > Date.now()
            );

            if (lastUndoable) {
                const timeLeft = Math.ceil((lastUndoable.undoExpires - Date.now()) / 1000);
                undoButton.textContent = `↶ Undo Last (${timeLeft}s)`;
            }
        } else {
            undoButton.textContent = '↶ Undo Last';
        }
    },

    /**
     * Update history display in UI
     */
    updateHistoryDisplay() {
        const historyList = document.getElementById('history-list');
        const history = this.getHistory();

        if (history.length === 0) {
            historyList.innerHTML = '<p class="empty-history">No deletions yet. Execute delete operations to start building your history.</p>';
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
        const statusCategory = DeleteUtils.getStatusCategory(entry.response.status);
        const isUndoable = entry.canUndo && !entry.undone && entry.undoExpires > Date.now();
        const isUndone = entry.undone;

        let urlDisplay, resourceInfo;
        if (Array.isArray(entry.request.url)) {
            urlDisplay = `Bulk Delete (${entry.request.resourceCount} resources)`;
            resourceInfo = `📋 ${entry.request.resourceCount} resources`;
        } else {
            urlDisplay = entry.request.url;
            const resourceType = DeleteUtils.getResourceType(entry.request.url);
            resourceInfo = `${this.getResourceIcon(resourceType)} ${resourceType}`;
        }

        const undoIndicator = isUndone ?
            '<span class="undo-indicator">↶ UNDONE</span>' :
            (isUndoable ? '<span class="undo-available">↶ Can Undo</span>' : '');

        return `
            <div class="history-item ${isUndone ? 'undone' : ''}" data-entry-id="${entry.id}">
                <div class="history-item-header">
                    <div class="history-left">
                        <span class="history-method">${entry.request.method}</span>
                        <span class="history-url">${DeleteUtils.escapeHtml(urlDisplay)}</span>
                        ${undoIndicator}
                    </div>
                    <div class="history-right">
                        <span class="history-status ${statusCategory}">${entry.response.status}</span>
                        <span class="history-time">${entry.timestamp}</span>
                    </div>
                </div>
                <div class="history-details">
                    <div class="history-meta">
                        <span>${resourceInfo}</span>
                        <span>⏱️ ${DeleteUtils.formatDuration(entry.response.duration)}</span>
                        <span>📦 ${DeleteUtils.formatFileSize(entry.response.size)}</span>
                        ${entry.response.error ? '<span class="error-indicator">❌ Error</span>' : ''}
                    </div>
                </div>
            </div>
        `;
    },

    /**
     * Get resource icon for resource type
     * @param {string} resourceType - Resource type
     * @returns {string} - Icon emoji
     */
    getResourceIcon(resourceType) {
        const icons = {
            file: '📄',
            user: '👤',
            product: '📦',
            post: '📝',
            comment: '💬',
            resource: '🗂️'
        };
        return icons[resourceType] || icons.resource;
    },

    /**
     * Start undo countdown timer
     */
    startUndoCountdown() {
        if (this.undoTimer) {
            clearInterval(this.undoTimer);
        }

        this.undoTimer = setInterval(() => {
            this.updateUndoButton();

            // Check if any undo opportunities have expired
            const history = this.getHistory();
            const hasUndoable = history.some(entry =>
                entry.canUndo &&
                !entry.undone &&
                entry.undoExpires > Date.now()
            );

            if (!hasUndoable && this.undoTimer) {
                clearInterval(this.undoTimer);
                this.undoTimer = null;
            }
        }, 1000);
    },

    /**
     * Initialize history display
     */
    init() {
        this.updateHistoryDisplay();
        this.updateUndoButton();

        // Start undo countdown if there are undoable operations
        const history = this.getHistory();
        const hasUndoable = history.some(entry =>
            entry.canUndo &&
            !entry.undone &&
            entry.undoExpires > Date.now()
        );

        if (hasUndoable) {
            this.startUndoCountdown();
        }

        console.log('📚 Delete history module initialized');
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = DeleteHistory;
}
