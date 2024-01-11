#include "etcetera.hpp"
#include <catch2/catch_test_macros.hpp>

#include "utfconv.hpp"

#include <sstream>

using namespace etcetera;

TEST_CASE("Int parsing test") {
  auto field = Int32sl::create();
  std::stringstream ss;
  int32_t i = 0x12345678;
  ss.write(reinterpret_cast<const char *>(&i), sizeof(i));
  REQUIRE(std::any_cast<int32_t>(field->parse(ss)) == i);
  REQUIRE(field->value == i);
  int32_t x = field->get<int32_t>();
  REQUIRE(x == i);
}

TEST_CASE("Int building test") {
  auto field = Int32sl::create();
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
  auto field = Int8ul::create();
  std::stringstream ss, ss2;
  int8_t i = 0x12;
  ss.write(reinterpret_cast<const char *>(&i), sizeof(i));
  REQUIRE(std::any_cast<uint8_t>(field->parse(ss)) == i);
  REQUIRE(field->value == i);

  field->build(ss2);

  ss.seekg(0);
  ss2.seekg(0);
  REQUIRE(ss.str() == ss2.str());
}

TEST_CASE("String UTF-32") {
  auto field = CString32::create();
  std::stringstream ss, orig;
  std::u32string s = U"abcd";
  orig.write((char *)s.c_str(), s.length() * sizeof(char32_t));
  auto ret = field->parse(orig);
  REQUIRE(Utf8To32(field->value) == s);
  field->build(ss);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("String UTF-16") {
  auto field = CString16::create();
  std::stringstream ss, orig;
  std::u16string s = u"abcd";
  orig.write((char *)s.c_str(), s.length() * sizeof(char16_t));
  auto ret = field->parse(orig);
  REQUIRE(Utf32To16(Utf8To32(field->value)) == s);
  field->build(ss);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("String UTF-8") {
  auto field = CString8::create();
  std::stringstream ss, orig;
  std::string s = "abcd";
  orig.write(s.c_str(), s.length());
  REQUIRE(std::any_cast<std::string>(field->parse(orig)) == s);
  REQUIRE(field->value == s);
  REQUIRE(field->get<std::string>() == s);
  field->build(ss);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
}

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
                                          c.lock()->get<int32_t>("a") + 333);
                                    },
                                    Int32sl::create())),
                     Field("a", Int32sl::create()));
  int32_t a = 123;
  int32_t b = a + 333;

  s->get_field<Int32sl>("a").lock()->value = a;
  std::stringstream orig;
  orig.write(reinterpret_cast<const char *>(&a), sizeof(int32_t));
  orig.write(reinterpret_cast<const char *>(&b), sizeof(int32_t));
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
                                          c.lock()->get<int32_t>("c") + 111);
                                    },
                                    Int32sl::create())),
                     Field("b", Int32sl::create()),
                     Field("c", Rebuild::create(
                                    [](std::weak_ptr<Base> c) {
                                      return std::make_any<int32_t>(
                                          c.lock()->get<int32_t>("b") + 111);
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

TEST_CASE("LazyBound") {
  auto s = LazyBound::create([](std::weak_ptr<LazyBound> p) {
    return Struct::create(
        Field("a", Int32sl::create()), Field("b", Int32sl::create()),
        Field("c", IfThenElse::create(
                       [](std::weak_ptr<Base> c) {
                         return c.lock()->get<int32_t>("a") == 123;
                       },
                       LazyBound::create(p.lock()->get_lazy_fn()))));
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
/*
TEST_CASE("Switch") {
  auto switch = Switch([](Base *c) { return "t1"; },
                            SwitchField("t1", new Int32sl()),
                            SwitchField("t2", new Int64ul()));
}
*/
