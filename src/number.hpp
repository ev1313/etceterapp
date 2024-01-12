#pragma once

#include "basic.hpp"

namespace etcetera {

template <typename TNumberType, std::endian Endianess>
class NumberType : public Base {
public:
  using Base::get;
  using Base::get_field;
  TNumberType value = 0;
  NumberType(PrivateBase) : Base(PrivateBase()) {}
  static std::shared_ptr<NumberType> create() {
    return std::make_shared<NumberType>(PrivateBase());
  }
  std::any parse(std::iostream &stream) override {
    stream.read(reinterpret_cast<char *>(&value), sizeof(value));
    if constexpr (Endianess != std::endian::native) {
      union {
        TNumberType value;
        char bytes[sizeof(TNumberType)];
      } bswap;
      bswap.value = value;
      for (size_t i = 0; i < sizeof(TNumberType) / 2; i++) {
        std::swap(bswap.bytes[i], bswap.bytes[sizeof(TNumberType) - i - 1]);
      }
      value = bswap.value;
    }
    return value;
  }
  void build(std::iostream &stream) override {
    auto val = value;
    if constexpr (Endianess != std::endian::native) {
      union {
        TNumberType value;
        char bytes[sizeof(TNumberType)];
      } bswap;
      bswap.value = value;
      for (size_t i = 0; i < sizeof(TNumberType) / 2; i++) {
        std::swap(bswap.bytes[i], bswap.bytes[sizeof(TNumberType) - i - 1]);
      }
      val = bswap.value;
    }
    stream.write(reinterpret_cast<char *>(&val), sizeof(val));
  }
  std::any get() override { return value; }
  void set(std::any value) override {
    this->value = std::any_cast<TNumberType>(value);
  }
  bool is_simple_type() override { return true; }

  size_t get_size(std::weak_ptr<Base>) override { return sizeof(TNumberType); }

  void parse_xml(pugi::xml_node const &node, std::string name) override {
    auto s = node.attribute(name.c_str());
    value = s.as_int();
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    parent.append_attribute(name.c_str()) = value;
    return parent;
  }
};
using Int8sl = NumberType<int8_t, std::endian::little>;
using Int16sl = NumberType<int16_t, std::endian::little>;
using Int32sl = NumberType<int32_t, std::endian::little>;
using Int64sl = NumberType<int64_t, std::endian::little>;
using Int8ul = NumberType<uint8_t, std::endian::little>;
using Int16ul = NumberType<uint16_t, std::endian::little>;
using Int32ul = NumberType<uint32_t, std::endian::little>;
using Int64ul = NumberType<uint64_t, std::endian::little>;
using Float32l = NumberType<float, std::endian::little>;
using Float64l = NumberType<double, std::endian::little>;
using Int8sb = NumberType<int8_t, std::endian::big>;
using Int16sb = NumberType<int16_t, std::endian::big>;
using Int32sb = NumberType<int32_t, std::endian::big>;
using Int64sb = NumberType<int64_t, std::endian::big>;
using Int8ub = NumberType<uint8_t, std::endian::big>;
using Int16ub = NumberType<uint16_t, std::endian::big>;
using Int32ub = NumberType<uint32_t, std::endian::big>;
using Int64ub = NumberType<uint64_t, std::endian::big>;
using Float32b = NumberType<float, std::endian::big>;
using Float64b = NumberType<double, std::endian::big>;

} // namespace etcetera
