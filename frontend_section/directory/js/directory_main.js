/**
 * Directory Explorer - Main Module
 * Inicialización y coordinación del módulo Directory Explorer
 * Parte del proyecto 42_webserv_html
 */

class DirectoryMain {
    constructor() {
        this.isInitialized = false;
        this.handlers = null;
        this.navigation = null;
        this.currentTheme = 'light';
        this.isResponsive = true;
    }

    /**
     * Inicializa el Directory Explorer
     */
    async init() {
        try {
            console.log('🚀 Initializing Directory Explorer...');

            // Verificar que el DOM esté listo
            if (document.readyState === 'loading') {
                document.addEventListener('DOMContentLoaded', () => this.init());
                return;
            }

            // Verificar dependencias
            if (!this.checkDependencies()) {
                throw new Error('Missing required dependencies');
            }

            // Inicializar módulos
            await this.initializeModules();

            // Configurar UI
            this.setupUI();

            // Cargar preferencias
            this.loadUserPreferences();

            // Cargar directorio inicial
            await this.loadInitialDirectory();

            // Configurar eventos globales
            this.setupGlobalEvents();

            // Configurar integración con otros módulos
            this.setupModuleIntegration();

            this.isInitialized = true;
            console.log('✅ Directory Explorer initialized successfully');

            // Disparar evento de inicialización
            this.dispatchEvent('directoryExplorerReady');

        } catch (error) {
            console.error('❌ Failed to initialize Directory Explorer:', error);
            this.showInitializationError(error);
        }
    }

    /**
     * Verifica que todas las dependencias estén disponibles
     */
    checkDependencies() {
        const required = [
            { name: 'DirectoryConfig', type: 'class' },
            { name: 'DirectoryUtils', type: 'object' },
            { name: 'DirectoryHandlers', type: 'class' },
            { name: 'DirectoryNavigation', type: 'class' }
        ];

        console.log('Checking dependencies...');

        const missing = required.filter(dep => {
            const exists = window[dep.name] && typeof window[dep.name] !== 'undefined';
            console.log(`${exists ? '✅' : '❌'} ${dep.name} (${dep.type}): ${typeof window[dep.name]}`);
            return !exists;
        });

        if (missing.length > 0) {
            console.error('Missing dependencies:', missing.map(dep => dep.name));
            console.error('Available Directory objects:', Object.keys(window).filter(k => k.startsWith('Directory')));

            // Intentar esperar un poco más y reintentar
            console.log('Retrying dependency check in 100ms...');
            setTimeout(() => {
                if (this.checkDependencies()) {
                    console.log('Dependencies found on retry, reinitializing...');
                    this.init();
                }
            }, 100);

            return false;
        }

        console.log('All dependencies found!');
        return true;
    }

    /**
     * Inicializa los módulos principales
     */
    async initializeModules() {
        console.log('📦 Initializing modules...');

        try {
            // Inicializar navegación
            this.navigation = new DirectoryNavigation();
            await this.navigation.init();

            // Inicializar handlers
            this.handlers = new DirectoryHandlers();

            // Pasar referencia de navegación a handlers
            this.handlers.setNavigation(this.navigation);

            await this.handlers.init();

            // Hacer las instancias disponibles globalmente para integración
            window.directoryNavigation = this.navigation;
            window.directoryHandlers = this.handlers;

            console.log('✅ Modules initialized successfully');

        } catch (error) {
            console.error('❌ Module initialization failed:', error);
            throw error;
        }
    }

    /**
     * Configurar UI
     */
    setupUI() {
        console.log('🎨 Setting up UI...');

        // Configurar tema
        this.setupTheme();

        // Configurar responsive
        this.setupResponsive();

        // Configurar vista por defecto
        this.setupDefaultView();

        console.log('✅ UI setup complete');
    }

    /**
     * Configura la vista por defecto
     */
    setupDefaultView() {
        // Obtener vista guardada o usar grid por defecto
        const savedView = localStorage.getItem('directoryViewType') || 'grid';

        // Verificar que la vista sea válida
        const validViews = ['grid', 'list', 'details'];
        const viewType = validViews.includes(savedView) ? savedView : 'grid';

        // Aplicar vista si existe el handlers
        if (this.handlers && this.handlers.changeView) {
            this.handlers.changeView(viewType);
        } else {
            // Fallback: aplicar directamente los estilos
            const directoryListing = document.getElementById('directory-listing');
            if (directoryListing) {
                directoryListing.classList.add(`view-${viewType}`);
            }

            // Actualizar botón activo
            document.querySelectorAll('.view-button').forEach(btn => btn.classList.remove('active'));
            const activeButton = document.getElementById(`view-${viewType}`);
            if (activeButton) {
                activeButton.classList.add('active');
            }
        }

        console.log(`Default view set to: ${viewType}`);
    }

    /**
     * Configura el tema
     */
    setupTheme() {
        // Implementación básica del tema
        const savedTheme = localStorage.getItem('directoryTheme') || 'light';
        this.setTheme(savedTheme);
    }

    /**
     * Establece el tema
     */
    setTheme(theme) {
        this.currentTheme = theme;
        document.documentElement.setAttribute('data-theme', theme);
        localStorage.setItem('directoryTheme', theme);
    }

    /**
     * Configura responsive
     */
    setupResponsive() {
        // Configuración básica responsive
        this.handleResize();
        window.addEventListener('resize', () => this.handleResize());
    }

    /**
     * Maneja cambios de tamaño de ventana
     */
    handleResize() {
        const isMobile = window.innerWidth < 768;
        const isTablet = window.innerWidth < 1024;

        document.documentElement.classList.toggle('mobile', isMobile);
        document.documentElement.classList.toggle('tablet', isTablet);
    }

    /**
     * Carga preferencias del usuario
     */
    loadUserPreferences() {
        // Cargar preferencias básicas
        console.log('📋 Loading user preferences...');
    }

    /**
     * Carga directorio inicial
     */
    async loadInitialDirectory() {
        console.log('📂 Loading initial directory...');

        if (this.handlers && this.handlers.loadDirectory) {
            try {
                await this.handlers.loadDirectory(DirectoryConfig.defaultPath);
                console.log('✅ Initial directory loaded successfully');
            } catch (error) {
                console.error('❌ Error loading initial directory:', error);
            }
        } else {
            console.warn('⚠️ Handlers not available for loading directory');
        }
    }

    /**
     * Configura eventos globales
     */
    setupGlobalEvents() {
        console.log('🔗 Setting up global events...');
        // Implementación básica de eventos globales
    }

    /**
     * Configura integración con otros módulos
     */
    setupModuleIntegration() {
        console.log('🔌 Setting up module integration...');
        // Implementación básica de integración
    }

    /**
     * Dispara un evento personalizado
     */
    dispatchEvent(eventName, detail = {}) {
        const event = new CustomEvent(eventName, { detail });
        document.dispatchEvent(event);
    }

    /**
     * Muestra error de inicialización
     */
    showInitializationError(error) {
        console.error('Initialization error:', error);

        // Mostrar error en la UI si es posible
        const container = document.querySelector('.container');
        if (container) {
            const errorDiv = document.createElement('div');
            errorDiv.className = 'initialization-error';
            errorDiv.innerHTML = `
                <h3>⚠️ Initialization Error</h3>
                <p>Failed to initialize Directory Explorer: ${error.message}</p>
                <button onclick="location.reload()">🔄 Retry</button>
            `;
            errorDiv.style.cssText = `
                background: rgba(255, 107, 107, 0.1);
                border: 1px solid rgba(255, 107, 107, 0.3);
                padding: 20px;
                border-radius: 10px;
                margin: 20px;
                text-align: center;
                color: white;
            `;
            container.insertBefore(errorDiv, container.firstChild);
        }
    }
}

// Instancia global
const directoryMain = new DirectoryMain();

// Auto-inicialización con múltiples estrategias
function initializeDirectoryExplorer() {
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initializeDirectoryExplorer);
        return;
    }

    // Esperar un poco para asegurar que todos los scripts se hayan cargado
    setTimeout(() => {
        directoryMain.init().catch(error => {
            console.error('Auto-initialization failed:', error);
        });
    }, 50);
}

// Múltiples puntos de inicialización
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeDirectoryExplorer);
} else {
    initializeDirectoryExplorer();
}

// Backup de inicialización
window.addEventListener('load', () => {
    if (!directoryMain.isInitialized) {
        console.log('Backup initialization triggered...');
        directoryMain.init().catch(console.error);
    }
});

// Exportar para uso global
window.DirectoryMain = DirectoryMain;
window.directoryMain = directoryMain;
