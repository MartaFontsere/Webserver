/**
 * ================================================================
 * ⚙️ CGI TESTING - MAIN APPLICATION
 * Main initialization and coordination for CGI script testing
 * ================================================================
 */

// Main application object
const CGI_APP = {

    // Application state
    initialized: false,
    currentMode: 'simulation', // 'simulation' or 'production'

    /**
     * Initialize the CGI testing application
     */
    async init() {
        if (this.initialized) {
            console.warn('CGI App already initialized');
            return;
        }

        try {
            console.log('🚀 Initializing CGI Testing Application...');

            // Initialize core modules
            await this.initializeModules();

            // Setup UI
            this.setupUI();

            // Check backend connectivity
            await this.checkBackendConnectivity();

            // Load initial configuration
            this.loadInitialConfiguration();

            // Mark as initialized
            this.initialized = true;

            console.log('✅ CGI Testing Application initialized successfully');
            CGI_UTILS.showMessage('CGI Testing interface ready', 'success');

        } catch (error) {
            console.error('❌ Failed to initialize CGI App:', error);
            CGI_UTILS.showMessage('Failed to initialize application', 'error');
        }
    },

    /**
     * Initialize core modules
     */
    async initializeModules() {
        // Initialize utilities first
        if (typeof CGI_UTILS !== 'undefined') {
            console.log('✅ CGI Utils loaded');
        } else {
            throw new Error('CGI_UTILS not loaded');
        }

        // Initialize history management
        if (typeof CGI_HISTORY !== 'undefined') {
            CGI_HISTORY.init();
            console.log('✅ CGI History initialized');
        } else {
            throw new Error('CGI_HISTORY not loaded');
        }

        // Initialize event handlers
        if (typeof CGI_HANDLERS !== 'undefined') {
            CGI_HANDLERS.init();
            console.log('✅ CGI Handlers initialized');
        } else {
            throw new Error('CGI_HANDLERS not loaded');
        }

        // Initialize configuration
        if (typeof CGI_CONFIG !== 'undefined') {
            console.log('✅ CGI Config loaded');
        } else {
            throw new Error('CGI_CONFIG not loaded');
        }
    },

    /**
     * Setup initial UI state
     */
    setupUI() {
        // Set default script
        const scriptPath = document.getElementById('script-path');
        if (scriptPath && !scriptPath.value) {
            scriptPath.value = CGI_CONFIG.DEFAULT_SCRIPT;
        }

        // Initialize parameter sections
        this.initializeParameterSections();

        // Setup mode indicator
        this.updateModeIndicator();

        // Set initial view
        this.setInitialView();

        console.log('✅ UI setup completed');
    },

    /**
     * Initialize parameter sections
     */
    initializeParameterSections() {
        // Add initial query parameter row if none exist
        const queryParams = document.getElementById('query-params');
        if (queryParams && queryParams.children.length === 0) {
            CGI_HANDLERS.addParameter('query-params');
        }

        // Add initial environment variable row if none exist
        const envVars = document.getElementById('env-vars');
        if (envVars && envVars.children.length === 0) {
            CGI_HANDLERS.addParameter('env-vars');
        }

        // Set default POST data placeholder
        const contentType = document.getElementById('content-type');
        if (contentType) {
            CGI_HANDLERS.handleContentTypeChange(contentType.value);
        }
    },

    /**
     * Check backend connectivity
     */
    async checkBackendConnectivity() {
        if (!CGI_CONFIG.CGI_ENDPOINT) {
            this.currentMode = 'simulation';
            console.log('ℹ️ Backend endpoint not configured, using simulation mode');
            return;
        }

        try {
            // Try to ping the backend
            const response = await fetch(CGI_CONFIG.CGI_ENDPOINT + '/health', {
                method: 'GET',
                timeout: 5000
            });

            if (response.ok) {
                this.currentMode = 'production';
                console.log('✅ Backend connected, using production mode');
                CGI_UTILS.showMessage('Connected to backend server', 'success');
            } else {
                throw new Error('Backend not responding');
            }

        } catch (error) {
            this.currentMode = 'simulation';
            console.log('ℹ️ Backend not available, using simulation mode');
            CGI_UTILS.showMessage('Using simulation mode (backend not available)', 'info');
        }
    },

    /**
     * Load initial configuration
     */
    loadInitialConfiguration() {
        // Load quick scripts
        this.loadQuickScripts();

        // Load common environment variables
        this.loadCommonEnvVars();

        // Load saved preferences
        this.loadUserPreferences();
    },

    /**
     * Load quick scripts configuration
     */
    loadQuickScripts() {
        const quickButtons = document.querySelectorAll('.quick-btn');
        CGI_CONFIG.QUICK_SCRIPTS.forEach((script, index) => {
            const button = quickButtons[index];
            if (button) {
                button.title = script.description;
            }
        });
    },

    /**
     * Load common environment variables
     */
    loadCommonEnvVars() {
        // This could populate a dropdown or suggestions for env vars
        // For now, just log that common vars are available
        console.log('ℹ️ Common environment variables loaded:', CGI_CONFIG.COMMON_ENV_VARS);
    },

    /**
     * Load user preferences from localStorage
     */
    loadUserPreferences() {
        try {
            const preferences = localStorage.getItem('cgi_preferences');
            if (preferences) {
                const prefs = JSON.parse(preferences);
                this.applyPreferences(prefs);
                console.log('✅ User preferences loaded');
            }
        } catch (error) {
            console.warn('Failed to load user preferences:', error);
        }
    },

    /**
     * Apply user preferences
     */
    applyPreferences(preferences) {
        // Apply default script preference
        if (preferences.defaultScript) {
            const scriptPath = document.getElementById('script-path');
            if (scriptPath) {
                scriptPath.value = preferences.defaultScript;
            }
        }

        // Apply default method preference
        if (preferences.defaultMethod) {
            const methodSelect = document.getElementById('method-select');
            if (methodSelect) {
                methodSelect.value = preferences.defaultMethod;
                CGI_HANDLERS.handleMethodChange(preferences.defaultMethod);
            }
        }

        // Apply theme preference (if implemented)
        if (preferences.theme) {
            document.body.setAttribute('data-theme', preferences.theme);
        }
    },

    /**
     * Save user preferences
     */
    saveUserPreferences() {
        try {
            const preferences = {
                defaultScript: document.getElementById('script-path')?.value || CGI_CONFIG.DEFAULT_SCRIPT,
                defaultMethod: document.getElementById('method-select')?.value || 'GET',
                theme: document.body.getAttribute('data-theme') || 'default',
                lastSaved: new Date().toISOString()
            };

            localStorage.setItem('cgi_preferences', JSON.stringify(preferences));
            console.log('✅ User preferences saved');
        } catch (error) {
            console.warn('Failed to save user preferences:', error);
        }
    },

    /**
     * Update mode indicator
     */
    updateModeIndicator() {
        // Update production mode in config
        CGI_CONFIG.PRODUCTION_MODE = (this.currentMode === 'production');

        // Add mode indicator to UI if needed
        const modeIndicator = document.createElement('div');
        modeIndicator.className = `mode-indicator mode-${this.currentMode}`;
        modeIndicator.textContent = this.currentMode === 'production' ? '🌐 Production' : '🧪 Simulation';
        modeIndicator.title = this.currentMode === 'production'
            ? 'Connected to real backend server'
            : 'Using simulated responses for testing';

        // Insert into header
        const header = document.querySelector('header');
        if (header && !document.querySelector('.mode-indicator')) {
            header.appendChild(modeIndicator);
        }
    },

    /**
     * Set initial view
     */
    setInitialView() {
        // Ensure output view is set to default
        const defaultView = document.getElementById('view-output');
        if (defaultView) {
            defaultView.classList.add('active');
        }

        // Clear any existing output
        const outputContainer = document.getElementById('script-output');
        if (outputContainer) {
            outputContainer.innerHTML = '<p class="no-data">No script executed yet</p>';
        }
    },

    /**
     * Handle application errors
     */
    handleError(error, context = 'Application') {
        console.error(`[${context}] Error:`, error);

        const errorMessage = error.message || 'An unexpected error occurred';
        CGI_UTILS.showMessage(`${context}: ${errorMessage}`, 'error');

        // Could also send error reports to backend in production
        if (this.currentMode === 'production' && CGI_CONFIG.ERROR_REPORTING) {
            this.reportError(error, context);
        }
    },

    /**
     * Report error to backend (if in production mode)
     */
    async reportError(error, context) {
        try {
            await fetch(CGI_CONFIG.CGI_ENDPOINT + '/error-report', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    error: error.message,
                    stack: error.stack,
                    context: context,
                    timestamp: new Date().toISOString(),
                    userAgent: navigator.userAgent,
                    url: window.location.href
                })
            });
        } catch (reportError) {
            console.warn('Failed to report error to backend:', reportError);
        }
    },

    /**
     * Get application status
     */
    getStatus() {
        return {
            initialized: this.initialized,
            mode: this.currentMode,
            modulesLoaded: {
                config: typeof CGI_CONFIG !== 'undefined',
                utils: typeof CGI_UTILS !== 'undefined',
                handlers: typeof CGI_HANDLERS !== 'undefined',
                history: typeof CGI_HISTORY !== 'undefined'
            },
            historyCount: CGI_HISTORY?.executions?.length || 0,
            lastExecution: CGI_HISTORY?.executions?.[0]?.timestamp || null
        };
    },

    /**
     * Cleanup application
     */
    cleanup() {
        // Save user preferences
        this.saveUserPreferences();

        // Clear any running timers or intervals
        // (None in current implementation, but good practice)

        console.log('✅ CGI App cleanup completed');
    }
};

// Global error handler
window.addEventListener('error', (event) => {
    CGI_APP.handleError(event.error, 'Window');
});

// Unhandled promise rejection handler
window.addEventListener('unhandledrejection', (event) => {
    CGI_APP.handleError(new Error(event.reason), 'Promise');
});

// Initialize application when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    CGI_APP.init();
});

// Save preferences before page unload
window.addEventListener('beforeunload', () => {
    CGI_APP.cleanup();
});

// Export for debugging in console
window.CGI_APP = CGI_APP;
