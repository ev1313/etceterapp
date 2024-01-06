#pragma once

#include <any>
#include <functional>
#include <istream>
#include <map>
#include <memory>

namespace etcetera {

class Base {
public:
  virtual std::any parse(std::iostream &stream) = 0;
  virtual void build(std::iostream &stream) = 0;
  virtual std::any get() = 0;
};

typedef std::tuple<std::string, std::unique_ptr<Base>> Field;

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
  std::any get() { return value; }
};

class Struct : public Base {
  std::map<std::string, std::unique_ptr<Base>> fields;

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

  std::any get() {
    std::map<std::string, std::any> ret;
    for (auto &[key, field] : fields) {
      ret.emplace(key, field->get());
    }
  }
};

} // namespace etcetera
