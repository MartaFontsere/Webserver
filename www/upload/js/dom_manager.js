/**
 * ========================================================================
 * DOM ELEMENTS MANAGER
 * ========================================================================
 * 
 * Manages all DOM element references for the upload system
 */

const DOM = {
    // Upload form elements
    dropZone: document.getElementById('dropZone'),
    fileInput: document.getElementById('fileInput'),
    fileList: document.getElementById('fileList'),
    filesContainer: document.getElementById('filesContainer'),
    uploadButton: document.getElementById('uploadButton'),
    clearButton: document.getElementById('clearButton'),
    uploadForm: document.getElementById('uploadForm'),

    // Progress and results
    progressSection: document.getElementById('progressSection'),
    progressFill: document.getElementById('progressFill'),
    progressText: document.getElementById('progressText'),
    resultsSection: document.getElementById('resultsSection'),
    resultMessage: document.getElementById('resultMessage'),

    // Server files management
    refreshFilesButton: document.getElementById('refreshFilesButton'),
    serverFilesList: document.getElementById('serverFilesList'),
    loadingFiles: document.getElementById('loadingFiles'),
    noFilesMessage: document.getElementById('noFilesMessage'),
    serverFilesGrid: document.getElementById('serverFilesGrid'),

    // Form fields
    description: document.getElementById('description')
};

/**
 * Validate that all required DOM elements are present
 */
function validateDOMElements() {
    const missing = [];
    for (const [key, element] of Object.entries(DOM)) {
        if (!element) {
            missing.push(key);
        }
    }

    if (missing.length > 0) {
        console.error('Missing DOM elements:', missing);
        return false;
    }

    return true;
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { DOM, validateDOMElements };
}
