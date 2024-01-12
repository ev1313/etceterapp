#include "conditional.hpp"
#include "number.hpp"
#include "special.hpp"
#include "string.hpp"
#include <catch2/catch_test_macros.hpp>

#include <sstream>

using namespace etcetera;

TEST_CASE("Int XML parsing") {
  auto field = Int32sl::create();
  field->value = 0x0;

  auto xml_str = R"(<root test="123456789"></root>)";
  pugi::xml_document doc;
  doc.load_string(xml_str);
  field->parse_xml(doc.child("root"), "test");
  REQUIRE(field->value == 123456789);
}

TEST_CASE("Struct XML parsing") {
  auto field = Struct::create(Field("a", Int32sl::create()),
                              Field("b", Int32sl::create()));

  auto xml_str = R"(<root><test a="1" b="2"></root>)";

  pugi::xml_document doc;
  doc.load_string(xml_str);
  field->parse_xml(doc.child("root"), "test");
  REQUIRE(field->get<int32_t>("a") == 1);
  REQUIRE(field->get<int32_t>("b") == 2);
}

TEST_CASE("Nested Struct XML parsing") {
  auto field =
      Struct::create(Field("a", Int32sl::create()),
                     Field("b", Struct::create(Field("c", Int32sl::create()))));

  auto xml_str = R"(<root><test a="1"><b c="2"></b></test></root>)";

  pugi::xml_document doc;
  doc.load_string(xml_str);
  field->parse_xml(doc.child("root"), "test");
  REQUIRE(field->get<int32_t>("a") == 1);
  REQUIRE(field->get<int32_t>("b", "c") == 2);
}

TEST_CASE("Int XML building") {
  auto field = Int32sl::create();
  field->value = 123456789;

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field->build_xml(root, "test");
  std::stringstream ss;
  doc.save(ss);

  auto expected = R"(<?xml version="1.0"?>
<root test="123456789" />
)";
  REQUIRE(ss.str() == expected);
}

TEST_CASE("Struct XML building") {
  auto field = Struct::create(Field("a", Int32sl::create()),
                              Field("b", Int32sl::create()));
  field->get_field<Int32sl>("a").lock()->value = 1;
  field->get_field<Int32sl>("b").lock()->value = 2;

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field->build_xml(root, "test");
  std::stringstream ss;
  doc.save(ss);

  auto expected = R"(<?xml version="1.0"?>
<root>
	<test a="1" b="2" />
</root>
)";
  REQUIRE(ss.str() == expected);
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

TEST_CASE("LazyBound XML Parsing") {
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
}
