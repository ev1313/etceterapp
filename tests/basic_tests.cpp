#include "basic.hpp"
#include "number.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace etcetera;

TEST_CASE("Const int") {
  auto field = Const<int32_t>::create(0x12345678);
  std::stringstream data;
  int32_t a = 0x12345678;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  REQUIRE(std::any_cast<int32_t>(field->parse(data)) == a);
  std::stringstream ss;
  field->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str() == data.str());
}

TEST_CASE("Const int XML") {
  auto field = Const<int32_t>::create(0x12345678);
  std::stringstream data;
  int32_t a = 0x12345678;
  data.write(reinterpret_cast<const char *>(&a), sizeof(a));
  REQUIRE(std::any_cast<int32_t>(field->parse(data)) == a);
  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field->build_xml(root, "test");
  REQUIRE(!root.attribute("test"));
  std::stringstream ss;
  field->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str() == data.str());
}

TEST_CASE("Const string") {
  auto field = BytesConst::create("\x00\x00\x01\x02\x03\x04");
  std::stringstream data;
  std::string a = "\x00\x00\x01\x02\x03\x04";
  data << a;
  REQUIRE(std::any_cast<std::string>(field->parse(data)) == a);
  std::stringstream ss;
  field->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str() == data.str());
}

TEST_CASE("Const string XML empty") {
  auto field = BytesConst::create("\x00\x00\x01\x02\x03\x04");
  std::stringstream data;
  std::string a = "\x00\x00\x01\x02\x03\x04";
  data << a;
  REQUIRE(std::any_cast<std::string>(field->parse(data)) == a);
  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field->build_xml(root, "test");
  REQUIRE(!root.attribute("test"));
  std::stringstream ss;
  field->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str() == data.str());
}

TEST_CASE("Bytes") {
  auto field = Bytes::create(6);
  std::stringstream data;
  std::vector<uint8_t> b = {0x00, 0x00, 0x01, 0x02, 0x03, 0x04};
  data.write(reinterpret_cast<char *>(b.data()), b.size());
  REQUIRE(std::any_cast<std::vector<uint8_t>>(field->parse(data)) == b);
  std::stringstream ss;
  field->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str().size() == data.str().size());
  REQUIRE(ss.str() == data.str());
}

TEST_CASE("Bytes XML") {
  auto field = Bytes::create(6);
  std::stringstream data;
  std::vector<uint8_t> b = {0x00, 0x00, 0x01, 0x02, 0x03, 0x04};
  data.write(reinterpret_cast<char *>(b.data()), b.size());
  REQUIRE(std::any_cast<std::vector<uint8_t>>(field->parse(data)) == b);
  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field->build_xml(root, "test");
  REQUIRE(std::string(root.attribute("test").as_string()) == "000001020304");
  std::stringstream ss;
  auto field2 = Bytes::create(6);
  field2->parse_xml(root, "test", false);
  field2->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str().size() == data.str().size());
  REQUIRE(ss.str() == data.str());
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

TEST_CASE("Size Test Structs") {
  auto s = Struct::create(Field("a", Int32sl::create()),
                          Field("b", Int32sl::create()));
  REQUIRE(s->get_size({}) == 8);
}

TEST_CASE("Size Test Arrays") {
  auto arr = Array::create(4, []() { return Int32sl::create(); });
  arr->init_fields();
  REQUIRE(arr->get_size({}) == 16);
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

TEST_CASE("Enum") {
  using EnumField = Enum<int32_t>::Field;
  auto e = Enum<int32_t>::create(EnumField("off", 0), EnumField("on", 1),
                                 EnumField("unknown", 2));
  std::stringstream data;
  data.write("\x00\x00\x00\x00", 4);
  data.write("\x00\x00\x00\x01", 4);
  data.write("\x00\x00\x00\x02", 4);
  e.parse(data);
  REQUIRE(e->get<int32_t>() == 0);
  e.parse(data);
  REQUIRE(e->get<int32_t>() == 1);
  e.parse(data);
  REQUIRE(e->get<int32_t>() == 2);

  std::stringstream ss;
  e->value = 0;
  e->build(ss);
  e->value = 1;
  e->build(ss);
  e->value = 2;
  e->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str() == data.str());
}
