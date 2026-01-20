/**
 * ========================================================================
 * UPLOAD HANDLERS
 * ========================================================================
 * 
 * Functions for handling individual and collective file uploads
 */

/**
 * Upload individual file with progress tracking
 * @param {number} index - Index of file in selectedFiles array
 */
async function uploadFile(index) {
    const file = selectedFiles[index];
    if (!file) return;

    // Disable button during upload
    const button = document.querySelector(`button[onclick="uploadFile(${index})"]`);
    const originalText = button.innerHTML;
    button.disabled = true;
    button.innerHTML = '⏳ Uploading...';

    console.log(`[${CONFIG.SIMULATION_MODE ? 'SIMULATION' : 'PRODUCTION'}] Uploading file: ${file.name}`);
    showProgress();

    if (CONFIG.SIMULATION_MODE) {
        // Simulation mode
        simulateIndividualUpload(file, index, button, originalText);
    } else {
        // Production mode
        await productionIndividualUpload(file, index, button, originalText);
    }
}

/**
 * Simulate individual file upload
 * @param {File} file - File to upload
 * @param {number} index - File index
 * @param {HTMLElement} button - Upload button element
 * @param {string} originalText - Original button text
 */
function simulateIndividualUpload(file, index, button, originalText) {
    let progress = 0;
    const progressInterval = setInterval(() => {
        progress += 20;
        updateProgress(progress);

        if (progress >= 100) {
            clearInterval(progressInterval);

            setTimeout(() => {
                const success = Math.random() > (1 - CONFIG.SUCCESS_RATE);
                console.log(`[SIMULATION] Upload result for ${file.name}: ${success ? 'SUCCESS' : 'ERROR'}`);

                if (success) {
                    showMessage(`File "${file.name}" uploaded successfully`, 'success');
                    removeFile(index);
                    DOM.description.value = '';
                    // Refresh server files list
                    setTimeout(() => loadServerFiles(), 1000);
                } else {
                    showMessage(`Error uploading "${file.name}". Please try again.`, 'error');
                    button.disabled = false;
                    button.innerHTML = originalText;
                }

                hideProgress();
            }, 500);
        }
    }, 150);
}

/**
 * Production individual file upload
 * @param {File} file - File to upload
 * @param {number} index - File index
 * @param {HTMLElement} button - Upload button element
 * @param {string} originalText - Original button text
 */
async function productionIndividualUpload(file, index, button, originalText) {
    try {
        const formData = new FormData();
        formData.append('file', file);

        const description = DOM.description.value;
        if (description) {
            formData.append('description', description);
        }

        const response = await fetch(CONFIG.UPLOAD_ENDPOINT, {
            method: 'POST',
            body: formData
        });

        const result = await response.json();

        if (result.success) {
            showMessage(`File "${file.name}" uploaded successfully`, 'success');
            removeFile(index);
            DOM.description.value = '';
            loadServerFiles();
        } else {
            showMessage(`Error uploading "${file.name}": ${result.error}`, 'error');
            button.disabled = false;
            button.innerHTML = originalText;
        }
    } catch (error) {
        showMessage(`Error uploading "${file.name}": ${error.message}`, 'error');
        button.disabled = false;
        button.innerHTML = originalText;
    } finally {
        hideProgress();
    }
}

/**
 * Handle collective upload of all selected files
 */
async function handleCollectiveUpload() {
    if (selectedFiles.length === 0) {
        showMessage('Please select at least one file', 'error');
        return;
    }

    console.log(`[${CONFIG.SIMULATION_MODE ? 'SIMULATION' : 'PRODUCTION'}] Starting collective upload of ${selectedFiles.length} files`);
    showProgress();

    if (CONFIG.SIMULATION_MODE) {
        simulateCollectiveUpload();
    } else {
        await productionCollectiveUpload();
    }
}

/**
 * Simulate collective upload
 */
function simulateCollectiveUpload() {
    let progress = 0;
    const progressInterval = setInterval(() => {
        progress += 10;
        updateProgress(progress);

        if (progress >= 100) {
            clearInterval(progressInterval);
            simulateUpload();
        }
    }, 200);
}

/**
 * Simulate upload completion
 */
function simulateUpload() {
    setTimeout(() => {
        const randomValue = Math.random();
        const success = randomValue > (1 - CONFIG.SUCCESS_RATE);
        console.log(`[SIMULATION] Collective upload result: ${success ? 'SUCCESS' : 'ERROR'}`);

        if (success) {
            showMessage(`${selectedFiles.length} file(s) uploaded successfully`, 'success');
            DOM.description.value = '';
            setTimeout(() => loadServerFiles(), 1000);
            setTimeout(() => {
                DOM.clearButton.click();
            }, 2000);
        } else {
            showMessage(`Error uploading files. Please try again.`, 'error');
        }

        hideProgress();
    }, CONFIG.UPLOAD_DELAY);
}

/**
 * Production collective upload
 */
async function productionCollectiveUpload() {
    try {
        const formData = new FormData();
        selectedFiles.forEach(file => {
            formData.append('files', file);
        });

        const description = DOM.description.value;
        if (description) {
            formData.append('description', description);
        }

        const response = await fetch(CONFIG.UPLOAD_ENDPOINT, {
            method: 'POST',
            body: formData
        });

        const result = await response.json();

        if (result.success) {
            showMessage(`${selectedFiles.length} file(s) uploaded successfully`, 'success');
            DOM.description.value = '';
            loadServerFiles();
            setTimeout(() => {
                DOM.clearButton.click();
            }, 2000);
        } else {
            showMessage(`Error uploading files: ${result.error}`, 'error');
        }
    } catch (error) {
        showMessage(`Error uploading files: ${error.message}`, 'error');
    } finally {
        hideProgress();
    }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        uploadFile,
        handleCollectiveUpload
    };
}
