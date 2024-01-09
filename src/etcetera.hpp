#pragma once

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

class Base : public std::enable_shared_from_this<Base> {
protected:
  std::type_info const &type_ = typeid(Base);

  std::weak_ptr<Base> parent;
  friend class Struct;
  friend class Array;

  struct PrivateBase {};
  void set_parent(std::weak_ptr<Base> parent) { this->parent = parent; }

public:
  Base(PrivateBase) {}
  virtual std::any parse(std::iostream &stream) = 0;
  virtual void build(std::iostream &stream) = 0;

  virtual bool is_struct() { return false; }
  virtual bool is_array() { return false; }
  virtual bool is_simple_type() { return false; }

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

  virtual std::any get_parsed() = 0;
  template <typename T> T get_parsed() { return std::any_cast<T>(get()); }
  virtual std::any get_parsed(std::string) {
    throw std::runtime_error("Not implemented");
  };
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

template <typename TIntType> class IntegralType : public Base {
public:
  using Base::get;
  using Base::get_field;
  TIntType value = 0;
  IntegralType(PrivateBase) : Base(PrivateBase()) {}
  static std::shared_ptr<IntegralType> create() {
    return std::make_shared<IntegralType>(PrivateBase());
  }
  std::any parse(std::iostream &stream) override {
    stream.read(reinterpret_cast<char *>(&value), sizeof(value));
    return value;
  }
  void build(std::iostream &stream) override {
    stream.write(reinterpret_cast<char *>(&value), sizeof(value));
  }
  std::any get() override { return value; }
  std::any get_parsed() override { return value; }
  void set(std::any value) override {
    this->value = std::any_cast<TIntType>(value);
  }
  bool is_simple_type() override { return true; }

  void parse_xml(pugi::xml_node const &node, std::string name) override {
    auto s = node.attribute(name.c_str());
    value = s.as_int();
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    parent.append_attribute(name.c_str()) = value;
    return parent;
  }
};
using Int8ul = IntegralType<int8_t>;
using Int16ul = IntegralType<int16_t>;
using Int32ul = IntegralType<int32_t>;
using Int64ul = IntegralType<int64_t>;
using UInt8ul = IntegralType<uint8_t>;
using UInt16ul = IntegralType<uint16_t>;
using UInt32ul = IntegralType<uint32_t>;
using UInt64ul = IntegralType<uint64_t>;

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

  std::any get() override { throw std::runtime_error("Not implemented"); }
  std::any get(std::string key) { return fields[key]->get(); }
  std::any get_parsed() override {
    throw std::runtime_error("Not implemented");
  }
  std::any get_parsed(std::string key) { return fields[key]->get(); }

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
  std::function<size_t(std::weak_ptr<Base>)> size_fn;
  std::function<std::shared_ptr<Base>()> type_constructor;
  std::vector<std::shared_ptr<Base>> data;

public:
  using Base::get;
  using Base::get_field;
  Array(PrivateBase, size_t size,
        std::function<std::shared_ptr<Base>()> m_type_constructor)
      : Base(PrivateBase()), size(size), type_constructor(m_type_constructor) {}
  static std::shared_ptr<Array>
  create(size_t size,
         std::function<std::shared_ptr<Base>()> m_type_constructor) {
    return std::make_shared<Array>(PrivateBase(), size, m_type_constructor);
  }

  Array(PrivateBase, std::function<size_t(std::weak_ptr<Base>)> size_fn,
        std::function<std::shared_ptr<Base>()> m_type_constructor)
      : Base(PrivateBase()), size_fn(size_fn),
        type_constructor(m_type_constructor) {}
  static std::shared_ptr<Array>
  create(std::function<size_t(std::weak_ptr<Base>)> size_fn,
         std::function<std::shared_ptr<Base>()> m_type_constructor) {
    return std::make_shared<Array>(PrivateBase(), size_fn, m_type_constructor);
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
  std::any get_parsed() override {
    throw std::runtime_error("Not implemented");
  }
  std::any get_parsed(size_t key) override { return data[key]->get(); }

  std::weak_ptr<Base> get_field(size_t key) override { return data[key]; }
};

#define FIELD(A, B) etcetera::Field(A, etcetera::B::create())
#define ARRAY(...) etcetera::Array::create(__VA_ARGS__)
#define ARR_ITEM(X) []() { return X; }

class Rebuild : public Base {
protected:
  std::function<std::any(std::weak_ptr<Base>)> rebuild_fn;
  std::shared_ptr<Base> child;

public:
  using Base::get;
  using Base::get_field;

  Rebuild(PrivateBase, std::function<std::any(std::weak_ptr<Base>)> rebuild_fn,
          std::shared_ptr<Base> child)
      : Base(PrivateBase()), rebuild_fn(rebuild_fn), child(child) {}

  static std::shared_ptr<Rebuild>
  create(std::function<std::any(std::weak_ptr<Base>)> rebuild_fn,
         std::shared_ptr<Base> child) {
    return std::make_shared<Rebuild>(PrivateBase(), rebuild_fn, child);
  }

  std::any get() override { return rebuild_fn(this->parent); }
  std::any get_parsed() override { return child->get(); }

  std::any parse(std::iostream &stream) override {
    return child->parse(stream);
  }

  void build(std::iostream &stream) override {
    auto data = this->get();
    child->set(data);
    child->build(stream);
  }
};

/*
class LazyBound : public Base {
protected:
  std::function<std::shared_ptr<Base>(std::weak_ptr<Base> c,
                                      std::weak_ptr<Base> s)>
      lazy_fn;
  std::shared_ptr<Base> child;

public:
  using Base::get;
  using Base::get_field;

  LazyBound(PrivateBase, std::function<std::shared_ptr<Base>(Base *c)> lazy_fn)
      : lazy_fn(lazy_fn), Base(PrivateBase()) {}

  static std::shared_ptr<LazyBound>
  create(std::function<std::shared_ptr<Base>(Base *c)> lazy_fn) {
    return std::make_shared<LazyBound>(PrivateBase(), lazy_fn);
  }

  std::any get() override { return child->get(); }
  std::any get_parsed() override { return child->get(); }

  std::any parse(std::iostream &stream) override {
    child = lazy_fn(this->parent, weak_from_this());
    child->set_parent(weak_from_this());
    return child->parse(stream);
  }

  void build(std::iostream &stream) override { child->build(stream); }
};
*/

} // namespace etcetera
