/**
 * Directory Explorer - Configuration Module
 * Configuración global del módulo Directory Explorer
 * Parte del proyecto 42_webserv_html
 */

// Main configuration object
const DirectoryConfig = {
    // Default path
    defaultPath: '/',

    // API Endpoints
    endpoints: {
        list: '/api/directory/list',
        download: '/api/files/download',
        preview: '/api/files/preview',
        delete: '/api/files/delete',
        rename: '/api/files/rename',
        copy: '/api/files/copy',
        move: '/api/files/move',
        mkdir: '/api/directory/create',
        properties: '/api/files/info',
        search: '/api/directory/search',
        suggestions: '/api/directory/suggest'
    },

    // UI Configuration
    ui: {
        defaultView: 'grid',
        breakpoints: {
            mobile: 768,
            tablet: 1024
        }
    },

    // Security settings
    security: {
        headers: {
            'X-Requested-With': 'XMLHttpRequest'
        }
    },

    // Performance settings
    performance: {
        requestTimeout: 10000
    },

    // Navigation settings
    navigation: {
        maxHistorySize: 50,
        defaultBookmarks: [
            { name: 'Home', path: '/' },
            { name: 'Documents', path: '/documents' },
            { name: 'Downloads', path: '/downloads' }
        ]
    },

    // File types
    fileTypes: {
        images: {
            extensions: ['jpg', 'jpeg', 'png', 'gif', 'svg', 'bmp', 'webp'],
            icon: '🖼️',
            previewable: true
        },
        videos: {
            extensions: ['mp4', 'avi', 'mkv', 'mov', 'wmv', 'webm'],
            icon: '🎬',
            previewable: true
        },
        audio: {
            extensions: ['mp3', 'wav', 'flac', 'ogg', 'm4a'],
            icon: '🎵',
            previewable: true
        },
        documents: {
            extensions: ['pdf', 'doc', 'docx', 'txt', 'md', 'rtf'],
            icon: '📄',
            previewable: false
        },
        code: {
            extensions: ['js', 'html', 'css', 'php', 'py', 'java', 'cpp', 'c', 'json', 'xml'],
            icon: '📜',
            previewable: true
        },
        archives: {
            extensions: ['zip', 'rar', '7z', 'tar', 'gz'],
            icon: '📦',
            previewable: false
        }
    },

    // Simulation mode
    simulation: {
        enabled: false,
        delay: 500,
        sampleData: {
            path: "/",
            items: [
                {
                    name: "documents",
                    path: "/documents",
                    type: "directory",
                    modified: "2024-01-15T10:30:00Z"
                },
                {
                    name: "images",
                    path: "/images",
                    type: "directory",
                    modified: "2024-01-14T15:20:00Z"
                },
                {
                    name: "example.txt",
                    path: "/example.txt",
                    type: "file",
                    size: 1024,
                    modified: "2024-01-15T10:30:00Z"
                },
                {
                    name: "photo.jpg",
                    path: "/photo.jpg",
                    type: "file",
                    size: 2048576,
                    modified: "2024-01-13T09:15:00Z"
                },
                {
                    name: "script.js",
                    path: "/script.js",
                    type: "file",
                    size: 8192,
                    modified: "2024-01-16T14:45:00Z"
                }
            ]
        }
    }
};

// Legacy configuration object for compatibility with DirectoryUtils
const DIRECTORY_CONFIG = {
    SIZE_UNITS: ['B', 'KB', 'MB', 'GB', 'TB'],
    DATE_OPTIONS: {
        year: 'numeric',
        month: 'short',
        day: 'numeric',
        hour: '2-digit',
        minute: '2-digit'
    },
    FILE_TYPES: {
        directory: { icon: '📁', type: 'directory' },
        unknown: { icon: '📄', type: 'file' },
        txt: { icon: '📝', type: 'text' },
        pdf: { icon: '📄', type: 'document' },
        jpg: { icon: '🖼️', type: 'image' },
        png: { icon: '🖼️', type: 'image' },
        zip: { icon: '📦', type: 'archive' }
    },
    SECURITY: {
        MAX_PATH_LENGTH: 4096,
        FORBIDDEN_CHARS: ['<', '>', ':', '"', '|', '?', '*'],
        DANGEROUS_EXTENSIONS: ['exe', 'bat', 'cmd', 'com', 'scr']
    },
    DEFAULTS: {
        MAX_FILE_SIZE_PREVIEW: 10485760 // 10MB
    },
    PREVIEW: {
        SUPPORTED_IMAGES: ['jpg', 'jpeg', 'png', 'gif', 'svg', 'webp'],
        SUPPORTED_TEXT: ['txt', 'md', 'json', 'xml', 'html', 'css', 'js']
    },
    STORAGE_KEYS: {
        PREFERENCES: 'directoryExplorerPrefs'
    }
};

// Export for browser
window.DirectoryConfig = DirectoryConfig;

// Export for Node.js if available
if (typeof module !== 'undefined' && module.exports) {
    module.exports = DirectoryConfig;
}
