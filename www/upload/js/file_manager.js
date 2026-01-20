/**
 * ========================================================================
 * FILE MANAGEMENT
 * ========================================================================
 * 
 * Functions for handling file selection, validation, and UI updates
 */

/**
 * Handle new files (from drag&drop or file input)
 * Validates file size and adds to selectedFiles array
 * @param {FileList} files - Files to handle
 */
function handleFiles(files) {
    for (let file of files) {
        if (file.size > CONFIG.MAX_FILE_SIZE) {
            showMessage(`File "${file.name}" is too large (maximum 10MB)`, 'error');
            continue;
        }
        selectedFiles.push(file);
    }
    updateFileList();
    updateUploadButton();
}

/**
 * Update the visual file list display
 * Shows file name, size, and action buttons for each file
 */
function updateFileList() {
    if (selectedFiles.length === 0) {
        DOM.fileList.style.display = 'none';
        return;
    }

    DOM.fileList.style.display = 'block';
    DOM.filesContainer.innerHTML = '';

    selectedFiles.forEach((file, index) => {
        const fileItem = document.createElement('div');
        fileItem.className = 'file-item';
        fileItem.innerHTML = `
            <span class="file-name">📄 ${file.name}</span>
            <span class="file-size">${formatFileSize(file.size)}</span>
            <div class="file-actions">
                <button type="button" class="upload-individual" onclick="uploadFile(${index})" title="Upload this file">
                    📤 Upload
                </button>
                <button type="button" class="remove-file" onclick="removeFile(${index})" title="Remove this file">
                    ❌ Remove
                </button>
            </div>
        `;
        DOM.filesContainer.appendChild(fileItem);
    });
}

/**
 * Remove a specific file from the selected files array
 * @param {number} index - Index of file to remove
 */
function removeFile(index) {
    selectedFiles.splice(index, 1);
    updateFileList();
    updateUploadButton();
}

/**
 * Clear all selected files and reset form
 */
function clearAllFiles() {
    selectedFiles = [];
    DOM.fileInput.value = '';
    DOM.description.value = '';
    updateFileList();
    updateUploadButton();
    hideResults();
}

/**
 * Update the state of the upload button based on selected files
 */
function updateUploadButton() {
    DOM.uploadButton.disabled = selectedFiles.length === 0;
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        handleFiles,
        updateFileList,
        removeFile,
        clearAllFiles,
        updateUploadButton
    };
}
