#pragma once

#include "basic.hpp"

namespace etcetera {

class IfThenElse : public Base {
protected:
  typedef std::function<bool(std::weak_ptr<Base>)> FIfFn;
  FIfFn if_fn;
  std::shared_ptr<Base> if_child;
  std::shared_ptr<Base> else_child;

public:
  using Base::get;
  using Base::get_field;

  IfThenElse(PrivateBase, FIfFn if_fn, std::shared_ptr<Base> if_child,
             std::shared_ptr<Base> else_child)
      : Base(PrivateBase()), if_fn(if_fn), if_child(if_child),
        else_child(else_child) {}

  static std::shared_ptr<IfThenElse>
  create(FIfFn if_fn, std::shared_ptr<Base> if_child,
         std::shared_ptr<Base> else_child = nullptr) {
    return std::make_shared<IfThenElse>(PrivateBase(), if_fn, if_child,
                                        else_child);
  }

  size_t get_size(std::weak_ptr<Base> c) override {
    if (if_fn(this->parent)) {
      if (!if_child) {
        return 0;
      }
      return if_child->get_size(c);
    } else {
      if (!else_child) {
        return 0;
      }
      return else_child->get_size(c);
    }
  }

  std::any get() override {
    if (if_fn(this->parent)) {
      if (!if_child) {
        return std::any();
      }
      return if_child->get();
    } else {
      if (!else_child) {
        return std::any();
      }
      return else_child->get();
    }
  }

  std::weak_ptr<Base> get_field(std::string key) override {
    if (if_fn(this->parent)) {
      return if_child->get_field(key);
    } else {
      return else_child->get_field(key);
    }
  }

  std::any parse(std::iostream &stream) override {
    if (if_fn(this->parent)) {
      if (!if_child) {
        return std::any();
      }
      return if_child->parse(stream);
    } else {
      if (!else_child) {
        return std::any();
      }
      return else_child->parse(stream);
    }
  }

  void build(std::iostream &stream) override {
    if (if_fn(this->parent)) {
      if (if_child) {
        if_child->build(stream);
      }
    } else {
      if (else_child) {
        else_child->build(stream);
      }
    }
  }
  /*
    void parse_xml(pugi::xml_node const &node, std::string name) override {
      return child->parse_xml(node, name);
      if (if_fn(this->parent)) {
        if (if_child) {
          if_child->build(stream);
        }
      } else {
        if (else_child) {
          else_child->build(stream);
        }
      }
    }

    pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override
    { return child->build_xml(parent, name);
    }*/
};

using If = IfThenElse;

} // namespace etcetera
