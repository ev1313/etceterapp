#pragma once

#include "basic.hpp"

namespace etcetera {

/*
 * Pointer is a field with size 0 which gets an offset and parses its field
 * from that offset.
 */
class Pointer : public Base {
protected:
  typedef std::function<size_t(std::weak_ptr<Base>)> FOffsetFn;
  FOffsetFn offset_fn;
  size_t offset;
  std::shared_ptr<Base> sub;

public:
  using Base::get;
  using Base::get_field;

  Pointer(PrivateBase, FOffsetFn offset_fn, std::shared_ptr<Base> s)
      : Base(PrivateBase()), offset_fn(offset_fn), sub(s) {}

  static std::shared_ptr<Pointer> create(FOffsetFn offset_fn,
                                         std::shared_ptr<Base> s) {
    auto ret = std::make_shared<Pointer>(PrivateBase(), offset_fn, s);
    ret->sub->set_parent(ret);
    return ret;
  }

  bool is_pointer_type() override { return true; }

  size_t get_size() override { return 0; }

  size_t get_ptr_offset(std::weak_ptr<Base>) override {
    return offset_fn(this->parent);
  }
  size_t get_ptr_size(std::weak_ptr<Base>) override { return sub->get_size(); }

  std::any get() override { return sub->get(); }
  std::weak_ptr<Base> get_field(size_t key) override {
    return lock(this->parent)->get_field(key);
  }
  std::weak_ptr<Base> get_field(std::string key) override {
    return lock(this->parent)->get_field(key);
  }

  std::any parse(std::istream &s) override {
    offset = offset_fn(this->parent);
    size_t old_offset = s.tellg();
    s.seekg(offset);
    auto ret = sub->parse(s);
    s.seekg(old_offset);
    return ret;
  }

  void build(std::ostream &s) override {
    offset = offset_fn(this->parent);
    size_t old_offset = s.tellp();
    s.seekp(offset);
    sub->build(s);
    s.seekp(old_offset);
  }

  void set(std::any value) override { sub->set(value); }

  void parse_xml(pugi::xml_node const &node, std::string name,
                 bool is_root) override {
    sub->parse_xml(node, name, is_root);
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    return sub->build_xml(parent, name);
  }
};

/*
 * Area is a Pointer array. It is a special case, where you get an offset and a
 * size and parse new elements from that offset until you reach the size.
 *
 * The offset function will be used when parsing and building to determine the
 * offset. Using a rebuild here works.
 *
 * The size function will only be used when parsing to determine the size and
 * get_ptr_size can be used to determine the size when building (e.g. for a
 * rebuild).
 */
class Area : public Base {
  typedef std::function<int64_t(std::weak_ptr<Base>)> FOffsetFn;
  typedef std::function<int64_t(std::weak_ptr<Base>)> FSizeFn;
  typedef std::function<std::shared_ptr<Base>()> FTypeFn;

  FOffsetFn offset_fn;
  FSizeFn size_fn;
  FTypeFn type_fn;

  std::vector<std::shared_ptr<Base>> data;

public:
  using Base::get;
  using Base::get_field;

  Area(PrivateBase, FOffsetFn offset_fn, FSizeFn size_fn, FTypeFn type_fn)
      : Base(PrivateBase()), offset_fn(offset_fn), size_fn(size_fn),
        type_fn(type_fn) {}

  static std::shared_ptr<Area> create(FOffsetFn offset_fn, FSizeFn size_fn,
                                      FTypeFn type_fn) {
    return std::make_shared<Area>(PrivateBase(), offset_fn, size_fn, type_fn);
  }

  bool is_pointer_type() override { return true; }

  std::any get() override { throw cpptrace::runtime_error("Not implemented"); }
  std::any get(size_t key) override { return data[key]->get(); }

  std::weak_ptr<Base> get_field(size_t key) override { return data[key]; }

  size_t get_size() override { return 0; }

  size_t get_ptr_offset(std::weak_ptr<Base>) override {
    return offset_fn(this->parent);
  }
  size_t get_ptr_size(std::weak_ptr<Base>) override {
    size_t size = 0;
    for (auto &sub : data) {
      size += sub->get_size();
    }
    return size;
  }

  std::any parse(std::istream &stream) override {
    auto offset = offset_fn(this->parent);
    auto size = size_fn(this->parent);

    data.clear();

    size_t old_offset = stream.tellg();
    stream.seekg(offset);

    int64_t end_pos = offset + size;
    size_t i = 0;
    while (stream.tellg() < end_pos && stream.tellg() >= 0) {
      auto sub = type_fn();
      sub->set_parent(this->shared_from_this());
      try {
        sub->parse(stream);
        i+=1;
      } catch (std::exception &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
            std::string(e.what()));
      }
      data.push_back(sub);
    }

    spdlog::debug("Area::parse assert {} {}", (size_t)stream.tellg(), end_pos);
    assert(stream.tellg() == end_pos);

    stream.seekg(old_offset);

    return data;
  }

  void build(std::ostream &stream) override {
    auto offset = offset_fn(this->parent);
    size_t old_offset = stream.tellp();
    stream.seekp(offset);

    size_t i = 0;
    for (auto &sub : data) {
      try {
        sub->build(stream);
        i+=1;
      } catch (std::exception &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
            std::string(e.what()));
      }
    }

    int64_t test_pos = offset + get_ptr_size(this->parent);

    assert(stream.tellp() == test_pos);

    stream.seekp(old_offset);
  }

  void parse_xml(pugi::xml_node const &node, std::string name,
                 bool) override {
    // TODO: do we need is_root here? probably not.
    spdlog::debug("Area::parse_xml {}", name);
    size_t i = 0;
    data.clear();
    for (auto &child_node : node.children(name.c_str())) {
      spdlog::debug("Area::parse_xml {} {}", name, i);
      auto obj = type_fn();
      obj->set_parent(weak_from_this());
      obj->set_idx(i);
      data.push_back(obj);
      try {
        data[i]->parse_xml(child_node, name, true);
        i+=1;
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
        i+=1;
      } catch (std::exception &e) {
        throw std::runtime_error(std::to_string(i) + "->" +
            std::string(e.what()));
      }
    }
    return parent;
  }
};

} // namespace etcetera
