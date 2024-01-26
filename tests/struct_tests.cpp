#include "basic.hpp"
#include "number.hpp"
#include "struct.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace etcetera;

TEST_CASE("Structs parsing") {
  auto s = Struct::create(Field("b", Int32sl::create()),
                          Field("a", Int32sl::create()));
  std::stringstream data;
  int32_t b = 0x87654321;
  int32_t a = 0x12345678;
  data.write(reinterpret_cast<const char *>(&b), sizeof(b));
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));

  tsl::ordered_map<std::string, std::any> obj =
      std::any_cast<tsl::ordered_map<std::string, std::any>>(s->parse(data));
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

  tsl::ordered_map<std::string, std::any> obj =
      std::any_cast<tsl::ordered_map<std::string, std::any>>(s->parse(data));

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

TEST_CASE("Struct Reuse Pattern") {
  auto Header = []() {
    return Struct::create(Field("offset", Int32ul::create()),
                          Field("size", Int32ul::create()),
                          Field("count", Int32ul::create()));
  };

  auto s =
      Struct::create(Field("header1", Header()), Field("header2", Header()),
                     Field("header3", Header()), Field("header4", Header()));
}

TEST_CASE("Size Test Structs") {
  auto s = Struct::create(Field("a", Int32sl::create()),
                          Field("b", Int32sl::create()));
  REQUIRE(s->get_size({}) == 8);
}

TEST_CASE("Struct XML parsing") {
  auto field = Struct::create(Field("a", Int32sl::create()),
                              Field("b", Int32sl::create()));

  auto xml_str = R"(<root><test a="1" b="2"></root>)";

  pugi::xml_document doc;
  doc.load_string(xml_str);
  field->parse_xml(doc.child("root"), "test", false);
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
  field->parse_xml(doc.child("root"), "test", false);
  REQUIRE(field->get<int32_t>("a") == 1);
  REQUIRE(field->get<int32_t>("b", "c") == 2);
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
