#pragma once

#include "basic.hpp"
#include "struct.hpp"

#include <optional>

namespace etcetera {

class IfThenElse : public Base {
protected:
  typedef std::function<bool(std::weak_ptr<Base>)> FIfFn;
  FIfFn if_fn;
  // FIXME: if_child doesn't need to be optional
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
    auto ret = std::make_shared<IfThenElse>(PrivateBase(), if_fn, if_child,
                                            else_child);
    if_child.second->set_parent(ret);
    if_child.second->set_name(if_child.first);
    else_child.second->set_parent(ret);
    else_child.second->set_name(else_child.first);
    return ret;
  }
  static std::shared_ptr<IfThenElse> create(FIfFn if_fn, Field if_child) {
    auto ret = std::make_shared<IfThenElse>(PrivateBase(), if_fn, if_child,
                                            std::nullopt);
    if_child.second->set_parent(ret);
    if_child.second->set_name(if_child.first);
    return ret;
  }

  size_t get_size() override {
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
    return child->get_size();
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

  std::vector<std::string> get_names() override {
    std::vector<std::string> ret;

    if (if_child) {
      ret.push_back(if_child.value().first);
    }
    if (else_child) {
      ret.push_back(else_child.value().first);
    }

    return ret;
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

  std::any parse(std::istream &stream) override {
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

  void build(std::ostream &stream) override {
    if (if_fn(this->parent)) {
      if (if_child) {
        spdlog::debug("IfThenElse::build {}", if_child.value().first);
        try {
          if_child.value().second->build(stream);
        } catch (std::exception &e) {
          throw std::runtime_error(if_child.value().first + "->" +
                                   std::string(e.what()));
        }
      }
    } else {
      if (else_child) {
        try {
          spdlog::debug("IfThenElse::build {}", else_child.value().first);
          else_child.value().second->build(stream);
        } catch (std::exception &e) {
          throw std::runtime_error(else_child.value().first + "->" +
                                   std::string(e.what()));
        }
      }
    }
  }

  void parse_xml(pugi::xml_node const &node, std::string,
                 bool is_root) override {
    std::string child_name;
    std::shared_ptr<Base> child;
    if (if_child) {
      spdlog::debug("IfThenElse::parse_xml trying if {}",
                    if_child.value().first);
      for (auto &name : if_child.value().second->get_names()) {
        spdlog::debug("IfThenElse::parse_xml trying name {}", name);
        if (node.child(name.c_str())) {
          spdlog::debug("IfThenElse::parse_xml found if {}", name);
          std::tie(child_name, child) = if_child.value();
          break;
        }
      }
    } else {
      if (else_child) {
        spdlog::debug("IfThenElse::parse_xml trying else {}",
                      else_child.value().first);
        for (auto &name : else_child.value().second->get_names()) {
          spdlog::debug("IfThenElse::parse_xml trying name {}", name);
          if (node.child(name.c_str())) {
            spdlog::debug("IfThenElse::parse_xml found else {}", name);
            std::tie(child_name, child) = else_child.value();
            break;
          }
        }
      }
    }
    if (!child) {
      spdlog::debug("IfThenElse::parse_xml {} no child found", name);
      return;
    }
    return child->parse_xml(node, child_name, is_root);
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    if (if_fn(this->parent)) {
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
  tsl::ordered_map<T, std::string> names;
  tsl::ordered_map<T, FTypeFn> fields;
  T value;
  std::shared_ptr<Base> current;

public:
  typedef std::tuple<T, std::string, FTypeFn> SwitchField;
  using Base::get;
  using Base::get_field;
  using Base::get_offset;

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

  size_t get_size() override { return current->get_size(); }

  std::any get() override { return current->get(); }

  std::vector<std::string> get_names() override {
    std::vector<std::string> ret;
    for (auto &[_, value] : names) {
      ret.push_back(value);
    }
    return ret;
  }

  std::weak_ptr<Base> get_field(std::string key) override {
    return current->get_field(key);
  }

  std::any parse(std::istream &stream) override {
    value = switch_fn(this->parent);
    spdlog::debug("stream pos {:02X} reading {:02X} {}",
                  (int64_t)stream.tellg(), value, names[value]);
    if (!fields.contains(value)) {
      throw std::runtime_error("Switch: " + std::to_string(value) +
                               " not found!");
    }
    current = fields[value]();
    current->set_parent(weak_from_this());
    current->set_name(names[value]);
    return current->parse(stream);
  }

  void build(std::ostream &stream) override {
    spdlog::debug("Switch::build {}", (size_t)stream.tellp());
    if (!current) {
      throw cpptrace::runtime_error("Switch: no current child");
    }
    current->build(stream);
  }

  void parse_xml(pugi::xml_node const &node, std::string,
                 bool is_root) override {
    spdlog::debug("Switch start");
    for (auto &[key, value] : names) {
      spdlog::debug("Switch trying {}", key);
      // FIXME: what if the field itself is a switch? currently everything needs
      // to be nested in a Struct anyway, so maybe just force everything to be
      // in a Struct by default / put everything in it's own node by default?
      current = fields[key]();
      bool valid = false;
      if (current->is_simple_type()) {
        valid = node.attribute(value.c_str());
      } else {
        valid = node.child(value.c_str());
      }
      if (valid) {
        spdlog::debug("Switch::parse_xml {} {}", (size_t)node.offset_debug(),
                      value);
        return current->parse_xml(node.child(value.c_str()), value, is_root);
      }
    }
    current = {};
    throw std::runtime_error("No matching child");
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    // we need to create a new xml node here with the correct name
    auto s = parent.append_child(names[value].c_str());
    current->build_xml(s, names[value]);
    return s;
  }
};

} // namespace etcetera
