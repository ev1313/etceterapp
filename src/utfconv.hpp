#pragma once

#include <codecvt>
#include <locale>
#include <string>

template <typename U8StrT>
inline static std::u32string Utf8To32(U8StrT const &s) {
  static_assert(sizeof(typename U8StrT::value_type) == 1,
                "Char byte-size should be 1 for UTF-8 strings!");
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf_8_32_conv_;
  return utf_8_32_conv_.from_bytes((char const *)s.c_str(),
                                   (char const *)(s.c_str() + s.length()));
}

template <typename U8StrT = std::string>
inline static U8StrT Utf32To8(std::u32string const &s) {
  static_assert(sizeof(typename U8StrT::value_type) == 1,
                "Char byte-size should be 1 for UTF-8 strings!");
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf_8_32_conv_;
  std::string res = utf_8_32_conv_.to_bytes(s.c_str(), s.c_str() + s.length());
  return U8StrT(
      (typename U8StrT::value_type const *)(res.c_str()),
      (typename U8StrT::value_type const *)(res.c_str() + res.length()));
}

template <typename U16StrT>
inline static std::u32string Utf16To32(U16StrT const &s) {
  static_assert(sizeof(typename U16StrT::value_type) == 2,
                "Char byte-size should be 2 for UTF-16 strings!");
  std::wstring_convert<
      std::codecvt_utf16<char32_t, 0x10ffff, std::little_endian>, char32_t>
      utf_16_32_conv_;
  return utf_16_32_conv_.from_bytes((char const *)s.c_str(),
                                    (char const *)(s.c_str() + s.length()));
}

template <typename U16StrT = std::u16string>
inline static U16StrT Utf32To16(std::u32string const &s) {
  static_assert(sizeof(typename U16StrT::value_type) == 2,
                "Char byte-size should be 2 for UTF-16 strings!");
  std::wstring_convert<
      std::codecvt_utf16<char32_t, 0x10ffff, std::little_endian>, char32_t>
      utf_16_32_conv_;
  std::string res = utf_16_32_conv_.to_bytes(s.c_str(), s.c_str() + s.length());
  return U16StrT(
      (typename U16StrT::value_type const *)(res.c_str()),
      (typename U16StrT::value_type const *)(res.c_str() + res.length()));
}

template <typename StrT, size_t NumBytes = sizeof(typename StrT::value_type)>
struct UtfHelper;
template <typename StrT> struct UtfHelper<StrT, 1> {
  inline static std::u32string UtfTo32(StrT const &s) { return Utf8To32(s); }
  inline static StrT UtfFrom32(std::u32string const &s) {
    return Utf32To8<StrT>(s);
  }
};
template <typename StrT> struct UtfHelper<StrT, 2> {
  inline static std::u32string UtfTo32(StrT const &s) { return Utf16To32(s); }
  inline static StrT UtfFrom32(std::u32string const &s) {
    return Utf32To16<StrT>(s);
  }
};
template <typename StrT> struct UtfHelper<StrT, 4> {
  inline static std::u32string UtfTo32(StrT const &s) {
    return std::u32string((char32_t const *)(s.c_str()),
                          (char32_t const *)(s.c_str() + s.length()));
  }
  inline static StrT UtfFrom32(std::u32string const &s) {
    return StrT((typename StrT::value_type const *)(s.c_str()),
                (typename StrT::value_type const *)(s.c_str() + s.length()));
  }
};
template <typename StrT> inline static std::u32string UtfTo32(StrT const &s) {
  return UtfHelper<StrT>::UtfTo32(s);
}
template <typename StrT> inline static StrT UtfFrom32(std::u32string const &s) {
  return UtfHelper<StrT>::UtfFrom32(s);
}
template <typename StrToT, typename StrFromT>
inline static StrToT UtfConv(StrFromT const &s) {
  return UtfFrom32<StrToT>(UtfTo32(s));
}
