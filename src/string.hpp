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

template <typename TStringType = std::string> class CString : public String {
public:
  std::endian endianess;
  using Base::get;
  using Base::get_field;
  CString(std::endian e, Base::PrivateBase)
      : String(PrivateBase()), endianess(e) {}
  static std::shared_ptr<CString> create(std::endian e = std::endian::native) {
    return std::make_shared<CString>(e, PrivateBase());
  }

  size_t get_size(std::weak_ptr<Base>) override {
    return value.length() * sizeof(char8_t);
  }

  std::any parse(std::iostream &stream) override {
    char c[sizeof(typename TStringType::value_type)];
    TStringType s;
    s.clear();
    while (stream.read(c, sizeof(typename TStringType::value_type))) {
      s.push_back(*((typename TStringType::value_type *)c));
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
      stream.write((char *)&c, sizeof(typename TStringType::value_type));
    }
  }
};
using CString8 = CString<std::string>;
using CString16 = CString<std::u16string>;
using CString32 = CString<std::u32string>;

} // namespace etcetera
