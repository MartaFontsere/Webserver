#include "network/PollManager.hpp"
#include <algorithm>

/**
 * @file PollManager.cpp
 * @brief Wrapper for poll() system call - manages multiple file descriptors
 *
 * This module provides a C++ wrapper around the POSIX poll() system call,
 * allowing the server to monitor multiple file descriptors (sockets, pipes)
 * for I/O readiness in a single call.
 *
 * Key functionality:
 * - Add/remove file descriptors to monitor
 * - Update events to watch (POLLIN, POLLOUT)
 * - Wait for events with timeout
 * - Access resulting events after poll returns
 *
 * @note Uses std::vector<pollfd> for dynamic fd management
 * @see poll(2) man page for POSIX poll() details
 */

/**
 * @brief Default constructor
 *
 * Initializes an empty PollManager with no file descriptors to monitor.
 * The internal _pollFds vector is automatically initialized empty.
 */
PollManager::PollManager() {}

/**
 * @brief Destructor
 *
 * Cleans up the PollManager. No explicit cleanup needed because _pollFds
 * is a std::vector which handles its own memory management (RAII).
 *
 * @note Does not close any file descriptors - caller is responsible for that
 */
PollManager::~PollManager() {}

/**
 * @brief Adds a file descriptor to monitor
 *
 * Creates a new pollfd structure and adds it to the internal vector.
 * The fd will be monitored for the specified events on next wait() call.
 *
 * @param fd File descriptor to monitor (socket, pipe, etc.)
 * @param events Events to watch for (POLLIN, POLLOUT, or both ORed together)
 *
 * @note revents is initialized to 0 and will be set by poll()
 * @see POLLIN for read readiness, POLLOUT for write readiness
 */
void PollManager::addFd(int fd, short events) {
  struct pollfd pollFd;
  pollFd.fd = fd;
  pollFd.events = events;
  pollFd.revents = 0;
  _pollFds.push_back(pollFd);
}

/**
 * @brief Removes a file descriptor from monitoring
 *
 * Searches for the specified fd and removes it from the internal vector.
 * If fd is not found, no action is taken.
 *
 * @param fd File descriptor to stop monitoring
 *
 * @note Uses linear search O(n) - acceptable for typical server fd counts
 */
void PollManager::removeFd(int fd) {
  for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
       it != _pollFds.end(); ++it) {
    if (it->fd == fd) {
      _pollFds.erase(it);
      return;
    }
  }
}

/**
 * @brief Updates the events to watch for a file descriptor
 *
 * Changes which events to monitor for an existing fd. Used to enable/disable
 * POLLOUT when there is data to send.
 *
 * @param fd File descriptor to update
 * @param events New events to watch for
 *
 * @note No effect if fd is not in the poll set
 */
void PollManager::updateEvents(int fd, short events) {
  for (size_t i = 0; i < _pollFds.size(); ++i) {
    if (_pollFds[i].fd == fd) {
      _pollFds[i].events = events;
      return;
    }
  }
}

/**
 * @brief Updates events by vector index
 *
 * More efficient than updateEvents() when the index is already known.
 * Used during the poll loop iteration.
 *
 * @param index Index in the _pollFds vector
 * @param events New events to watch for
 *
 * @note Bounds-checked - no effect if index is out of range
 */
void PollManager::updateEventsByIndex(size_t index, short events) {
  if (index < _pollFds.size()) {
    _pollFds[index].events = events;
  }
}

/**
 * @brief Waits for events on monitored file descriptors
 *
 * Calls poll() to block until one or more fds have events ready,
 * a timeout occurs, or an error happens.
 *
 * @param timeoutMs Timeout in milliseconds (-1 = infinite, 0 = immediate)
 * @return Number of fds with events ready, 0 on timeout, -1 on error
 *
 * @note After this call, use getRevents() to check which events occurred
 * @see poll(2) for complete semantics
 */
int PollManager::wait(int timeoutMs) {
  return poll(_pollFds.data(), _pollFds.size(), timeoutMs);
}

/**
 * @brief Returns the complete pollfd vector
 *
 * Provides read-only access to the internal pollfd array.
 * Used for iterating over all monitored fds.
 *
 * @return Const reference to the vector of pollfd structures
 */
const std::vector<struct pollfd> &PollManager::getPollFds() const {
  return _pollFds;
}

/**
 * @brief Gets the events that occurred for a specific index
 *
 * Returns the revents field set by poll() for the fd at the given index.
 * Check with POLLIN, POLLOUT, POLLERR, POLLHUP masks.
 *
 * @param index Index in the _pollFds vector
 * @return Events that occurred (revents), or 0 if index out of range
 */
short PollManager::getRevents(size_t index) const {
  if (index < _pollFds.size())
    return _pollFds[index].revents;
  return 0;
}

/**
 * @brief Gets the file descriptor at a specific index
 *
 * @param index Index in the _pollFds vector
 * @return File descriptor, or -1 if index out of range
 */
int PollManager::getFd(size_t index) const {
  if (index < _pollFds.size())
    return _pollFds[index].fd;
  return -1;
}

/**
 * @brief Returns the number of monitored file descriptors
 *
 * @return Current count of fds being monitored
 */
size_t PollManager::getSize() const { return _pollFds.size(); }

/**
 * @brief Removes all file descriptors from monitoring
 *
 * Clears the internal vector. Used during server shutdown.
 */
void PollManager::clear() { _pollFds.clear(); }
