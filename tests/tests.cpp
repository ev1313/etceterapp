#include "etcetera.hpp"
#include <catch2/catch_test_macros.hpp>

#include <sstream>

namespace etc = etcetera;

TEST_CASE("Int parsing test") {
  etc::Int32ul field;
  std::stringstream ss;
  int32_t i = 0x12345678;
  ss.write(reinterpret_cast<const char *>(&i), sizeof(i));
  REQUIRE(std::any_cast<int32_t>(field.parse(ss)) == i);
  REQUIRE(field.value == i);
}

TEST_CASE("Int building test") {
  etc::Int32ul field;
  int32_t i = 0x12345678;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&i), sizeof(i));
  std::stringstream ss;
  field.value = i;
  field.build(ss);
  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.get() == orig.get());
  REQUIRE(ss.get() == orig.get());
  REQUIRE(ss.get() == orig.get());
  REQUIRE(ss.get() == orig.get());
}

TEST_CASE("Int8ul parsing test") {
  etc::Int8ul field;
  std::stringstream ss, orig;
  int8_t i = 0x12;
  ss.write(reinterpret_cast<const char *>(&i), sizeof(i));
  orig.write(reinterpret_cast<const char *>(&i), sizeof(i));
  REQUIRE(std::any_cast<int8_t>(field.parse(ss)) == i);
  REQUIRE(field.value == i);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.get() == orig.get());
}

TEST_CASE("Structs parsing") {
  etc::Struct s(etc::Field("a", new etc::Int32ul()),
                etc::Field("b", new etc::Int32ul()));
  std::stringstream data;
  int32_t a = 0x12345678;
  int32_t b = 0x87654321;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));

  std::map<std::string, std::any> obj =
      std::any_cast<std::map<std::string, std::any>>(s.parse(data));
  REQUIRE(std::any_cast<int32_t>(obj["a"]) == a);
  REQUIRE(std::any_cast<int32_t>(obj["b"]) == b);

  auto a_out = s.get<int32_t>("a");
  auto b_out = s.get<int32_t>("b");

  REQUIRE(a_out == a);
  REQUIRE(b_out == b);
}

TEST_CASE("nested Structs") {
  etc::Struct s(
      etc::Field("a", new etc::Int32ul()), etc::Field("b", new etc::Int32ul()),
      etc::Field("c", new etc::Struct(etc::Field("d", new etc::Int32ul()),
                                      etc::Field("e", new etc::Int32ul()))));
  std::stringstream data;
  int32_t a = 0x12345678;
  int32_t b = 0x87654321;
  int32_t d = 0x11111111;
  int32_t e = 0x22222222;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  data.write(reinterpret_cast<const char *>(&d), sizeof(d));
  data.write(reinterpret_cast<const char *>(&e), sizeof(e));

  std::map<std::string, std::any> obj =
      std::any_cast<std::map<std::string, std::any>>(s.parse(data));

  REQUIRE(s.get<int32_t>("a") == a);
  REQUIRE(s.get<int32_t>("b") == b);
  REQUIRE(s.get<int32_t>("c", "d") == d);
  REQUIRE(s.get<int32_t>("c", "e") == e);
  REQUIRE(s.get<int32_t>("c", "_", "a") == a);
  REQUIRE(s.get<int32_t>("c", "_", "b") == b);

  s.get_field<etc::Int32ul>("c", "_", "a")->value = 12;
  REQUIRE(s.get<int32_t>("c", "_", "a") == 12);
}

TEST_CASE("Struct building") {
  etc::Struct s(etc::Field("a", new etc::Int32ul()),
                etc::Field("b", new etc::Int32ul()));
  std::stringstream data;
  int32_t a = 0x12345678;
  int32_t b = 0x87654321;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  s.get_field<etc::Int32ul>("a")->value = a;
  s.get_field<etc::Int32ul>("b")->value = b;
  std::stringstream ss;
  s.build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.get() == data.get());
  REQUIRE(ss.get() == data.get());
  REQUIRE(ss.get() == data.get());
  REQUIRE(ss.get() == data.get());
}

TEST_CASE("Array") { etc::Array<etc::Int32ul>(8); }
