#include "string.hpp"
#include <catch2/catch_test_macros.hpp>

#include "utfconv.hpp"

using namespace etcetera;

TEST_CASE("String UTF-32") {
  auto field = CString32l::create();
  std::stringstream ss, orig;
  std::u32string s = U"abcd";
  orig.write((char *)s.c_str(), s.length() * sizeof(char32_t));
  auto ret = field->parse(orig);
  REQUIRE(Utf8To32(field->value) == s);
  field->build(ss);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
  REQUIRE(field->get_size({}) == s.length() * sizeof(char32_t));
}

TEST_CASE("String UTF-16") {
  auto field = CString16l::create();
  std::stringstream ss, orig;
  std::u16string s = u"abcd";
  orig.write((char *)s.c_str(), s.length() * sizeof(char16_t));
  auto ret = field->parse(orig);
  REQUIRE(Utf32To16(Utf8To32(field->value)) == s);
  field->build(ss);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
  REQUIRE(field->get_size({}) == s.length() * sizeof(char16_t));
}

TEST_CASE("String UTF-16 be") {
  auto field = CString16b::create();
  std::stringstream ss, orig;
  std::u16string s = u"abcdäöüß";
  std::u16string orig_s;
  for (auto &c : s) {
    c = std::byteswap(c);
    orig.write((char *)&c, sizeof(char16_t));
    orig_s.push_back(c);
  }
  auto ret = field->parse(orig);
  field->build(ss);
  REQUIRE(field->value == "abcdäöüß");

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
  REQUIRE(field->get_size({}) == s.length() * sizeof(char16_t));
}

TEST_CASE("String UTF-8") {
  auto field = CString8l::create();
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
  REQUIRE(field->get_size({}) == s.length() * sizeof(char8_t));
}

TEST_CASE("UTF-8 String to XML") {
  auto field = CString8l::create();
  field->value = "abcd";

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field->build_xml(root, "test");
  std::stringstream ss;
  doc.save(ss);
  REQUIRE(ss.str() == R"(<?xml version="1.0"?>
<root test="abcd" />
)");
}

TEST_CASE("UTF-16 String to XML") {
  auto field = CString16l::create();
  field->value = "abcd";

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field->build_xml(root, "test");
  std::stringstream ss;
  doc.save(ss);
  REQUIRE(ss.str() == R"(<?xml version="1.0"?>
<root test="abcd" />
)");
}

TEST_CASE("UTF-32 String to XML") {
  auto field = CString32l::create();
  field->value = "abcd";

  pugi::xml_document doc;
  auto root = doc.append_child("root");
  field->build_xml(root, "test");
  std::stringstream ss;
  doc.save(ss);
  REQUIRE(ss.str() == R"(<?xml version="1.0"?>
<root test="abcd" />
)");
}
