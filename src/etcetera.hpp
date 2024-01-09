#pragma once

#include <any>
#include <cassert>
#include <functional>
#include <istream>
#include <map>
#include <memory>

#include <spdlog/spdlog.h>

namespace etcetera {

class Struct;
class Array;

class Base {
protected:
  std::type_info const &type_ = typeid(Base);

  Base *parent = nullptr;
  friend class Struct;
  friend class Array;

  void set_parent(Base *parent) { this->parent = parent; }

public:
  virtual std::any parse(std::iostream &stream) = 0;
  virtual void build(std::iostream &stream) = 0;

  virtual bool is_struct() { return false; }
  virtual bool is_array() { return false; }
  virtual bool is_simple_type() { return false; }

  virtual Base *get_field(std::string key) {
    throw std::runtime_error("Not implemented");
  };
  template <typename T> T *get_field(std::string key) {
    return dynamic_cast<T *>(get_field(key));
  }
  virtual Base *get_field(size_t key) {
    throw std::runtime_error("Not implemented");
  };
  template <typename T> T *get_field(size_t key) {
    return dynamic_cast<T *>(get_field(key));
  }
  template <typename T, typename K, typename... Ts>
  T *get_field(K key, Ts &&...args) {
    Base *field = get_field(key);
    if (field == nullptr) {
      throw std::runtime_error("nullptr field in Struct::get");
    }
    if constexpr (sizeof...(Ts) == 0) {
      return dynamic_cast<T *>(field);
    } else {
      return field->get_field<T>(args...);
    }
  }

  // returns all the data
  virtual std::any get() = 0;
  template <typename T> T get() { return std::any_cast<T>(get()); }
  virtual std::any get(std::string key) {
    throw std::runtime_error("Not implemented");
  };
  template <typename T> T get(std::string key) {
    return std::any_cast<T>(get(key));
  }
  virtual std::any get(size_t key) {
    throw std::runtime_error("Not implemented");
  };
  template <typename T> T get(size_t key) { return std::any_cast<T>(get(key)); }
  template <typename T, typename K, typename... Ts> T get(K key, Ts &&...args) {
    Base *field = get_field(key);
    if (field == nullptr) {
      throw std::runtime_error("nullptr field in Struct::get");
    }
    return field->get<T>(args...);
  }
};

typedef std::pair<std::string, Base *> Field;

template <typename T> class IntegralType : public Base {
public:
  using Base::get;
  using Base::get_field;
  T value = 0;
  std::any parse(std::iostream &stream) override {
    stream.read(reinterpret_cast<char *>(&value), sizeof(value));
    return value;
  }
  void build(std::iostream &stream) override {
    stream.write(reinterpret_cast<char *>(&value), sizeof(value));
  }
  std::any get() override { return value; }
  bool is_simple_type() override { return true; }
};
using Int8ul = IntegralType<int8_t>;
using Int16ul = IntegralType<int16_t>;
using Int32ul = IntegralType<int32_t>;
using Int64ul = IntegralType<int64_t>;
using UInt8ul = IntegralType<uint8_t>;
using UInt16ul = IntegralType<uint16_t>;
using UInt32ul = IntegralType<uint32_t>;
using UInt64ul = IntegralType<uint64_t>;

class Struct : public Base {
  std::map<std::string, Base *> fields;

public:
  using Base::get;
  using Base::get_field;
  template <typename... Args> Struct(Args &&...args) {
    (fields.emplace(std::get<0>(std::forward<Args>(args)),
                    std::get<1>(std::forward<Args>(args))),
     ...);

    for (auto &[key, field] : fields) {
      field->set_parent(this);
    }
  }

  std::any parse(std::iostream &stream) override {
    std::map<std::string, std::any> obj;
    for (auto &[key, field] : fields) {
      std::any value = field->parse(stream);
      obj.emplace(key, value);
    }
    return obj;
  }

  void build(std::iostream &stream) override {
    for (auto &[key, field] : fields) {
      field->build(stream);
    }
  }

  bool is_struct() override { return true; }

  std::any get() override { throw std::runtime_error("Not implemented"); }
  std::any get(std::string key) { return fields[key]->get(); }

  Base *get_field(std::string key) override {
    if (key == "_") {
      return this->parent;
    }
    return fields[key];
  }
};

class Array : public Base {
protected:
  size_t size;
  std::function<size_t(Base *)> size_fn;
  std::function<Base *()> type_constructor;
  std::vector<Base *> data;

public:
  using Base::get;
  using Base::get_field;
  Array(size_t size, std::function<Base *()> type_constructor)
      : size(size), type_constructor(type_constructor) {}
  Array(std::function<size_t(Base *)> size_fn,
        std::function<Base *()> type_constructor)
      : size_fn(size_fn), type_constructor(type_constructor) {}
  std::any parse(std::iostream &stream) override {
    if (size_fn) {
      size = size_fn(this);
    }
    data.clear();
    for (size_t i = 0; i < size; i++) {
      auto obj = type_constructor();
      obj->set_parent(this);
      data.push_back(obj);
      data.back()->parse(stream);
    }
    return data;
  }
  void build(std::iostream &stream) override {}

  std::any get() override { throw std::runtime_error("Not implemented"); }
  std::any get(size_t key) override { return data[key]->get(); }

  Base *get_field(size_t key) override { return data[key]; }
};

#define ARR_ITEM(X) []() { return new X; }

} // namespace etcetera
