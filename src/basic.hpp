#pragma once

#include "utfconv.hpp"
#include <iostream>

#include <any>
#include <cassert>
#include <functional>
#include <istream>
#include <map>
#include <memory>

#include <pugixml.hpp>
#include <spdlog/spdlog.h>

namespace etcetera {

class Struct;
class Array;
class LazyBound;

class Base : public std::enable_shared_from_this<Base> {
protected:
  std::type_info const &type_ = typeid(Base);

  std::weak_ptr<Base> parent;
  friend class Struct;
  friend class Array;
  friend class LazyBound;

  struct PrivateBase {};
  void set_parent(std::weak_ptr<Base> parent) { this->parent = parent; }

public:
  Base(PrivateBase) {}
  virtual std::any parse(std::iostream &stream) = 0;
  virtual void build(std::iostream &stream) = 0;

  virtual bool is_struct() { return false; }
  virtual bool is_array() { return false; }
  virtual bool is_simple_type() { return false; }
  virtual size_t get_size(std::weak_ptr<Base> c) = 0;

  virtual std::weak_ptr<Base> get_field(std::string) {
    throw std::runtime_error("Not implemented");
  };
  template <typename T> std::weak_ptr<T> get_field(std::string key) {
    return static_pointer_cast<T>(get_field(key));
  }
  virtual std::weak_ptr<Base> get_field(size_t) {
    throw std::runtime_error("Not implemented");
  };
  template <typename T> std::weak_ptr<T> get_field(size_t key) {
    return static_pointer_cast<T>(get_field(key));
  }
  template <typename T, typename K, typename... Ts>
  std::weak_ptr<T> get_field(K key, Ts &&...args) {
    std::weak_ptr<Base> field = get_field(key);
    if constexpr (sizeof...(Ts) == 0) {
      return static_pointer_cast<T>(field.lock());
    } else {
      return field.lock()->get_field<T>(args...);
    }
  }

  // returns all the data
  virtual std::any get() = 0;
  template <typename T> T get() { return std::any_cast<T>(get()); }
  virtual std::any get(std::string) {
    throw std::runtime_error("Not implemented");
  };
  template <typename T> T get(std::string key) {
    return std::any_cast<T>(get(key));
  }
  virtual std::any get(size_t) { throw std::runtime_error("Not implemented"); };
  template <typename T> T get(size_t key) { return std::any_cast<T>(get(key)); }
  template <typename T, typename K, typename... Ts> T get(K key, Ts &&...args) {
    std::weak_ptr<Base> field = get_field(key);
    return field.lock()->get<T>(args...);
  }

  virtual std::any get_parsed() { return get(); };
  template <typename T> T get_parsed() { return std::any_cast<T>(get()); }
  virtual std::any get_parsed(std::string key) { return get(key); };
  template <typename T> T get_parsed(std::string key) {
    return std::any_cast<T>(get_parsed(key));
  }
  virtual std::any get_parsed(size_t) {
    throw std::runtime_error("Not implemented");
  };
  template <typename T> T get_parsed(size_t key) {
    return std::any_cast<T>(get(key));
  }
  template <typename T, typename K, typename... Ts>
  T get_parsed(K key, Ts &&...args) {
    std::weak_ptr<Base> field = get_field(key);
    return field.lock()->get<T>(args...);
  }

  virtual void set(std::any) { throw std::runtime_error("Not implemented"); }

  virtual void parse_xml(pugi::xml_node const &, std::string) {
    throw std::runtime_error("Not implemented");
  }

  virtual pugi::xml_node build_xml(pugi::xml_node &, std::string) {
    throw std::runtime_error("Not implemented");
  }
};

typedef std::pair<std::string, std::shared_ptr<Base>> Field;

class Struct : public Base {
  std::map<std::string, std::shared_ptr<Base>> fields;

public:
  using Base::get;
  using Base::get_field;
  template <typename... Args>
  Struct(PrivateBase, Args &&...args) : Base(PrivateBase()) {
    (fields.emplace(std::get<0>(std::forward<Args>(args)),
                    std::get<1>(std::forward<Args>(args))),
     ...);
  }
  template <typename... Args>
  static std::shared_ptr<Struct> create(Args &&...args) {
    auto ret = std::make_shared<Struct>(PrivateBase(), args...);

    for (auto &[key, field] : ret->fields) {
      field->set_parent(ret);
    }

    return ret;
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

  size_t get_size(std::weak_ptr<Base>) override {
    size_t size = 0;
    for (auto &[key, field] : fields) {
      size += field->get_size(weak_from_this());
    }
    return size;
  }

  std::any get() override { throw std::runtime_error("Not implemented"); }
  std::any get(std::string key) { return fields[key]->get(); }

  std::weak_ptr<Base> get_field(std::string key) override {
    if (key == "_") {
      return this->parent;
    }
    return fields[key];
  }

  void parse_xml(pugi::xml_node const &node, std::string name) override {
    auto s = node.child(name.c_str());
    for (auto &[key, field] : fields) {
      field->parse_xml(s, key);
    }
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    auto s = parent.append_child(name.c_str());
    for (auto &[key, field] : fields) {
      field->build_xml(s, key);
    }
    return s;
  }
};

class Array : public Base {
protected:
  size_t size;
  typedef std::function<size_t(std::weak_ptr<Base>)> FSizeFn;
  typedef std::function<std::shared_ptr<Base>()> FTypeFn;
  FSizeFn size_fn;
  FTypeFn type_constructor;
  std::vector<std::shared_ptr<Base>> data;

public:
  using Base::get;
  using Base::get_field;
  Array(PrivateBase, size_t size, FTypeFn m_type_constructor)
      : Base(PrivateBase()), size(size), type_constructor(m_type_constructor) {}
  static std::shared_ptr<Array> create(size_t size,
                                       FTypeFn m_type_constructor) {
    return std::make_shared<Array>(PrivateBase(), size, m_type_constructor);
  }

  Array(PrivateBase, FSizeFn size_fn, FTypeFn m_type_constructor)
      : Base(PrivateBase()), size_fn(size_fn),
        type_constructor(m_type_constructor) {}
  static std::shared_ptr<Array> create(FSizeFn size_fn,
                                       FTypeFn m_type_constructor) {
    return std::make_shared<Array>(PrivateBase(), size_fn, m_type_constructor);
  }

  size_t get_size(std::weak_ptr<Base> c) override {
    size_t s = 0;
    for (auto &obj : data) {
      s += obj->get_size(c);
    }
    return s;
  }

  std::any parse(std::iostream &stream) override {
    if (size_fn) {
      size = size_fn(weak_from_this());
    }
    data.clear();
    for (size_t i = 0; i < size; i++) {
      auto obj = type_constructor();
      obj->set_parent(weak_from_this());
      data.push_back(obj);
      data.back()->parse(stream);
    }
    return data;
  }

  void build(std::iostream &stream) override {
    for (auto &obj : data) {
      obj->build(stream);
    }
  }

  std::any get() override { throw std::runtime_error("Not implemented"); }
  std::any get(size_t key) override { return data[key]->get(); }

  std::weak_ptr<Base> get_field(size_t key) override { return data[key]; }

  bool is_array() override { return true; }

  void init_fields(size_t n = 0, bool clear = false) {
    data.clear();
    if (clear) {
      return;
    }
    if (n == 0) {
      n = size;
    }
    for (size_t i = 0; i < n; i++) {
      auto obj = type_constructor();
      obj->set_parent(weak_from_this());
      data.push_back(obj);
    }
  }

  void parse_xml(pugi::xml_node const &node, std::string name) override {
    for (auto &obj : data) {
      obj->parse_xml(node, name);
    }
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    for (auto &obj : data) {
      obj->build_xml(parent, name);
    }
    return parent;
  }
};

template <typename T> class Const : public Base {
public:
  using Base::get;
  using Base::get_field;
  T value;
  Const(T val, PrivateBase) : Base(PrivateBase()), value(val) {}
  static std::shared_ptr<Const> create(T val) {
    return std::make_shared<Const>(val, PrivateBase());
  }

  std::any get() override { return value; }

  size_t get_size(std::weak_ptr<Base>) override { return sizeof(T); }

  std::any parse(std::iostream &stream) override {
    stream.read(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
  }

  void build(std::iostream &stream) override {
    stream.write(reinterpret_cast<char *>(&value), sizeof(T));
  }

  void parse_xml(pugi::xml_node const &, std::string) override {}

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
  BytesConst(std::string val, PrivateBase) : Base(PrivateBase()), value(val) {}
  static std::shared_ptr<BytesConst> create(std::string val) {
    return std::make_shared<BytesConst>(val, PrivateBase());
  }

  std::any get() override { return value; }

  size_t get_size(std::weak_ptr<Base>) override { return value.length(); }

  std::any parse(std::iostream &stream) override {
    std::string tmp;
    tmp.resize(value.length());
    stream.read(reinterpret_cast<char *>(&value), tmp.length());
    if (tmp != value) {
      throw std::runtime_error("BytesConst: expected " + value + ", got " +
                               tmp);
    }
    return value;
  }

  void build(std::iostream &stream) override {
    stream.write(value.data(), value.length());
  }
};

class Bytes : public Base {
protected:
  size_t size;

public:
  using Base::get;
  using Base::get_field;
  std::vector<uint8_t> value;
  Bytes(size_t s, PrivateBase) : Base(PrivateBase()), size(s) {
    value.resize(s);
  }
  static std::shared_ptr<Bytes> create(size_t s) {
    return std::make_shared<Bytes>(s, PrivateBase());
  }

  bool is_simple_type() override { return true; }

  std::any get() override { return value; }

  size_t get_size(std::weak_ptr<Base>) override { return size; }

  std::any parse(std::iostream &stream) override {
    value.resize(size);
    stream.read(reinterpret_cast<char *>(value.data()), value.size());
    return value;
  }

  void build(std::iostream &stream) override {
    stream.write(reinterpret_cast<char *>(value.data()), value.size());
  }

  void parse_xml(pugi::xml_node const &node, std::string name) override {}

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    std::string s;
    for (auto &c : value) {
      s += std::format("{}", c);
    }
    parent.append_attribute(name.c_str()) = s.c_str();
    return parent;
  }
};

} // namespace etcetera
