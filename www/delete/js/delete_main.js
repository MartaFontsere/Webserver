/**
 * DELETE Module - Main Entry Point
 * 
 * Initializes the DELETE testing interface, binds events, and coordinates
 * all DELETE-related functionality including resource discovery, bulk operations,
 * confirmation dialogs, response analysis, and history management.
 * 
 * Author: 42 Webserver Frontend Team
 * Created: November 2025
 */

/**
 * DELETE Main Application Class
 * Manages the entire DELETE testing interface lifecycle
 */
class DeleteApp {
    constructor() {
        this.initialized = false;
        this.currentRequest = null;
        this.bulkOperations = [];
    }

    /**
     * Initialize the DELETE application
     * Sets up event listeners and loads saved state
     */
    init() {
        if (this.initialized) return;

        console.log('[DELETE] Starting initialization...');

        try {
            console.log('[DELETE] Checking dependencies...');
            if (typeof DELETE_CONFIG === 'undefined') {
                throw new Error('DELETE_CONFIG is not defined');
            }
            if (typeof DeleteUtils === 'undefined') {
                throw new Error('DeleteUtils is not defined');
            }
            if (typeof DeleteHandlers === 'undefined') {
                throw new Error('DeleteHandlers is not defined');
            }
            if (typeof DeleteHistory === 'undefined') {
                throw new Error('DeleteHistory is not defined');
            }

            console.log('[DELETE] Dependencies OK, binding events...');
            this.bindEvents();

            console.log('[DELETE] Loading saved state...');
            this.loadSavedState();

            console.log('[DELETE] Setting up dynamic elements...');
            this.setupDynamicElements();

            this.initialized = true;

            DeleteUtils.showNotification('DELETE module initialized successfully', 'success');
            console.log('[DELETE] Application initialized successfully');
        } catch (error) {
            console.error('[DELETE] Initialization failed:', error);
            DeleteUtils.showNotification('Failed to initialize DELETE module: ' + error.message, 'error');
        }
    }

    /**
     * Bind all event listeners
     */
    bindEvents() {
        console.log('[DELETE] Binding events...');

        try {
            // Resource discovery events
            this.bindResourceDiscoveryEvents();

            // Bulk operations events
            this.bindBulkOperationEvents();

            // Quick tests events
            this.bindQuickTestEvents();

            // Form submission events
            this.bindFormEvents();

            // History events
            this.bindHistoryEvents();

            // UI interaction events
            this.bindUIEvents();

            // Keyboard shortcuts
            this.bindKeyboardShortcuts();

            console.log('[DELETE] All events bound successfully');
        } catch (error) {
            console.error('[DELETE] Error binding events:', error);
            throw error;
        }
    }    /**
     * Bind resource discovery events
     */
    bindResourceDiscoveryEvents() {
        console.log('[DELETE] Binding resource discovery events...');

        const discoverBtn = document.getElementById('discover-resources');
        const resourceTypeSelect = document.getElementById('resource-type');
        const customEndpointInput = document.getElementById('custom-endpoint');

        if (discoverBtn) {
            discoverBtn.addEventListener('click', async () => {
                try {
                    let path = '/';

                    if (resourceTypeSelect) {
                        const selectedType = resourceTypeSelect.value;
                        if (selectedType === 'custom' && customEndpointInput) {
                            path = customEndpointInput.value || '/';
                        } else if (DELETE_CONFIG.DISCOVERY_ENDPOINTS[selectedType]) {
                            path = DELETE_CONFIG.DISCOVERY_ENDPOINTS[selectedType];
                        }
                    }

                    if (DeleteHandlers && typeof DeleteHandlers.discoverResources === 'function') {
                        await DeleteHandlers.discoverResources(path);
                    } else {
                        console.warn('[DELETE] DeleteHandlers.discoverResources not available');
                    }
                } catch (error) {
                    console.error('[DELETE] Resource discovery failed:', error);
                }
            });
        } else {
            console.warn('[DELETE] discover-resources button not found');
        }

        // Handle resource type selection change
        if (resourceTypeSelect) {
            resourceTypeSelect.addEventListener('change', (e) => {
                if (customEndpointInput) {
                    if (e.target.value === 'custom') {
                        customEndpointInput.classList.remove('hidden');
                    } else {
                        customEndpointInput.classList.add('hidden');
                    }
                }
            });
        } else {
            console.warn('[DELETE] resource-type select not found');
        }

        // Handle custom endpoint enter key
        if (customEndpointInput) {
            customEndpointInput.addEventListener('keypress', async (e) => {
                if (e.key === 'Enter') {
                    try {
                        const path = e.target.value || '/';
                        if (DeleteHandlers && typeof DeleteHandlers.discoverResources === 'function') {
                            await DeleteHandlers.discoverResources(path);
                        } else {
                            console.warn('[DELETE] DeleteHandlers.discoverResources not available');
                        }
                    } catch (error) {
                        console.error('[DELETE] Resource discovery failed:', error);
                    }
                }
            });
        } else {
            console.warn('[DELETE] custom-endpoint input not found');
        }

        // Resource selection events
        document.addEventListener('change', (e) => {
            if (e.target.classList.contains('resource-checkbox')) {
                this.updateBulkOperationState();
            }
        });
    }

    /**
     * Bind bulk operation events
     */
    bindBulkOperationEvents() {
        const selectAllBtn = document.getElementById('select-all-resources');
        const clearSelectionBtn = document.getElementById('clear-selection');
        const bulkDeleteBtn = document.getElementById('bulk-delete');

        if (selectAllBtn) {
            selectAllBtn.addEventListener('click', () => {
                DeleteHandlers.selectAllResources();
                this.updateBulkOperationState();
            });
        }

        if (clearSelectionBtn) {
            clearSelectionBtn.addEventListener('click', () => {
                DeleteHandlers.clearSelection();
                this.updateBulkOperationState();
            });
        }

        if (bulkDeleteBtn) {
            bulkDeleteBtn.addEventListener('click', async () => {
                await DeleteHandlers.executeBulkDelete();
            });
        }
    }    /**
     * Bind quick test events
     */
    bindQuickTestEvents() {
        console.log('[DELETE] Binding quick test events...');

        const quickTestBtns = document.querySelectorAll('.quick-button');

        console.log('[DELETE] Found quick test buttons:', quickTestBtns.length);

        quickTestBtns.forEach(btn => {
            console.log('[DELETE] Binding event to button:', btn.dataset.test);
            btn.addEventListener('click', async (e) => {
                const testType = e.target.dataset.test;
                console.log('[DELETE] Quick test button clicked:', testType);

                if (!testType) {
                    console.error('[DELETE] No test type found in button dataset');
                    if (DeleteUtils && typeof DeleteUtils.showNotification === 'function') {
                        DeleteUtils.showNotification('Invalid test configuration', 'error');
                    }
                    return;
                }

                try {
                    if (DeleteHandlers && typeof DeleteHandlers.runQuickTest === 'function') {
                        await DeleteHandlers.runQuickTest(testType);
                    } else {
                        console.error('[DELETE] DeleteHandlers.runQuickTest not available');
                        if (DeleteUtils && typeof DeleteUtils.showNotification === 'function') {
                            DeleteUtils.showNotification('Quick test function not available', 'error');
                        }
                    }
                } catch (error) {
                    console.error('[DELETE] Quick test failed:', error);
                    if (DeleteUtils && typeof DeleteUtils.showNotification === 'function') {
                        DeleteUtils.showNotification(`Quick test failed: ${error.message}`, 'error');
                    }
                }
            });
        });
    }

    /**
     * Bind form submission events
     */
    bindFormEvents() {
        const deleteForm = document.getElementById('delete-form');
        const confirmBtn = document.getElementById('confirm-delete');
        const cancelBtn = document.getElementById('cancel-delete');

        if (deleteForm) {
            deleteForm.addEventListener('submit', async (e) => {
                e.preventDefault();
                await this.handleDeleteSubmission();
            });
        }

        if (confirmBtn) {
            confirmBtn.addEventListener('click', async () => {
                await this.handleDeleteSubmission();
            });
        }

        if (cancelBtn) {
            cancelBtn.addEventListener('click', () => {
                DeleteHandlers.hideConfirmationDialog();
            });
        }

        // Custom headers management
        const addHeaderBtn = document.getElementById('add-header-row');
        if (addHeaderBtn) {
            addHeaderBtn.addEventListener('click', () => {
                console.log('[DELETE] Add header button clicked');
                if (DeleteHandlers && typeof DeleteHandlers.addCustomHeader === 'function') {
                    DeleteHandlers.addCustomHeader();
                } else {
                    console.warn('[DELETE] DeleteHandlers.addCustomHeader not available');
                }
            });
        } else {
            console.warn('[DELETE] add-header-row button not found');
        }

        document.addEventListener('click', (e) => {
            if (e.target.classList.contains('remove-header')) {
                console.log('[DELETE] Remove header button clicked');
                if (DeleteHandlers && typeof DeleteHandlers.removeCustomHeader === 'function') {
                    DeleteHandlers.removeCustomHeader(e.target);
                } else {
                    console.warn('[DELETE] DeleteHandlers.removeCustomHeader not available');
                }
            }
        });
    }

    /**
     * Bind history events
     */
    bindHistoryEvents() {
        console.log('[DELETE] Binding history events...');

        const clearHistoryBtn = document.getElementById('clear-history');
        const exportHistoryBtn = document.getElementById('export-history');

        if (clearHistoryBtn) {
            console.log('[DELETE] Clear history button found, binding event');
            clearHistoryBtn.addEventListener('click', async () => {
                console.log('[DELETE] Clear history button clicked');
                try {
                    const confirmed = await DeleteUtils.confirmAction('Clear all history?', 'This action cannot be undone.');
                    console.log('[DELETE] User confirmation result:', confirmed);
                    if (confirmed) {
                        if (DeleteHistory && typeof DeleteHistory.clearHistory === 'function') {
                            DeleteHistory.clearHistory();
                        } else {
                            console.error('[DELETE] DeleteHistory.clearHistory not available');
                        }
                    }
                } catch (error) {
                    console.error('[DELETE] Error in clear history:', error);
                }
            });
        } else {
            console.warn('[DELETE] clear-history button not found');
        }

        if (exportHistoryBtn) {
            exportHistoryBtn.addEventListener('click', () => {
                console.log('[DELETE] Export history button clicked');
                if (DeleteHistory && typeof DeleteHistory.exportHistory === 'function') {
                    DeleteHistory.exportHistory();
                } else {
                    console.warn('[DELETE] DeleteHistory.exportHistory not available');
                }
            });
        } else {
            console.warn('[DELETE] export-history button not found');
        }

        // History item interactions
        document.addEventListener('click', (e) => {
            if (e.target.classList.contains('replay-request')) {
                const requestId = e.target.dataset.requestId;
                DeleteHistory.replayRequest(requestId);
            }

            if (e.target.classList.contains('delete-history-item')) {
                const requestId = e.target.dataset.requestId;
                DeleteHistory.deleteHistoryItem(requestId);
            }
        });
    }

    /**
     * Bind UI interaction events
     */
    bindUIEvents() {
        // Section toggles
        const sectionHeaders = document.querySelectorAll('.section-header');
        sectionHeaders.forEach(header => {
            header.addEventListener('click', () => {
                const section = header.closest('.section');
                section.classList.toggle('collapsed');
            });
        });

        // Response format toggle
        const formatBtns = document.querySelectorAll('.format-btn');
        formatBtns.forEach(btn => {
            btn.addEventListener('click', (e) => {
                const format = e.target.dataset.format;
                DeleteHandlers.changeResponseFormat(format);
            });
        });

        // Copy buttons
        document.addEventListener('click', (e) => {
            if (e.target.classList.contains('copy-btn')) {
                const target = e.target.dataset.copyTarget;
                DeleteUtils.copyToClipboard(target);
            }
        });
    }

    /**
     * Bind keyboard shortcuts
     */
    bindKeyboardShortcuts() {
        document.addEventListener('keydown', (e) => {
            // Ctrl/Cmd + Enter: Submit DELETE request
            if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
                e.preventDefault();
                this.handleDeleteSubmission();
            }

            // Ctrl/Cmd + A: Select all resources (when in resource list)
            if ((e.ctrlKey || e.metaKey) && e.key === 'a' &&
                document.activeElement.closest('#resource-list')) {
                e.preventDefault();
                DeleteHandlers.selectAllResources();
                this.updateBulkOperationState();
            }

            // Escape: Close dialogs
            if (e.key === 'Escape') {
                DeleteHandlers.hideConfirmationDialog();
            }

            // Ctrl/Cmd + Z: Undo last operation
            if ((e.ctrlKey || e.metaKey) && e.key === 'z') {
                e.preventDefault();
                DeleteHistory.undoLastOperation();
            }
        });
    }

    /**
     * Handle DELETE request submission
     */
    async handleDeleteSubmission() {
        try {
            const endpoint = document.getElementById('delete-endpoint').value.trim();

            if (!endpoint) {
                DeleteUtils.showNotification('Please enter a DELETE endpoint', 'warning');
                return;
            }

            if (!DeleteUtils.validateDeleteEndpoint(endpoint)) {
                DeleteUtils.showNotification('Invalid endpoint format', 'error');
                return;
            }

            // Show confirmation dialog if not already shown
            const confirmationSection = document.getElementById('confirmation-section');
            if (confirmationSection.style.display === 'none') {
                await DeleteHandlers.showConfirmationDialog(endpoint);
                return;
            }

            // Execute the DELETE request
            await DeleteHandlers.sendDeleteRequest(endpoint);

        } catch (error) {
            console.error('[DELETE] Submission failed:', error);
            DeleteUtils.showNotification(`Request failed: ${error.message}`, 'error');
        }
    }

    /**
     * Update bulk operation button states
     */
    updateBulkOperationState() {
        const selectedResources = document.querySelectorAll('.resource-checkbox:checked');
        const bulkDeleteBtn = document.getElementById('bulk-delete');
        const selectionCount = document.getElementById('selection-count');

        if (bulkDeleteBtn) {
            bulkDeleteBtn.disabled = selectedResources.length === 0;
        }

        if (selectionCount) {
            selectionCount.textContent = selectedResources.length;
        }
    }

    /**
     * Setup dynamic elements that need initialization
     */
    setupDynamicElements() {
        console.log('[DELETE] Setting up dynamic elements...');

        // Initialize endpoint input with current server
        const endpointInput = document.getElementById('resource-url');
        if (endpointInput && !endpointInput.value) {
            endpointInput.value = DELETE_CONFIG.DEFAULT_ENDPOINTS.TEST_DELETE;
        }

        // Setup response display
        if (DeleteHandlers && typeof DeleteHandlers.initializeResponseDisplay === 'function') {
            DeleteHandlers.initializeResponseDisplay();
        } else {
            console.warn('[DELETE] initializeResponseDisplay method not available');
        }

        // Load history
        if (DeleteHistory && typeof DeleteHistory.init === 'function') {
            DeleteHistory.init();
        } else {
            console.warn('[DELETE] DeleteHistory.init not available');
        }

        // Update UI state
        this.updateBulkOperationState();

        console.log('[DELETE] Dynamic elements setup complete');
    }

    /**
     * Load saved application state
     */
    loadSavedState() {
        console.log('[DELETE] Loading saved state...');

        try {
            const savedState = localStorage.getItem('delete_app_state');
            if (savedState) {
                const state = JSON.parse(savedState);

                // Restore form values
                if (state.endpoint) {
                    const endpointInput = document.getElementById('resource-url');
                    if (endpointInput) endpointInput.value = state.endpoint;
                }

                // Restore custom headers
                if (state.headers && DeleteHandlers && typeof DeleteHandlers.addCustomHeader === 'function') {
                    state.headers.forEach(header => {
                        DeleteHandlers.addCustomHeader(header.name, header.value);
                    });
                } else if (state.headers) {
                    console.warn('[DELETE] Cannot restore headers: addCustomHeader method not available');
                }

                console.log('[DELETE] Saved state restored');
            } else {
                console.log('[DELETE] No saved state found');
            }
        } catch (error) {
            console.warn('[DELETE] Failed to load saved state:', error);
        }
    }

    /**
     * Save current application state
     */
    saveState() {
        try {
            const state = {
                endpoint: document.getElementById('resource-url')?.value || '',
                headers: DeleteHandlers.getCustomHeaders(),
                timestamp: Date.now()
            };

            localStorage.setItem('delete_app_state', JSON.stringify(state));
        } catch (error) {
            console.warn('[DELETE] Failed to save state:', error);
        }
    }

    /**
     * Cleanup and destroy the application
     */
    destroy() {
        this.saveState();
        this.initialized = false;
        console.log('[DELETE] Application destroyed');
    }
}

// Global application instance
let deleteApp = null;

/**
 * Initialize the DELETE application when DOM is ready
 */
document.addEventListener('DOMContentLoaded', () => {
    deleteApp = new DeleteApp();
    deleteApp.init();
});

/**
 * Cleanup when page unloads
 */
window.addEventListener('beforeunload', () => {
    if (deleteApp) {
        deleteApp.destroy();
    }
});

/**
 * Export for testing and external access
 */
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { DeleteApp };
}
