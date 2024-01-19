#pragma once

#include <memory>

namespace etcetera {

template <typename T> inline std::shared_ptr<T> lock(std::weak_ptr<T> ptr) {
  if (auto ret = ptr.lock()) {
    return ret;
  }
  throw std::runtime_error("Failed to lock weak_ptr");
}

} // namespace etcetera
