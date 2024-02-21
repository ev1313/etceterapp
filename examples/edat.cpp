#include "array.hpp"
#include "basic.hpp"
#include "conditional.hpp"
#include "helpers.hpp"
#include "number.hpp"
#include "pointer.hpp"
#include "special.hpp"
#include "string.hpp"
#include "struct.hpp"

#include "md5.h"

#include "argparse/argparse.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace etcetera;
using namespace std::string_literals;

struct dictionarySort {
  bool operator()(const std::string& a, const std::string& b) const {
    auto R = [](char a, char b) {
      std::string l;
      for (char c = a; c <= b; c++) {
        l += c;
      }
      return l;
    };
    std::string rank = "\\"s + "-"s + "."s + R('0', '9') + "_"s + " "s + R('a', 'z') + R('A', 'Z');

    for(size_t i = 0; i < std::min(a.size(), b.size()); i++) {
      if(a[i] != b[i]) {
        custom_assert(rank.find(a[i]) != std::string::npos);
        custom_assert(rank.find(b[i]) != std::string::npos);
        return rank.find(a[i]) < rank.find(b[i]);
      }
    }
  }
};

class EDat : public Base {
private:

  struct EDatHeader {
    uint8_t magic[4];
    uint32_t unk0;
    uint8_t pad0[17];
    uint32_t offset_dictionary;
    uint32_t size_dictionary;
    uint32_t offset_files;
    uint32_t size_files;
    uint8_t pad1[4];
    uint32_t sectorSize;
    uint8_t checksum[16];
    uint8_t pad2[972];
  } __attribute__((packed));

  struct EDatFileHeader {
    uint32_t offset;
    uint32_t pad0;
    uint32_t size;
    uint32_t pad;
    uint8_t checksum[16];
  } __attribute__((packed));

  std::map<std::string, EDatFileHeader> file_headers;

public:
  using Base::get;
  using Base::get_field;
  using Base::get_offset;

  bool read_files = true;
  uint32_t sectorSize = 8192;
  std::string outpath = "./out/";

  explicit EDat(Base::PrivateBase) : Base(Base::PrivateBase()) {}

  static std::shared_ptr<EDat> create() {
    return std::make_shared<EDat>(Base::PrivateBase());
  }

  std::any get() override {
    auto ret = std::vector<std::string>();
    ret.reserve(file_headers.size());
    for(auto &[path, _] : file_headers) {
      ret.push_back(path);
    }
    return ret;
  }

  size_t get_size() override {
    // we would need to build the radixtree here to get the size...
    custom_assert(false);
    return 0;
  }

  /*
   * This method only parses the dictionary itself and fills the file_headers
   * */
  void parsePath(std::istream &stream, std::string path, uint32_t ending) {
    spdlog::debug("EDat::parsePath start {:02X} {} {:02X}", (size_t)stream.tellg(), path, ending);
    while(stream.tellg() < ending) {
      spdlog::debug("EDat::parsePath loopstart {:02X} {:02X}", (size_t)stream.tellg(), ending);
      size_t c = stream.tellg();

      uint32_t pathSize = 0;
      stream.read((char *)&pathSize, sizeof(pathSize));
      uint32_t entrySize = 0;
      stream.read((char *)&entrySize, sizeof(entrySize));

      size_t endpos = ending;
      if (entrySize != 0) {
        endpos = c + entrySize;
      }
      spdlog::debug("EDat::parsePath {:02X} pathSize {:02X} entrySize {:02X} endpos {:02X}", (size_t)stream.tellg(), pathSize, entrySize, endpos);

      // pathSize != 0 -> more parts of the path
      // pathSize == 0 -> the FileHeader
      if (pathSize != 0 and stream.tellg() != ending) {
        auto sc = Aligned::create(2, CString8l::create());
        sc->parse(stream);
        custom_assert((size_t)stream.tellg() == (size_t)(c + pathSize));

        std::string subpath = sc->get<std::string>();

        spdlog::debug("EDat::parsePath {:02X} endpos {:02X} ending {:02X}", (size_t)stream.tellg(), endpos, ending);

        parsePath(stream, path + subpath, endpos);
      } else {
        spdlog::debug("EDat::parsePath end path {:02X} {}", (size_t)stream.tellg(), path);
        EDatFileHeader header = {};
        spdlog::debug("EDat::parsePath before reading file header {:02X}", (size_t)stream.tellg());
        stream.read((char *)&header, sizeof(EDatFileHeader));
        spdlog::debug("EDat::parsePath after reading file header {:02X}", (size_t)stream.tellg());

        auto sc = Aligned::create(2, CString8l::create());
        sc->parse(stream);

        std::string fPath = path + sc->get<std::string>();
        std::replace(fPath.begin(), fPath.end(), '\\', '/');
        std::filesystem::path filePath = fPath;
        file_headers.emplace(filePath, header);

        size_t endoffset = (size_t)stream.tellg();
        size_t tmp_offset = header.offset;
        size_t tmp_size = header.size;
        spdlog::debug("EDat::parsePath reading file {:02X} {}", (size_t)stream.tellg(), filePath.c_str());
        spdlog::debug("EDat::parsePath reading file @{:02X} {}", tmp_offset, tmp_size);

        if (header.size == 0) {
          continue;
        }

        if (!read_files) {
          continue;
        }

        // create directory for path
        std::filesystem::path dirpath = outpath / filePath;
        dirpath = dirpath.parent_path();
        std::filesystem::create_directories(dirpath);

        // copy file to outpath
        char buf[4096];
        std::ofstream file(outpath / filePath, std::ios::binary);
        stream.seekg(header.offset);
        size_t remaining = header.size;
        while(remaining > 0) {
          size_t toRead = std::min(remaining, (size_t)sizeof(buf));
          stream.read(buf, toRead);
          file.write(buf, toRead);
          remaining -= toRead;
        }
        custom_assert(remaining == 0);

        stream.seekg(endoffset);
      }

      spdlog::debug("EDat::parsePath end of fn {:02X} {} {:02X} {:02X}", (size_t)stream.tellg(), path, endpos, ending);
      custom_assert((size_t)stream.tellg() == endpos);
    }
  }

  std::any parse(std::istream &stream) override {
    file_headers.clear();
    EDatHeader header = {};
    stream.read((char *)&header, sizeof(header));
    spdlog::debug("EDat::parse {:02X} EDatHeader", (size_t)stream.tellg());

    assert (header.magic[0] == 'e');
    assert (header.magic[1] == 'd');
    assert (header.magic[2] == 'a');
    assert (header.magic[3] == 't');
    assert (header.unk0 == 2);
    for (unsigned char i : header.pad0) {
      custom_assert(i == 0);
    }
    for (unsigned char i : header.pad1) {
      custom_assert(i == 0);
    }
    for (unsigned char i : header.pad2) {
      custom_assert(i == 0);
    }
    custom_assert(header.offset_dictionary == sizeof(header));

    sectorSize = header.sectorSize;

    custom_assert(header.offset_dictionary == stream.tellg());

    uint32_t empty = 0;
    stream.read((char *)&empty, sizeof(empty));
    if(empty == 0x01) {
      return get();
    }
    if(empty != 0xA) {
      throw std::runtime_error("Expected 0x01 or 0x0A, got " + std::to_string(empty));
    }
    uint8_t pad[6];
    stream.read((char *)pad, sizeof(pad));
    for (unsigned char i : pad) {
      custom_assert(i == 0);
    }

    // ensure outpath exists
    std::filesystem::create_directories(outpath);

    parsePath(stream, "", header.offset_dictionary + header.size_dictionary);

    return get();
  }

  void build(std::ostream &stream) override {
    EDatHeader header = {};
    strcpy((char *)header.magic, "edat");
    header.unk0 = 2;
    header.offset_dictionary = sizeof(header);

    stream.seekp(sizeof(header), std::ios_base::beg);

    // first sort all file paths
    std::vector<std::string> paths;
    for(auto &[path, _] : file_headers) {
      paths.push_back(path);
    }
    dictionarySort sorter;
    std::sort(paths.begin(), paths.end(), sorter);

    // now build the trie

  }

  void parse_xml(pugi::xml_node const &node, std::string name, bool) override {
  }

  pugi::xml_node build_xml(pugi::xml_node &parent, std::string name) override {
    auto root = parent.append_child(name.c_str());
    root.append_attribute("sectorSize").set_value(sectorSize);
    for(auto &[path, header] : file_headers) {
      auto file = root.append_child("File");
      file.append_attribute("path").set_value(path.c_str());
      if(!read_files) {
        file.append_attribute("offset").set_value(header.offset);
        file.append_attribute("pad0").set_value(header.pad0);
        file.append_attribute("size").set_value(header.size);
        file.append_attribute("pad").set_value(header.pad);
        // checksum as hex
        std::stringstream ss;
        for (unsigned char i : header.checksum) {
          ss << std::hex << std::setw(2) << std::setfill('0') << (int)i;
        }
        file.append_attribute("checksum").set_value(ss.str().c_str());
      }
    }
    return root;
  }

};


int main(int argc, char **argv) {
  argparse::ArgumentParser program("EDat");
  program.add_argument("input").help("Input file");
  program.add_argument("output").help("Output folder");
  program.add_argument("-v", "--verbose")
      .default_value(false)
      .implicit_value(true)
      .help("Verbose output");
  program.add_argument("--dont-read-files")
      .default_value(false)
      .implicit_value(true)
      .help("Don't read the files, just parse the dictionary, necessary for some ndfbin edats.");
  program.add_argument("-p", "--pack").default_value(false).implicit_value(true).help(
      "instead of parsing the input file, pack the input xml file into an edat file");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cout << err.what() << std::endl;
    std::cout << program;
    exit(1);
  }

  if (!program.get<bool>("-v")) {
    spdlog::set_level(spdlog::level::warn);
  } else {
    spdlog::set_level(spdlog::level::debug);
  }

  auto edat = EDat::create();
  edat->outpath = program.get("output");
  edat->read_files = !program.get<bool>("--dont-read-files");

  if(!program.get<bool>("-p")) {
    std::ifstream input;
    input.open(program.get("input"), std::ios::binary);
    edat->parse(input);

    pugi::xml_document doc;
    auto root = doc.append_child("root");
    edat->build_xml(root, "EDat");
    std::string outpath = program.get("output") + "/edat.xml";
    doc.save_file(outpath.c_str());
  } else {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(program.get("input").c_str());
    spdlog::debug("Load result: {}", result.description());
    edat->parse_xml(doc.child("root").child("EDat"), "EDat", true);

    std::ofstream ofs(program.get("output") + "/edat.bin", std::ios::binary);
    edat->build(ofs);
  }
}