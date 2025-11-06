/**
 * ========================================================================
 * UTILITY FUNCTIONS
 * ========================================================================
 * 
 * Common utility functions used throughout the upload system
 */

/**
 * Format file size for display
 * @param {number} bytes - File size in bytes
 * @returns {string} Formatted file size
 */
function formatFileSize(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

/**
 * Format date for display
 * @param {string} dateString - Date string to format
 * @returns {string} Formatted date
 */
function formatDate(dateString) {
    const date = new Date(dateString);
    return date.toLocaleDateString('en-US', {
        year: 'numeric',
        month: 'short',
        day: 'numeric'
    });
}

/**
 * Get appropriate icon for file type
 * @param {string} mimeType - MIME type of the file
 * @returns {string} Emoji icon for the file type
 */
function getFileIcon(mimeType) {
    if (mimeType.startsWith('image/')) return '🖼️';
    if (mimeType.startsWith('video/')) return '🎥';
    if (mimeType.startsWith('audio/')) return '🎵';
    if (mimeType === 'application/pdf') return '📄';
    if (mimeType.includes('text/') || mimeType.includes('json')) return '📝';
    if (mimeType.includes('zip') || mimeType.includes('archive')) return '📦';
    return '📄'; // Default file icon
}

/**
 * Show message to user
 * @param {string} message - Message text
 * @param {string} type - Message type ('success' or 'error')
 */
function showMessage(message, type) {
    const resultMessage = DOM.resultMessage;
    resultMessage.className = `result-message ${type}`;
    resultMessage.innerHTML = `
        <div class="message-content">
            <span class="message-icon">${type === 'success' ? '✅' : '❌'}</span>
            <span class="message-text">${message}</span>
        </div>
    `;
    DOM.resultsSection.style.display = 'block';
}

/**
 * Hide result messages and progress
 */
function hideResults() {
    DOM.resultsSection.style.display = 'none';
    DOM.progressSection.style.display = 'none';
}

/**
 * Show progress bar
 * @param {number} percentage - Progress percentage (0-100)
 */
function updateProgress(percentage) {
    DOM.progressFill.style.width = percentage + '%';
    DOM.progressText.textContent = percentage + '%';
}

/**
 * Show progress section
 */
function showProgress() {
    DOM.progressSection.style.display = 'block';
}

/**
 * Hide progress section
 */
function hideProgress() {
    DOM.progressSection.style.display = 'none';
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        formatFileSize,
        formatDate,
        getFileIcon,
        showMessage,
        hideResults,
        updateProgress,
        showProgress,
        hideProgress
    };
}
