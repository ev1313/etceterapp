#include "array.hpp"
#include "basic.hpp"
#include "helpers.hpp"
#include "number.hpp"
#include "struct.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace etcetera;

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

TEST_CASE("Size Test Arrays") {
  auto arr = Array::create(4, []() { return Int32sl::create(); });
  arr->init_fields();
  REQUIRE(arr->get_size({}) == 16);
}

TEST_CASE("Array XML Building") {
  auto field = Array::create(2, []() {
    return Struct::create(Field("a", Int32sl::create()),
                          Field("b", Int32sl::create()));
  });
  field->init_fields();
  field->get_field<Int32sl>(0, "a").lock()->value = 1;
  field->get_field<Int32sl>(0, "b").lock()->value = 2;
  field->get_field<Int32sl>(1, "a").lock()->value = 3;
  field->get_field<Int32sl>(1, "b").lock()->value = 4;

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field->build_xml(root, "test");
  std::stringstream ss;
  doc.save(ss);

  auto expected = R"(<?xml version="1.0"?>
<root>
	<test a="1" b="2" />
	<test a="3" b="4" />
</root>
)";
  REQUIRE(ss.str() == expected);
}

TEST_CASE("RepeatUntil until parse specific value") {
  auto f = RepeatUntil::create(
      [](std::weak_ptr<Base> v, std::weak_ptr<Base>) -> bool {
        return lock(v)->get<uint32_t>() == 123;
      },
      []() { return Int32ul::create(); });
  std::stringstream data;
  uint32_t a[10] = {1, 2, 3, 4, 5, 6, 7, 123, 8, 9};
  data.write(reinterpret_cast<const char *>(a), sizeof(a));
  f->parse(data);
  REQUIRE(f->get_size({}) == 32);
  REQUIRE(f->get<uint32_t>(0) == 1);
  REQUIRE(f->get<uint32_t>(1) == 2);
  REQUIRE(f->get<uint32_t>(2) == 3);
  REQUIRE(f->get<uint32_t>(3) == 4);
  REQUIRE(f->get<uint32_t>(4) == 5);
  REQUIRE(f->get<uint32_t>(5) == 6);
  REQUIRE(f->get<uint32_t>(6) == 7);
  REQUIRE(f->get<uint32_t>(7) == 123);
}

TEST_CASE("RepeatUntil until size") {
  auto f = RepeatUntil::create(
      [](std::weak_ptr<Base>, std::weak_ptr<Base>) { return false; },
      []() { return Int32ul::create(); },
      [](std::weak_ptr<Base>) { return 20; });

  std::stringstream data;
  uint32_t a[10] = {1, 2, 3, 4, 5, 6, 7, 123, 8, 9};
  data.write(reinterpret_cast<const char *>(a), sizeof(a));
  f->parse(data);
  REQUIRE(f->get_size({}) == 20);
  REQUIRE(f->get<uint32_t>(0) == 1);
  REQUIRE(f->get<uint32_t>(1) == 2);
  REQUIRE(f->get<uint32_t>(2) == 3);
  REQUIRE(f->get<uint32_t>(3) == 4);
  REQUIRE(f->get<uint32_t>(4) == 5);
}
