#pragma once

#include "utfconv.hpp"
#include <iostream>

#include <any>
#include <cassert>
#include <functional>
#include <istream>
#include <memory>
#include <tsl/ordered_map.h>

#include "helpers.hpp"

#include <pugixml.hpp>
#include <spdlog/spdlog.h>

#include <cpptrace/cpptrace.hpp>

namespace etcetera {

class Base : public std::enable_shared_from_this<Base> {
protected:
  std::type_info const &type_ = typeid(Base);

  std::string name;
  size_t idx;
  std::weak_ptr<Base> parent;

  struct PrivateBase {};

public:
  void set_parent(std::weak_ptr<Base> parent) { this->parent = parent; }
  void set_name(std::string name) { this->name = name; }
  void set_idx(size_t idx) { this->idx = idx; }

  virtual std::vector<std::string> get_names() {
    return {name};
  }

  Base(PrivateBase) {}
  virtual std::any parse(std::istream &stream) = 0;
  virtual void build(std::ostream &stream) = 0;

  /*
   * Returns true if the object is a struct
   * */
  virtual bool is_struct() { return false; }
  /*
   * Returns true if the object is an array
   * */
  virtual bool is_array() { return false; }
  /*
   * Returns true if the object is a simple type (i.e. not a struct or array)
   * */
  virtual bool is_simple_type() { return false; }
  /*
   * Returns true, if the object is a pointer type.
   *
   * If true get_ptr_offset and get_ptr_size need to be implemented.
   * */
  virtual bool is_pointer_type() { return false; }

  /*
   * Returns the size of the object in bytes
   * */
  virtual size_t get_size() = 0;

  /*
   * Only for Pointers, returns the offset of the pointer data.
   * */
  virtual size_t get_ptr_offset(std::weak_ptr<Base>) {
    throw cpptrace::runtime_error("Not implemented");
  }
  /*
   * Only for Pointers, returns the size of the pointer data.
   * */
  virtual size_t get_ptr_size(std::weak_ptr<Base>) {
    throw cpptrace::runtime_error("Not implemented");
  }

  virtual size_t length() {
    throw cpptrace::runtime_error("length: Not implemented name: " + name +
                             " idx: " + std::to_string(idx));
  }

  /*
   * Returns the child field itself. Used for modifying the fields in test
   * cases.
   * */
  virtual std::weak_ptr<Base> get_field(std::string key) {
    throw cpptrace::runtime_error("get_field(" + key + "): Not implemented name: " +
                             name + " idx: " + std::to_string(idx));
  };
  template <typename T> std::weak_ptr<T> get_field(std::string key) {
    return static_pointer_cast<T>(get_field(key));
  }
  virtual std::weak_ptr<Base> get_field(size_t key) {
    throw cpptrace::runtime_error("get_field(" + std::to_string(key) +
                             "): Not implemented name: " + name +
                             " idx: " + std::to_string(idx));
  };
  template <typename T> std::weak_ptr<T> get_field(size_t key) {
    return static_pointer_cast<T>(get_field(key));
  }
  template <typename T, typename K, typename... Ts>
  std::weak_ptr<T> get_field(K key, Ts &&...args) {
    std::weak_ptr<Base> field = get_field(key);
    if constexpr (sizeof...(Ts) == 0) {
      return static_pointer_cast<T>(lock(field));
    } else {
      return lock(field)->get_field<T>(args...);
    }
  }

  // returns all the data
  virtual std::any get() = 0;
  template <typename T> T get() { return std::any_cast<T>(get()); }
  virtual std::any get(std::string) {
    throw cpptrace::runtime_error("get: Not implemented name: " + name +
                             " idx: " + std::to_string(idx));
  };
  template <typename T> T get(std::string key) {
    return std::any_cast<T>(get(key));
  }
  virtual std::any get(size_t) {
    throw cpptrace::runtime_error("get: Not implemented name: " + name +
                             " idx: " + std::to_string(idx));
  };
  template <typename T> T get(size_t key) { return std::any_cast<T>(get(key)); }
  template <typename T, typename K, typename... Ts> T get(K key, Ts &&...args) {
    std::weak_ptr<Base> field = get_field(key);
    // spdlog::warn("get: {} {} {}", key, lock(field)->name, lock(field)->idx);
    return lock(field)->get<T>(args...);
  }

  virtual std::any get_parsed() { return get(); };
  template <typename T> T get_parsed() {
    return std::any_cast<T>(get_parsed());
  }
  virtual std::any get_parsed(std::string key) { return get(key); };
  template <typename T> T get_parsed(std::string key) {
    return std::any_cast<T>(get_parsed(key));
  }
  virtual std::any get_parsed(size_t) {
    throw cpptrace::runtime_error("Not implemented");
  };
  template <typename T> T get_parsed(size_t key) {
    return std::any_cast<T>(get_parsed(key));
  }
  template <typename T, typename K, typename... Ts>
  T get_parsed(K key, Ts &&...args) {
    std::weak_ptr<Base> field = get_field(key);
    return lock(field)->get_parsed<T>(args...);
  }

  virtual size_t get_offset() {
    if (parent.lock()) {
      if (lock(parent)->is_array()) {
        return lock(parent)->get_offset(idx);
      } else if (lock(parent)->is_struct()) {
        return lock(parent)->get_offset(name);
      }
      throw cpptrace::runtime_error("Base: parent is not array or struct!");
    }
    return 0;
  }
  virtual size_t get_offset(std::string) {
    throw cpptrace::runtime_error("Not implemented");
  };
  virtual size_t get_offset(size_t) {
    throw cpptrace::runtime_error("Not implemented");
  }
  template <typename K, typename K2, typename... Ts>
  size_t get_offset(K key, K2 key2, Ts &&...args) {
    std::weak_ptr<Base> field = get_field(key);
    return lock(field)->get_offset(key2, args...);
  }

  virtual void set(std::any) { throw cpptrace::runtime_error("Not implemented"); }

  virtual void parse_xml(pugi::xml_node const &, std::string, bool) {
    throw cpptrace::runtime_error("Not implemented");
  }

  virtual pugi::xml_node build_xml(pugi::xml_node &, std::string) {
    throw cpptrace::runtime_error("Not implemented");
  }
};

template <typename T> class Const : public Base {
public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;
  T value;
  Const(T val, PrivateBase) : Base(PrivateBase()), value(val) {}
  static std::shared_ptr<Const> create(T val) {
    return std::make_shared<Const>(val, PrivateBase());
  }

  std::any get() override { return value; }

  bool is_simple_type() override { return true; }

  size_t get_size() override { return sizeof(T); }

  std::any parse(std::istream &stream) override {
    stream.read(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
  }

  void build(std::ostream &stream) override {
    stream.write(reinterpret_cast<char *>(&value), sizeof(T));
  }

  void parse_xml(pugi::xml_node const &, std::string, bool) override {}

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string) override {
    return parent;
  }
};

class BytesConst : public Base {
protected:
  std::string value;

public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;
  BytesConst(std::string val, PrivateBase) : Base(PrivateBase()), value(val) {}
  static std::shared_ptr<BytesConst> create(const std::string &val) {
    return std::make_shared<BytesConst>(val, PrivateBase());
  }

  bool is_simple_type() override { return true; }

  std::any get() override { return value; }

  size_t get_size() override { return value.length(); }

  std::any parse(std::istream &stream) override {
    std::string tmp;
    tmp.resize(value.length());
    stream.read(reinterpret_cast<char *>(tmp.data()), tmp.length());
    if (tmp != value) {
      throw cpptrace::runtime_error("BytesConst @" + std::to_string(stream.tellg()) +
                               ": expected " + value + ", got " + tmp);
    }
    return value;
  }

  void build(std::ostream &stream) override {
    stream.write(value.data(), value.length());
  }

  void parse_xml(pugi::xml_node const &, std::string, bool) override {}

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string) override {
    return parent;
  }
};

class Bytes : public Base {
protected:
  typedef std::function<size_t(std::weak_ptr<Base>)> FSizeFn;
  FSizeFn size_fn;
  size_t size;

public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;
  std::vector<uint8_t> value;
  Bytes(PrivateBase, FSizeFn size_fn, size_t size)
      : Base(PrivateBase()), size_fn(size_fn), size(size) {
    value.resize(size);
  }
  static std::shared_ptr<Bytes> create(size_t s) {
    return std::make_shared<Bytes>(PrivateBase(), nullptr, s);
  }
  static std::shared_ptr<Bytes> create(FSizeFn size_fn) {
    return std::make_shared<Bytes>(PrivateBase(), size_fn, 0);
  }

  bool is_simple_type() override { return true; }

  std::any get() override { return value; }

  size_t get_size() override {
    if (size_fn) {
      size = size_fn(this->parent);
    }
    return size;
  }

  std::any parse(std::istream &stream) override {
    get_size();
    value.resize(size);

    spdlog::info("Bytes {:02X} {} read", (size_t)stream.tellg(), value.size());
    stream.read(reinterpret_cast<char *>(value.data()), value.size());

    std::string s;
    for (auto &c : value) {
      s += std::format("{:02X}", static_cast<uint8_t>(c));
    }
    return value;
  }

  void build(std::ostream &stream) override {
    assert(value.size() == size);
    spdlog::debug("Bytes {:02X} {} write", (size_t)stream.tellp(), value.size());
    stream.write(reinterpret_cast<char *>(value.data()), value.size());
  }

  void parse_xml(pugi::xml_node const &node, std::string name, bool) override {
    get_size();
    std::string attr = node.attribute(name.c_str()).as_string();
    if (attr.length() != size * 2) {
      throw cpptrace::runtime_error("Bytes: expected " + std::to_string(size * 2) +
                               " characters, got " +
                               std::to_string(attr.length()));
    }
    value.clear();
    value.resize(size);

    for (size_t i = 0; i < size; i++) {
      std::string tmp = attr.substr(i * 2, 2);
      value[i] = std::stoll(tmp, nullptr, 16);
      spdlog::debug("Bytes::parse_xml reading {} {} {}", i, tmp, value[i]);
    }
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    std::string s;
    for (auto &c : value) {
      s += std::format("{:02X}", static_cast<uint8_t>(c));
    }
    parent.append_attribute(name.c_str()) = s.c_str();
    return parent;
  }
};

template <typename T, std::endian Endianess = std::endian::native>
class Enum : public Base {
protected:
public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;

  T value;
  typedef std::pair<std::string, T> Field;
  tsl::ordered_map<std::string, T> fields;

  template <typename... Args>
  Enum(PrivateBase, Args &&...args) : Base(PrivateBase()) {
    (fields.emplace(std::get<0>(std::forward<Args>(args)),
                    std::get<1>(std::forward<Args>(args))),
     ...);
  }
  template <typename... Args>
  static std::shared_ptr<Enum> create(Args &&...args) {
    return std::make_shared<Enum>(PrivateBase(), args...);
  }

  bool is_simple_type() override { return true; }

  std::any get() override { return value; }

  size_t get_size() override { return sizeof(T); }

  std::any parse(std::istream &stream) override {
    stream.read(reinterpret_cast<char *>(&value), sizeof(value));
    if constexpr (Endianess != std::endian::native) {
      union {
        T value;
        char bytes[sizeof(T)];
      } bswap;
      bswap.value = value;
      for (size_t i = 0; i < sizeof(T) / 2; i++) {
        std::swap(bswap.bytes[i], bswap.bytes[sizeof(T) - i - 1]);
      }
      value = bswap.value;
    }
    return value;
  }

  void build(std::ostream &stream) override {
    auto val = value;
    if constexpr (Endianess != std::endian::native) {
      union {
        T value;
        char bytes[sizeof(T)];
      } bswap;
      bswap.value = value;
      for (size_t i = 0; i < sizeof(T) / 2; i++) {
        std::swap(bswap.bytes[i], bswap.bytes[sizeof(T) - i - 1]);
      }
      val = bswap.value;
    }
    stream.write(reinterpret_cast<char *>(&val), sizeof(val));
  }

  void parse_xml(pugi::xml_node const &node, std::string name, bool) override {
    std::string attr = node.attribute(name.c_str()).as_string();

    auto it = fields.find(attr);
    if (it == fields.end()) {
      value = static_cast<T>(std::stoi(attr));
      return;
    }
    value = it->second;
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    for (auto &[key, val] : fields) {
      if (val == value) {
        parent.append_attribute(name.c_str()).set_value(key.c_str());
        return parent;
      }
    }
    parent.append_attribute(name.c_str())
        .set_value(std::to_string(value).c_str());
    return parent;
  }
};

} // namespace etcetera
