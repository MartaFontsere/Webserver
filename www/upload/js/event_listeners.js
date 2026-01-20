/**
 * ========================================================================
 * EVENT LISTENERS
 * ========================================================================
 * 
 * Setup all event listeners for the upload system
 */

/**
 * Initialize all event listeners
 */
function initializeEventListeners() {
    // Validate DOM elements first
    if (!validateDOMElements()) {
        console.error('Cannot initialize event listeners: missing DOM elements');
        return false;
    }

    // Drag & Drop Events
    DOM.dropZone.addEventListener('dragover', (e) => {
        e.preventDefault();
        DOM.dropZone.classList.add('drag-over');
    });

    DOM.dropZone.addEventListener('dragleave', () => {
        DOM.dropZone.classList.remove('drag-over');
    });

    DOM.dropZone.addEventListener('drop', (e) => {
        e.preventDefault();
        DOM.dropZone.classList.remove('drag-over');
        handleFiles(e.dataTransfer.files);
    });

    // File Input Change Event
    DOM.fileInput.addEventListener('change', (e) => {
        handleFiles(e.target.files);
    });

    // Clear Button Event
    DOM.clearButton.addEventListener('click', () => {
        clearAllFiles();
    });

    // Form Submit Event (Collective Upload)
    DOM.uploadForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        handleCollectiveUpload();
    });

    // Refresh Server Files Event
    DOM.refreshFilesButton.addEventListener('click', () => {
        loadServerFiles();
    });

    console.log('✅ Event listeners initialized successfully');
    return true;
}

/**
 * Initialize the upload system
 */
function initializeUploadSystem() {
    console.log('🚀 Initializing upload system...');

    // Initialize event listeners
    if (!initializeEventListeners()) {
        console.error('❌ Failed to initialize upload system');
        return;
    }

    // Load server files when page loads
    loadServerFiles();

    console.log('✅ Upload system initialized successfully');
    console.log(`📋 Configuration: ${CONFIG.SIMULATION_MODE ? 'SIMULATION' : 'PRODUCTION'} mode`);
}

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', initializeUploadSystem);

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        initializeEventListeners,
        initializeUploadSystem
    };
}
