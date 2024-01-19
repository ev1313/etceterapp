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
  a->init_fields();
  REQUIRE(a->get_offset() == 0);
  REQUIRE(a->get_offset(0) == 0);
  REQUIRE(a->get_offset(1) == 4);
  REQUIRE(a->get_offset(2) == 8);
}

TEST_CASE("Offset Tests Nested Array") {
  auto a = Array::create(
      3, []() { return Array::create(2, []() { return Int32ul::create(); }); });

  int32_t data[6] = {0, 1, 2, 3, 4, 5};
  std::stringstream ss;
  ss.write(reinterpret_cast<char *>(data), sizeof(data));

  a->parse(ss);

  REQUIRE(a->get_offset() == 0);
  REQUIRE(a->get_offset(0) == 0);
  REQUIRE(a->get_offset(1) == 8);
  REQUIRE(a->get_offset(2) == 16);
  REQUIRE(a->get_offset(0, 0) == 0);
  REQUIRE(a->get_offset(0, 1) == 4);
  REQUIRE(a->get_offset(1, 0) == 8);
  REQUIRE(a->get_offset(1, 1) == 12);
  REQUIRE(a->get_offset(2, 0) == 16);
  REQUIRE(a->get_offset(2, 1) == 20);
}
