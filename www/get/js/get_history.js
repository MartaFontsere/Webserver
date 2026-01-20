/**
 * ================================================================
 * 📥 GET TESTING - HISTORY MANAGEMENT
 * Manage request history with local storage persistence
 * ================================================================
 */

class GetHistory {
    constructor() {
        this.history = this.loadHistory();
        this.maxItems = GET_CONFIG.MAX_HISTORY_ITEMS;
    }

    /**
     * Add request to history
     * @param {Object} requestData - Request and response data
     */
    addRequest(requestData) {
        const historyItem = {
            id: GetUtils.generateId(),
            method: 'GET',
            url: requestData.url,
            status: requestData.status,
            statusText: requestData.statusText,
            duration: requestData.duration,
            size: requestData.size,
            timestamp: requestData.timestamp,
            headers: requestData.headers,
            body: requestData.body,
            success: requestData.success,
            error: requestData.error || false
        };

        // Add to beginning of array
        this.history.unshift(historyItem);

        // Limit history size
        if (this.history.length > this.maxItems) {
            this.history = this.history.slice(0, this.maxItems);
        }

        // Save to localStorage
        this.saveHistory();

        // Update UI
        this.renderHistory();
    }

    /**
     * Clear all history
     */
    clearHistory() {
        this.history = [];
        this.saveHistory();
        this.renderHistory();
        GetUtils.showNotification(GET_CONFIG.MESSAGES.HISTORY_CLEARED, 'success');
    }

    /**
     * Export history as JSON file
     */
    exportHistory() {
        if (this.history.length === 0) {
            GetUtils.showNotification('No history to export', 'warning');
            return;
        }

        const exportData = {
            exported_at: new Date().toISOString(),
            total_requests: this.history.length,
            requests: this.history
        };

        const filename = `get_test_history_${new Date().toISOString().split('T')[0]}.json`;
        GetUtils.downloadAsFile(
            JSON.stringify(exportData, null, 2),
            filename,
            'application/json'
        );

        GetUtils.showNotification(GET_CONFIG.MESSAGES.HISTORY_EXPORTED, 'success');
    }

    /**
     * Get request from history by ID
     * @param {string} id - Request ID
     * @returns {Object|null} Request data or null
     */
    getRequest(id) {
        return this.history.find(item => item.id === id) || null;
    }

    /**
     * Replay request from history
     * @param {string} id - Request ID
     */
    async replayRequest(id) {
        const request = this.getRequest(id);
        if (!request) {
            GetUtils.showNotification('Request not found in history', 'error');
            return;
        }

        // Set URL in input
        const urlInput = document.getElementById('request-url');
        if (urlInput) {
            urlInput.value = request.url;
        }

        // Send the request
        await window.sendGetRequest();
    }

    /**
     * Load history from localStorage
     * @returns {Array} History items
     */
    loadHistory() {
        try {
            const stored = localStorage.getItem(GET_CONFIG.STORAGE_KEYS.HISTORY);
            return stored ? JSON.parse(stored) : [];
        } catch (error) {
            console.warn('Failed to load history from localStorage:', error);
            return [];
        }
    }

    /**
     * Save history to localStorage
     */
    saveHistory() {
        try {
            localStorage.setItem(GET_CONFIG.STORAGE_KEYS.HISTORY, JSON.stringify(this.history));
        } catch (error) {
            console.warn('Failed to save history to localStorage:', error);
        }
    }

    /**
     * Render history in the UI
     */
    renderHistory() {
        const historyList = document.getElementById('history-list');
        if (!historyList) return;

        if (this.history.length === 0) {
            historyList.innerHTML = '<p class="no-data">No requests in history</p>';
            return;
        }

        historyList.innerHTML = this.history.map(item => this.createHistoryItemHTML(item)).join('');

        // Add click listeners for history items
        historyList.querySelectorAll('.history-item').forEach(element => {
            element.addEventListener('click', () => {
                const id = element.dataset.id;
                this.showRequestDetails(id);
            });
        });
    }

    /**
     * Create HTML for history item
     * @param {Object} item - History item
     * @returns {string} HTML string
     */
    createHistoryItemHTML(item) {
        const statusInfo = GetUtils.getStatusInfo(item.status);
        const timeAgo = this.getTimeAgo(item.timestamp);

        return `
            <div class="history-item" data-id="${item.id}" title="Click to view details">
                <span class="history-method">${item.method}</span>
                <span class="history-url">${GetUtils.truncateText(item.url, 50)}</span>
                <span class="history-status ${statusInfo.class}" style="background-color: ${statusInfo.color}">
                    ${item.status}
                </span>
                <span class="history-time">${timeAgo}</span>
            </div>
        `;
    }

    /**
     * Show detailed view of a request
     * @param {string} id - Request ID
     */
    showRequestDetails(id) {
        const request = this.getRequest(id);
        if (!request) return;

        // Update URL input
        const urlInput = document.getElementById('request-url');
        if (urlInput) {
            urlInput.value = request.url;
        }

        // Update response display
        window.updateResponseDisplay(request);

        // Scroll to response section
        const responseSection = document.querySelector('.response-section');
        if (responseSection) {
            responseSection.scrollIntoView({ behavior: 'smooth' });
        }

        GetUtils.showNotification('Loaded request from history', 'info');
    }

    /**
     * Get time ago string
     * @param {string} timestamp - ISO timestamp
     * @returns {string} Time ago string
     */
    getTimeAgo(timestamp) {
        const now = new Date();
        const past = new Date(timestamp);
        const diffMs = now - past;

        const seconds = Math.floor(diffMs / 1000);
        const minutes = Math.floor(seconds / 60);
        const hours = Math.floor(minutes / 60);
        const days = Math.floor(hours / 24);

        if (days > 0) {
            return `${days}d ago`;
        } else if (hours > 0) {
            return `${hours}h ago`;
        } else if (minutes > 0) {
            return `${minutes}m ago`;
        } else {
            return `${seconds}s ago`;
        }
    }

    /**
     * Get statistics about the history
     * @returns {Object} Statistics
     */
    getStatistics() {
        const total = this.history.length;
        const successful = this.history.filter(item => item.success).length;
        const failed = this.history.filter(item => !item.success).length;
        const avgDuration = total > 0
            ? this.history.reduce((sum, item) => sum + item.duration, 0) / total
            : 0;

        // Status code distribution
        const statusCounts = {};
        this.history.forEach(item => {
            statusCounts[item.status] = (statusCounts[item.status] || 0) + 1;
        });

        // Most requested URLs
        const urlCounts = {};
        this.history.forEach(item => {
            urlCounts[item.url] = (urlCounts[item.url] || 0) + 1;
        });

        const mostRequested = Object.entries(urlCounts)
            .sort(([, a], [, b]) => b - a)
            .slice(0, 5);

        return {
            total,
            successful,
            failed,
            successRate: total > 0 ? (successful / total * 100).toFixed(1) : 0,
            avgDuration,
            statusCounts,
            mostRequested
        };
    }

    /**
     * Filter history by criteria
     * @param {Object} criteria - Filter criteria
     * @returns {Array} Filtered history
     */
    filterHistory(criteria = {}) {
        let filtered = [...this.history];

        if (criteria.status) {
            filtered = filtered.filter(item => item.status === criteria.status);
        }

        if (criteria.success !== undefined) {
            filtered = filtered.filter(item => item.success === criteria.success);
        }

        if (criteria.url) {
            const urlPattern = new RegExp(criteria.url, 'i');
            filtered = filtered.filter(item => urlPattern.test(item.url));
        }

        if (criteria.timeRange) {
            const now = new Date();
            const cutoff = new Date(now.getTime() - criteria.timeRange);
            filtered = filtered.filter(item => new Date(item.timestamp) >= cutoff);
        }

        return filtered;
    }
}

// Create global instance
const getHistory = new GetHistory();
