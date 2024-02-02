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
  std::any get_parsed() override {
    spdlog::debug("Rebuild::get_parsed {}", name);
    return child->get();
  }
  size_t get_offset(std::string key) override {
    return lock(this->parent)->get_offset(key);
  }
  size_t get_offset(size_t key) override {
    return lock(this->parent)->get_offset(key);
  }

  bool is_struct() override { return child->is_struct(); }
  bool is_array() override { return child->is_array(); }
  bool is_simple_type() override { return child->is_simple_type(); }

  size_t get_size() override { return child->get_size(); }

  std::any parse(std::istream &stream) override {
    spdlog::debug("Rebuild::parse {:02X} {}", (size_t)stream.tellg(), name);
    return child->parse(stream);
  }

  void build(std::ostream &stream) override {
    auto data = this->get();
    child->set(data);
    child->build(stream);
  }

  void parse_xml(pugi::xml_node const &node, std::string name,
                 bool is_root) override {
    // child->parse_xml(node, name, is_root);
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    return parent;
    // child->build_xml(parent, name);
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

  size_t get_size() override { return child->get_size(); }

  std::any get() override { return child->get(); }
  std::any get_parsed() override { return child->get(); }
  std::any get(std::string key) override { return child->get(key); };
  std::weak_ptr<Base> get_field(std::string key) override {
    return child->get_field(key);
  };

  std::vector<std::string> get_names() override {
    // FIXME: hack
    child = lazy_fn(static_pointer_cast<LazyBound>(weak_from_this().lock()));
    child->set_parent(weak_from_this());
    child->set_name(name);
    return child->get_names();
  }

  FLazyFn get_lazy_fn() { return lazy_fn; }

  std::any parse(std::istream &stream) override {
    spdlog::debug("LazyBound::parse {:02X} {}", (size_t)stream.tellg(), name);
    child = lazy_fn(static_pointer_cast<LazyBound>(weak_from_this().lock()));
    child->set_parent(weak_from_this());
    child->set_name(name);
    return child->parse(stream);
  }

  void build(std::ostream &stream) override { child->build(stream); }

  void parse_xml(pugi::xml_node const &node, std::string name,
                 bool is_root) override {
    spdlog::debug("Lazybound parsing {}", name);
    child = lazy_fn(static_pointer_cast<LazyBound>(weak_from_this().lock()));
    child->set_parent(weak_from_this());
    child->set_name(name);
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

  size_t get_size() override {
    if (alignment_fn) {
      alignment = alignment_fn.value()(this->parent);
    }
    auto csize = child->get_size();
    return csize + modulo(-csize, alignment);
  }

  std::any get() override { return child->get(); }
  std::any get_parsed() override { return child->get(); }
  std::any get(std::string key) override { return child->get(key); };
  std::weak_ptr<Base> get_field(std::string key) override {
    return child->get_field(key);
  };

  std::any parse(std::istream &stream) override {
    if (alignment_fn) {
      alignment = alignment_fn.value()(this->parent);
    }
    if (alignment < 2) {
      throw cpptrace::runtime_error("Alignment must be at least 2");
    }
    size_t before_offset = stream.tellg();
    auto ret = child->parse(stream);
    size_t after_offset = stream.tellg();
    size_t pad = modulo(-(after_offset - before_offset), alignment);
    if (pad > 0) {
      stream.seekg(pad, std::ios_base::cur);
    }
    assert(!stream.fail());
    return ret;
  }

  void build(std::ostream &stream) override {
    if (alignment_fn) {
      alignment = alignment_fn.value()(this->parent);
    }
    size_t before_offset = stream.tellp();
    child->build(stream);
    size_t after_offset = stream.tellp();
    size_t pad = modulo(-(after_offset - before_offset), alignment);
    if (pad > 0) {
      stream.seekp(pad, std::ios_base::cur);
    }
    assert(!stream.fail());
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
