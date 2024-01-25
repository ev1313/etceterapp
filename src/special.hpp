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

class Aligned : public Base {
protected:
  typedef std::function<size_t(std::weak_ptr<Base>)> FAlignmentFn;
  std::optional<FAlignmentFn> alignment_fn;
  size_t alignment;
  std::shared_ptr<Base> child;

public:
  using Base::get;
  using Base::get_field;

  Aligned(PrivateBase, std::optional<FAlignmentFn> alignment_fn,
          size_t alignment, std::shared_ptr<Base> child)
      : Base(PrivateBase()), alignment_fn(alignment_fn), alignment(alignment),
        child(child) {}

  static std::shared_ptr<Aligned>
  create(std::optional<FAlignmentFn> alignment_fn,
         std::shared_ptr<Base> child) {
    return std::make_shared<Aligned>(PrivateBase(), alignment_fn, 0, child);
  }

  static std::shared_ptr<Aligned> create(size_t alignment,
                                         std::shared_ptr<Base> child) {
    return std::make_shared<Aligned>(PrivateBase(), std::nullopt, alignment,
                                     child);
  }

  size_t get_size(std::weak_ptr<Base> c) override { return child->get_size(c); }

  std::any get() override { return child->get(); }
  std::any get_parsed() override { return child->get(); }
  std::any get(std::string key) override { return child->get(key); };
  std::weak_ptr<Base> get_field(std::string key) override {
    return child->get_field(key);
  };

  std::any parse(std::iostream &stream) override {
    if (alignment_fn) {
      alignment = alignment_fn.value()(this->parent);
    }
    if (alignment < 2) {
      throw std::runtime_error("Alignment must be at least 2");
    }
    size_t before_offset = stream.tellg();
    auto ret = child->parse(stream);
    size_t after_offset = stream.tellg();
    spdlog::warn("before: {}, after: {}", before_offset, after_offset);
    size_t pad = modulo(-(after_offset - before_offset), alignment);
    spdlog::warn("pad: {}", pad);
    if (pad > 0) {
      stream.seekg(pad, std::ios_base::cur);
    }
    return ret;
  }

  void build(std::iostream &stream) override {
    if (alignment_fn) {
      alignment = alignment_fn.value()(this->parent);
    }
    size_t before_offset = stream.tellg();
    child->build(stream);
    size_t after_offset = stream.tellp();
    size_t pad = modulo(-(after_offset - before_offset), alignment);
    if (pad > 0) {
      stream.seekp(pad, std::ios_base::cur);
    }
  }

  void parse_xml(pugi::xml_node const &node, std::string name,
                 bool is_root) override {
    child->parse_xml(node, name, is_root);
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    return child->build_xml(parent, name);
  }
};

} // namespace etcetera
