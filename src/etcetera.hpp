#pragma once

#include <any>
#include <cassert>
#include <functional>
#include <istream>
#include <map>
#include <memory>

namespace etcetera {

class Base {
protected:
  Base *parent = nullptr;
  friend class Struct;

public:
  virtual std::any parse(std::iostream &stream) = 0;
  virtual void build(std::iostream &stream) = 0;

  virtual bool is_struct() { return false; }
  virtual bool is_array() { return false; }

  // for structs and the like
  virtual std::any get(std::string) {
    throw std::runtime_error("Not implemented");
  }
  // returns all the data
  virtual std::any get() = 0;
};

typedef std::pair<std::string, Base *> Field;

class Int32ul : public Base {
public:
  int32_t value = 0;
  std::any parse(std::iostream &stream) override {
    stream.read(reinterpret_cast<char *>(&value), sizeof(value));
    return value;
  }
  void build(std::iostream &stream) override {
    stream.write(reinterpret_cast<char *>(&value), sizeof(value));
  }
  std::any get() override { return value; }
};

class Struct : public Base {
  std::map<std::string, Base *> fields;

public:
  template <typename... Args> Struct(Args &&...args) {
    (fields.emplace(std::get<0>(std::forward<Args>(args)),
                    std::get<1>(std::forward<Args>(args))),
     ...);
  }

  std::any parse(std::iostream &stream) override {
    std::map<std::string, std::any> obj;
    for (auto &[key, field] : fields) {
      std::any value = field->parse(stream);
      obj.emplace(key, value);
    }
    return obj;
  }

  void build(std::iostream &stream) override {
    for (auto &[key, field] : fields) {
      field->build(stream);
    }
  }

  bool is_struct() override { return true; }

  std::any get() override { throw std::runtime_error("Not implemented"); }

  std::any get(std::string key) override { return fields[key]->get(); }

  template <typename... Ts> std::any get(std::string key, Ts... args) {
    if (key == "_") {
      assert(this->parent->is_struct());
      return this->parent->get(args...);
    } else {
      assert(this->parent->is_struct());
      return fields[key]->get(args...);
    }
  }
};

} // namespace etcetera
