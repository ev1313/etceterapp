#pragma once

#include "basic.hpp"

namespace etcetera {

typedef std::pair<std::string, std::shared_ptr<Base>> Field;

class Struct : public Base {
  tsl::ordered_map<std::string, std::shared_ptr<Base>> fields;

public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;
  template <typename... Args>
  Struct(PrivateBase, Args &&...args) : Base(PrivateBase()) {
    (fields.emplace(std::get<0>(std::forward<Args>(args)),
                    std::get<1>(std::forward<Args>(args))),
     ...);
  }
  template <typename... Args>
  static std::shared_ptr<Struct> create(Args &&...args) {
    auto ret = std::make_shared<Struct>(PrivateBase(), args...);

    for (auto &[key, field] : ret->fields) {
      field->set_parent(ret);
      field->set_name(key);
    }

    return ret;
  }

  std::any parse(std::istream &stream) override {
    tsl::ordered_map<std::string, std::any> obj;
    for (auto &[key, field] : fields) {
      try {
        spdlog::info("Struct::parse {:02X} {}", (size_t)stream.tellg(), key);
        std::any value = field->parse(stream);
        obj.emplace(key, value);
      } catch (std::runtime_error &e) {
        throw std::runtime_error(name + "[" + key + "]->" +
                                 std::string(e.what()));
      }
    }
    return obj;
  }

  void build(std::ostream &stream) override {
    for (auto &[key, field] : fields) {
      field->build(stream);
    }
  }

  bool is_struct() override { return true; }

  size_t get_offset(std::string key) override {
    size_t ret = get_offset();
    for (auto &[k, field] : fields) {
      if (k == key) {
        return ret;
      }
      ret += field->get_size();
    }
    throw std::runtime_error("Struct: " + key + " not found!");
    return 0;
  }

  size_t get_size() override {
    size_t size = 0;
    for (auto &[key, field] : fields) {
      try {
        size += field->get_size();
      } catch (std::runtime_error &e) {
        throw std::runtime_error(key + "->" + std::string(e.what()));
      }
    }
    return size;
  }

  std::any get() override {
    throw std::runtime_error("Struct: Not implemented");
  }
  std::any get(std::string key) {
    try {
      return fields[key]->get();
    } catch (std::runtime_error &e) {
      throw std::runtime_error(name + "[" + key + "]->" +
                               std::string(e.what()));
    }
  }

  std::weak_ptr<Base> get_field(std::string key) override {
    if (key == "_") {
      return this->parent;
    }

    return fields[key];
  }

  void parse_xml(pugi::xml_node const &node, std::string name,
                 bool is_root) override {
    auto s = is_root ? node : node.child(name.c_str());
    for (auto &[key, field] : fields) {
      try {
        field->parse_xml(s, key, false);
      } catch (std::runtime_error &e) {
        throw std::runtime_error(key + "->" + std::string(e.what()));
      }
    }
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    auto s = parent.append_child(name.c_str());
    for (auto &[key, field] : fields) {
      try {
        field->build_xml(s, key);
      } catch (std::runtime_error &e) {
        throw std::runtime_error(key + "->" + std::string(e.what()));
      }
    }
    return s;
  }
};

} // namespace etcetera
