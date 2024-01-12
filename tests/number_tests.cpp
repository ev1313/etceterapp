#include "number.hpp"
#include <catch2/catch_test_macros.hpp>

#include <sstream>

using namespace etcetera;

TEST_CASE("Int parsing test") {
  auto field = Int32sl::create();
  std::stringstream ss;
  int32_t i = 0x12345678;
  ss.write(reinterpret_cast<const char *>(&i), sizeof(i));
  REQUIRE(std::any_cast<int32_t>(field->parse(ss)) == i);
  REQUIRE(field->value == i);
  int32_t x = field->get<int32_t>();
  REQUIRE(x == i);
}

TEST_CASE("Int big endian parsing test") {
  auto field = Int32sb::create();
  std::stringstream ss;
  int32_t i = 0x12345678;
  ss.write(reinterpret_cast<const char *>(&i), sizeof(i));
  REQUIRE(std::any_cast<int32_t>(field->parse(ss)) == std::byteswap(i));
  REQUIRE(field->value == std::byteswap(i));
  int32_t x = field->get<int32_t>();
  REQUIRE(x == std::byteswap(i));
}

TEST_CASE("Int building test") {
  auto field = Int32sl::create();
  int32_t i = 0x12345678;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&i), sizeof(i));
  std::stringstream ss;
  field->value = i;
  field->build(ss);
  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("Int32 big endian building test") {
  auto field = Int32sb::create();
  int32_t i = 0x12345678;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&i), sizeof(i));
  std::stringstream ss;
  field->value = std::byteswap(i);
  field->build(ss);
  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("Int64 big endian building test") {
  auto field = Int64sb::create();
  int64_t i = 0x123456789abcdef;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&i), sizeof(i));
  std::stringstream ss;
  field->value = std::byteswap(i);
  field->build(ss);
  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("Int8ul parsing test") {
  auto field = Int8ul::create();
  std::stringstream ss, ss2;
  int8_t i = 0x12;
  ss.write(reinterpret_cast<const char *>(&i), sizeof(i));
  REQUIRE(std::any_cast<uint8_t>(field->parse(ss)) == i);
  REQUIRE(field->value == i);

  field->build(ss2);

  ss.seekg(0);
  ss2.seekg(0);
  REQUIRE(ss.str() == ss2.str());
}

TEST_CASE("Test number sizes") {
  REQUIRE(Int8ul::create()->get_size({}) == 1);
  REQUIRE(Int16ul::create()->get_size({}) == 2);
  REQUIRE(Int32ul::create()->get_size({}) == 4);
  REQUIRE(Int64ul::create()->get_size({}) == 8);

  REQUIRE(Int8sl::create()->get_size({}) == 1);
  REQUIRE(Int16sl::create()->get_size({}) == 2);
  REQUIRE(Int32sl::create()->get_size({}) == 4);
  REQUIRE(Int64sl::create()->get_size({}) == 8);

  REQUIRE(Int8ub::create()->get_size({}) == 1);
  REQUIRE(Int16ub::create()->get_size({}) == 2);
  REQUIRE(Int32ub::create()->get_size({}) == 4);
  REQUIRE(Int64ub::create()->get_size({}) == 8);

  REQUIRE(Int8sb::create()->get_size({}) == 1);
  REQUIRE(Int16sb::create()->get_size({}) == 2);
  REQUIRE(Int32sb::create()->get_size({}) == 4);
  REQUIRE(Int64sb::create()->get_size({}) == 8);

  REQUIRE(Float32l::create()->get_size({}) == 4);
  REQUIRE(Float64l::create()->get_size({}) == 8);
  REQUIRE(Float32b::create()->get_size({}) == 4);
  REQUIRE(Float64b::create()->get_size({}) == 8);
}
