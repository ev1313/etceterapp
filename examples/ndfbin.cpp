#include "array.hpp"
#include "basic.hpp"
#include "conditional.hpp"
#include "helpers.hpp"
#include "number.hpp"
#include "pointer.hpp"
#include "special.hpp"
#include "struct.hpp"

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
  /*

NDFType = Struct(
    "typeId" / Rebuild(Int32ul, this._switch_id_data),
    "data" / Switch(this.typeId, {
        0x00000000: "Boolean" / Struct("value" / Enum(Int8ul, false=0, true=1)),
        0x00000001: "Int8" / Struct("value" / Int8ul),
        0x00000002: "Int32" / Struct("value" / Int32sl),
        0x00000003: "UInt32" / Struct("value" / Int32ul),
        0x00000005: "Float32" / Struct("value" / Float32l),
        0x00000006: "Float64" / Struct("value" / Float64l),
        0x00000007: "StringReference" / Struct("stringIndex" / Int32ul),
        0x00000008: "WideString" / Struct("str" / PascalString(Int32ul,
"utf-16-le")), 0x00000009: "Reference" / Struct( "typeId" / Rebuild(Int32ul,
this._switch_id_ref), "ref" / Switch(this.typeId, { 0xAAAAAAAA: "TranReference"
/ Struct("tranIndex" / Int32ul), 0xBBBBBBBB: "ObjectReference" / Struct(
                    "objectIndex" / Int32ul,
                    "classIndex" / Int32ul,
                ),
            }),
        ),
        0x0000000B: "F32_vec3" / Struct("x" / Float32l, "y" / Float32l, "z" /
Float32l), 0x0000000C: "F32_vec4" / Struct("x" / Float32l, "y" / Float32l, "z" /
Float32l, "w" / Float32l), 0x0000000D: "Color" / Struct("r" / Int8ul, "g" /
Int8ul, "b" / Int8ul, "a" / Int8ul), 0x0000000E: "S32_vec3" / Struct("x" /
Int32sl, "y" / Int32sl, "z" / Int32sl), 0x0000000F: "Matrix" / Struct("Matrix" /
Array(16, Float32l)), 0x00000011: "List" / Struct("length" / Rebuild(Int32ul,
len_(this.items)), "items" / LazyBound(lambda: NDFType)[this.length]),
        0x00000012: "Map" / Struct(
            "count" / Rebuild(Int32ul, len_(this.mapitem)),
            "mapitem" / Struct(
                "key" / LazyBound(lambda: NDFType),
                "value" / LazyBound(lambda: NDFType),
            )[this.count]),
        0x00000013: "Long" / Struct("value" / Int64ul),
        0x00000014: "Blob" / Struct(
            "size" / Int32ul,
            "data" / Bytes(this.size),
        ),
        0x00000018: "S16" / Struct("value" / Int16sl),
        0x00000019: "U16" / Struct("value" / Int16ul),
        0x0000001A: "GUID" / Struct("data" / Bytes(16)),
        0x0000001C: "PathReference" / Struct("stringIndex" / Int32ul),
        0x0000001D: "LocalisationHash" / Struct("data" / Bytes(8)),
        0x0000001E: "UnknownBlob" / Struct(  # StringBlob?
            "size" / Int32ul,
            "unk0" / Int8ul,  # Const(b'\x01'),
            "data" / Switch(this.unk0, {
                0: "Raw" / Struct(
                    "data" / Bytes(this._.size)
                ),
                1: "Zlib" / Struct(
                    "uncompressedSize" / Int32ul,
                    # FIXME: Zero pad output of ZlibCompressed to
`uncompressedSize` "data" / ZlibCompressed(Bytes(this._.size - 4)),
                )
            })
        ),
        0x00000022: "Pair" / Struct(
            "first" / LazyBound(lambda: NDFType),
            "second" / LazyBound(lambda: NDFType),
        ),
        0x0000001F: "S32_vec2" / Struct("x" / Int32sl, "y" / Int32sl),
        0x00000021: "F32_vec2" / Struct("x" / Float32l, "y" / Float32l),
        0x00000025: "Hash" / Struct("hash" / Bytes(16)),
    }, default=Error),
)

NDFProperty = Struct(
    "propertyIndex" / Int32ul,
    "value" / If(lambda ctx: ctx.propertyIndex != 0xABABABAB, NDFType),
)

NDFObject = Struct(
    "classIndex" / Int32ul,
    "properties" / RepeatUntil(lambda obj, lst, ctx: obj.propertyIndex ==
0xABABABAB, "Property" / NDFProperty),
)

   */

  auto NDFObject = Struct::create(Field("classIndex", Int32ul::create())
                                  // Field("properties")
  );

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
