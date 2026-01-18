#pragma once

#include <cstddef>
#include <poll.h>
#include <vector>

/**
 * @brief Manages pollfd vector and poll() system call
 */
class PollManager {
private:
  std::vector<struct pollfd> _pollFds;

public:
  PollManager();
  ~PollManager();

  void addFd(int fd, short events);
  void removeFd(int fd);
  void updateEvents(int fd, short events);
  void updateEventsByIndex(size_t index, short events);

  /** @brief Block until events occur or timeout (returns event count) */
  int wait(int timeoutMs);

  const std::vector<struct pollfd> &getPollFds() const;
  short getRevents(size_t index) const;
  int getFd(size_t index) const;
  size_t getSize() const;
  void clear();
};
