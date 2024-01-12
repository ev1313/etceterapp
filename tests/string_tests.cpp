#include "string.hpp"
#include <catch2/catch_test_macros.hpp>

#include "utfconv.hpp"

using namespace etcetera;

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
