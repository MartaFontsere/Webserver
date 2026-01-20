/**
 * ================================================================
 * 📥 GET TESTING - MAIN APPLICATION
 * Main initialization and event handling for GET testing interface
 * ================================================================
 */

// Override console.log to show in page
const originalLog = console.log;
console.log = function (...args) {
    originalLog.apply(console, args);
    // Show in page title for debugging
    document.title = args.join(' ').substring(0, 50);
};

/**
 * Initialize the application when DOM is loaded
 */
document.addEventListener('DOMContentLoaded', function () {
    console.log('🚀 GET Testing Interface loaded');

    // Initialize components
    initializeEventListeners();
    initializeQuickTests();
    setupResponseViewControls();

    // Load initial history
    getHistory.renderHistory();

    // Show welcome message
    showWelcomeMessage();
});

/**
 * Initialize all event listeners
 */
function initializeEventListeners() {
    // Send request button
    const sendButton = document.getElementById('send-request');
    if (sendButton) {
        sendButton.addEventListener('click', sendGetRequest);
    }

    // URL input - send on Enter
    const urlInput = document.getElementById('request-url');
    if (urlInput) {
        urlInput.addEventListener('keypress', function (e) {
            if (e.key === 'Enter') {
                sendGetRequest();
            }
        });
    }

    // History controls
    const clearHistoryBtn = document.getElementById('clear-history');
    if (clearHistoryBtn) {
        clearHistoryBtn.addEventListener('click', () => {
            if (confirm('Are you sure you want to clear all history?')) {
                getHistory.clearHistory();
            }
        });
    }

    const exportHistoryBtn = document.getElementById('export-history');
    if (exportHistoryBtn) {
        exportHistoryBtn.addEventListener('click', () => {
            getHistory.exportHistory();
        });
    }
}

/**
 * Initialize quick test buttons
 */
function initializeQuickTests() {
    const quickButtons = document.querySelectorAll('.quick-btn');

    quickButtons.forEach(button => {
        button.addEventListener('click', function () {
            const url = this.dataset.url;
            if (url) {
                // Set URL in input
                const urlInput = document.getElementById('request-url');
                if (urlInput) {
                    urlInput.value = url;
                }

                // Send request
                sendGetRequest();
            }
        });
    });
}

/**
 * Setup response view controls
 */
function setupResponseViewControls() {
    const viewButtons = document.querySelectorAll('.view-btn');
    console.log('Found view buttons:', viewButtons.length);

    viewButtons.forEach(button => {
        console.log('Setting up button:', button.id);
        button.addEventListener('click', function () {
            console.log('Button clicked:', this.id);

            // Remove active class from all buttons
            viewButtons.forEach(btn => btn.classList.remove('active'));

            // Add active class to clicked button
            this.classList.add('active');

            // Update view mode
            currentViewMode = this.id.replace('view-', '');
            console.log('New view mode:', currentViewMode);

            // Re-render response body if we have data
            const responseBody = document.getElementById('response-body');
            if (responseBody && responseBody.dataset.currentData) {
                console.log('Re-rendering with data:', responseBody.dataset.currentData.substring(0, 50));
                updateResponseBodyView(responseBody.dataset.currentData, responseBody.dataset.contentType);
            } else {
                console.log('No data to re-render');
            }
        });
    });
}

/**
 * Send GET request
 */
async function sendGetRequest() {
    const urlInput = document.getElementById('request-url');
    const url = urlInput ? GetUtils.normalizeUrl(urlInput.value) : '/';

    // Validate URL
    if (!GetUtils.isValidUrl(url)) {
        GetUtils.showNotification('Please enter a valid URL', 'error');
        return;
    }

    console.log(`📤 Sending GET request to: ${url}`);

    try {
        // Send the request
        const response = await getRequestHandler.sendRequest(url);

        // Update UI with response
        updateResponseDisplay(response);

        // Add to history
        getHistory.addRequest(response);

        console.log('✅ Request completed:', response);

    } catch (error) {
        console.error('❌ Request failed:', error);
        GetUtils.showNotification('Request failed: ' + error.message, 'error');
    }
}

/**
 * Update response display in UI
 * @param {Object} response - Response data
 */
function updateResponseDisplay(response) {
    // Update status info
    updateStatusInfo(response);

    // Update headers
    updateHeadersDisplay(response.headers);

    // Update body
    const contentType = response.headers['Content-Type'] || response.headers['content-type'] || 'text/plain';
    updateResponseBodyView(response.body, contentType);
}

/**
 * Update status information display
 * @param {Object} response - Response data
 */
function updateStatusInfo(response) {
    // Status code
    const statusElement = document.getElementById('response-status');
    if (statusElement) {
        const statusInfo = GetUtils.getStatusInfo(response.status);
        statusElement.textContent = `${response.status} ${response.statusText}`;
        statusElement.className = `status-code ${statusInfo.class}`;
    }

    // Response time
    const timeElement = document.getElementById('response-time');
    if (timeElement) {
        timeElement.textContent = GetUtils.formatDuration(response.duration);
    }

    // Response size
    const sizeElement = document.getElementById('response-size');
    if (sizeElement) {
        sizeElement.textContent = GetUtils.formatBytes(response.size);
    }
}

/**
 * Update headers display
 * @param {Object} headers - Response headers
 */
function updateHeadersDisplay(headers) {
    const headersContainer = document.getElementById('response-headers');
    if (!headersContainer) return;

    if (!headers || Object.keys(headers).length === 0) {
        headersContainer.innerHTML = '<p class="no-data">No headers received</p>';
        return;
    }

    // Sort headers with important ones first
    const sortedHeaders = Object.entries(headers).sort(([a], [b]) => {
        const aImportant = GET_CONFIG.IMPORTANT_HEADERS.includes(a);
        const bImportant = GET_CONFIG.IMPORTANT_HEADERS.includes(b);

        if (aImportant && !bImportant) return -1;
        if (!aImportant && bImportant) return 1;
        return a.localeCompare(b);
    });

    headersContainer.innerHTML = sortedHeaders.map(([name, value]) => `
        <div class="header-item">
            <span class="header-name">${GetUtils.escapeHtml(name)}:</span>
            <span class="header-value">${GetUtils.escapeHtml(value)}</span>
        </div>
    `).join('');
}

/**
 * Update response body view based on current mode
 * @param {string} body - Response body
 * @param {string} contentType - Content type
 */
function updateResponseBodyView(body, contentType) {
    const bodyContainer = document.getElementById('response-body');
    if (!bodyContainer) return;

    // Store data for view switching
    bodyContainer.dataset.currentData = body;
    bodyContainer.dataset.contentType = contentType;

    if (!body) {
        bodyContainer.innerHTML = '<p class="no-data">No response body</p>';
        return;
    }

    const contentInfo = GetUtils.getContentTypeInfo(contentType);
    let displayContent = body;

    // Process based on view mode
    switch (currentViewMode) {
        case 'raw':
            displayContent = GetUtils.escapeHtml(body);
            bodyContainer.classList.remove('preview-mode');
            // Debug: Add visual indicator
            displayContent = `<!-- RAW MODE -->\n${displayContent}`;
            break;

        case 'formatted':
            if (contentType.includes('json')) {
                const formattedJson = GetUtils.tryFormatJson(body);
                displayContent = GetUtils.escapeHtml(formattedJson);
                // Debug: Add visual indicator
                displayContent = `<!-- FORMATTED JSON MODE -->\n${displayContent}`;
            } else if (contentType.includes('xml')) {
                displayContent = GetUtils.escapeHtml(GetUtils.tryFormatXml(body));
                displayContent = `<!-- FORMATTED XML MODE -->\n${displayContent}`;
            } else {
                displayContent = GetUtils.escapeHtml(body);
                displayContent = `<!-- FORMATTED TEXT MODE -->\n${displayContent}`;
            }
            bodyContainer.classList.remove('preview-mode');
            break;

        case 'preview':
            if (contentType.includes('html')) {
                // For HTML, show rendered preview (be careful with security)
                displayContent = `<iframe srcdoc="${GetUtils.escapeHtml(body)}" style="width: 100%; height: 300px; border: 1px solid #ddd; border-radius: 4px;"></iframe>`;
                bodyContainer.classList.add('preview-mode');
            } else if (contentType.includes('json')) {
                // For JSON, show formatted version
                displayContent = GetUtils.escapeHtml(GetUtils.tryFormatJson(body));
                bodyContainer.classList.remove('preview-mode');
            } else {
                // For other types, show raw
                displayContent = GetUtils.escapeHtml(body);
                bodyContainer.classList.remove('preview-mode');
            }
            break;
    }

    bodyContainer.innerHTML = displayContent;
}

/**
 * Show welcome message on first load
 */
function showWelcomeMessage() {
    const isFirstVisit = !localStorage.getItem('get_test_visited');

    if (isFirstVisit) {
        localStorage.setItem('get_test_visited', 'true');

        setTimeout(() => {
            GetUtils.showNotification(
                '👋 Welcome! Try the quick test buttons or enter a custom URL',
                'info',
                5000
            );
        }, 1000);
    }
}

/**
 * Handle keyboard shortcuts
 */
document.addEventListener('keydown', function (e) {
    // Ctrl/Cmd + Enter to send request
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
        e.preventDefault();
        sendGetRequest();
    }

    // Escape to clear indicators
    if (e.key === 'Escape') {
        getRequestHandler.hideIndicators();
    }
});

// Make functions available globally for history replay
window.sendGetRequest = sendGetRequest;
window.updateResponseDisplay = updateResponseDisplay;

// Log initialization
console.log('📥 GET Testing Interface initialized');
console.log('🔧 Available endpoints:', GET_CONFIG.ENDPOINTS);
console.log('⚡ Quick tests:', GET_CONFIG.QUICK_TESTS.length + ' tests available');
