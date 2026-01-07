#pragma once

#include <map>
#include <poll.h>
#include <vector>

/**
 * @brief Gestiona el vector de estructuras pollfd y la llamada al sistema
 * poll().
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

  int wait(int timeoutMs);

  const std::vector<struct pollfd> &getPollFds() const;
  short getRevents(size_t index) const;
  int getFd(size_t index) const;
  size_t getSize() const;

  void clear();
};
