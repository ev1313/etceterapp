#pragma once

#include "basic.hpp"

namespace etcetera {

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

  size_t get_size(std::weak_ptr<Base>) override { return 0; }

  size_t get_ptr_offset(std::weak_ptr<Base>) override {
    return offset_fn(this->parent);
  }
  size_t get_ptr_size(std::weak_ptr<Base>) override {
    return sub->get_size(this->parent);
  }

  std::any get() override { return sub->get(); }

  std::any parse(std::iostream &s) override {
    offset = offset_fn(this->parent);
    size_t old_offset = s.tellg();
    s.seekg(old_offset + offset);
    auto ret = sub->parse(s);
    s.seekg(old_offset);
    return ret;
  }

  void build(std::iostream &s) override {
    offset = offset_fn(this->parent);
    size_t old_offset = s.tellp();
    s.seekp(old_offset + offset);
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

} // namespace etcetera
