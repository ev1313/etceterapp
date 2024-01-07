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

  auto a_out = std::any_cast<int32_t>(s.get("a"));
  auto b_out = std::any_cast<int32_t>(s.get("b"));

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
}

/*
TEST_CASE("Struct building") {
  etc::Struct s(etc::Field("a", new etc::Int32ul()),
                etc::Field("b", new etc::Int32ul()));
  std::stringstream data;
  int32_t a = 0x12345678;
  int32_t b = 0x87654321;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  s.get("a") = a;
  s.get("b") = b;
  std::stringstream ss;
  s.build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.get() == data.get());
  REQUIRE(ss.get() == data.get());
  REQUIRE(ss.get() == data.get());
  REQUIRE(ss.get() == data.get());
}
*/
