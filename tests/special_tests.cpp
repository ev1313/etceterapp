#include "conditional.hpp"
#include "helpers.hpp"
#include "number.hpp"
#include "special.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace etcetera;

TEST_CASE("Rebuild static") {
  auto s = Struct::create(Field("a", Int32sl::create()),
                          Field("b", Rebuild::create(
                                         [](std::weak_ptr<Base>) {
                                           return std::make_any<int32_t>(456);
                                         },
                                         Int32sl::create())));

  int32_t a = 123;
  int32_t b = 456;

  s->get_field<Int32sl>("a").lock()->value = a;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(int32_t));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(int32_t));
  std::stringstream ss;
  s->build(ss);
  ss.seekg(0);
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("Rebuild dynamic") {
  auto s =
      Struct::create(Field("b", Rebuild::create(
                                    [](std::weak_ptr<Base> c) {
                                      return std::make_any<int32_t>(
                                          lock(c)->get<int32_t>("a") + 333);
                                    },
                                    Int32sl::create())),
                     Field("a", Int32sl::create()));

  int32_t a = 123;
  int32_t b = a + 333;

  s->get_field<Int32sl>("a").lock()->value = a;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&b), sizeof(int32_t));
  orig.write(reinterpret_cast<const char *>(&a), sizeof(int32_t));
  std::stringstream ss;
  s->build(ss);
  ss.seekg(0);
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("Rebuild nested") {
  auto s =
      Struct::create(Field("a", Rebuild::create(
                                    [](std::weak_ptr<Base> c) {
                                      return std::make_any<int32_t>(
                                          lock(c)->get<int32_t>("c") + 111);
                                    },
                                    Int32sl::create())),
                     Field("b", Int32sl::create()),
                     Field("c", Rebuild::create(
                                    [](std::weak_ptr<Base> c) {
                                      return std::make_any<int32_t>(
                                          lock(c)->get<int32_t>("b") + 111);
                                    },
                                    Int32sl::create())));
  int32_t b = 123;
  int32_t c = b + 111;
  int32_t a = c + 111;

  s->get_field<Int32sl>("b").lock()->value = b;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(int32_t));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(int32_t));
  orig.write(reinterpret_cast<const char *>(&c), sizeof(int32_t));
  std::stringstream ss;
  s->build(ss);
  ss.seekg(0);
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("LazyBound") {
  auto s = LazyBound::create([](std::weak_ptr<LazyBound> p) {
    return Struct::create(
        Field("a", Int32sl::create()), Field("b", Int32sl::create()),
        Field("c", IfThenElse::create(
                       [](std::weak_ptr<Base> c) {
                         return lock(c)->get<int32_t>("a") == 123;
                       },
                       Field("d", LazyBound::create(lock(p)->get_lazy_fn())))));
  });

  int32_t a = 123;
  int32_t b = 456;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));
  orig.write(reinterpret_cast<const char *>(&a), sizeof(a));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(b));
  s->parse(orig);
  REQUIRE(s->get<int32_t>("a") == a);
  REQUIRE(s->get<int32_t>("b") == b);
  REQUIRE(s->get<int32_t>("c", "a") == a);
  REQUIRE(s->get<int32_t>("c", "b") == b);
  REQUIRE(s->get<int32_t>("c", "c", "a") == b);
  REQUIRE(s->get<int32_t>("c", "c", "b") == b);
  REQUIRE(orig.peek() == EOF);
}

TEST_CASE("LazyBound XML Parsing") {
  auto s = LazyBound::create([](std::weak_ptr<LazyBound> p) {
    return Struct::create(
        Field("a", Int32sl::create()), Field("b", Int32sl::create()),
        Field("c", IfThenElse::create(
                       [](std::weak_ptr<Base> c) {
                         return lock(c)->get<int32_t>("a") == 123;
                       },
                       Field("d", LazyBound::create(lock(p)->get_lazy_fn())))));
  });

  auto xml_str =
      R"(<root a="123" b="2"><d a="123" b="3"><d a="0" b="4"/></d></root>)";
  pugi::xml_document doc;
  doc.load_string(xml_str);

  s->parse_xml(doc.child("root"), "root", true);
  REQUIRE(s->get<int32_t>("a") == 123);
  REQUIRE(s->get<int32_t>("b") == 2);
  REQUIRE(s->get<int32_t>("c", "a") == 123);
  REQUIRE(s->get<int32_t>("c", "b") == 3);
  REQUIRE(s->get<int32_t>("c", "c", "a") == 0);
  REQUIRE(s->get<int32_t>("c", "c", "b") == 4);
}

TEST_CASE("Aligned parsing") {
  auto s =
      Struct::create(Field("front", Int32ul::create()),
                     Field("aligned", Aligned::create(7, Int32ul::create())),
                     Field("back", Int32ul::create()));

  uint32_t front = 123;
  uint32_t aligned = 888;
  char pad[3] = {0};
  uint32_t back = 456;

  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&front), sizeof(front));
  orig.write(reinterpret_cast<const char *>(&aligned), sizeof(aligned));
  orig.write(pad, sizeof(pad));
  orig.write(reinterpret_cast<const char *>(&back), sizeof(back));

  s->parse(orig);
  REQUIRE(s->get<uint32_t>("front") == front);
  REQUIRE(s->get<uint32_t>("aligned") == aligned);
  REQUIRE(s->get<uint32_t>("back") == back);
  REQUIRE(orig.peek() == EOF);
  REQUIRE(s->get_size() == 15);
}

TEST_CASE("Aligned building") {
  auto s =
      Struct::create(Field("front", Int32ul::create()),
                     Field("aligned", Aligned::create(7, Int32ul::create())),
                     Field("back", Int32ul::create()));

  uint32_t front = 123;
  uint32_t aligned = 888;
  char pad[3] = {0};
  uint32_t back = 456;

  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&front), sizeof(front));
  orig.write(reinterpret_cast<const char *>(&aligned), sizeof(aligned));
  orig.write(pad, sizeof(pad));
  orig.write(reinterpret_cast<const char *>(&back), sizeof(back));

  s->parse(orig);
  std::stringstream ss(std::string(s->get_size(), '\0'));
  ss.seekp(0, std::ios_base::beg);
  s->build(ss);
  REQUIRE(ss.str().size() == orig.str().size());
  REQUIRE(ss.str() == orig.str());
}
