#include "basic.hpp"
#include "number.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace etcetera;

TEST_CASE("Structs parsing") {
  auto s = Struct::create(Field("a", Int32sl::create()),
                          Field("b", Int32sl::create()));
  std::stringstream data;
  int32_t a = 0x12345678;
  int32_t b = 0x87654321;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));

  std::map<std::string, std::any> obj =
      std::any_cast<std::map<std::string, std::any>>(s->parse(data));
  REQUIRE(std::any_cast<int32_t>(obj["a"]) == a);
  REQUIRE(std::any_cast<int32_t>(obj["b"]) == b);

  auto a_out = s->get<int32_t>("a");
  auto b_out = s->get<int32_t>("b");

  REQUIRE(a_out == a);
  REQUIRE(b_out == b);
}

TEST_CASE("nested Structs") {
  auto s = Struct::create(
      Field("a", Int32sl::create()), Field("b", Int32sl::create()),
      Field("c", Struct::create(Field("d", Int32sl::create()),
                                Field("e", Int32sl::create()))));
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
      std::any_cast<std::map<std::string, std::any>>(s->parse(data));

  REQUIRE(s->get<int32_t>("a") == a);
  REQUIRE(s->get<int32_t>("b") == b);
  REQUIRE(s->get<int32_t>("c", "d") == d);
  REQUIRE(s->get<int32_t>("c", "e") == e);
  REQUIRE(s->get<int32_t>("c", "_", "a") == a);
  REQUIRE(s->get<int32_t>("c", "_", "b") == b);

  s->get_field<Int32sl>("c", "_", "a").lock()->value = 12;
  REQUIRE(s->get<int32_t>("c", "_", "a") == 12);
}

TEST_CASE("Struct building") {
  auto s = Struct::create(Field("a", Int32sl::create()),
                          Field("b", Int32sl::create()));
  std::stringstream data;
  int32_t a = 0x12345678;
  int32_t b = 0x87654321;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  s->get_field<Int32sl>("a").lock()->value = a;
  s->get_field<Int32sl>("b").lock()->value = b;
  std::stringstream ss;
  s->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str() == data.str());
}

TEST_CASE("Array") {
  auto arr = Array::create(4, []() { return Int32sl::create(); });
  std::stringstream data;
  int32_t a = 0x12345678;
  int32_t b = 0x87654321;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  arr->parse(data);
  REQUIRE(arr->get<int32_t>(0) == a);
  REQUIRE(arr->get<int32_t>(1) == b);
  REQUIRE(arr->get<int32_t>(2) == a);
  REQUIRE(arr->get<int32_t>(3) == b);

  data.seekg(0);
  auto arr2 = Array::create(2, []() {
    return Struct::create(Field("a", Int32sl::create()),
                          Field("b", Int32sl::create()));
  });
  arr2->parse(data);
  for (size_t i = 0; i < 2; i++) {
    REQUIRE(arr2->get<int32_t>(i, "a") == a);
    REQUIRE(arr2->get<int32_t>(i, "b") == b);
  }
  std::stringstream ss;
  arr2->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str() == data.str());
}

TEST_CASE("Nested Arrays") {
  auto arr = Array::create(
      2, []() { return Array::create(2, []() { return Int32sl::create(); }); });

  std::stringstream data;
  std::stringstream orig;
  int32_t a = 0x12345678;
  int32_t b = 0x87654321;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));
  arr->parse(data);
  REQUIRE(arr->get<int32_t>(0, 0) == a);
  REQUIRE(arr->get<int32_t>(0, 1) == b);
  REQUIRE(arr->get<int32_t>(1, 0) == a);
  REQUIRE(arr->get<int32_t>(1, 1) == b);
}
