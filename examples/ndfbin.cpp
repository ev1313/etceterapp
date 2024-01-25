#include "basic.hpp"
#include "helpers.hpp"
#include "number.hpp"
#include "pointer.hpp"
#include "special.hpp"

#include "argparse/argparse.hpp"

#include <fstream>
#include <iostream>

using namespace etcetera;
using namespace std::string_literals;

int main(int argc, char **argv) {
  argparse::ArgumentParser program("ndfbin");
  program.add_argument("input").help("Input file");
  program.add_argument("output").help("Output folder");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cout << err.what() << std::endl;
    std::cout << program;
    exit(1);
  }

  auto OBJETable = Struct::create(
      Field("magic", BytesConst::create("OBJE"s)),
      Field("pad0", BytesConst::create("\x00\x00\x00\x00"s)),
      Field("offset", Rebuild::create(
                          [](std::weak_ptr<Base> c) {
                            return std::make_any<uint32_t>(
                                lock(c)->get<uint32_t>("_", "headerSize"));
                          },
                          Int32ul::create())),
      Field("pad1", BytesConst::create("\x00\x00\x00\x00"s)),
      Field("size", Int32ul::create()),
      Field("pad2", BytesConst::create("\x00\x00\x00\x00"s))
      // Field("objects", Area("Object" / NDFObject,
      // offset=this.offset, size=this.size),
  );

  auto TOC0Header = Struct::create(
      Field("magic", BytesConst::create("TOC0"s)),
      Field("tableCount", Const<uint32_t>::create(9)), Field("OBJE", OBJETable)
      // Field("TOPO", TOPOTable),
      // Field("CHNK", CHNKTable),
      // Field("CLAS", CLASTable),
      // Field("PROP", PROPTable),
      // Field("STRG", STRGTable),
      // Field("TRAN", TRANTable),
      // Field("IMPR", IMPRTable),
      // Field("EXPR", EXPRTable)
  );

  auto NdfBin = Struct::create(
      Field("magic", BytesConst::create("EUG0"s)),
      Field("magic2", BytesConst::create("\x00\x00\x00\x00"s)),
      Field("magic3", BytesConst::create("CNDF"s)),
      Field("compressed", Int32ul::create()),
      Field("toc0offset", Int32ul::create()),
      Field("unk0", BytesConst::create("\x00\x00\x00\x00"s)),
      Field("headerSize",
            Rebuild::create(
                [](std::weak_ptr<Base> c) {
                  return std::make_any<uint32_t>(
                      lock(lock(c)->get_field<Int32ul>("uncompressedSize"))
                          ->get_offset());
                },
                Int32ul::create())),
      Field("unk2", BytesConst::create("\x00\x00\x00\x00"s)),
      Field("size", Int32ul::create()),
      Field("unk4", BytesConst::create("\x00\x00\x00\x00"s)),
      Field("uncompressedSize", Int32ul::create()),
      Field("toc0header", Pointer::create(
                              [](std::weak_ptr<Base> c) {
                                return lock(c)->get<uint32_t>("toc0offset");
                              },
                              TOC0Header)));

  std::ifstream input;
  input.open(program.get("input"), std::ios::binary);
  NdfBin->parse(input);

  spdlog::info("compressed: {}", NdfBin->get<uint32_t>("compressed"));
  spdlog::info("toc0offset: {}", NdfBin->get<uint32_t>("toc0offset"));
  spdlog::info("headerSize: {}", NdfBin->get<uint32_t>("headerSize"));
  spdlog::info("size: {}", NdfBin->get<uint32_t>("size"));
  spdlog::info("uncompressedSize: {}",
               NdfBin->get<uint32_t>("uncompressedSize"));
}
