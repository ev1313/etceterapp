#pragma once

#include "basic.hpp"

namespace etcetera {

class Rebuild : public Base {
protected:
  typedef std::function<std::any(std::weak_ptr<Base>)> FRebuildFn;
  FRebuildFn rebuild_fn;
  std::shared_ptr<Base> child;

public:
  using Base::get;
  using Base::get_field;

  Rebuild(PrivateBase, FRebuildFn rebuild_fn, std::shared_ptr<Base> child)
      : Base(PrivateBase()), rebuild_fn(rebuild_fn), child(child) {}

  static std::shared_ptr<Rebuild> create(FRebuildFn rebuild_fn,
                                         std::shared_ptr<Base> child) {
    return std::make_shared<Rebuild>(PrivateBase(), rebuild_fn, child);
  }

  std::any get() override { return rebuild_fn(this->parent); }
  std::any get_parsed() override { return child->get(); }

  bool is_struct() override { return child->is_struct(); }
  bool is_array() override { return child->is_array(); }
  bool is_simple_type() override { return child->is_simple_type(); }

  size_t get_size(std::weak_ptr<Base> c) override { return child->get_size(c); }

  std::any parse(std::iostream &stream) override {
    return child->parse(stream);
  }

  void build(std::iostream &stream) override {
    auto data = this->get();
    child->set(data);
    child->build(stream);
  }
};

class LazyBound : public Base {
protected:
  typedef std::function<std::shared_ptr<Base>(std::weak_ptr<LazyBound> p)>
      FLazyFn;

  FLazyFn lazy_fn;
  std::shared_ptr<Base> child;

public:
  using Base::get;
  using Base::get_field;

  LazyBound(PrivateBase, FLazyFn lazy_fn)
      : Base(PrivateBase()), lazy_fn(lazy_fn) {}

  static std::shared_ptr<LazyBound> create(FLazyFn lazy_fn) {
    return std::make_shared<LazyBound>(PrivateBase(), lazy_fn);
  }

  size_t get_size(std::weak_ptr<Base> c) override { return child->get_size(c); }

  std::any get() override { return child->get(); }
  std::any get_parsed() override { return child->get(); }
  std::any get(std::string key) override { return child->get(key); };
  std::weak_ptr<Base> get_field(std::string key) override {
    return child->get_field(key);
  };

  FLazyFn get_lazy_fn() { return lazy_fn; }

  std::any parse(std::iostream &stream) override {
    child = lazy_fn(static_pointer_cast<LazyBound>(weak_from_this().lock()));
    child->set_parent(weak_from_this());
    return child->parse(stream);
  }

  void build(std::iostream &stream) override { child->build(stream); }

  void parse_xml(pugi::xml_node const &node, std::string name,
                 bool is_root) override {
    child = lazy_fn(static_pointer_cast<LazyBound>(weak_from_this().lock()));
    child->set_parent(weak_from_this());
    return child->parse_xml(node, name, is_root);
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    return child->build_xml(parent, name);
  }
};

} // namespace etcetera
