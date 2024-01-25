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
  uint32_t a = 14;
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
  REQUIRE(lock(s->get_field<Pointer>("b"))->get_ptr_offset({}) == 14);
  REQUIRE(lock(s->get_field<Pointer>("b"))->get_ptr_size({}) == 4);
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
  uint32_t a = 16;
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
  REQUIRE(ss.str() == orig.str());

  REQUIRE(lock(s->get_field<Pointer>("b"))->get_ptr_offset({}) == 16);
  REQUIRE(lock(s->get_field<Pointer>("b"))->get_ptr_size({}) == 4);
}

TEST_CASE("Area parse test") {
  auto s = Struct::create(
      Field("off", Int32ul::create()), Field("size", Int32ul::create()),
      Field("b", Area::create(
                     [](std::weak_ptr<Base> c) {
                       return lock(c)->get<uint32_t>("off");
                     },
                     [](std::weak_ptr<Base> c) {
                       return lock(c)->get<uint32_t>("size");
                     },
                     []() { return Int32ul::create(); })));
  uint32_t off = 18;
  uint32_t size = 40;
  char pad[10] = {0};
  uint32_t data[10] = {456, 789, 123, 456, 789, 123, 456, 789, 123, 456};
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&off), sizeof(off));
  orig.write(reinterpret_cast<const char *>(&size), sizeof(size));
  orig.write(pad, sizeof(pad));
  orig.write(reinterpret_cast<const char *>(&data), sizeof(data));

  s->parse(orig);
  REQUIRE(s->get<uint32_t>("off") == off);
  REQUIRE(s->get<uint32_t>("size") == size);
  REQUIRE(orig.tellg() == 8);
  REQUIRE(lock(s->get_field<Area>("b"))->get_ptr_offset({}) == off);
  REQUIRE(lock(s->get_field<Area>("b"))->get_ptr_size({}) == size);
}

TEST_CASE("Area build test") {
  auto s = Struct::create(
      Field("off", Int32ul::create()), Field("size", Int32ul::create()),
      Field("b", Area::create(
                     [](std::weak_ptr<Base> c) {
                       return lock(c)->get<uint32_t>("off");
                     },
                     [](std::weak_ptr<Base> c) {
                       return lock(c)->get<uint32_t>("size");
                     },
                     []() { return Int32ul::create(); })));
  uint32_t off = 18;
  uint32_t size = 40;
  char pad[10] = {0};
  uint32_t data[10] = {456, 789, 123, 456, 789, 123, 456, 789, 123, 456};
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&off), sizeof(off));
  orig.write(reinterpret_cast<const char *>(&size), sizeof(size));
  orig.write(pad, sizeof(pad));
  orig.write(reinterpret_cast<const char *>(&data), sizeof(data));

  s->parse(orig);
  std::stringstream ss(
      std::string(s->get_size({}) + sizeof(data) + sizeof(pad), '\0'));
  s->build(ss);
  ss.seekg(0, std::ios::beg);
  orig.seekg(0, std::ios::beg);
  REQUIRE(ss.str() == orig.str());
}
