/**
 * ================================================================
 * ⚙️ CGI TESTING - HISTORY MANAGEMENT
 * History management for CGI script executions
 * ================================================================
 */

// History management object
const CGI_HISTORY = {

    // History storage
    executions: [],
    storageKey: 'cgi_execution_history',

    /**
     * Initialize history management
     */
    init() {
        this.loadFromStorage();
        this.renderHistory();
        console.log('✅ CGI history initialized');
    },

    /**
     * Add execution to history
     */
    addExecution(execution) {
        const historyItem = {
            id: CGI_UTILS.generateExecutionId(),
            timestamp: execution.timestamp || CGI_UTILS.getCurrentTimestamp(),
            script: execution.script,
            method: execution.method,
            queryParams: execution.queryParams || {},
            postData: execution.postData || null,
            envVars: execution.envVars || {},
            result: execution.result,
            success: execution.result?.success || false
        };

        // Add to beginning of array
        this.executions.unshift(historyItem);

        // Limit history size
        if (this.executions.length > CGI_CONFIG.MAX_HISTORY_ITEMS) {
            this.executions = this.executions.slice(0, CGI_CONFIG.MAX_HISTORY_ITEMS);
        }

        this.saveToStorage();
        this.renderHistory();

        CGI_UTILS.debug('Added execution to history:', historyItem);
    },

    /**
     * Get execution by index
     */
    getExecution(index) {
        return this.executions[index] || null;
    },

    /**
     * Get execution by ID
     */
    getExecutionById(id) {
        return this.executions.find(exec => exec.id === id) || null;
    },

    /**
     * Remove execution from history
     */
    removeExecution(index) {
        if (index >= 0 && index < this.executions.length) {
            const removed = this.executions.splice(index, 1)[0];
            this.saveToStorage();
            this.renderHistory();
            return removed;
        }
        return null;
    },

    /**
     * Clear all history
     */
    clear() {
        this.executions = [];
        this.saveToStorage();
        this.renderHistory();
        CGI_UTILS.debug('History cleared');
    },

    /**
     * Load history from localStorage
     */
    loadFromStorage() {
        try {
            const stored = localStorage.getItem(this.storageKey);
            if (stored) {
                this.executions = JSON.parse(stored);
                CGI_UTILS.debug('History loaded from storage:', this.executions.length, 'items');
            }
        } catch (error) {
            CGI_UTILS.error('Failed to load history from storage:', error);
            this.executions = [];
        }
    },

    /**
     * Save history to localStorage
     */
    saveToStorage() {
        try {
            localStorage.setItem(this.storageKey, JSON.stringify(this.executions));
            CGI_UTILS.debug('History saved to storage');
        } catch (error) {
            CGI_UTILS.error('Failed to save history to storage:', error);
        }
    },

    /**
     * Render history list
     */
    renderHistory() {
        const historyList = document.getElementById('history-list');
        if (!historyList) return;

        if (this.executions.length === 0) {
            historyList.innerHTML = '<p class="no-data">No executions in history</p>';
            return;
        }

        const historyHtml = this.executions.map((execution, index) => {
            return this.renderHistoryItem(execution, index);
        }).join('');

        historyList.innerHTML = historyHtml;
    },

    /**
     * Render individual history item
     */
    renderHistoryItem(execution, index) {
        const timestamp = CGI_UTILS.formatTimestamp(execution.timestamp);
        const statusClass = execution.success ? 'success' : 'error';
        const statusIcon = execution.success ? '✅' : '❌';
        const status = execution.result?.status || 'Unknown';
        const executionTime = execution.result?.executionTime || 0;
        const outputSize = execution.result?.outputSize || 0;

        // Generate query string for display
        const queryString = CGI_UTILS.buildQueryString(execution.queryParams);
        const hasPostData = execution.method === 'POST' && execution.postData;
        const hasEnvVars = Object.keys(execution.envVars).length > 0;

        return `
            <div class="history-item" data-index="${index}">
                <div class="history-header">
                    <div class="history-info">
                        <span class="history-method method-${execution.method.toLowerCase()}">${execution.method}</span>
                        <span class="history-script">${execution.script}</span>
                        <span class="history-status status-${statusClass}">${statusIcon} ${status}</span>
                    </div>
                    <div class="history-meta">
                        <span class="history-time">${timestamp}</span>
                        <div class="history-actions">
                            <button class="history-action" onclick="CGI_HISTORY.loadExecution(${index})" title="Load execution">🔄</button>
                            <button class="history-action" onclick="CGI_HISTORY.copyExecution(${index})" title="Copy details">📋</button>
                            <button class="history-action" onclick="CGI_HISTORY.removeExecution(${index})" title="Remove from history">🗑️</button>
                        </div>
                    </div>
                </div>
                
                <div class="history-details">
                    <div class="history-detail-row">
                        <span class="detail-label">Execution Time:</span>
                        <span class="detail-value">${CGI_UTILS.formatTime(executionTime)}</span>
                    </div>
                    <div class="history-detail-row">
                        <span class="detail-label">Output Size:</span>
                        <span class="detail-value">${CGI_UTILS.formatBytes(outputSize)}</span>
                    </div>
                    ${queryString ? `
                        <div class="history-detail-row">
                            <span class="detail-label">Query:</span>
                            <span class="detail-value">${CGI_UTILS.escapeHtml(queryString)}</span>
                        </div>
                    ` : ''}
                    ${hasPostData ? `
                        <div class="history-detail-row">
                            <span class="detail-label">POST Data:</span>
                            <span class="detail-value">${CGI_UTILS.truncateText(execution.postData.content || '', 50)}</span>
                        </div>
                    ` : ''}
                    ${hasEnvVars ? `
                        <div class="history-detail-row">
                            <span class="detail-label">Env Vars:</span>
                            <span class="detail-value">${Object.keys(execution.envVars).length} variables</span>
                        </div>
                    ` : ''}
                </div>
            </div>
        `;
    },

    /**
     * Load execution from history
     */
    loadExecution(index) {
        const execution = this.getExecution(index);
        if (!execution) return;

        // Load script configuration
        const scriptPath = document.getElementById('script-path');
        const methodSelect = document.getElementById('method-select');

        if (scriptPath) scriptPath.value = execution.script;
        if (methodSelect) {
            methodSelect.value = execution.method;
            CGI_HANDLERS.handleMethodChange(execution.method);
        }

        // Load query parameters
        this.loadQueryParameters(execution.queryParams);

        // Load POST data if applicable
        if (execution.method === 'POST' && execution.postData) {
            this.loadPostData(execution.postData);
        }

        // Load environment variables
        this.loadEnvironmentVariables(execution.envVars);

        // Display results
        if (execution.result) {
            CGI_HANDLERS.currentResult = execution.result;
            CGI_HANDLERS.displayResults(execution.result);
        }

        CGI_UTILS.showMessage(`Loaded execution from ${CGI_UTILS.formatTimestamp(execution.timestamp)}`, 'success');
    },

    /**
     * Load query parameters into form
     */
    loadQueryParameters(params) {
        const container = document.getElementById('query-params');
        if (!container) return;

        // Clear existing parameters
        container.innerHTML = '';

        // Add parameters
        Object.entries(params).forEach(([key, value]) => {
            const paramRow = document.createElement('div');
            paramRow.className = 'param-row';
            paramRow.innerHTML = `
                <input type="text" class="param-key" value="${CGI_UTILS.escapeHtml(key)}">
                <input type="text" class="param-value" value="${CGI_UTILS.escapeHtml(value)}">
                <button class="remove-param">🗑️</button>
            `;
            container.appendChild(paramRow);
        });

        // Add empty row if none exist
        if (Object.keys(params).length === 0) {
            CGI_HANDLERS.addParameter('query-params');
        }
    },

    /**
     * Load POST data into form
     */
    loadPostData(postData) {
        const postDataTextarea = document.getElementById('post-data');
        const contentTypeSelect = document.getElementById('content-type');

        if (postDataTextarea && postData.content) {
            postDataTextarea.value = postData.content;
        }

        if (contentTypeSelect && postData.contentType) {
            contentTypeSelect.value = postData.contentType;
        }
    },

    /**
     * Load environment variables into form
     */
    loadEnvironmentVariables(envVars) {
        const container = document.getElementById('env-vars');
        if (!container) return;

        // Clear existing variables
        container.innerHTML = '';

        // Add environment variables
        Object.entries(envVars).forEach(([key, value]) => {
            const paramRow = document.createElement('div');
            paramRow.className = 'param-row';
            paramRow.innerHTML = `
                <input type="text" class="param-key" value="${CGI_UTILS.escapeHtml(key)}">
                <input type="text" class="param-value" value="${CGI_UTILS.escapeHtml(value)}">
                <button class="remove-param">🗑️</button>
            `;
            container.appendChild(paramRow);
        });

        // Add empty row if none exist
        if (Object.keys(envVars).length === 0) {
            CGI_HANDLERS.addParameter('env-vars');
        }
    },

    /**
     * Copy execution details to clipboard
     */
    async copyExecution(index) {
        const execution = this.getExecution(index);
        if (!execution) return;

        const details = this.formatExecutionForCopy(execution);
        const success = await CGI_UTILS.copyToClipboard(details);

        if (success) {
            CGI_UTILS.showMessage('Execution details copied to clipboard', 'success');
        }
    },

    /**
     * Format execution for copying
     */
    formatExecutionForCopy(execution) {
        const lines = [
            '=== CGI EXECUTION DETAILS ===',
            `Timestamp: ${execution.timestamp}`,
            `Script: ${execution.script}`,
            `Method: ${execution.method}`,
            `Status: ${execution.result?.status || 'Unknown'}`,
            `Execution Time: ${CGI_UTILS.formatTime(execution.result?.executionTime || 0)}`,
            `Output Size: ${CGI_UTILS.formatBytes(execution.result?.outputSize || 0)}`,
            ''
        ];

        // Query parameters
        if (Object.keys(execution.queryParams).length > 0) {
            lines.push('Query Parameters:');
            Object.entries(execution.queryParams).forEach(([key, value]) => {
                lines.push(`  ${key}=${value}`);
            });
            lines.push('');
        }

        // POST data
        if (execution.postData && execution.postData.content) {
            lines.push('POST Data:');
            lines.push(`  Content-Type: ${execution.postData.contentType || 'Unknown'}`);
            lines.push(`  Data: ${execution.postData.content}`);
            lines.push('');
        }

        // Environment variables
        if (Object.keys(execution.envVars).length > 0) {
            lines.push('Environment Variables:');
            Object.entries(execution.envVars).forEach(([key, value]) => {
                lines.push(`  ${key}=${value}`);
            });
            lines.push('');
        }

        // Output
        if (execution.result?.output) {
            lines.push('Output:');
            lines.push(execution.result.output);
            lines.push('');
        }

        // Errors
        if (execution.result?.errors) {
            lines.push('Errors:');
            lines.push(execution.result.errors);
        }

        return lines.join('\n');
    },

    /**
     * Export history to file
     */
    export() {
        if (this.executions.length === 0) {
            CGI_UTILS.showMessage('No history to export', 'warning');
            return;
        }

        const exportData = {
            timestamp: CGI_UTILS.getCurrentTimestamp(),
            version: '1.0',
            totalExecutions: this.executions.length,
            executions: this.executions
        };

        const content = JSON.stringify(exportData, null, 2);
        const filename = `cgi_history_${new Date().toISOString().split('T')[0]}.json`;

        CGI_UTILS.downloadAsFile(content, filename, 'application/json');
    },

    /**
     * Import history from file
     */
    async import(file) {
        try {
            const content = await this.readFileContent(file);
            const importData = JSON.parse(content);

            if (!importData.executions || !Array.isArray(importData.executions)) {
                throw new Error('Invalid history file format');
            }

            // Merge with existing history
            const newExecutions = importData.executions.filter(exec => {
                return !this.executions.find(existing => existing.id === exec.id);
            });

            this.executions = [...newExecutions, ...this.executions];

            // Limit history size
            if (this.executions.length > CGI_CONFIG.MAX_HISTORY_ITEMS) {
                this.executions = this.executions.slice(0, CGI_CONFIG.MAX_HISTORY_ITEMS);
            }

            this.saveToStorage();
            this.renderHistory();

            CGI_UTILS.showMessage(`Imported ${newExecutions.length} new executions`, 'success');

        } catch (error) {
            CGI_UTILS.error('Failed to import history:', error);
            CGI_UTILS.showMessage('Failed to import history file', 'error');
        }
    },

    /**
     * Read file content
     */
    readFileContent(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = (e) => resolve(e.target.result);
            reader.onerror = (e) => reject(new Error('Failed to read file'));
            reader.readAsText(file);
        });
    },

    /**
     * Search history
     */
    search(query) {
        if (!query || query.trim() === '') {
            this.renderHistory();
            return;
        }

        const searchTerm = query.toLowerCase();
        const filtered = this.executions.filter(execution => {
            return execution.script.toLowerCase().includes(searchTerm) ||
                execution.method.toLowerCase().includes(searchTerm) ||
                (execution.result?.status || '').toLowerCase().includes(searchTerm) ||
                JSON.stringify(execution.queryParams).toLowerCase().includes(searchTerm);
        });

        this.renderFilteredHistory(filtered);
    },

    /**
     * Render filtered history
     */
    renderFilteredHistory(executions) {
        const historyList = document.getElementById('history-list');
        if (!historyList) return;

        if (executions.length === 0) {
            historyList.innerHTML = '<p class="no-data">No matching executions found</p>';
            return;
        }

        const historyHtml = executions.map((execution, index) => {
            // Find original index
            const originalIndex = this.executions.indexOf(execution);
            return this.renderHistoryItem(execution, originalIndex);
        }).join('');

        historyList.innerHTML = historyHtml;
    },

    /**
     * Get history statistics
     */
    getStatistics() {
        const total = this.executions.length;
        const successful = this.executions.filter(exec => exec.success).length;
        const failed = total - successful;

        const methods = {};
        const scripts = {};

        this.executions.forEach(exec => {
            methods[exec.method] = (methods[exec.method] || 0) + 1;
            scripts[exec.script] = (scripts[exec.script] || 0) + 1;
        });

        return {
            total,
            successful,
            failed,
            successRate: total > 0 ? (successful / total * 100).toFixed(1) : 0,
            methods,
            scripts,
            mostUsedScript: Object.keys(scripts).reduce((a, b) => scripts[a] > scripts[b] ? a : b, ''),
            mostUsedMethod: Object.keys(methods).reduce((a, b) => methods[a] > methods[b] ? a : b, '')
        };
    }
};
