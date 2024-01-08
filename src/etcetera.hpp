#pragma once

#include <any>
#include <cassert>
#include <functional>
#include <istream>
#include <map>
#include <memory>

namespace etcetera {

class Struct;

class Base {
protected:
  std::type_info const &type_ = typeid(Base);

  Base *parent = nullptr;
  friend class Struct;

  void set_parent(Base *parent) { this->parent = parent; }

public:
  virtual std::any parse(std::iostream &stream) = 0;
  virtual void build(std::iostream &stream) = 0;

  virtual bool is_struct() { return false; }
  virtual bool is_array() { return false; }

  // returns all the data
  virtual std::any get() = 0;
};

typedef std::pair<std::string, Base *> Field;

template <typename T> class IntegralType : public Base {
public:
  T value = 0;
  std::any parse(std::iostream &stream) override {
    stream.read(reinterpret_cast<char *>(&value), sizeof(value));
    return value;
  }
  void build(std::iostream &stream) override {
    stream.write(reinterpret_cast<char *>(&value), sizeof(value));
  }
  std::any get() override { return value; }
};
using Int8ul = IntegralType<int8_t>;
using Int16ul = IntegralType<int16_t>;
using Int32ul = IntegralType<int32_t>;
using Int64ul = IntegralType<int64_t>;
using UInt8ul = IntegralType<uint8_t>;
using UInt16ul = IntegralType<uint16_t>;
using UInt32ul = IntegralType<uint32_t>;
using UInt64ul = IntegralType<uint64_t>;

class Struct : public Base {
  std::map<std::string, Base *> fields;

public:
  template <typename... Args> Struct(Args &&...args) {
    (fields.emplace(std::get<0>(std::forward<Args>(args)),
                    std::get<1>(std::forward<Args>(args))),
     ...);

    for (auto &[key, field] : fields) {
      field->set_parent(this);
    }
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
  std::any get(std::string key) { return fields[key]->get(); }

  template <typename T, typename... Ts> T get(std::string key, Ts &&...args) {
    if (key == "_") {
      if constexpr (sizeof...(args) > 0) {
        assert(this->parent != nullptr);
        assert(this->parent->is_struct());
        return ((Struct *)this->parent)->get<T>(args...);
      } else {
        throw std::runtime_error("Not implemented");
      }
    } else {
      if constexpr (sizeof...(args) == 0) {
        return std::any_cast<T>(fields[key]->get());
      } else {
        assert(fields[key]->is_struct());
        return ((Struct *)fields[key])->get<T>(args...);
      }
    }
  }

  Base *get_field(std::string key) { return fields[key]; }

  template <typename T, typename... Ts>
  T *get_field(std::string key, Ts &&...args) {
    if (key == "_") {
      if constexpr (sizeof...(args) > 0) {
        assert(this->parent != nullptr);
        assert(this->parent->is_struct());
        return ((Struct *)this->parent)->get_field<T>(args...);
      } else {
        throw std::runtime_error("Not implemented");
      }
    } else {
      if constexpr (sizeof...(args) == 0) {
        return (T *)fields[key];
      } else {
        assert(fields[key]->is_struct());
        return ((Struct *)fields[key])->get_field<T>(args...);
      }
    }
  }
};

} // namespace etcetera
