#include "etcetera.hpp"
#include <catch2/catch_test_macros.hpp>

#include <sstream>

namespace etc = etcetera;
/*
TEST_CASE("Int XML parsing") {
  etc::Int32ul field;
  field.value = 0x0;

  auto xml_str = R"(<root test="123456789"></root>)";
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(xml_str);
  field.parse_xml(doc.child("root"), "test");
  REQUIRE(field.value == 123456789);
}

TEST_CASE("Struct XML parsing") {
  auto field = etc::Struct(etc::Field("a", new etc::Int32ul()),
                           etc::Field("b", new etc::Int32ul()));

  auto xml_str = R"(<root><test a="1" b="2"></root>)";

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(xml_str);
  field.parse_xml(doc.child("root"), "test");
  REQUIRE(field.get<int32_t>("a") == 1);
  REQUIRE(field.get<int32_t>("b") == 2);
}

TEST_CASE("Nested Struct XML parsing") {
  auto field = etc::Struct(
      etc::Field("a", new etc::Int32ul()),
      etc::Field("b", new etc::Struct(etc::Field("c", new etc::Int32ul()))));

  auto xml_str = R"(<root><test a="1"><b c="2"></b></test></root>)";

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(xml_str);
  field.parse_xml(doc.child("root"), "test");
  REQUIRE(field.get<int32_t>("a") == 1);
  REQUIRE(field.get<int32_t>("b", "c") == 2);
}

TEST_CASE("Int XML building") {
  etc::Int32ul field;
  field.value = 123456789;

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field.build_xml(root, "test");
  std::stringstream ss;
  doc.save(ss);

  auto expected = R"(<?xml version="1.0"?>
<root test="123456789" />
)";
  REQUIRE(ss.str() == expected);
}

TEST_CASE("Struct XML building") {
  auto field = etc::Struct(etc::Field("a", new etc::Int32ul()),
                           etc::Field("b", new etc::Int32ul()));
  field.get_field<etc::Int32ul>("a")->value = 1;
  field.get_field<etc::Int32ul>("b")->value = 2;

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field.build_xml(root, "test");
  std::stringstream ss;
  doc.save(ss);

  auto expected = R"(<?xml version="1.0"?>
<root>
        <test a="1" b="2" />
</root>
)";
  REQUIRE(ss.str() == expected);
}
*/
/*
TEST_CASE("Array XML Building") {
  auto field =
      etc::Array(2, ARR_ITEM(etc::Struct(etc::Field("a", new etc::Int32ul()),
                                         etc::Field("b", new etc::Int32ul()))));
  field.get_field<etc::Int32ul>(0, "a")->value = 1;
  field.get_field<etc::Int32ul>(0, "b")->value = 2;
  field.get_field<etc::Int32ul>(1, "a")->value = 3;
  field.get_field<etc::Int32ul>(1, "b")->value = 4;

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field.build_xml(root, "test");
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
*/
