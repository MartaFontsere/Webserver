/**
 * ================================================================
 * ⚙️ CGI TESTING - HANDLERS
 * Event handlers and user interaction logic for CGI script testing
 * ================================================================
 */

// Main handlers object
const CGI_HANDLERS = {

    /**
     * Initialize all event handlers
     */
    init() {
        this.setupExecutionHandlers();
        this.setupParameterHandlers();
        this.setupQuickScriptHandlers();
        this.setupViewHandlers();
        this.setupHistoryHandlers();
        this.setupKeyboardShortcuts();
        console.log('✅ CGI handlers initialized');
    },

    /**
     * Setup script execution handlers
     */
    setupExecutionHandlers() {
        const executeButton = document.getElementById('execute-script');
        const methodSelect = document.getElementById('method-select');
        const scriptPath = document.getElementById('script-path');

        executeButton?.addEventListener('click', () => {
            this.executeScript();
        });

        // Method change handler
        methodSelect?.addEventListener('change', (e) => {
            this.handleMethodChange(e.target.value);
        });

        // Script path validation
        scriptPath?.addEventListener('input', (e) => {
            this.validateScriptPath(e.target.value);
        });
    },

    /**
     * Setup parameter management handlers
     */
    setupParameterHandlers() {
        // Query parameters
        const addQueryParam = document.getElementById('add-query-param');
        addQueryParam?.addEventListener('click', () => {
            this.addParameter('query-params');
        });

        // Environment variables
        const addEnvVar = document.getElementById('add-env-var');
        addEnvVar?.addEventListener('click', () => {
            this.addParameter('env-vars');
        });

        // Content type change
        const contentType = document.getElementById('content-type');
        contentType?.addEventListener('change', (e) => {
            this.handleContentTypeChange(e.target.value);
        });

        // Dynamic parameter removal
        document.addEventListener('click', (e) => {
            if (e.target.classList.contains('remove-param')) {
                this.removeParameter(e.target);
            }
        });
    },

    /**
     * Setup quick script buttons
     */
    setupQuickScriptHandlers() {
        const quickButtons = document.querySelectorAll('.quick-btn');
        quickButtons.forEach(button => {
            button.addEventListener('click', () => {
                const script = button.dataset.script;
                const method = button.dataset.method;
                this.loadQuickScript(script, method);
            });
        });
    },

    /**
     * Setup output view handlers
     */
    setupViewHandlers() {
        const viewButtons = document.querySelectorAll('.view-btn');
        viewButtons.forEach(button => {
            button.addEventListener('click', () => {
                this.switchView(button.id);
            });
        });
    },

    /**
     * Setup history handlers
     */
    setupHistoryHandlers() {
        const clearHistory = document.getElementById('clear-history');
        clearHistory?.addEventListener('click', () => {
            this.clearHistory();
        });

        const exportHistory = document.getElementById('export-history');
        exportHistory?.addEventListener('click', () => {
            this.exportHistory();
        });

        // History item clicks
        document.addEventListener('click', (e) => {
            if (e.target.closest('.history-item')) {
                const historyItem = e.target.closest('.history-item');
                this.loadHistoryItem(historyItem);
            }
        });
    },

    /**
     * Setup keyboard shortcuts
     */
    setupKeyboardShortcuts() {
        document.addEventListener('keydown', (e) => {
            // Ctrl/Cmd + Enter: Execute script
            if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
                e.preventDefault();
                this.executeScript();
            }

            // Ctrl/Cmd + K: Clear history
            if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
                e.preventDefault();
                this.clearHistory();
            }

            // Ctrl/Cmd + E: Export history
            if ((e.ctrlKey || e.metaKey) && e.key === 'e') {
                e.preventDefault();
                this.exportHistory();
            }
        });
    },

    /**
     * Execute CGI script
     */
    async executeScript() {
        const scriptPath = document.getElementById('script-path').value.trim();
        const method = document.getElementById('method-select').value;

        if (!scriptPath) {
            CGI_UTILS.showMessage('Please enter a script path', 'error');
            return;
        }

        // Prepare execution data
        const executionData = {
            script: scriptPath,
            method: method,
            queryParams: this.getQueryParameters() || {},
            postData: method === 'POST' ? this.getPostData() : null,
            envVars: this.getEnvironmentVariables() || {},
            timestamp: new Date().toISOString()
        };

        try {
            CGI_UTILS.showIndicator('loading');
            CGI_UTILS.updateExecutionStatus('Executing...', 'executing');

            // Execute script (simulation or real backend call)
            const result = await this.performExecution(executionData);

            // Display results
            this.displayResults(result);

            // Add to history
            CGI_HISTORY.addExecution({
                ...executionData,
                result: result
            });

            // Show appropriate indicator based on success
            CGI_UTILS.showIndicator(result.success ? 'success' : 'error');
            CGI_UTILS.updateExecutionStatus(result.status, result.success ? 'success' : 'error');

        } catch (error) {
            console.error('Script execution failed:', error);

            // Create error result for history
            const errorResult = {
                status: '500 Internal Server Error',
                success: false,
                output: `Error: ${error.message}`,
                errors: error.stack || error.message,
                executionTime: 0,
                outputSize: 0
            };

            // Add failed execution to history too
            CGI_HISTORY.addExecution({
                ...executionData,
                result: errorResult
            });

            // Display error result
            this.displayResults(errorResult);

            CGI_UTILS.showMessage(`Execution failed: ${error.message}`, 'error');
            CGI_UTILS.showIndicator('error');
            CGI_UTILS.updateExecutionStatus('Failed', 'error');
        }
    },

    /**
     * Perform actual script execution
     */
    async performExecution(executionData) {
        const startTime = Date.now();

        // In production, this would call the actual backend
        // For now, we simulate different responses
        if (CGI_CONFIG.PRODUCTION_MODE) {
            return await this.executeRealScript(executionData);
        } else {
            return await this.simulateExecution(executionData);
        }
    },

    /**
     * Execute real script via backend
     */
    async executeRealScript(executionData) {
        const url = CGI_CONFIG.CGI_ENDPOINT;
        const startTime = Date.now();

        const response = await fetch(url, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(executionData),
            signal: AbortSignal.timeout(CGI_CONFIG.REQUEST_TIMEOUT)
        });

        const executionTime = Date.now() - startTime;
        const result = await response.json();

        return {
            ...result,
            executionTime: executionTime,
            success: response.ok
        };
    },

    /**
     * Simulate script execution for testing
     */
    async simulateExecution(executionData) {
        const startTime = Date.now();

        // Simulate network delay
        await CGI_UTILS.delay(CGI_UTILS.randomBetween(500, 2000));

        const script = executionData.script;
        let response;

        // First check if we have a predefined simulation response
        if (CGI_CONFIG.SIMULATION_RESPONSES[script]) {
            const simResponse = CGI_CONFIG.SIMULATION_RESPONSES[script];
            let output = simResponse.output;
            let status = simResponse.status;
            let errors = '';            // Check for method mismatches that should cause errors
            const shouldFail = this.checkForSimulatedErrors(script, executionData);

            if (shouldFail) {
                status = 405;
                errors = shouldFail.error;
                output = `Content-Type: text/plain\n\nError 405: Method Not Allowed\n${shouldFail.error}`;
            } else {
                // For mini.py, make output dynamic with actual parameters
                if (script === '/cgi-bin/mini.py') {
                    const queryParams = executionData.queryParams || {};
                    const queryString = new URLSearchParams(queryParams).toString();
                    output = `Content-Type: text/plain; charset=utf-8\n\n=== MINI CGI TEST ===\n✅ Script ejecutado correctamente\n\nMétodo: ${executionData.method}\nQuery: ${queryString || '(vacío)'}\nServidor: localhost\n\n`;

                    if (queryString) {
                        output += 'Parámetros:\n';
                        for (const [key, value] of new URLSearchParams(queryParams)) {
                            output += `  ${key} = ${value}\n`;
                        }
                        output += '\n';
                    }

                    output += '=== FIN TEST ===\n🎯 CGI funciona perfectamente!';
                }
            }

            response = {
                status: `${status} ${status === 200 ? 'OK' : status === 405 ? 'Method Not Allowed' : 'Error'}`,
                success: status === 200,
                headers: status === 200 ? simResponse.headers : { 'Content-Type': 'text/plain' },
                output: output,
                errors: errors,
                environment: this.getSimulatedEnvironment(executionData)
            };
        } else {
            // Fallback to old logic for backward compatibility
            if (script.includes('env')) {
                response = this.simulateEnvScript(executionData);
            } else if (script.includes('form')) {
                response = this.simulateFormScript(executionData);
            } else if (script.includes('error')) {
                response = this.simulateErrorScript(executionData);
            } else {
                response = this.simulateGenericScript(executionData);
            }
        }

        const executionTime = Date.now() - startTime;

        return {
            ...response,
            executionTime: executionTime,
            outputSize: response.output ? response.output.length : 0
        };
    },

    /**
     * Simulate environment info script
     */
    simulateEnvScript(data) {
        const envOutput = Object.entries(this.getSimulatedEnvironment(data))
            .map(([key, value]) => `${key}=${value}`)
            .join('\n');

        return {
            status: '200 OK',
            success: true,
            headers: {
                'Content-Type': 'text/plain'
            },
            output: `Content-Type: text/plain\n\n${envOutput}`,
            errors: '',
            environment: this.getSimulatedEnvironment(data)
        };
    },

    /**
     * Simulate form processing script
     */
    simulateFormScript(data) {
        const postData = data.postData || '{}';
        return {
            status: '200 OK',
            success: true,
            headers: {
                'Content-Type': 'application/json'
            },
            output: `Content-Type: application/json\n\n{"received": ${JSON.stringify(postData)}, "processed": true, "timestamp": "${new Date().toISOString()}"}`,
            errors: '',
            environment: this.getSimulatedEnvironment(data)
        };
    },

    /**
     * Simulate error script
     */
    simulateErrorScript(data) {
        return {
            status: '500 Internal Server Error',
            success: false,
            headers: {
                'Content-Type': 'text/plain'
            },
            output: 'Content-Type: text/plain\n\nCGI Script Error: Simulated error for testing purposes.',
            errors: 'ERROR: Simulated CGI script error\nLine 42: Division by zero\nScript execution terminated',
            environment: this.getSimulatedEnvironment(data)
        };
    },

    /**
     * Simulate generic script
     */
    simulateGenericScript(data) {
        return {
            status: '200 OK',
            success: true,
            headers: {
                'Content-Type': 'text/plain'
            },
            output: `Content-Type: text/plain\n\nGeneric CGI Script Output\nScript: ${data.script}\nMethod: ${data.method}\nExecuted at: ${new Date().toISOString()}`,
            errors: '',
            environment: this.getSimulatedEnvironment(data)
        };
    },

    /**
     * Check if we should simulate errors based on method mismatches
     */
    checkForSimulatedErrors(script, executionData) {
        // Define expected methods for each script (multi-language)
        const expectedMethods = {
            // Python scripts
            '/cgi-bin/mini.py': 'GET',
            '/cgi-bin/env.py': 'GET',
            '/cgi-bin/form.py': 'POST',

            // Shell scripts
            '/cgi-bin/hello.sh': 'GET',

            // C++ executables
            '/cgi-bin/hello.cgi': 'GET'
        };

        const expectedMethod = expectedMethods[script];
        const actualMethod = executionData.method;

        if (expectedMethod && actualMethod !== expectedMethod) {
            return {
                error: `Script ${script} expects ${expectedMethod} method, but received ${actualMethod}`
            };
        }

        return null;
    },

    /**
     * Get simulated environment variables
     */
    getSimulatedEnvironment(data) {
        // Ensure all required fields exist and are valid
        const queryParams = data.queryParams || {};
        const queryString = new URLSearchParams(queryParams).toString();
        const postData = data.postData || null;
        const envVars = data.envVars || {};

        return {
            REQUEST_METHOD: data.method || 'GET',
            QUERY_STRING: queryString,
            CONTENT_TYPE: postData ? 'application/x-www-form-urlencoded' : '',
            CONTENT_LENGTH: postData && typeof postData === 'string' ? postData.length.toString() : '0',
            SCRIPT_NAME: data.script || '',
            PATH_INFO: '',
            SERVER_NAME: 'localhost',
            SERVER_PORT: '8080',
            HTTP_HOST: 'localhost:8080',
            HTTP_USER_AGENT: navigator.userAgent || 'Unknown',
            ...envVars
        };
    },

    /**
     * Handle method change
     */
    handleMethodChange(method) {
        const queryParamsGroup = document.getElementById('query-params-group');
        const postDataGroup = document.getElementById('post-data-group');

        if (method === 'POST') {
            postDataGroup.style.display = 'block';
        } else {
            postDataGroup.style.display = 'none';
        }
    },

    /**
     * Handle content type change
     */
    handleContentTypeChange(contentType) {
        const postDataTextarea = document.getElementById('post-data');
        const defaultData = CGI_CONFIG.CONTENT_TYPES[contentType]?.default || '';

        if (postDataTextarea && !postDataTextarea.value.trim()) {
            postDataTextarea.value = defaultData;
        }
    },

    /**
     * Validate script path
     */
    validateScriptPath(path) {
        const executeButton = document.getElementById('execute-script');
        const isValid = path.trim().length > 0 && path.startsWith('/');

        if (executeButton) {
            executeButton.disabled = !isValid;
        }
    },

    /**
     * Load quick script
     */
    loadQuickScript(script, method) {
        const scriptPath = document.getElementById('script-path');
        const methodSelect = document.getElementById('method-select');

        if (scriptPath) scriptPath.value = script;
        if (methodSelect) methodSelect.value = method;

        this.handleMethodChange(method);
        CGI_UTILS.showMessage(`Loaded ${script}`, 'success');
    },

    /**
     * Add parameter row
     */
    addParameter(containerId) {
        const container = document.getElementById(containerId);
        if (!container) return;

        const paramRow = document.createElement('div');
        paramRow.className = 'param-row';
        paramRow.innerHTML = `
            <input type="text" class="param-key" placeholder="Parameter name">
            <input type="text" class="param-value" placeholder="Parameter value">
            <button class="remove-param">🗑️</button>
        `;

        container.appendChild(paramRow);
    },

    /**
     * Remove parameter row
     */
    removeParameter(button) {
        const paramRow = button.closest('.param-row');
        if (paramRow) {
            paramRow.remove();
        }
    },

    /**
     * Get query parameters
     */
    getQueryParameters() {
        const params = {};
        const paramRows = document.querySelectorAll('#query-params .param-row');

        paramRows.forEach(row => {
            const key = row.querySelector('.param-key').value.trim();
            const value = row.querySelector('.param-value').value.trim();
            if (key) {
                params[key] = value;
            }
        });

        return params;
    },

    /**
     * Get POST data
     */
    getPostData() {
        const postDataTextarea = document.getElementById('post-data');
        const contentType = document.getElementById('content-type').value;

        return {
            content: postDataTextarea ? postDataTextarea.value : '',
            contentType: contentType
        };
    },

    /**
     * Get environment variables
     */
    getEnvironmentVariables() {
        const envVars = {};
        const paramRows = document.querySelectorAll('#env-vars .param-row');

        paramRows.forEach(row => {
            const key = row.querySelector('.param-key').value.trim();
            const value = row.querySelector('.param-value').value.trim();
            if (key) {
                envVars[key] = value;
            }
        });

        return envVars;
    },

    /**
     * Display execution results
     */
    displayResults(result) {
        // Update execution info
        const statusElement = document.getElementById('execution-status');
        const timeElement = document.getElementById('execution-time');
        const sizeElement = document.getElementById('output-size');

        if (statusElement) statusElement.textContent = result.status;
        if (timeElement) timeElement.textContent = `${result.executionTime}ms`;
        if (sizeElement) sizeElement.textContent = CGI_UTILS.formatBytes(result.outputSize);

        // Store result for view switching
        this.currentResult = result;
        this.switchView('view-output');
    },

    /**
     * Switch output view
     */
    switchView(viewId) {
        if (!this.currentResult) return;

        // Update active button
        document.querySelectorAll('.view-btn').forEach(btn => btn.classList.remove('active'));
        document.getElementById(viewId)?.classList.add('active');

        const outputContainer = document.getElementById('script-output');
        if (!outputContainer) return;

        let content = '';

        switch (viewId) {
            case 'view-output':
                content = this.formatOutput(this.currentResult.output);
                break;
            case 'view-headers':
                content = this.formatHeaders(this.currentResult.headers);
                break;
            case 'view-errors':
                content = this.formatErrors(this.currentResult.errors);
                break;
            case 'view-env':
                content = this.formatEnvironment(this.currentResult.environment);
                break;
        }

        outputContainer.innerHTML = content;
    },

    /**
     * Format output for display
     */
    formatOutput(output) {
        if (!output) return '<p class="no-data">No output</p>';
        return `<pre class="output-content">${CGI_UTILS.escapeHtml(output)}</pre>`;
    },

    /**
     * Format headers for display
     */
    formatHeaders(headers) {
        if (!headers || Object.keys(headers).length === 0) {
            return '<p class="no-data">No headers</p>';
        }

        const headersList = Object.entries(headers)
            .map(([key, value]) => `<div class="header-item"><strong>${key}:</strong> ${value}</div>`)
            .join('');

        return `<div class="headers-content">${headersList}</div>`;
    },

    /**
     * Format errors for display
     */
    formatErrors(errors) {
        if (!errors || errors.trim() === '') {
            return '<p class="no-data">No errors</p>';
        }
        return `<pre class="error-content">${CGI_UTILS.escapeHtml(errors)}</pre>`;
    },

    /**
     * Format environment for display
     */
    formatEnvironment(environment) {
        if (!environment || Object.keys(environment).length === 0) {
            return '<p class="no-data">No environment variables</p>';
        }

        const envList = Object.entries(environment)
            .map(([key, value]) => `<div class="env-item"><strong>${key}:</strong> ${value}</div>`)
            .join('');

        return `<div class="env-content">${envList}</div>`;
    },

    /**
     * Clear execution history
     */
    clearHistory() {
        if (confirm('Are you sure you want to clear all execution history?')) {
            CGI_HISTORY.clear();
            CGI_UTILS.showMessage('History cleared', 'success');
        }
    },

    /**
     * Export execution history
     */
    exportHistory() {
        try {
            CGI_HISTORY.export();
            CGI_UTILS.showMessage('History exported successfully', 'success');
        } catch (error) {
            CGI_UTILS.showMessage('Failed to export history', 'error');
        }
    },

    /**
     * Load history item
     */
    loadHistoryItem(historyItem) {
        const index = parseInt(historyItem.dataset.index);
        const execution = CGI_HISTORY.getExecution(index);

        if (execution) {
            // Load script configuration
            document.getElementById('script-path').value = execution.script;
            document.getElementById('method-select').value = execution.method;

            // Load results
            this.currentResult = execution.result;
            this.displayResults(execution.result);

            CGI_UTILS.showMessage('History item loaded', 'success');
        }
    }
};
