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
#include "edat.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace etcetera;
using namespace std::string_literals;

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

    std::fstream ofs(program.get("output") + "/edat.bin", std::ios::in | std::ios::out | std::ios::binary);
    edat->build(ofs);
  }
}