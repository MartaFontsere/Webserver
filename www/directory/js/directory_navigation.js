/**
 * Directory Explorer - Navigation Module
 * Maneja la navegación, historial y breadcrumbs
 * Parte del proyecto 42_webserv_html
 */

class DirectoryNavigation {
    constructor() {
        this.currentPath = DirectoryConfig.defaultPath;
        this.history = [DirectoryConfig.defaultPath];
        this.historyIndex = 0;
        this.maxHistorySize = DirectoryConfig.navigation.maxHistorySize;
        this.bookmarks = this.loadBookmarks();
    }

    /**
     * Inicializa la navegación
     */
    init() {
        this.setupBreadcrumbs();
        this.setupAddressBar();
        this.setupBookmarks();
        this.updateNavigationState();
        this.loadSavedPath();
    }

    /**
     * Navega a una ruta específica
     */
    async navigateTo(path) {
        try {
            // Validar y normalizar la ruta
            const normalizedPath = this.normalizePath(path);

            if (!this.isValidPath(normalizedPath)) {
                throw new Error('Invalid path');
            }

            // Agregar al historial si es diferente de la ruta actual
            if (normalizedPath !== this.currentPath) {
                this.addToHistory(normalizedPath);
            }

            // Actualizar ruta actual
            this.currentPath = normalizedPath;

            // Cargar directorio
            await window.directoryHandlers.loadDirectory(normalizedPath);

            // Actualizar UI
            this.updateAddressBar();
            this.updateBreadcrumbs();
            this.updateNavigationState();
            this.savePath();

            return true;

        } catch (error) {
            console.error('Navigation error:', error);
            DirectoryUtils.showNotification('Navigation failed: ' + error.message, 'error');
            return false;
        }
    }

    /**
     * Navega hacia atrás en el historial
     */
    async goBack() {
        if (this.historyIndex > 0) {
            this.historyIndex--;
            const path = this.history[this.historyIndex];
            await this.navigateToHistoryPath(path);
        }
    }

    /**
     * Navega hacia adelante en el historial
     */
    async goForward() {
        if (this.historyIndex < this.history.length - 1) {
            this.historyIndex++;
            const path = this.history[this.historyIndex];
            await this.navigateToHistoryPath(path);
        }
    }

    /**
     * Navega al directorio padre
     */
    async goUp() {
        const parentPath = this.getParentPath(this.currentPath);
        if (parentPath !== this.currentPath) {
            await this.navigateTo(parentPath);
        }
    }

    /**
     * Navega al directorio home
     */
    async goHome() {
        await this.navigateTo(DirectoryConfig.defaultPath);
    }

    /**
     * Navega a una ruta del historial sin modificar el índice
     */
    async navigateToHistoryPath(path) {
        try {
            this.currentPath = path;
            await window.directoryHandlers.loadDirectory(path);
            this.updateAddressBar();
            this.updateBreadcrumbs();
            this.updateNavigationState();
            this.savePath();
        } catch (error) {
            console.error('History navigation error:', error);
            DirectoryUtils.showNotification('Failed to navigate to: ' + path, 'error');
        }
    }

    /**
     * Agrega una ruta al historial
     */
    addToHistory(path) {
        // Eliminar entradas futuras si estamos en el medio del historial
        if (this.historyIndex < this.history.length - 1) {
            this.history.splice(this.historyIndex + 1);
        }

        // Agregar nueva entrada
        this.history.push(path);
        this.historyIndex = this.history.length - 1;

        // Mantener el tamaño máximo del historial
        if (this.history.length > this.maxHistorySize) {
            this.history.shift();
            this.historyIndex--;
        }
    }

    /**
     * Configura los breadcrumbs
     */
    setupBreadcrumbs() {
        const breadcrumbs = document.getElementById('breadcrumbs');
        if (!breadcrumbs) return;

        breadcrumbs.addEventListener('click', (e) => {
            const breadcrumb = e.target.closest('.breadcrumb-item');
            if (breadcrumb && breadcrumb.dataset.path) {
                this.navigateTo(breadcrumb.dataset.path);
            }
        });
    }

    /**
     * Configura la barra de direcciones
     */
    setupAddressBar() {
        const addressBar = document.getElementById('address-bar');
        if (!addressBar) return;

        // Autocompletar
        addressBar.addEventListener('input', DirectoryUtils.debounce((e) => {
            this.showPathSuggestions(e.target.value);
        }, 300));

        // Navegación al presionar Enter
        addressBar.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.navigateTo(e.target.value);
                this.hideSuggestions();
            }
        });

        // Ocultar sugerencias al perder foco
        addressBar.addEventListener('blur', () => {
            setTimeout(() => this.hideSuggestions(), 200);
        });
    }

    /**
     * Configura los bookmarks
     */
    setupBookmarks() {
        const bookmarksList = document.getElementById('bookmarks-list');
        if (!bookmarksList) return;

        this.renderBookmarks();

        // Manejar clicks en bookmarks
        bookmarksList.addEventListener('click', (e) => {
            const bookmark = e.target.closest('.bookmark-item');
            if (bookmark && bookmark.dataset.path) {
                this.navigateTo(bookmark.dataset.path);
            }
        });
    }

    /**
     * Actualiza los breadcrumbs
     */
    updateBreadcrumbs() {
        const breadcrumbs = document.getElementById('breadcrumbs');
        if (!breadcrumbs) return;

        const pathParts = this.getPathParts(this.currentPath);
        let currentPath = '';

        const breadcrumbsHTML = pathParts.map((part, index) => {
            currentPath = index === 0 ? part : currentPath + '/' + part;
            const isLast = index === pathParts.length - 1;

            return `
                <div class="breadcrumb-item ${isLast ? 'current' : ''}" 
                     data-path="${currentPath}"
                     title="${currentPath}">
                    ${part || 'Root'}
                    ${!isLast ? '<i class="breadcrumb-separator">›</i>' : ''}
                </div>
            `;
        }).join('');

        breadcrumbs.innerHTML = breadcrumbsHTML;
    }

    /**
     * Actualiza la barra de direcciones
     */
    updateAddressBar() {
        const addressBar = document.getElementById('address-bar');
        if (addressBar) {
            addressBar.value = this.currentPath;
        }
    }

    /**
     * Actualiza el estado de los botones de navegación
     */
    updateNavigationState() {
        const backBtn = document.getElementById('back-btn');
        const forwardBtn = document.getElementById('forward-btn');
        const upBtn = document.getElementById('up-btn');

        if (backBtn) {
            backBtn.disabled = this.historyIndex <= 0;
            backBtn.classList.toggle('disabled', this.historyIndex <= 0);
        }

        if (forwardBtn) {
            forwardBtn.disabled = this.historyIndex >= this.history.length - 1;
            forwardBtn.classList.toggle('disabled', this.historyIndex >= this.history.length - 1);
        }

        if (upBtn) {
            const canGoUp = this.getParentPath(this.currentPath) !== this.currentPath;
            upBtn.disabled = !canGoUp;
            upBtn.classList.toggle('disabled', !canGoUp);
        }
    }

    /**
     * Muestra sugerencias de rutas
     */
    async showPathSuggestions(input) {
        if (!input || input.length < 2) {
            this.hideSuggestions();
            return;
        }

        try {
            const suggestions = await this.getSuggestions(input);
            this.renderSuggestions(suggestions);
        } catch (error) {
            console.error('Error getting path suggestions:', error);
            this.hideSuggestions();
        }
    }

    /**
     * Obtiene sugerencias de rutas
     */
    async getSuggestions(input) {
        if (DirectoryConfig.simulation.enabled) {
            // Simular sugerencias
            const basePath = this.getBasePath(input);
            return [
                basePath + '/documents',
                basePath + '/downloads',
                basePath + '/images',
                basePath + '/videos'
            ].filter(path => path.toLowerCase().includes(input.toLowerCase()));
        }

        // En implementación real, hacer petición al backend
        const url = `${DirectoryConfig.endpoints.suggestions}?path=${encodeURIComponent(input)}`;
        const response = await window.directoryHandlers.makeRequest(url);
        const data = await response.json();
        return data.suggestions || [];
    }

    /**
     * Renderiza las sugerencias
     */
    renderSuggestions(suggestions) {
        let suggestionsContainer = document.getElementById('path-suggestions');

        if (!suggestionsContainer) {
            suggestionsContainer = document.createElement('div');
            suggestionsContainer.id = 'path-suggestions';
            suggestionsContainer.className = 'path-suggestions';
            document.getElementById('address-bar').parentNode.appendChild(suggestionsContainer);
        }

        if (suggestions.length === 0) {
            this.hideSuggestions();
            return;
        }

        const suggestionsHTML = suggestions.map(suggestion => `
            <div class="suggestion-item" data-path="${suggestion}">
                <i class="icon-folder"></i>
                <span>${suggestion}</span>
            </div>
        `).join('');

        suggestionsContainer.innerHTML = suggestionsHTML;
        suggestionsContainer.style.display = 'block';

        // Manejar clicks en sugerencias
        suggestionsContainer.addEventListener('click', (e) => {
            const item = e.target.closest('.suggestion-item');
            if (item) {
                document.getElementById('address-bar').value = item.dataset.path;
                this.navigateTo(item.dataset.path);
                this.hideSuggestions();
            }
        });
    }

    /**
     * Oculta las sugerencias
     */
    hideSuggestions() {
        const suggestionsContainer = document.getElementById('path-suggestions');
        if (suggestionsContainer) {
            suggestionsContainer.style.display = 'none';
        }
    }

    /**
     * Renderiza los bookmarks
     */
    renderBookmarks() {
        const bookmarksList = document.getElementById('bookmarks-list');
        if (!bookmarksList) return;

        const bookmarksHTML = this.bookmarks.map(bookmark => `
            <div class="bookmark-item" data-path="${bookmark.path}" title="${bookmark.path}">
                <i class="icon-bookmark"></i>
                <span class="bookmark-name">${bookmark.name}</span>
                <button class="bookmark-remove" data-path="${bookmark.path}" title="Remove bookmark">
                    <i class="icon-close"></i>
                </button>
            </div>
        `).join('');

        bookmarksList.innerHTML = bookmarksHTML;

        // Manejar eliminación de bookmarks
        bookmarksList.addEventListener('click', (e) => {
            if (e.target.closest('.bookmark-remove')) {
                e.stopPropagation();
                const path = e.target.closest('.bookmark-remove').dataset.path;
                this.removeBookmark(path);
            }
        });
    }

    /**
     * Agrega un bookmark
     */
    addBookmark(path = this.currentPath, name = null) {
        if (this.bookmarks.some(b => b.path === path)) {
            DirectoryUtils.showNotification('Bookmark already exists', 'warning');
            return;
        }

        const bookmarkName = name || this.getDirectoryName(path) || 'Root';
        this.bookmarks.push({ path, name: bookmarkName });
        this.saveBookmarks();
        this.renderBookmarks();
        DirectoryUtils.showNotification('Bookmark added', 'success');
    }

    /**
     * Elimina un bookmark
     */
    removeBookmark(path) {
        this.bookmarks = this.bookmarks.filter(b => b.path !== path);
        this.saveBookmarks();
        this.renderBookmarks();
        DirectoryUtils.showNotification('Bookmark removed', 'success');
    }

    /**
     * Carga bookmarks desde localStorage
     */
    loadBookmarks() {
        try {
            const saved = localStorage.getItem('directory_bookmarks');
            return saved ? JSON.parse(saved) : DirectoryConfig.navigation.defaultBookmarks;
        } catch (error) {
            console.error('Error loading bookmarks:', error);
            return DirectoryConfig.navigation.defaultBookmarks;
        }
    }

    /**
     * Guarda bookmarks en localStorage
     */
    saveBookmarks() {
        try {
            localStorage.setItem('directory_bookmarks', JSON.stringify(this.bookmarks));
        } catch (error) {
            console.error('Error saving bookmarks:', error);
        }
    }

    /**
     * Guarda la ruta actual
     */
    savePath() {
        try {
            localStorage.setItem('directory_current_path', this.currentPath);
        } catch (error) {
            console.error('Error saving current path:', error);
        }
    }

    /**
     * Carga la ruta guardada
     */
    loadSavedPath() {
        try {
            const savedPath = localStorage.getItem('directory_current_path');
            if (savedPath && this.isValidPath(savedPath)) {
                this.currentPath = savedPath;
                this.updateAddressBar();
                this.updateBreadcrumbs();
            }
        } catch (error) {
            console.error('Error loading saved path:', error);
        }
    }

    /**
     * Obtiene el historial de navegación
     */
    getHistory() {
        return {
            history: [...this.history],
            currentIndex: this.historyIndex
        };
    }

    /**
     * Limpia el historial
     */
    clearHistory() {
        this.history = [this.currentPath];
        this.historyIndex = 0;
        this.updateNavigationState();
    }

    // Métodos utilitarios

    /**
     * Normaliza una ruta
     */
    normalizePath(path) {
        if (!path) return DirectoryConfig.defaultPath;

        // Remover barras múltiples
        path = path.replace(/\/+/g, '/');

        // Remover barra final excepto para root
        if (path.length > 1 && path.endsWith('/')) {
            path = path.slice(0, -1);
        }

        // Asegurar que comience con /
        if (!path.startsWith('/')) {
            path = '/' + path;
        }

        return path;
    }

    /**
     * Valida si una ruta es válida
     */
    isValidPath(path) {
        if (!path || typeof path !== 'string') return false;

        // Verificar caracteres permitidos
        const validPathRegex = /^\/[a-zA-Z0-9._\-\/]*$/;
        if (!validPathRegex.test(path)) return false;

        // Verificar que no contenga .. para evitar directory traversal
        if (path.includes('..')) return false;

        return true;
    }

    /**
     * Obtiene el directorio padre
     */
    getParentPath(path) {
        if (path === '/' || path === '') return '/';

        const parentPath = path.substring(0, path.lastIndexOf('/'));
        return parentPath || '/';
    }

    /**
     * Obtiene las partes de una ruta
     */
    getPathParts(path) {
        if (path === '/') return [''];

        return path.split('/').filter(part => part !== '');
    }

    /**
     * Obtiene la ruta base de una ruta
     */
    getBasePath(path) {
        const lastSlash = path.lastIndexOf('/');
        return lastSlash > 0 ? path.substring(0, lastSlash) : '/';
    }

    /**
     * Obtiene el nombre del directorio
     */
    getDirectoryName(path) {
        if (path === '/') return 'Root';

        const parts = path.split('/');
        return parts[parts.length - 1] || 'Root';
    }

    /**
     * Obtiene la ruta actual
     */
    getCurrentPath() {
        return this.currentPath;
    }

    /**
     * Actualiza la ruta actual (para uso externo)
     */
    updateCurrentPath(path) {
        this.currentPath = this.normalizePath(path);
        this.updateAddressBar();
        this.updateBreadcrumbs();
        this.updateNavigationState();
        this.savePath();
    }

    /**
     * Verifica si se puede navegar hacia atrás
     */
    canGoBack() {
        return this.historyIndex > 0;
    }

    /**
     * Verifica si se puede navegar hacia adelante
     */
    canGoForward() {
        return this.historyIndex < this.history.length - 1;
    }

    /**
     * Verifica si se puede navegar hacia arriba
     */
    canGoUp() {
        return this.getParentPath(this.currentPath) !== this.currentPath;
    }
}

// Exportar para uso global
window.DirectoryNavigation = DirectoryNavigation;
