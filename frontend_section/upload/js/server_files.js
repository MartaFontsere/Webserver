/**
 * ========================================================================
 * SERVER FILES MANAGEMENT
 * ========================================================================
 * 
 * Functions for managing files on the server (list, download, delete)
 */

/**
 * Load and display files from server
 */
async function loadServerFiles() {
    console.log(`[${CONFIG.SIMULATION_MODE ? 'SIMULATION' : 'PRODUCTION'}] Loading server files...`);

    // Show loading state
    DOM.loadingFiles.style.display = 'block';
    DOM.noFilesMessage.style.display = 'none';
    DOM.serverFilesGrid.style.display = 'none';

    if (CONFIG.SIMULATION_MODE) {
        simulateLoadServerFiles();
    } else {
        await productionLoadServerFiles();
    }
}

/**
 * Simulate loading server files
 */
function simulateLoadServerFiles() {
    setTimeout(() => {
        // Simulate server response with example files
        const serverFiles = [
            { name: 'document.pdf', size: 2048576, date: '2025-11-03', type: 'application/pdf' },
            { name: 'image.jpg', size: 1536000, date: '2025-11-02', type: 'image/jpeg' },
            { name: 'script.js', size: 8192, date: '2025-11-01', type: 'text/javascript' },
            { name: 'data.csv', size: 4096, date: '2025-10-31', type: 'text/csv' }
        ];

        displayServerFiles(serverFiles);
        console.log(`[SIMULATION] Loaded ${serverFiles.length} files from server`);
    }, 1000);
}

/**
 * Production load server files
 */
async function productionLoadServerFiles() {
    try {
        const response = await fetch(CONFIG.FILES_ENDPOINT);
        const result = await response.json();

        if (result.success) {
            displayServerFiles(result.files);
        } else {
            showMessage(`Error loading files: ${result.error}`, 'error');
            showNoFiles();
        }
    } catch (error) {
        showMessage(`Error loading files: ${error.message}`, 'error');
        showNoFiles();
    }
}

/**
 * Display server files in the grid
 * @param {Array} files - Array of file objects
 */
function displayServerFiles(files) {
    DOM.loadingFiles.style.display = 'none';

    if (files.length === 0) {
        showNoFiles();
        return;
    }

    DOM.noFilesMessage.style.display = 'none';
    DOM.serverFilesGrid.style.display = 'grid';
    DOM.serverFilesGrid.innerHTML = '';

    files.forEach(file => {
        const fileCard = document.createElement('div');
        fileCard.className = 'server-file-card';
        fileCard.innerHTML = `
            <div class="file-info">
                <div class="file-icon">${getFileIcon(file.type)}</div>
                <div class="file-details">
                    <div class="file-name" title="${file.name}">${file.name}</div>
                    <div class="file-meta">
                        <span class="file-size">${formatFileSize(file.size)}</span>
                        <span class="file-date">${formatDate(file.date)}</span>
                    </div>
                </div>
            </div>
            <div class="file-actions">
                <button type="button" class="download-btn" onclick="downloadFile('${file.name}')" title="Download file">
                    ⬇️ Download
                </button>
                <button type="button" class="delete-btn" onclick="deleteFile('${file.name}')" title="Delete file">
                    🗑️ Delete
                </button>
            </div>
        `;
        DOM.serverFilesGrid.appendChild(fileCard);
    });
}

/**
 * Show no files message
 */
function showNoFiles() {
    DOM.loadingFiles.style.display = 'none';
    DOM.serverFilesGrid.style.display = 'none';
    DOM.noFilesMessage.style.display = 'block';
}

/**
 * Download file from server
 * @param {string} filename - Name of file to download
 */
async function downloadFile(filename) {
    console.log(`[${CONFIG.SIMULATION_MODE ? 'SIMULATION' : 'PRODUCTION'}] Downloading file: ${filename}`);

    if (CONFIG.SIMULATION_MODE) {
        showMessage(`Downloading "${filename}"...`, 'success');
        return;
    }

    // Production download
    try {
        const response = await fetch(`${CONFIG.FILES_ENDPOINT}/${encodeURIComponent(filename)}`);

        if (response.ok) {
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.style.display = 'none';
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
            showMessage(`File "${filename}" downloaded successfully`, 'success');
        } else {
            showMessage(`Error downloading "${filename}"`, 'error');
        }
    } catch (error) {
        showMessage(`Error downloading "${filename}": ${error.message}`, 'error');
    }
}

/**
 * Delete file from server
 * @param {string} filename - Name of file to delete
 */
async function deleteFile(filename) {
    if (!confirm(`Are you sure you want to delete "${filename}"?`)) {
        return;
    }

    console.log(`[${CONFIG.SIMULATION_MODE ? 'SIMULATION' : 'PRODUCTION'}] Deleting file: ${filename}`);

    if (CONFIG.SIMULATION_MODE) {
        setTimeout(() => {
            const success = Math.random() > 0.1; // 90% success rate for demo

            if (success) {
                showMessage(`File "${filename}" deleted successfully`, 'success');
                setTimeout(() => {
                    loadServerFiles();
                }, 1000);
            } else {
                showMessage(`Error deleting "${filename}". Please try again.`, 'error');
            }
        }, 500);
        return;
    }

    // Production delete
    try {
        const response = await fetch(`${CONFIG.FILES_ENDPOINT}/${encodeURIComponent(filename)}`, {
            method: 'DELETE'
        });

        const result = await response.json();

        if (result.success) {
            showMessage(`File "${filename}" deleted successfully`, 'success');
            loadServerFiles();
        } else {
            showMessage(`Error deleting "${filename}": ${result.error}`, 'error');
        }
    } catch (error) {
        showMessage(`Error deleting "${filename}": ${error.message}`, 'error');
    }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        loadServerFiles,
        downloadFile,
        deleteFile
    };
}
