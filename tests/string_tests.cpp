#include "helpers.hpp"
#include "number.hpp"
#include "string.hpp"
#include "struct.hpp"
#include <catch2/catch_test_macros.hpp>

#include "utfconv.hpp"

using namespace std::string_literals;
using namespace etcetera;

TEST_CASE("CString8 in struct") {
  auto s = Struct::create(Field("a", CString8l::create()),
                          Field("b", CString8l::create()));
  std::stringstream data;
  std::string a = "abcd";
  std::string b = "efgh";
  data.write(a.c_str(), a.length() + 1);
  data.write(b.c_str(), b.length() + 1);
  auto obj =
      std::any_cast<tsl::ordered_map<std::string, std::any>>(s->parse(data));
  REQUIRE(std::any_cast<std::string>(obj["a"]).length() == a.length());
  REQUIRE(std::any_cast<std::string>(obj["b"]).length() == b.length());
  REQUIRE(std::any_cast<std::string>(obj["a"]) == a);
  REQUIRE(std::any_cast<std::string>(obj["b"]) == b);
}

TEST_CASE("CString16 in struct") {
  auto s = Struct::create(Field("a", CString16l::create()),
                          Field("b", CString16l::create()));
  std::stringstream data;
  std::u16string a = u"abcd";
  std::u16string b = u"efgh";
  data.write((char *)a.c_str(), (a.length() + 1) * sizeof(char16_t));
  data.write((char *)b.c_str(), (b.length() + 1) * sizeof(char16_t));
  auto obj =
      std::any_cast<tsl::ordered_map<std::string, std::any>>(s->parse(data));
  REQUIRE(std::any_cast<std::string>(obj["a"]).length() == 4);
  REQUIRE(std::any_cast<std::string>(obj["b"]).length() == 4);
  REQUIRE(std::any_cast<std::string>(obj["a"]) == "abcd");
  REQUIRE(std::any_cast<std::string>(obj["b"]) == "efgh");
}

TEST_CASE("CString16 in struct building") {
  auto s = Struct::create(Field("a", CString16l::create()),
                          Field("b", CString16l::create()));
  std::stringstream data;
  std::u16string a = u"abcd";
  std::u16string b = u"efgh";
  data.write((char *)a.c_str(), (a.length() + 1) * sizeof(char16_t));
  data.write((char *)b.c_str(), (b.length() + 1) * sizeof(char16_t));
  std::stringstream ss;
  s->parse(data);
  s->build(ss);
  ss.seekg(0);
  data.seekg(0);
  REQUIRE(ss.str().length() == data.str().length());
  REQUIRE(ss.str() == data.str());
}

TEST_CASE("String UTF-32") {
  auto field = CString32l::create();
  std::stringstream ss, orig;
  std::u32string s = U"abcd";
  orig.write((char *)s.c_str(), (s.length() + 1) * sizeof(char32_t));
  auto ret = field->parse(orig);
  REQUIRE(Utf8To32(field->value) == s);
  field->build(ss);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
  REQUIRE(field->get_size() == (s.length() + 1) * sizeof(char32_t));
}

TEST_CASE("String UTF-16") {
  auto field = CString16l::create();
  std::stringstream ss, orig;
  std::u16string s = u"abcd";
  orig.write((char *)s.c_str(), (s.length() + 1) * sizeof(char16_t));
  auto ret = field->parse(orig);
  REQUIRE(Utf32To16(Utf8To32(field->value)) == s);
  field->build(ss);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
  REQUIRE(field->get_size() == (s.length() + 1) * sizeof(char16_t));
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
  orig.write("\0\0", 2);
  auto ret = field->parse(orig);
  field->build(ss);
  REQUIRE(field->value == "abcdäöüß");

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
  REQUIRE(field->get_size() == (s.length() + 1) * sizeof(char16_t));
}

TEST_CASE("String UTF-8") {
  auto field = CString8l::create();
  std::stringstream ss, orig;
  std::string s = "abcd";
  orig.write(s.c_str(), (s.length() + 1));
  REQUIRE(std::any_cast<std::string>(field->parse(orig)) == s);
  REQUIRE(field->value == s);
  REQUIRE(field->get<std::string>() == s);
  field->build(ss);

  ss.seekg(0);
  orig.seekg(0);
  REQUIRE(ss.str() == orig.str());
  REQUIRE(field->get_size() == (s.length() + 1) * sizeof(char8_t));
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

TEST_CASE("Padded UTF-8 String") {
  auto field = PaddedString8l::create(10);
  std::stringstream ss, orig;
  std::string s = "abcd\0\0\0\0\0\0"s;
  orig.write(s.c_str(), s.length());
  field->parse(orig);
  REQUIRE(field->value.length() == s.length());
  REQUIRE(field->value == s);
  REQUIRE(field->get<std::string>() == s);
  field->build(ss);
  REQUIRE(ss.str().length() == orig.str().length());
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("Padded UTF-16 String") {
  auto field = PaddedString16l::create(20);
  std::stringstream ss, orig;
  std::u16string s = u"abcd\0\0\0\0\0\0"s;
  orig.write((char *)s.c_str(), (s.length()) * sizeof(char16_t));
  field->parse(orig);
  REQUIRE(field->value.length() == s.length());
  field->build(ss);
  REQUIRE(ss.str().length() == orig.str().length());
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("Padded String") {
  auto field = Struct::create(
      Field("size", Int32ul::create()),
      Field("string",
            PaddedString8l::create([](std::weak_ptr<Base> parent) -> size_t {
              return lock(parent)->get<uint32_t>("size");
            })));

  std::stringstream ss, orig;
  std::string s = "abcd";
  uint32_t len = s.length();
  orig.write((char *)&len, sizeof(len));
  orig.write(s.c_str(), s.length());
  field->parse(orig);
  REQUIRE(field->get<uint32_t>("size") == len);
  REQUIRE(field->get<std::string>("string") == s);
  field->build(ss);
  REQUIRE(ss.str().length() == orig.str().length());
  REQUIRE(ss.str() == orig.str());
}

TEST_CASE("Pascal String 8l") {
  auto field = PascalString8l<Int32ul>::create(Int32ul::create());
  std::stringstream ss, orig;
  std::string s = "abcd";
  uint32_t len = s.length();
  orig.write((char *)&len, sizeof(len));
  orig.write(s.c_str(), s.length());
  field->parse(orig);
  REQUIRE(field->length() == len);
  REQUIRE(field->get_size() == len * sizeof(char8_t));
  field->build(ss);
  REQUIRE(ss.str().length() == orig.str().length());
  REQUIRE(ss.str() == orig.str());
}
