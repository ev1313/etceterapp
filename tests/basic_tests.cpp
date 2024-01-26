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

TEST_CASE("Enum") {
  using EnumField = Enum<int32_t>::Field;
  auto e = Enum<int32_t, std::endian::big>::create(
      EnumField("off", 0), EnumField("on", 1), EnumField("unknown", 2));
  std::stringstream data;
  data.write("\x00\x00\x00\x00", 4);
  data.write("\x00\x00\x00\x01", 4);
  data.write("\x00\x00\x00\x02", 4);
  e->parse(data);
  REQUIRE(e->get<int32_t>() == 0);
  e->parse(data);
  REQUIRE(e->get<int32_t>() == 1);
  e->parse(data);
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

TEST_CASE("Enum XML") {
  using EnumField = Enum<int32_t>::Field;
  auto e = Enum<int32_t, std::endian::big>::create(
      EnumField("off", 0), EnumField("on", 1), EnumField("unknown", 2));

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  e->value = 0;
  e->build_xml(root, "test");
  REQUIRE(std::string(root.attribute("test").as_string()) == "off");
  e->value = 1;
  e->build_xml(root, "test2");
  REQUIRE(std::string(root.attribute("test2").as_string()) == "on");
  e->value = 2;
  e->build_xml(root, "test3");
  REQUIRE(std::string(root.attribute("test3").as_string()) == "unknown");
  e->value = 4;
  e->build_xml(root, "test4");
  REQUIRE(std::string(root.attribute("test4").as_string()) == "4");
}
