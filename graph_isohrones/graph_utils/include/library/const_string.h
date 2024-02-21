#pragma once

#include <atomic>
#include <cstring>
#include <nonstd/string_view.hpp>

namespace lib {

struct const_string {

  const_string(const char *str) {
    size_ = std::strlen(str);
    char *ptr = new char[size_ + 1 + sizeof(std::atomic<int>)];
    str_ = ptr;
    std::memcpy(ptr + sizeof(std::atomic<int>), str, size_ + 1);

    new (ptr) std::atomic<int>();
    counter().store(1, std::memory_order_relaxed);
  }

  ~const_string() { destroy(); }

  const_string(const const_string &other) {
    str_ = other.str_;
    size_ = other.size_;
    if (counter().load(std::memory_order_relaxed) >= 0) {
      counter().fetch_add(1, std::memory_order_relaxed);
    }
  }

  const_string &operator=(const const_string &) = delete;
  const_string &operator=(const_string &&) = delete;

  void emplace(const char *str) {
    destroy();
    new (this) const_string(str);
  }

  void emplace(const const_string &other) {
    destroy();
    new (this) const_string(other);
  }

  operator nonstd::string_view() const {
    return nonstd::string_view(c_str(), size());
  }

  constexpr const char *c_str() const {
    return str_ + sizeof(std::atomic<int>);
  }

  constexpr std::size_t size() const { return size_; }

  constexpr char operator[](std::size_t i) const { return c_str()[i]; }

private:
  void destroy() {
    if (counter().load(std::memory_order_relaxed) < 0) {
      return;
    }
    auto cnt = counter().fetch_sub(1, std::memory_order_relaxed);
    if (cnt == 1) {
      delete[] str_;
    }
  }

  std::atomic<int> &counter() const {
    return *reinterpret_cast<std::atomic<int> *>(
        reinterpret_cast<std::uintptr_t>(str_));
  }

  const char *str_;
  std::size_t size_;
};

inline bool operator==(const const_string &lhs, const const_string &rhs) {
  return std::strcmp(lhs.c_str(), rhs.c_str()) == 0;
}

inline bool operator!=(const const_string &lhs, const const_string &rhs) {
  return !(lhs == rhs);
}

} // namespace lib

template <> struct std::hash<lib::const_string> {
  std::size_t operator()(const lib::const_string &str) const {
    return std::hash<nonstd::string_view>{}(
        nonstd::string_view(str.c_str(), str.size()));
  }
};