#include "network/PollManager.hpp"
#include <algorithm>

PollManager::PollManager() {}

PollManager::~PollManager() {}

void PollManager::addFd(int fd, short events) {
  // pollfd: Estructura que contiene el descriptor de archivo (fd),
  // los eventos que queremos vigilar (events) y los que ocurrieron (revents).
  struct pollfd pollFd;
  pollFd.fd = fd;
  pollFd.events = events;
  pollFd.revents = 0;
  _pollFds.push_back(pollFd);
}

void PollManager::removeFd(int fd) {
  for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
       it != _pollFds.end(); ++it) {
    if (it->fd == fd) {
      _pollFds.erase(it);
      return;
    }
  }
}

void PollManager::updateEvents(int fd, short events) {
  for (size_t i = 0; i < _pollFds.size(); ++i) {
    if (_pollFds[i].fd == fd) {
      _pollFds[i].events = events;
      return;
    }
  }
}

int PollManager::wait(int timeoutMs) {
  // poll(): Vigilante que observa múltiples descriptores a la vez.
  // Devuelve el número de descriptores que tienen eventos listos, 0 si hubo
  // timeout, o -1 en error.
  return poll(_pollFds.data(), _pollFds.size(), timeoutMs);
}

const std::vector<struct pollfd> &PollManager::getPollFds() const {
  return _pollFds;
}

short PollManager::getRevents(size_t index) const {
  if (index < _pollFds.size())
    return _pollFds[index].revents;
  return 0;
}

int PollManager::getFd(size_t index) const {
  if (index < _pollFds.size())
    return _pollFds[index].fd;
  return -1;
}

size_t PollManager::getSize() const { return _pollFds.size(); }

void PollManager::clear() { _pollFds.clear(); }
