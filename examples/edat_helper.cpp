#include "edat.hpp"
#include <argparse/argparse.hpp>


int main(int argc, char **argv) {

  /*
   parser = argparse.ArgumentParser()
parser.add_argument("-w", "--wgrd_path", type=pathlib.Path,
                 help="path to the vanilla game files, e.g. C:/Program Files (x86)/Steam/steamapps/common/Wargame Red Dragon/Data/WARGAME/PC")
parser.add_argument("-m", "--modded_path", type=pathlib.Path,
                 help="path to the modded game files")
parser.add_argument("-o", "--output", default="out/", type=pathlib.Path,
                 help="path to the output directory")
args = parser.parse_args()

   * */
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

  return 0;
}