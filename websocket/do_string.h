//
// Created by 黎北辰 on 2022/2/2.
//

#ifndef SHARE_DO_STRING_H
#define SHARE_DO_STRING_H

#include <cassert>
#include <cryptopp/cryptlib.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
#include <iostream>
#include <vector>

namespace do_string {
using namespace std;
static const std::string WHITE_SPACE = " \n\r\t\f\v";
/////
std::string ltrim(const std::string &);
std::string rtrim(const std::string &);
std::string trim(const std::string &);
std::vector<std::string> split(const std::string &, const std::string &);
std::vector<std::string> split(const std::string &, const std::string &, bool);
/////
std::string ltrim(const std::string &s) {
  size_t start = s.find_first_not_of(WHITE_SPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}
std::string rtrim(const std::string &s) {
  size_t end = s.find_last_not_of(WHITE_SPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
std::string trim(const std::string &s) { return rtrim(ltrim(s)); }
std::vector<std::string> split(const std::string &str,
                               const std::string &delim) {
  return split(str, delim, false);
}
std::vector<std::string> split(const std::string &str, const std::string &delim,
                               bool is_trim) {
  std::vector<string> res;
  if (str.empty()) {
    return res;
  }
  std::string string1 =
      str + delim; //*****扩展字符串以方便检索最后一个分隔出的字符串
  std::string::size_type pos;
  std::size_t size = string1.size();
  for (int i = 0; i < size; ++i) {
    pos = string1.find(
        delim,
        i); // pos为分隔符第一次出现的位置，从i到pos之前的字符串是分隔出来的字符串
    if (pos !=
        string::npos) { //如果查找到，如果没有查找到分隔符，pos为string::npos
      string s = string1.substr(i, pos - i); //*****从i开始长度为pos-i的子字符串
      if (is_trim) {
        res.push_back(trim(
            s)); //两个连续空格之间切割出的字符串为空字符串，这里没有判断s是否为空，所以最后的结果中有空字符的输出，
      } else {
        res.push_back(
            s); //两个连续空格之间切割出的字符串为空字符串，这里没有判断s是否为空，所以最后的结果中有空字符的输出，
      }
      i = pos + delim.size() - 1;
    }
  }
  return res;
}
} // namespace do_string

namespace urldecode_urlencode {
unsigned char ToHex(unsigned char x) { return x > 9 ? x + 55 : x + 48; }

unsigned char FromHex(unsigned char x) {
  unsigned char y;
  if (x >= 'A' && x <= 'Z')
    y = x - 'A' + 10;
  else if (x >= 'a' && x <= 'z')
    y = x - 'a' + 10;
  else if (x >= '0' && x <= '9')
    y = x - '0';
  else
    assert(0);
  return y;
}
std::string UrlEncode(const std::string &str) {
  std::string strTemp = "";
  size_t length = str.length();
  for (size_t i = 0; i < length; i++) {
    if (isalnum((unsigned char)str[i]) || (str[i] == '-') || (str[i] == '_') ||
        (str[i] == '.') || (str[i] == '~'))
      strTemp += str[i]; // Reserved(保留字符) 不需要编码
    else if (str[i] == ' ')
      strTemp += "+"; // %20 rfc3986 + w3c
    else {
      strTemp += '%';
      strTemp += ToHex((unsigned char)str[i] >> 4);
      strTemp += ToHex((unsigned char)str[i] % 16);
    }
  }
  return strTemp;
}
std::string UrlDecode(const std::string &str) {
  std::string strTemp = "";
  size_t length = str.length();
  for (size_t i = 0; i < length; i++) {
    if (str[i] == '+')
      strTemp += ' ';
    else if (str[i] == '%') {
      assert(i + 2 < length);
      unsigned char high = FromHex((unsigned char)str[++i]);
      unsigned char low = FromHex((unsigned char)str[++i]);
      strTemp += high * 16 + low;
    } else
      strTemp += str[i];
  }
  return strTemp;
}
} // namespace urldecode_urlencode

namespace base64 {
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

static inline bool is_base64(const char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(const char *bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;
      for (i = 0; (i < 4); i++) {
        ret += base64_chars[char_array_4[i]];
      }
      i = 0;
    }
  }
  if (i) {
    for (j = i; j < 3; j++) {
      char_array_3[j] = '\0';
    }

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++) {
      ret += base64_chars[char_array_4[j]];
    }

    while ((i++ < 3)) {
      ret += '=';
    }
  }
  return ret;
}

std::string base64_decode(std::string const &encoded_string) {
  int in_len = (int)encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && (encoded_string[in_] != '=') &&
         is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }
  if (i) {
    for (j = i; j < 4; j++)
      char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++)
      ret += char_array_3[j];
  }

  return ret;
}

} // namespace base64

namespace encode {
using namespace CryptoPP;
std::string sha1_encode(const std::string &input) {
  std::string digest;
  SHA1 sha1;
  StringSource(
      input, true,
      new HashFilter(sha1, new CryptoPP::HexEncoder(new StringSink(digest))));
  return digest;
}
std::string sha1_encode_return_bit(const std::string &input) {
  std::string digest;
  SHA1 sha1;
  StringSource(input, true, new HashFilter(sha1, new StringSink(digest)));
  return digest;
}
std::string sha224_encode(const std::string &input) {
  std::string digest;
  SHA224 sha224;
  StringSource(
      input, true,
      new HashFilter(sha224, new CryptoPP::HexEncoder(new StringSink(digest))));
  return digest;
}
std::string sha256_encode(const std::string &input) {
  std::string digest;
  SHA256 sha256;
  StringSource(
      input, true,
      new HashFilter(sha256, new CryptoPP::HexEncoder(new StringSink(digest))));
  return digest;
}
std::string sha512_encode(const std::string &input) {
  std::string digest;
  SHA512 sha512;
  StringSource(
      input, true,
      new HashFilter(sha512, new CryptoPP::HexEncoder(new StringSink(digest))));
  return digest;
}

} // namespace encode

#endif // SHARE_DO_STRING_H