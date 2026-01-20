/**
 * ========================================================================
 * UPLOAD CONFIGURATION
 * ========================================================================
 * 
 * Global configuration constants and variables for the upload system
 */

// Configuration constants
const CONFIG = {
    MAX_FILE_SIZE: 10 * 1024 * 1024, // 10MB maximum file size
    UPLOAD_ENDPOINT: '/uploads',
    FILES_ENDPOINT: '/uploads',
    SIMULATION_MODE: false, // Set to false for production
    SUCCESS_RATE: 0.8, // 80% success rate for simulation
    UPLOAD_DELAY: 1000, // Delay for simulation in ms
};

// Global state
let selectedFiles = [];

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { CONFIG, selectedFiles };
}
