#include "basic.hpp"
#include "helpers.hpp"
#include "number.hpp"
#include "pointer.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace etcetera;

TEST_CASE("Pointer parse test") {
  auto s = Struct::create(Field("a", Int32ul::create()),
                          Field("b", Pointer::create(
                                         [](std::weak_ptr<Base> c) {
                                           return lock(c)->get<uint32_t>("a");
                                         },
                                         Int32ul::create())));
  uint32_t a = 10;
  char pad[10] = {0};
  uint32_t b = 456;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  orig.write(pad, sizeof(pad));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));

  s->parse(orig);
  REQUIRE(s->get<uint32_t>("a") == a);
  REQUIRE(s->get<uint32_t>("b") == b);
  REQUIRE(orig.tellg() == 4);
}

TEST_CASE("Pointer build test") {
  auto s = Struct::create(Field("a", Int32ul::create()),
                          Field("b", Pointer::create(
                                         [](std::weak_ptr<Base> c) {
                                           return lock(c)->get<uint32_t>("a");
                                         },
                                         Int32ul::create())),
                          Field("c", Int32ul::create()),
                          Field("d", Int32ul::create()),
                          Field("e", Int32ul::create()));
  uint32_t a = 12;
  uint32_t b = 456;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  REQUIRE(orig.tellp() == 4);
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  REQUIRE(orig.tellp() == 8);
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  REQUIRE(orig.tellp() == 12);
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  REQUIRE(orig.tellp() == 16);
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));
  REQUIRE(orig.tellp() == 20);

  std::string str;
  str.resize(s->get_size({}), '\0');
  std::stringstream ss(str);
  s->parse(orig);
  s->build(ss);
  ss.seekg(0, std::ios::beg);
  orig.seekg(0, std::ios::beg);
  for (int i = 0; i < 5; i++) {
    REQUIRE(ss.get() == orig.get());
  }
  REQUIRE(ss.str() == orig.str());
}
