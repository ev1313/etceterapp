#include "basic.hpp"
#include "number.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace etcetera;

TEST_CASE("Offset Tests without parent") {
  auto i = Int32ul::create();
  REQUIRE(i->get_offset() == 0);
  REQUIRE(i->get_size({}) == 4);
}

TEST_CASE("Offset Tests Struct") {
  auto s = Struct::create(Field("a", Int32ul::create()),
                          Field("b", Int32ul::create()));
  REQUIRE(s->get_offset() == 0);
  REQUIRE(s->get_offset("a") == 0);
  REQUIRE(s->get_offset("b") == 4);
}

TEST_CASE("Offset Tests Nested Struct") {
  auto s =
      Struct::create(Field("a", Int32ul::create()),
                     Field("b", Struct::create(Field("c", Int32ul::create()),
                                               Field("d", Int32ul::create()))));
  REQUIRE(s->get_offset() == 0);
  REQUIRE(s->get_offset("a") == 0);
  REQUIRE(s->get_offset("b") == 4);
  REQUIRE(s->get_offset("b", "c") == 4);
  REQUIRE(s->get_offset("b", "d") == 8);
}

TEST_CASE("Offset Tests Array") {
  auto a = Array::create(3, []() { return Int32ul::create(); });
  REQUIRE(a->get_offset() == 0);
  REQUIRE(a->get_offset(0) == 0);
  REQUIRE(a->get_offset(1) == 4);
  REQUIRE(a->get_offset(2) == 8);
}
