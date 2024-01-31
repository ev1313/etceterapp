#pragma once

#include "basic.hpp"

namespace etcetera {

class String : public Base {
protected:
public:
  using Base::get;
  using Base::get_field;
  std::string value;
  String(PrivateBase) : Base(PrivateBase()) {}

  std::any get() override { return value; }

  bool is_simple_type() override { return true; }

  void parse_xml(pugi::xml_node const &node, std::string name, bool) override {
    value = node.attribute(name.c_str()).as_string();
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    parent.append_attribute(name.c_str()) = this->value.c_str();
    return parent;
  }
};

template <typename TStringType = std::string,
          std::endian Endianess = std::endian::native>
class CString : public String {
public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;
  CString(Base::PrivateBase) : String(PrivateBase()) {}
  static std::shared_ptr<CString> create() {
    return std::make_shared<CString>(PrivateBase());
  }

  size_t get_size() override {
    TStringType s;
    if constexpr (std::is_same<std::u16string, TStringType>()) {
      // FIXME: this is a hack
      s = Utf32To16(Utf8To32(this->value));
    } else if constexpr (std::is_same<std::u32string, TStringType>()) {
      s = Utf8To32(this->value);
    } else {
      s = this->value;
    }
    return (s.length() + 1) * sizeof(typename TStringType::value_type);
  }

  size_t length() override {
    TStringType s;
    if constexpr (std::is_same<std::u16string, TStringType>()) {
      // FIXME: this is a hack
      s = Utf32To16(Utf8To32(this->value));
    } else if constexpr (std::is_same<std::u32string, TStringType>()) {
      s = Utf8To32(this->value);
    } else {
      s = this->value;
    }
    return s.length();
  }

  std::any parse(std::istream &stream) override {
    union {
      typename TStringType::value_type c;
      char bytes[sizeof(typename TStringType::value_type)];
    } data;
    TStringType s;
    s.clear();
    while (stream.read(data.bytes, sizeof(typename TStringType::value_type))) {
      if constexpr (Endianess != std::endian::native) {
        data.c = std::byteswap(data.c);
      }
      // std::string always includes a null terminator
      if (data.c == 0) {
        break;
      }
      s.push_back(data.c);
    }
    if constexpr (std::is_same<std::u16string, TStringType>()) {
      // FIXME: this is a hack
      this->value = Utf32To8(Utf16To32(s));
    } else if constexpr (std::is_same<std::u32string, TStringType>()) {
      this->value = Utf32To8(s);
    } else {
      this->value = s;
    }
    return this->value;
  }
  void build(std::ostream &stream) override {
    TStringType s;
    if constexpr (std::is_same<std::u16string, TStringType>()) {
      // FIXME: this is a hack
      s = Utf32To16(Utf8To32(this->value));
    } else if constexpr (std::is_same<std::u32string, TStringType>()) {
      s = Utf8To32(this->value);
    } else {
      s = this->value;
    }
    for (auto &c : s) {
      if constexpr (Endianess != std::endian::native) {
        c = std::byteswap(c);
      }
      stream.write((char *)&c, sizeof(typename TStringType::value_type));
    }
    typename TStringType::value_type c = 0;
    stream.write((char *)&c, sizeof(typename TStringType::value_type));
  }
};
using CString8l = CString<std::string, std::endian::little>;
using CString16l = CString<std::u16string, std::endian::little>;
using CString32l = CString<std::u32string, std::endian::little>;
using CString8b = CString<std::string, std::endian::big>;
using CString16b = CString<std::u16string, std::endian::big>;
using CString32b = CString<std::u32string, std::endian::big>;

template <typename TStringType = std::string,
          std::endian Endianess = std::endian::native>
class PaddedString : public String {
protected:
  typedef std::function<size_t(std::weak_ptr<Base>)> FSizeFn;
  FSizeFn size_fn;
  size_t size = 0;

public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;
  PaddedString(Base::PrivateBase, FSizeFn size_fn, size_t size)
      : String(PrivateBase()), size_fn(size_fn), size(size) {}
  static std::shared_ptr<PaddedString> create(FSizeFn size_fn) {
    return std::make_shared<PaddedString>(PrivateBase(), size_fn, 0);
  }
  static std::shared_ptr<PaddedString> create(size_t size) {
    return std::make_shared<PaddedString>(PrivateBase(), nullptr, size);
  }

  size_t get_size() override {
    if (size_fn) {
      size = size_fn(this->parent);
    }
    if constexpr (std::is_same<std::u16string, TStringType>()) {
      assert(size % sizeof(char16_t) == 0);
    } else if constexpr (std::is_same<std::u32string, TStringType>()) {
      assert(size % sizeof(char32_t) == 0);
    }
    return size;
  }

  size_t length() override {
    TStringType s;
    if constexpr (std::is_same<std::u16string, TStringType>()) {
      // FIXME: this is a hack
      s = Utf32To16(Utf8To32(this->value));
    } else if constexpr (std::is_same<std::u32string, TStringType>()) {
      s = Utf8To32(this->value);
    } else {
      s = this->value;
    }
    return s.length();
  }

  std::any parse(std::istream &stream) override {
    get_size();
    union {
      typename TStringType::value_type c;
      char bytes[sizeof(typename TStringType::value_type)];
    } data;
    TStringType s;
    s.clear();
    int64_t old_offset = stream.tellg();
    while ((int64_t)stream.tellg() < (old_offset + (int64_t)size)) {
      assert(stream.tellg() >= 0);
      stream.read(data.bytes, sizeof(typename TStringType::value_type));
      if constexpr (Endianess != std::endian::native) {
        data.c = std::byteswap(data.c);
      }
      s.push_back(data.c);
    }
    if constexpr (std::is_same<std::u16string, TStringType>()) {
      // FIXME: this is a hack
      this->value = Utf32To8(Utf16To32(s));
    } else if constexpr (std::is_same<std::u32string, TStringType>()) {
      this->value = Utf32To8(s);
    } else {
      this->value = s;
    }
    return this->value;
  }
  void build(std::ostream &stream) override {
    get_size();
    TStringType s;
    if constexpr (std::is_same<std::u16string, TStringType>()) {
      // FIXME: this is a hack
      s = Utf32To16(Utf8To32(this->value));
    } else if constexpr (std::is_same<std::u32string, TStringType>()) {
      s = Utf8To32(this->value);
    } else {
      s = this->value;
    }
    int64_t old_offset = stream.tellp();
    for (auto &c : s) {
      if constexpr (Endianess != std::endian::native) {
        c = std::byteswap(c);
      }
      stream.write((char *)&c, sizeof(typename TStringType::value_type));
    }
    assert(((int64_t)stream.tellp() - old_offset) <= (int64_t)size);
    for (size_t i = 0; i < size - (stream.tellp() - old_offset); i++) {
      stream.write("\0", 1);
    }
  }
};

using PaddedString8l = PaddedString<std::string, std::endian::little>;
using PaddedString16l = PaddedString<std::u16string, std::endian::little>;
using PaddedString32l = PaddedString<std::u32string, std::endian::little>;
using PaddedString8b = PaddedString<std::string, std::endian::big>;
using PaddedString16b = PaddedString<std::u16string, std::endian::big>;
using PaddedString32b = PaddedString<std::u32string, std::endian::big>;

} // namespace etcetera
