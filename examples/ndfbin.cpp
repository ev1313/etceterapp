#include "basic.hpp"
#include "helpers.hpp"
#include "number.hpp"
#include "pointer.hpp"
#include "special.hpp"

#include "argparse/argparse.hpp"

#include <fstream>
#include <iostream>

using namespace etcetera;

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

  auto NdfBin = Struct::create(
      Field("magic", BytesConst::create("EUG0")),
      Field("magic2", BytesConst::create("\x00\x00\x00\x00")),
      Field("magic3", BytesConst::create("CNDF")),
      Field("compressed", Int32ul::create()),
      Field("toc0offset", Int32ul::create()),
      Field("unk0", BytesConst::create("\x00\x00\x00\x00")),
      Field("headerSize",
            Rebuild::create(
                [](std::weak_ptr<Base> c) {
                  return std::make_any<int32_t>(
                      lock(lock(c)->get_field<Int32ul>("uncompressedSize"))
                          ->get_offset());
                },
                Int32ul::create())),
      Field("unk2", BytesConst::create("\x00\x00\x00\x00")),
      Field("size", Int32ul::create()),
      Field("unk4", BytesConst::create("\x00\x00\x00\x00")),
      Field("uncompressedSize", Int32ul::create())
      //  Field("toc0header", )
  );

  std::ifstream input;
  input.open(program.get("input"), std::ios::binary);
  NdfBin->parse(input);
}
