/**
 * ================================================================
 * 📥 GET TESTING - REQUEST HANDLERS
 * Core functionality for handling HTTP GET requests
 * ================================================================
 */

class GetRequestHandler {
    constructor() {
        this.isSimulationMode = true; // Set to false when connecting to real backend
        this.currentRequest = null;
    }

    /**
     * Send GET request to specified URL
     * @param {string} url - URL to send request to
     * @returns {Promise<Object>} Response data
     */
    async sendRequest(url) {
        const startTime = Date.now();

        try {
            // Show loading indicator
            this.showIndicator('loading');

            let response;
            let responseData;

            if (this.isSimulationMode) {
                // Simulation mode - generate mock responses
                response = await this.simulateRequest(url);
                responseData = response.data;
            } else {
                // Real mode - make actual HTTP request
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), GET_CONFIG.REQUEST_TIMEOUT);

                response = await fetch(url, {
                    method: 'GET',
                    signal: controller.signal,
                    headers: {
                        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
                        'Accept-Language': 'en-US,en;q=0.5',
                        'Accept-Encoding': 'gzip, deflate',
                        'User-Agent': 'WebServer-Test-Client/1.0'
                    }
                });

                clearTimeout(timeoutId);
                responseData = await response.text();
            }

            const endTime = Date.now();
            const duration = endTime - startTime;

            // Process response
            const processedResponse = this.processResponse(response, responseData, duration, url);

            // Show success indicator
            this.showIndicator('success');

            return processedResponse;

        } catch (error) {
            const endTime = Date.now();
            const duration = endTime - startTime;

            // Show error indicator
            this.showIndicator('error');

            return this.processError(error, duration, url);
        }
    }

    /**
     * Simulate HTTP request for development/testing
     * @param {string} url - URL to simulate
     * @returns {Promise<Object>} Simulated response
     */
    async simulateRequest(url) {
        // Add realistic delay
        await new Promise(resolve => setTimeout(resolve, 300 + Math.random() * 700));

        // Determine response based on URL
        if (url === '/' || url === '/index.html') {
            return this.createSimulatedResponse(200, 'OK', 'text/html', this.getHomePageSimulation());
        } else if (url === '/about.html') {
            return this.createSimulatedResponse(200, 'OK', 'text/html', this.getAboutPageSimulation());
        } else if (url === '/api/status') {
            return this.createSimulatedResponse(200, 'OK', 'application/json', this.getStatusApiSimulation());
        } else if (url.startsWith('/files/')) {
            return this.createSimulatedResponse(200, 'OK', 'application/json', this.getFilesApiSimulation());
        } else if (url.includes('nonexistent') || url.includes('404')) {
            return this.createSimulatedResponse(404, 'Not Found', 'text/html', this.get404Simulation());
        } else if (url.includes('forbidden') || url.includes('403')) {
            return this.createSimulatedResponse(403, 'Forbidden', 'text/html', this.get403Simulation());
        } else if (url.includes('error') || url.includes('500')) {
            return this.createSimulatedResponse(500, 'Internal Server Error', 'text/html', this.get500Simulation());
        } else {
            // Default response for unknown URLs
            return this.createSimulatedResponse(200, 'OK', 'text/plain', `Response for: ${url}\n\nThis is a simulated response.`);
        }
    }

    /**
     * Create simulated response object
     */
    createSimulatedResponse(status, statusText, contentType, data) {
        return {
            status,
            statusText,
            ok: status >= 200 && status < 300,
            headers: new Map([
                ['Content-Type', contentType],
                ['Content-Length', data.length.toString()],
                ['Server', 'WebServer/1.0 (Simulation Mode)'],
                ['Date', new Date().toUTCString()],
                ['Cache-Control', 'no-cache'],
                ['Connection', 'keep-alive']
            ]),
            data
        };
    }

    /**
     * Process successful response
     */
    processResponse(response, data, duration, url) {
        const headers = {};

        if (response.headers instanceof Map) {
            // Simulation mode
            response.headers.forEach((value, key) => {
                headers[key] = value;
            });
        } else {
            // Real mode
            response.headers.forEach((value, key) => {
                headers[key] = value;
            });
        }

        return {
            success: true,
            status: response.status,
            statusText: response.statusText,
            headers,
            body: data,
            duration,
            url,
            timestamp: new Date().toISOString(),
            size: data ? data.length : 0
        };
    }

    /**
     * Process error response
     */
    processError(error, duration, url) {
        let errorMessage = 'Unknown error';
        let status = 0;

        if (error.name === 'AbortError') {
            errorMessage = 'Request timeout';
            status = 408;
        } else if (error.name === 'TypeError') {
            errorMessage = 'Network error';
            status = 0;
        } else {
            errorMessage = error.message || 'Request failed';
        }

        return {
            success: false,
            status,
            statusText: errorMessage,
            headers: {},
            body: `Error: ${errorMessage}`,
            duration,
            url,
            timestamp: new Date().toISOString(),
            size: 0,
            error: true
        };
    }

    /**
     * Show status indicator
     */
    showIndicator(type) {
        // Hide all indicators
        document.querySelectorAll('.indicator').forEach(indicator => {
            indicator.classList.add('hidden');
        });

        // Show specific indicator
        const indicator = document.getElementById(`${type}-indicator`);
        if (indicator) {
            indicator.classList.remove('hidden');

            // Auto-hide after delay (except loading)
            if (type !== 'loading') {
                setTimeout(() => {
                    indicator.classList.add('hidden');
                }, 3000);
            }
        }
    }

    /**
     * Hide all indicators
     */
    hideIndicators() {
        document.querySelectorAll('.indicator').forEach(indicator => {
            indicator.classList.add('hidden');
        });
    }

    // SIMULATION DATA METHODS
    getHomePageSimulation() {
        return `<!DOCTYPE html>
<html>
<head>
    <title>Webserver Home</title>
</head>
<body>
    <h1>🚀 HTTP Webserver 🚀</h1>
    <p>Welcome to the webserver homepage!</p>
    <p>This is a simulated response for testing purposes.</p>
</body>
</html>`;
    }

    getAboutPageSimulation() {
        return `<!DOCTYPE html>
<html>
<head>
    <title>About - Webserver</title>
</head>
<body>
    <h1>About This Project</h1>
    <p>HTTP/1.1 webserver project for 42 Barcelona.</p>
    <p>This is a simulated response for testing purposes.</p>
</body>
</html>`;
    }

    getStatusApiSimulation() {
        return JSON.stringify({
            status: "active",
            uptime: "2h 30m 45s",
            connections: 42,
            requests_total: 1337,
            requests_per_second: 12.5,
            memory_usage: "64MB",
            cpu_usage: "15%",
            server_version: "WebServer/1.0",
            mode: "simulation"
        });
    }

    getFilesApiSimulation() {
        return JSON.stringify({
            directory: "/files/",
            files: [
                { name: "index.html", size: 2048, type: "text/html", modified: "2025-11-04T10:30:00Z" },
                { name: "styles.css", size: 4096, type: "text/css", modified: "2025-11-04T09:15:00Z" },
                { name: "script.js", size: 1024, type: "text/javascript", modified: "2025-11-04T11:00:00Z" },
                { name: "image.jpg", size: 8192, type: "image/jpeg", modified: "2025-11-03T14:20:00Z" }
            ],
            total_files: 4,
            total_size: 15360
        });
    }

    get404Simulation() {
        return `<!DOCTYPE html>
<html>
<head>
    <title>404 - Not Found</title>
</head>
<body>
    <h1>404 - Page Not Found</h1>
    <p>The requested resource could not be found.</p>
    <p>This is a simulated 404 error for testing purposes.</p>
</body>
</html>`;
    }

    get403Simulation() {
        return `<!DOCTYPE html>
<html>
<head>
    <title>403 - Forbidden</title>
</head>
<body>
    <h1>403 - Forbidden</h1>
    <p>You don't have permission to access this resource.</p>
    <p>This is a simulated 403 error for testing purposes.</p>
</body>
</html>`;
    }

    get500Simulation() {
        return `<!DOCTYPE html>
<html>
<head>
    <title>500 - Internal Server Error</title>
</head>
<body>
    <h1>500 - Internal Server Error</h1>
    <p>The server encountered an internal error.</p>
    <p>This is a simulated 500 error for testing purposes.</p>
</body>
</html>`;
    }
}

// Create global instance
const getRequestHandler = new GetRequestHandler();
