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

  void parse_xml(pugi::xml_node const &node, std::string name) override {
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
  CString(Base::PrivateBase) : String(PrivateBase()) {}
  static std::shared_ptr<CString> create() {
    return std::make_shared<CString>(PrivateBase());
  }

  size_t get_size(std::weak_ptr<Base>) override {
    TStringType s;
    if constexpr (std::is_same<std::u16string, TStringType>()) {
      // FIXME: this is a hack
      s = Utf32To16(Utf8To32(this->value));
    } else if constexpr (std::is_same<std::u32string, TStringType>()) {
      s = Utf8To32(this->value);
    } else {
      s = this->value;
    }
    return s.length() * sizeof(typename TStringType::value_type);
  }

  std::any parse(std::iostream &stream) override {
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
  void build(std::iostream &stream) override {
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
  }
};
using CString8l = CString<std::string, std::endian::little>;
using CString16l = CString<std::u16string, std::endian::little>;
using CString32l = CString<std::u32string, std::endian::little>;
using CString8b = CString<std::string, std::endian::big>;
using CString16b = CString<std::u16string, std::endian::big>;
using CString32b = CString<std::u32string, std::endian::big>;

} // namespace etcetera
