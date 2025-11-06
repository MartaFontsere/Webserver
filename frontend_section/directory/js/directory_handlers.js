/**
 * Directory Explorer - Handlers Module
 * Manejadores de eventos y comunicación con API del backend
 * Parte del proyecto 42_webserv_html
 */

class DirectoryHandlers {
    constructor() {
        this.currentRequest = null;
        this.requestQueue = [];
        this.retryAttempts = new Map();
        this.navigation = null; // Referencia a DirectoryNavigation
    }

    /**
     * Establece la referencia a DirectoryNavigation
     */
    setNavigation(navigationInstance) {
        this.navigation = navigationInstance;
    }

    /**
     * Inicializa los manejadores de eventos
     */
    init() {
        this.setupEventListeners();
        this.setupKeyboardShortcuts();
        this.setupContextMenu();
        this.setupDragAndDrop();
    }

    /**
     * Configura los event listeners principales
     */
    setupEventListeners() {
        // Navegación
        document.getElementById('nav-back')?.addEventListener('click', () => this.navigateBack());
        document.getElementById('nav-forward')?.addEventListener('click', () => this.navigateForward());
        document.getElementById('nav-up')?.addEventListener('click', () => this.navigateUp());
        document.getElementById('nav-home')?.addEventListener('click', () => this.navigateHome());
        document.getElementById('nav-refresh')?.addEventListener('click', () => this.refreshDirectory());

        // Controles de vista
        document.getElementById('view-grid')?.addEventListener('click', () => this.changeView('grid'));
        document.getElementById('view-list')?.addEventListener('click', () => this.changeView('list'));
        document.getElementById('view-details')?.addEventListener('click', () => this.changeView('details'));

        // Filtros y ordenación
        document.getElementById('sort-by')?.addEventListener('change', (e) => this.changeSorting(e.target.value));
        document.getElementById('filter-input')?.addEventListener('input', (e) => this.filterItems(e.target.value));

        // Búsqueda
        const searchInput = document.getElementById('filter-input');
        searchInput?.addEventListener('input', DirectoryUtils.debounce((e) => this.performSearch(e.target.value), 300));

        // Barra de dirección
        const addressBar = document.getElementById('address-bar');
        addressBar?.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.navigateToPath(e.target.value);
            }
        });

        // Cerrar modales
        document.addEventListener('click', (e) => {
            if (e.target.classList.contains('modal')) {
                this.closeModal();
            }
        });

        // Cerrar context menu
        document.addEventListener('click', () => this.hideContextMenu());
    }

    /**
     * Configura atajos de teclado
     */
    setupKeyboardShortcuts() {
        document.addEventListener('keydown', (e) => {
            // Escape - Deseleccionar todo / Cerrar modal
            if (e.key === 'Escape') {
                this.deselectAll();
                this.closeModal();
                this.hideContextMenu();
            }

            // F5 - Refrescar
            if (e.key === 'F5') {
                e.preventDefault();
                this.refreshDirectory();
            }
        });
    }

    /**
     * Configura menú contextual
     */
    setupContextMenu() {
        // Implementación básica del menú contextual
        console.log('Context menu setup');
    }

    /**
     * Configura drag and drop
     */
    setupDragAndDrop() {
        // Implementación básica de drag and drop
        console.log('Drag and drop setup');
    }

    // Métodos de navegación
    navigateBack() {
        if (this.navigation && this.navigation.goBack) {
            this.navigation.goBack();
        } else {
            console.warn('Navigation not available or goBack method missing');
        }
    }

    navigateForward() {
        if (this.navigation && this.navigation.goForward) {
            this.navigation.goForward();
        } else {
            console.warn('Navigation not available or goForward method missing');
        }
    }

    navigateUp() {
        if (this.navigation && this.navigation.goUp) {
            this.navigation.goUp();
        } else {
            console.warn('Navigation not available or goUp method missing');
        }
    }

    navigateHome() {
        if (this.navigation && this.navigation.goHome) {
            this.navigation.goHome();
        } else {
            console.warn('Navigation not available or goHome method missing');
        }
    }

    navigateToPath(path) {
        if (this.navigation && this.navigation.navigateTo) {
            this.navigation.navigateTo(path);
        } else {
            console.warn('Navigation not available or navigateTo method missing');
        }
    }

    refreshDirectory() {
        if (this.navigation && this.navigation.currentPath) {
            this.loadDirectory(this.navigation.currentPath);
        } else {
            console.warn('Navigation not available, using default path');
            this.loadDirectory('/');
        }
    }

    /**
     * Carga el contenido de un directorio
     */
    async loadDirectory(path = DirectoryConfig.defaultPath) {
        try {
            console.log(`Loading directory: ${path}`);
            this.showLoading();

            // Simular delay
            await new Promise(resolve => setTimeout(resolve, DirectoryConfig.simulation.delay || 300));

            // En modo simulación, usar datos de muestra
            if (DirectoryConfig.simulation.enabled) {
                const data = DirectoryConfig.simulation.sampleData;

                if (data && data.items && data.items.length > 0) {
                    this.displayDirectoryContent(data);
                } else {
                    this.showEmptyState();
                }
            } else {
                // Sin backend real, mostrar estado vacío
                this.showEmptyState();
            }

        } catch (error) {
            console.error('Error loading directory:', error);
            this.showError('Error loading directory: ' + error.message);
        }
    }

    /**
     * Cambia la vista de archivos (grid, list, details)
     */
    changeView(viewType) {
        // Validar tipo de vista
        const validViews = ['grid', 'list', 'details'];
        if (!validViews.includes(viewType)) {
            console.warn('Invalid view type:', viewType);
            return;
        }

        // Actualizar botones activos
        document.querySelectorAll('.view-button').forEach(btn => {
            btn.classList.remove('active');
        });

        const activeButton = document.getElementById(`view-${viewType}`);
        if (activeButton) {
            activeButton.classList.add('active');
        }

        // Guardar preferencia en localStorage
        localStorage.setItem('directoryViewType', viewType);

        // Aplicar estilos de vista al contenedor
        const directoryListing = document.getElementById('directory-listing');
        if (directoryListing) {
            // Remover clases de vista existentes
            directoryListing.classList.remove('view-grid', 'view-list', 'view-details');
            // Agregar nueva clase de vista
            directoryListing.classList.add(`view-${viewType}`);
        }

        console.log(`View changed to: ${viewType}`);
    }

    /**
     * Obtiene los elementos actuales del directorio
     */
    getCurrentItems() {
        return [];
    }

    /**
     * Refresca la visualización del directorio con la vista actual
     */
    refreshDirectoryDisplay() {
        console.log('Refreshing directory display...');
    }

    /**
     * Muestra el contenido del directorio
     */
    displayDirectoryContent(data) {
        if (!data || !data.items || data.items.length === 0) {
            this.showEmptyState();
            return;
        }

        // Ocultar otros estados y mostrar el listado
        document.getElementById('loading-state')?.classList.add('hidden');
        document.getElementById('empty-state')?.classList.add('hidden');
        document.getElementById('error-state')?.classList.add('hidden');
        document.getElementById('directory-listing')?.classList.remove('hidden');

        // Obtener el contenedor de archivos
        const directoryListing = document.getElementById('directory-listing');
        if (!directoryListing) {
            console.error('Directory listing container not found');
            return;
        }

        // Limpiar contenido anterior
        directoryListing.innerHTML = '';

        // Renderizar cada elemento
        data.items.forEach(item => {
            const itemElement = this.createFileItem(item);
            directoryListing.appendChild(itemElement);
        });

        console.log(`Displayed ${data.items.length} items in directory`);
    }

    /**
     * Crea un elemento HTML para un archivo o directorio
     */
    createFileItem(item) {
        const itemElement = document.createElement('div');
        itemElement.className = 'file-item';
        itemElement.dataset.path = item.path;
        itemElement.dataset.type = item.type;

        // Icono basado en el tipo
        let icon = '📄'; // Default file icon
        if (item.type === 'directory') {
            icon = '📁';
        } else if (item.name.match(/\.(jpg|jpeg|png|gif|webp)$/i)) {
            icon = '🖼️';
        } else if (item.name.match(/\.(js|ts|html|css|json)$/i)) {
            icon = '💻';
        } else if (item.name.match(/\.(txt|md|doc|docx)$/i)) {
            icon = '📝';
        } else if (item.name.match(/\.(pdf)$/i)) {
            icon = '📕';
        } else if (item.name.match(/\.(mp3|wav|m4a)$/i)) {
            icon = '🎵';
        } else if (item.name.match(/\.(mp4|avi|mov|webm)$/i)) {
            icon = '🎬';
        }

        // Formatear tamaño
        let sizeText = '';
        if (item.type === 'file' && item.size) {
            if (typeof DirectoryUtils !== 'undefined' && DirectoryUtils.formatFileSize) {
                sizeText = DirectoryUtils.formatFileSize(item.size);
            } else {
                sizeText = item.size + ' bytes';
            }
        }

        // Formatear fecha
        let dateText = '';
        if (item.modified) {
            const date = new Date(item.modified);
            dateText = date.toLocaleDateString();
        }

        itemElement.innerHTML = `
            <div class="file-icon">${icon}</div>
            <div class="file-info">
                <div class="file-name">${item.name}</div>
                ${sizeText ? `<div class="file-size">${sizeText}</div>` : ''}
                ${dateText ? `<div class="file-date">${dateText}</div>` : ''}
            </div>
        `;

        // Agregar event listeners
        itemElement.addEventListener('click', () => {
            if (item.type === 'directory') {
                this.navigateToPath(item.path);
            } else {
                console.log('File clicked:', item.name);
                // Aquí se podría abrir un preview o descargar
            }
        });

        itemElement.addEventListener('dblclick', () => {
            if (item.type === 'directory') {
                this.navigateToPath(item.path);
            }
        });

        return itemElement;
    }

    // Métodos auxiliares para UI
    toggleHiddenFiles(show) { this.refreshDirectory(); }
    filterItems(filter) { console.log('Filter:', filter); }
    changeSorting(sortBy) { console.log('Sort by:', sortBy); }
    performSearch(query) { console.log('Search:', query); }

    selectAll() {
        document.querySelectorAll('.directory-item').forEach(item => item.classList.add('selected'));
    }

    deselectAll() {
        document.querySelectorAll('.directory-item').forEach(item => item.classList.remove('selected'));
    }

    /**
     * Muestra el estado de carga
     */
    showLoading() {
        document.getElementById('loading-state')?.classList.remove('hidden');
        document.getElementById('directory-listing')?.classList.add('hidden');
        document.getElementById('empty-state')?.classList.add('hidden');
        document.getElementById('error-state')?.classList.add('hidden');
    }

    /**
     * Muestra el estado vacío
     */
    showEmptyState() {
        document.getElementById('loading-state')?.classList.add('hidden');
        document.getElementById('directory-listing')?.classList.add('hidden');
        document.getElementById('empty-state')?.classList.remove('hidden');
        document.getElementById('error-state')?.classList.add('hidden');
    }

    /**
     * Muestra error
     */
    showError(message) {
        document.getElementById('loading-state')?.classList.add('hidden');
        document.getElementById('directory-listing')?.classList.add('hidden');
        document.getElementById('empty-state')?.classList.add('hidden');
        document.getElementById('error-state')?.classList.remove('hidden');

        const errorMessage = document.getElementById('error-message');
        if (errorMessage) {
            errorMessage.textContent = message;
        }
    }

    // Métodos placeholder
    closeModal() { console.log('Close modal'); }
    hideContextMenu() { console.log('Hide context menu'); }
}

// Exportar para uso global
window.DirectoryHandlers = DirectoryHandlers;
