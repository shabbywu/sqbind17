#pragma once
#include <stdexcept>
#include <string>

namespace sqbind17 {
class value_error : public std::runtime_error {
  public:
    value_error(std::string &message) : std::runtime_error(message) {};
    value_error(std::string &&message) : std::runtime_error(message) {};
};

class key_error : public std::runtime_error {
  public:
    key_error(std::string &message) : std::runtime_error(message) {};
    key_error(std::string &&message) : std::runtime_error(message) {};
};

class index_error : public std::runtime_error {
  public:
    index_error(std::string &message) : std::runtime_error(message) {};
    index_error(std::string &&message) : std::runtime_error(message) {};
};

class stop_iteration : public std::exception {
  public:
    stop_iteration() : std::exception() {};
};
} // namespace sqbind17
