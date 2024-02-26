#include "edat.hpp"
#include <argparse/argparse.hpp>

using namespace etcetera;
namespace fs = std::filesystem;

int main(int argc, char **argv) {
  argparse::ArgumentParser program("EDat");
  program.add_argument("-w", "--wgrd_path").help("path to the vanilla game files, e.g. C:/Program Files (x86)/Steam/steamapps/common/Wargame Red Dragon/Data/WARGAME/PC");
  program.add_argument("-m", "--modded_path").help("path to the modded game files");
  program.add_argument("-o", "--output").default_value("out/").help("path to the output directory");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cout << err.what() << std::endl;
    std::cout << program;
    exit(1);
  }

  fs::path modded_path = program.get<std::string>("--modded_path");
  // iterate all modded game files
  for (const auto &entry : fs::recursive_directory_iterator(modded_path)) {
    if (entry.is_regular_file()) {
      fs::path part = entry.path().lexically_relative(modded_path);
      std::cout << "testing: " << entry << " " << part << std::endl;

      // parse the modded game file
      std::ifstream modded_input;
      modded_input.open(entry.path(), std::ios::binary);
      auto modded_edat = EDat::create();
      // only read the file headers
      modded_edat->read_files = false;
      modded_edat->parse(modded_input);

      // parse the original game file
      std::ifstream orig_input;
      orig_input.open(entry.path(), std::ios::binary);
      auto orig_edat = EDat::create();
      // only read the file headers
      orig_edat->read_files = false;
      orig_edat->parse(orig_input);

      // compare the original and modded game files and get the changed files
      for (const auto &[path, header] : modded_edat->file_headers) {
        auto orig_header = orig_edat->file_headers[path];
        std::cout << "path: " << path << std::endl;
        for(int i = 0; i < 16; i++) {
          if (header.checksum[i] != orig_header.checksum[i]) {
            std::cout << "changed file: " << entry << " " << path << std::endl;
            break;
          }
        }
      }
    }
  }

  return 0;
}