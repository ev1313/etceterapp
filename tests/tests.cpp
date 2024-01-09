#include "etcetera.hpp"
#include <catch2/catch_test_macros.hpp>

#include <sstream>

namespace etc = etcetera;

TEST_CASE("Int parsing test") {
  auto field = etc::Int32ul::create();
  std::stringstream ss;
  int32_t i = 0x12345678;
  ss.write(reinterpret_cast<const char *>(&i), sizeof(i));
  REQUIRE(std::any_cast<int32_t>(field->parse(ss)) == i);
  REQUIRE(field->value == i);
  int32_t x = field->get<int32_t>();
  REQUIRE(x == i);
}
TEST_CASE("Int building test") {
  auto field = etc::Int32ul::create();
  int32_t i = 0x12345678;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&i), sizeof(i));
  std::stringstream ss;
  field->value = i;
  field->build(ss);
  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.get() == orig.get());
  REQUIRE(ss.get() == orig.get());
  REQUIRE(ss.get() == orig.get());
  REQUIRE(ss.get() == orig.get());
}

TEST_CASE("Int8ul parsing test") {
  auto field = etc::Int8ul::create();
  std::stringstream ss, orig;
  int8_t i = 0x12;
  ss.write(reinterpret_cast<const char *>(&i), sizeof(i));
  orig.write(reinterpret_cast<const char *>(&i), sizeof(i));
  REQUIRE(std::any_cast<int8_t>(field->parse(ss)) == i);
  REQUIRE(field->value == i);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.get() == orig.get());
}

TEST_CASE("Structs parsing") {
  auto s = etc::Struct::create(etc::Field("a", etc::Int32ul::create()),
                               etc::Field("b", etc::Int32ul::create()));
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
  auto s = etc::Struct::create(
      etc::Field("a", etc::Int32ul::create()),
      etc::Field("b", etc::Int32ul::create()),
      etc::Field("c",
                 etc::Struct::create(etc::Field("d", etc::Int32ul::create()),
                                     etc::Field("e", etc::Int32ul::create()))));
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

  s->get_field<etc::Int32ul>("c", "_", "a").lock()->value = 12;
  REQUIRE(s->get<int32_t>("c", "_", "a") == 12);
}

TEST_CASE("Struct building") {
  auto s = etc::Struct::create(etc::Field("a", etc::Int32ul::create()),
                               etc::Field("b", etc::Int32ul::create()));
  std::stringstream data;
  int32_t a = 0x12345678;
  int32_t b = 0x87654321;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  s->get_field<etc::Int32ul>("a").lock()->value = a;
  s->get_field<etc::Int32ul>("b").lock()->value = b;
  std::stringstream ss;
  s->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str() == data.str());
}

TEST_CASE("Array") {
  auto arr = etc::Array::create(4, []() { return etc::Int32ul::create(); });
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
  auto arr2 = etc::Array::create(2, []() {
    return etc::Struct::create(etc::Field("a", etc::Int32ul::create()),
                               etc::Field("b", etc::Int32ul::create()));
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
  auto arr = etc::Array::create(
      2, ARR_ITEM(etc::Array::create(2, ARR_ITEM(etc::Int32ul::create()))));
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
/*
TEST_CASE("Switch") {
  auto switch = etc::Switch([](Base *c) { return "t1"; },
                            SwitchField("t1", new etc::Int32ul()),
                            SwitchField("t2", new etc::Int64ul()));
}
*/
