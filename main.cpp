#include "../includes/config/ConfigBuilder.hpp"
#include "../includes/config_parser/parser/UtilsConfigParser.hpp"
#include "core/Server.hpp"
#include <csignal>

/**
 * @file main.cpp
 * @brief Webserver entry point - Configuration loading and signal handling
 *
 * This is the main entry point for the nginx-style web server.
 * It handles:
 * - Command line argument parsing (config file path)
 * - Configuration file parsing and validation
 * - Signal handling for graceful shutdown (SIGINT, SIGTERM)
 * - Server initialization and main loop execution
 *
 * Usage:
 *   ./webServer              # Uses default config (tests/configs/default.conf)
 *   ./webServer config.conf  # Uses specified config file
 *
 * Signal handling:
 * - SIGINT (Ctrl+C): Triggers graceful shutdown
 * - SIGTERM (kill): Triggers graceful shutdown
 *
 * The server shuts down cleanly by setting g_running = false,
 * which breaks the poll() loop and allows proper resource cleanup.
 */

/**
 * @brief Global flag for graceful shutdown
 *
 * This variable is set to false by the signal handler to indicate
 * that the server should stop its main loop and shut down cleanly.
 *
 * Why global? The signal() API requires handlers with signature void(*)(int),
 * so there's no way to pass context. A global volatile sig_atomic_t is the
 * POSIX-standard way to communicate between signal handlers and main code.
 *
 * Thread safety:
 * - sig_atomic_t guarantees atomic read/write operations
 * - volatile prevents compiler optimization that could cache the value
 */
volatile sig_atomic_t g_running = true;

/**
 * @brief Signal handler for SIGINT and SIGTERM
 *
 * Called by the OS when the process receives a termination signal.
 * Sets g_running to false to trigger graceful shutdown.
 *
 * What this does NOT do:
 * - Does not call exit()
 * - Does not close sockets directly
 * - Does not free memory
 *
 * The actual cleanup happens when the main loop exits and destructors run.
 *
 * @param signum Signal number received (SIGINT=2, SIGTERM=15)
 */
void signalHandler(int signum) {
  if (signum == SIGINT)
    std::cout << "\n[Signal] SIGINT (Ctrl+C) received" << std::endl;
  else if (signum == SIGTERM)
    std::cout << "\n[Signal] SIGTERM received" << std::endl;

  g_running = false;
  std::cout << "[Info] 🛑 Shutting down gracefully..." << std::endl;
}

/**
 * @brief Main entry point
 *
 * Execution flow:
 * 1. Parse command line args (use default config if none provided)
 * 2. Parse and validate configuration file
 * 3. Build ServerConfig objects from parsed config
 * 4. Register signal handlers for graceful shutdown
 * 5. Initialize server sockets
 * 6. Run main poll() loop until shutdown signal
 * 7. Clean up resources and exit
 *
 * @param argc Argument count
 * @param argv Argument vector (argv[1] = config file path)
 * @return 0 on success, 1 on error
 */
int main(int argc, char **argv) {
  // Step 1: Get config file path
  std::string configPath;
  if (argc == 1)
    configPath = "tests/configs/default.conf";
  else
    configPath = argv[1];

  try {
    // Step 2-3: Parse config and build server configurations
    BlockParser root = parseAndValidateConfig(configPath);
    ConfigBuilder builder;
    std::vector<ServerConfig> servConfigsList =
        builder.buildFromBlockParser(root);

    std::cout << "[Info] ✅ Configuration loaded: " << servConfigsList.size()
              << " server(s)" << std::endl;

    // Step 4: Create server and register signal handlers
    Server server(servConfigsList);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Step 5: Initialize listening sockets
    if (!server.init()) {
      return 1;
    }

    // Step 6: Run main event loop
    server.run();

  } catch (std::exception &e) {
    std::cerr << "❌ [Error] Config error: " << e.what() << std::endl;
    return 1;
  }

  // Step 7: Cleanup happens automatically via RAII destructors
  return 0;
}