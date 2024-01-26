#pragma once

#include "basic.hpp"

namespace etcetera {

class Array : public Base {
protected:
  // TODO: this should probably not be necessary, as you can just take the size
  // of data?
  size_t size;
  typedef std::function<size_t(std::weak_ptr<Base>)> FSizeFn;
  typedef std::function<std::shared_ptr<Base>()> FTypeFn;
  FSizeFn size_fn;
  FTypeFn type_constructor;
  std::vector<std::shared_ptr<Base>> data;

public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;
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

  size_t get_offset(size_t key) {
    size_t ret = get_offset();
    assert(key < data.size());
    for (size_t i = 0; i < key; i++) {
      try {
        ret += data[i]->get_size(weak_from_this());
      } catch (std::runtime_error &e) {
        throw std::runtime_error(key + "->" + std::string(e.what()));
      }
    }
    return ret;
  }

  size_t get_size(std::weak_ptr<Base> c) override {
    size_t s = 0;
    size_t i = 0;
    for (auto &obj : data) {
      try {
        s += obj->get_size(c);
        i += 1;
      } catch (std::runtime_error &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
    return s;
  }

  std::any parse(std::istream &stream) override {
    if (size_fn) {
      size = size_fn(weak_from_this());
    }
    data.clear();
    for (size_t i = 0; i < size; i++) {
      auto obj = type_constructor();
      obj->set_parent(weak_from_this());
      obj->set_idx(i);
      data.push_back(obj);
      try {
        data.back()->parse(stream);
      } catch (std::runtime_error &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
    return data;
  }

  void build(std::ostream &stream) override {
    size_t i = 0;
    for (auto &obj : data) {
      try {
        obj->build(stream);
        i += 1;
      } catch (std::runtime_error &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
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
      obj->set_idx(i);
      data.push_back(obj);
    }
  }

  void parse_xml(pugi::xml_node const &node, std::string name,
                 bool is_root) override {
    auto arr = is_root ? node : node.child(name.c_str());
    size_t i = 0;
    for (auto &child_node : arr.children(name.c_str())) {
      if (i >= data.size()) {
        throw std::runtime_error("Array: " + name +
                                 "too many elements in XML found!");
      }
      try {
        data[i]->parse_xml(child_node, name, true);
      } catch (std::runtime_error &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    size_t i = 0;
    for (auto &obj : data) {
      try {
        obj->build_xml(parent, name);
        i += 1;
      } catch (std::runtime_error &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
    return parent;
  }
};

} // namespace etcetera
