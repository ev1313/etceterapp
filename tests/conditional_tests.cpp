#include "conditional.hpp"
#include "number.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace etcetera;

TEST_CASE("IfThenElse") {
  auto s =
      Struct::create(Field("a", Int32sl::create()),
                     Field("b", IfThenElse::create(
                                    [](std::weak_ptr<Base> c) {
                                      return c.lock()->get<int32_t>("a") == 123;
                                    },
                                    Int32sl::create(), Int64ul::create())));

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
                                      return c.lock()->get<int32_t>("a") == 123;
                                    },
                                    Int32sl::create())));
  auto s2 =
      Struct::create(Field("a", Int32sl::create()),
                     Field("b", IfThenElse::create(
                                    [](std::weak_ptr<Base> c) {
                                      return c.lock()->get<int32_t>("a") == 123;
                                    },
                                    nullptr, Int32sl::create())));
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

  orig.clear();
  orig.seekg(0);
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  s2->parse(orig);
  REQUIRE(s2->get<int32_t>("a") == a);
  REQUIRE(s2->get<int32_t>("b") == a);
  REQUIRE(orig.peek() == EOF);
}
/*
TEST_CASE("Switch") {
  auto switch = Switch([](Base *c) { return "t1"; },
                            SwitchField("t1", new Int32sl()),
                            SwitchField("t2", new Int64ul()));
}
*/
