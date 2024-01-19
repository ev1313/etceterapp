#include <catch2/catch_test_macros.hpp>

#include "conditional.hpp"
#include "helpers.hpp"
#include "number.hpp"

using namespace etcetera;

TEST_CASE("IfThenElse") {
  auto s =
      Struct::create(Field("a", Int32sl::create()),
                     Field("b", IfThenElse::create(
                                    [](std::weak_ptr<Base> c) {
                                      return lock(c)->get<int32_t>("a") == 123;
                                    },
                                    Field("c", Int32sl::create()),
                                    Field("d", Int64ul::create()))));

  int32_t a = 123;
  int32_t b = 456;
  uint64_t c = 789;

  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));

  s->parse(orig);
  REQUIRE(s->get<int32_t>("a") == a);
  REQUIRE(s->get<int32_t>("b") == b);
  REQUIRE(orig.peek() == EOF);

  orig.str(std::string());
  orig.seekg(0);
  a += 1;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  orig.write(reinterpret_cast<const char *>(&c), sizeof(c));
  s->parse(orig);
  REQUIRE(s->get<int32_t>("a") == a);
  REQUIRE(s->get<uint64_t>("b") == c);
  REQUIRE(orig.peek() == EOF);
}

TEST_CASE("If without Else") {
  auto s =
      Struct::create(Field("a", Int32sl::create()),
                     Field("b", IfThenElse::create(
                                    [](std::weak_ptr<Base> c) {
                                      return lock(c)->get<int32_t>("a") == 123;
                                    },
                                    Field("c", Int32sl::create()))));
  int32_t a = 123;
  int32_t b = 456;

  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));

  s->parse(orig);
  REQUIRE(s->get<int32_t>("a") == a);
  REQUIRE(s->get<int32_t>("b") == b);

  orig.str(std::string());
  orig.seekg(0);
  a += 1;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  s->parse(orig);
  REQUIRE(s->get<int32_t>("a") == a);
  REQUIRE(orig.peek() == EOF);
}

TEST_CASE("Switch parse") {
  using SField = Switch<int32_t>::SwitchField;
  auto s = Struct::create(
      Field("a", Int32sl::create()),
      Field(
          "b",
          Switch<int32_t>::create(
              [](std::weak_ptr<Base> c) { return lock(c)->get<int32_t>("a"); },
              SField(-0x1, "Int32ul", []() { return Int32ul::create(); }),
              SField(0x0, "Int32", []() { return Int32sl::create(); }),
              SField(0x1, "Int64", []() { return Int32ul::create(); }),
              SField(0x2, "Struct", []() {
                return Struct::create(Field("a", Int32ul::create()));
              }))));

  std::stringstream ss;
  int32_t a = 0x0;
  int32_t b = 0x12345678;
  int32_t c = 0x2;
  int32_t d = 0x87654321;
  ss.write(reinterpret_cast<const char *>(&a), sizeof(a));
  ss.write(reinterpret_cast<const char *>(&b), sizeof(b));
  ss.write(reinterpret_cast<const char *>(&c), sizeof(c));
  ss.write(reinterpret_cast<const char *>(&d), sizeof(d));

  s->parse(ss);
  REQUIRE(s->get<int32_t>("a") == a);
  REQUIRE(s->get<int32_t>("b") == b);
  s->parse(ss);
  REQUIRE(s->get<int32_t>("a") == c);
  REQUIRE(s->get<uint32_t>("b", "a") == d);
}

TEST_CASE("Switch build") {
  using SField = Switch<int32_t>::SwitchField;
  auto s = Struct::create(
      Field("a", Int32sl::create()),
      Field(
          "b",
          Switch<int32_t>::create(
              [](std::weak_ptr<Base> c) { return lock(c)->get<int32_t>("a"); },
              SField(-0x1, "Int32ul", []() { return Int32ul::create(); }),
              SField(0x0, "Int32", []() { return Int32sl::create(); }),
              SField(0x1, "Int64", []() { return Int32ul::create(); }),
              SField(0x2, "Struct", []() {
                return Struct::create(Field("a", Int32ul::create()));
              }))));

  std::stringstream orig;
  int32_t c = 0x2;
  int32_t d = 0x87654321;
  orig.write(reinterpret_cast<const char *>(&c), sizeof(c));
  orig.write(reinterpret_cast<const char *>(&d), sizeof(d));
  std::stringstream ss;
  s->parse(orig);
  s->build(ss);
  REQUIRE(ss.str() == orig.str());
}
