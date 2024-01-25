#pragma once

#include <memory>

namespace etcetera {

inline int64_t modulo(int64_t a, int64_t b) {
  return a >= 0 ? a % b : (b - abs(a % b)) % b;
}

template <typename T> inline std::shared_ptr<T> lock(std::weak_ptr<T> ptr) {
  if (auto ret = ptr.lock()) {
    return ret;
  }
  throw std::runtime_error("Failed to lock weak_ptr");
}

} // namespace etcetera
