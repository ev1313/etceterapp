#pragma once

#include "basic.hpp"

#include <optional>

namespace etcetera {

class IfThenElse : public Base {
protected:
  typedef std::function<bool(std::weak_ptr<Base>)> FIfFn;
  FIfFn if_fn;
  std::optional<Field> if_child;
  std::optional<Field> else_child;

public:
  using Base::get;
  using Base::get_field;

  IfThenElse(PrivateBase, FIfFn if_fn, std::optional<Field> if_child,
             std::optional<Field> else_child)
      : Base(PrivateBase()), if_fn(if_fn), if_child(if_child),
        else_child(else_child) {}

  static std::shared_ptr<IfThenElse> create(FIfFn if_fn, Field if_child,
                                            Field else_child) {
    return std::make_shared<IfThenElse>(PrivateBase(), if_fn, if_child,
                                        else_child);
  }
  static std::shared_ptr<IfThenElse> create(FIfFn if_fn, Field if_child) {
    return std::make_shared<IfThenElse>(PrivateBase(), if_fn, if_child,
                                        std::nullopt);
  }

  size_t get_size(std::weak_ptr<Base> c) override {
    std::shared_ptr<Base> child;
    if (if_fn(this->parent)) {
      if (!if_child) {
        return 0;
      }
      child = if_child.value().second;
    } else {
      if (!else_child) {
        return 0;
      }
      child = else_child.value().second;
    }
    return child->get_size(c);
  }

  std::any get() override {
    std::shared_ptr<Base> child;
    if (if_fn(this->parent)) {
      if (!if_child) {
        return std::any();
      }
      child = if_child.value().second;
    } else {
      if (!else_child) {
        return std::any();
      }
      child = else_child.value().second;
    }
    return child->get();
  }

  std::weak_ptr<Base> get_field(std::string key) override {
    std::shared_ptr<Base> child;
    if (if_fn(this->parent)) {
      if (!if_child) {
        return std::weak_ptr<Base>();
      }
      child = if_child.value().second;
    } else {
      if (!else_child) {
        return std::weak_ptr<Base>();
      }
      child = else_child.value().second;
    }
    return child->get_field(key);
  }

  std::any parse(std::iostream &stream) override {
    std::shared_ptr<Base> child;
    if (if_fn(this->parent)) {
      if (!if_child) {
        return std::any();
      }
      child = if_child.value().second;
    } else {
      if (!else_child) {
        return std::any();
      }
      child = else_child.value().second;
    }
    return child->parse(stream);
  }

  void build(std::iostream &stream) override {
    if (if_fn(this->parent)) {
      if (if_child) {
        if_child.value().second->build(stream);
      }
    } else {
      if (else_child) {
        else_child.value().second->build(stream);
      }
    }
  }

  void parse_xml(pugi::xml_node const &node, std::string,
                 bool is_root) override {
    std::string child_name;
    std::shared_ptr<Base> child;
    if (if_child) {
      if (node.child(if_child.value().first.c_str())) {
        std::tie(child_name, child) = if_child.value();
      }
    } else {
      if (else_child) {
        if (node.child(else_child.value().first.c_str())) {
          std::tie(child_name, child) = else_child.value();
        }
      }
    }
    if (!child) {
      return;
    }
    return child->parse_xml(node, child_name, is_root);
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    if (!if_fn(this->parent)) {
      if (!if_child) {
        return parent;
      }
      return if_child.value().second->build_xml(parent, name);
    } else {
      if (!else_child) {
        return parent;
      }
      return else_child.value().second->build_xml(parent, name);
    }
  }
};

using If = IfThenElse;

template <typename T> class Switch : public Base {
protected:
  typedef std::function<T(std::weak_ptr<Base>)> FSwitchFn;
  typedef std::function<std::shared_ptr<Base>()> FTypeFn;
  FSwitchFn switch_fn;
  std::map<T, std::string> names;
  std::map<T, FTypeFn> fields;
  T value;
  std::shared_ptr<Base> current;

public:
  typedef std::tuple<T, std::string, FTypeFn> SwitchField;
  using Base::get;
  using Base::get_field;

  template <typename... Args>
  Switch(PrivateBase, FSwitchFn switch_fn, Args &&...args)
      : Base(PrivateBase()), switch_fn(switch_fn) {
    (fields.emplace(std::get<0>(std::forward<Args>(args)),
                    std::get<2>(std::forward<Args>(args))),
     ...);
    (names.emplace(std::get<0>(std::forward<Args>(args)),
                   std::get<1>(std::forward<Args>(args))),
     ...);
  }

  template <typename... Args>
  static std::shared_ptr<Switch> create(FSwitchFn switch_fn, Args &&...args) {
    return std::make_shared<Switch>(PrivateBase(), switch_fn, args...);
  }

  size_t get_size(std::weak_ptr<Base> c) override {
    return current->get_size(c);
  }

  std::any get() override { return current->get(); }

  std::weak_ptr<Base> get_field(std::string key) override {
    return current->get_field(key);
  }

  std::any parse(std::iostream &stream) override {
    value = switch_fn(this->parent);
    current = fields[value]();
    return current->parse(stream);
  }

  void build(std::iostream &stream) override { current->build(stream); }

  void parse_xml(pugi::xml_node const &node, std::string,
                 bool is_root) override {
    for (auto &[key, value] : names) {
      if (node.child(value.c_str())) {
        current = fields[key]();
        return current->parse_xml(node, value, is_root);
      }
    }
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    return current->build_xml(parent, name);
  }
};

} // namespace etcetera
