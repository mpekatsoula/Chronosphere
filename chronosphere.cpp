#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>
#include "parser_helper.h"
#include "graph.h"

extern std::unordered_map <string, LibParserCellInfo> Cells;
extern std::unordered_map <string, VerParserPinInfo> Pins;
extern std::unordered_map <string, NetParserInfo> Nets;

int main(int args, char** argv) {

  if (args != 2) {
    cout << "Usage: " << argv[0] << " <.tau2015> <.timing> <.ops> <output_file>" << endl ;
    exit(0) ;
  }

  int result;


  // Start parsing files
  std::ifstream infile(argv[1]) ;
  
  std::basic_string<char>::size_type index;
  std::string filename, libfile, vfile, speffile, base_path;

  index = string(argv[1]).find_last_of("/");
  base_path = string(argv[1]).substr(0,index);

  // Get file paths from tau2015
  for ( int i = 0; i < 3; i++ ) {

    infile >> filename;
    index = filename.find_last_of(".");

    if ( filename.substr(index + 1, filename.length()) == "v" )
      vfile = base_path + "/" + filename;
    else if ( filename.substr(index + 1, filename.length()) == "lib" )
      libfile = base_path + "/" + filename;
    else if ( filename.substr(index + 1, filename.length()) == "spef" )
      speffile = base_path + "/" + filename;

  }

  // Start parsing input files
  result = wake_parser("lib", libfile);
  assert(result);
  result = wake_parser("verilog", vfile);
  assert(result);
  //result = wake_parser("spef", speffile);
  //assert(result);

  // Create the graph. Connect the pins
  result = create_graph();
  print_graph();
  //result = bfs_on_graph_fwd();

}
