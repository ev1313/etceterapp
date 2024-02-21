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
    custom_assert(key < data.size());
    for (size_t i = 0; i < key; i++) {
      try {
        ret += data[i]->get_size();
      } catch (std::exception &e) {
        throw std::runtime_error(key + "->" + std::string(e.what()));
      }
    }
    return ret;
  }

  size_t get_size() override {
    size_t s = 0;
    size_t i = 0;
    for (auto &obj : data) {
      try {
        s += obj->get_size();
        i += 1;
      } catch (std::exception &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
    return s;
  }

  std::any parse(std::istream &stream) override {
    if (size_fn) {
      size = size_fn(this->parent);
    }
    data.clear();
    spdlog::debug("Array::parsing {} {:02X} {}", name, (size_t)stream.tellg(),
                  size);
    for (size_t i = 0; i < size; i++) {
      auto obj = type_constructor();
      obj->set_parent(weak_from_this());
      obj->set_idx(i);
      data.push_back(obj);
      try {
        spdlog::debug("Array::itemparse {} {:02X} {}", name,
                      (size_t)stream.tellg(), i);
        data.back()->parse(stream);
      } catch (std::exception &e) {
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
      } catch (std::exception &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
  }

  std::any get() override {
    throw cpptrace::runtime_error("Array->get(): Not implemented");
  }
  std::any get(size_t key) override { return data[key]->get(); }

  void set(std::any value) override {
    std::vector<std::any> values = std::any_cast<std::vector<std::any>>(value);
    init_fields(values.size());
    for (size_t i = 0; i < values.size(); i++) {
      data[i]->set(values[i]);
    }
  }
  void set(size_t key, std::any value) override { data[key]->set(value); }

  size_t length() override { return data.size(); }

  std::weak_ptr<Base> get_field(size_t key) override { return data[key]; }

  bool is_array() override { return true; }

  /*
   * This is a helper function to initialize the fields of the array.
   *
   * Only for testing purposes.
   * */
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
    spdlog::debug("Array::parse_xml {}", name);
    size_t i = 0;
    data.clear();
    for (auto &child_node : node.children(name.c_str())) {
      spdlog::debug("Array::parse_xml {} {}", name, i);
      auto obj = type_constructor();
      obj->set_parent(weak_from_this());
      obj->set_idx(i);
      data.push_back(obj);
      try {
        data[i]->parse_xml(child_node, name, true);
        i += 1;
      } catch (std::exception &e) {
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
      } catch (std::exception &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
    return parent;
  }
};

/*
 * In contrast to construct this does not get the stream as an argument in its
 * lambda, but it gets only the parsed object itself and its parent.
 *
 * The lambda returns a bool that determines, if parsing should stop or should
 * not stop.
 *
 * The lambda gets the parsed object and its parent as arguments.
 *
 * It is possible to limit the size of this by setting the size attribute in
 * create. This is in bytes and stop successfully if the stream offset
 * difference hits this size. It throws if the offset is greater than the
 * difference.
 * */
class RepeatUntil : public Base {
protected:
  typedef std::function<bool(std::weak_ptr<Base>, std::weak_ptr<Base>)>
      RepeatFn;
  typedef std::function<std::shared_ptr<Base>()> FTypeFn;
  typedef std::function<size_t(std::weak_ptr<Base>)> FSizeFn;
  RepeatFn repeat_fn;
  FTypeFn type_constructor;
  FSizeFn size_fn;
  std::vector<std::shared_ptr<Base>> data;

public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;

  RepeatUntil(PrivateBase, RepeatFn repeat_fn, FTypeFn m_type_constructor,
              FSizeFn size_fn)
      : Base(PrivateBase()), repeat_fn(repeat_fn),
        type_constructor(m_type_constructor), size_fn(size_fn) {}

  static std::shared_ptr<RepeatUntil>
  create(RepeatFn repeat_fn, FTypeFn m_type_constructor, FSizeFn size_fn) {
    return std::make_shared<RepeatUntil>(PrivateBase(), repeat_fn,
                                         m_type_constructor, size_fn);
  }

  static std::shared_ptr<RepeatUntil> create(RepeatFn repeat_fn,
                                             FTypeFn m_type_constructor) {
    return std::make_shared<RepeatUntil>(PrivateBase(), repeat_fn,
                                         m_type_constructor, nullptr);
  }

  size_t length() override { return data.size(); }

  size_t get_offset(size_t key) {
    size_t ret = get_offset();
    custom_assert(key < data.size());
    for (size_t i = 0; i < key; i++) {
      try {
        ret += data[i]->get_size();
      } catch (std::exception &e) {
        throw std::runtime_error(name + "[" + std::to_string(key) + "]->" +
                                 std::string(e.what()));
      }
    }
    return ret;
  }

  size_t get_size() override {
    size_t s = 0;
    size_t i = 0;
    for (auto &obj : data) {
      try {
        s += obj->get_size();
        i += 1;
      } catch (std::exception &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
    return s;
  }

  std::any parse(std::istream &stream) override {
    int64_t before_offset = stream.tellg();
    custom_assert(before_offset >= 0);
    data.clear();
    size_t i = 0;

    int64_t opt_size = 0;
    if (size_fn) {
      opt_size = (int64_t)size_fn(this->parent);
    }

    auto check_size = [&]() -> bool {
      if (!size_fn) {
        return true;
      }
      return stream.tellg() < (before_offset + opt_size);
    };

    while (check_size()) {
      auto obj = type_constructor();
      obj->set_parent(weak_from_this());
      obj->set_idx(i);
      data.push_back(obj);
      try {
        data.back()->parse(stream);
        i += 1;
        if (repeat_fn(data.back(), this->parent)) {
          break;
        }
      } catch (std::exception &e) {
        throw std::runtime_error(name + "[" + std::to_string(i) + "]->" +
                                 std::string(e.what()));
      }
    }
    if (size_fn && (stream.tellg() > (before_offset + opt_size))) {
      throw cpptrace::runtime_error("RepeatUntil: size limit exceeded!");
    }
    return data;
  }

  void build(std::ostream &stream) override {
    size_t i = 0;
    for (auto &obj : data) {
      try {
        obj->build(stream);
        i += 1;
      } catch (std::exception &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
  }

  std::any get() override { throw cpptrace::runtime_error("Not implemented"); }
  std::any get(size_t key) override { return data[key]->get(); }
  std::weak_ptr<Base> get_field(size_t key) override { return data[key]; }

  void parse_xml(pugi::xml_node const &node, std::string name,
                 bool is_root) override {
    spdlog::debug("RepeatUntil::parse_xml {}", name);
    size_t i = 0;
    data.clear();
    for (auto &child_node : node.children(name.c_str())) {
      spdlog::debug("RepeatUntil::parse_xml {} {}", name, i);
      auto obj = type_constructor();
      obj->set_parent(weak_from_this());
      obj->set_idx(i);
      data.push_back(obj);
      try {
        data[i]->parse_xml(child_node, name, true);
        i += 1;
      } catch (std::exception &e) {
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
      } catch (std::exception &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
                                 std::string(e.what()));
      }
    }
    return parent;
  }
};

} // namespace etcetera
