/**
    \file CodeGen.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#include <sstream>
#include <iostream>

#include "infracore/codegen/Template.hpp"
#include "infracore/codegen/TemplateParser.hpp"
/**
 * This class takes the input the template struct and generates static code
 * which can be used to decode fixfast message for the corresponding template file
 */

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <template-file> <exch>\n";
    exit(1);
  }

  std::string inputFile = argv[1];
  std::string exch = argv[2];

  FFCodeGen::TemplateParser t;
  std::vector<FFCodeGen::Template> templates = t.parseXMLFile(inputFile);

  std::stringstream ss;
  ss << "//AUTO GENERATED CLASS FOR TEMPLATE " << inputFile << "\n"
     << "//DO NOT MODIFY\n\n\n";

  ss << "#pragma once\n\n";

  ss << "#include \"infracore/lwfixfast/FastDecoder.hpp\"\n";

  ss << "namespace " << exch << "_TEMPLATE_DECODER {\n";

  for (unsigned int i = 0; i < templates.size(); ++i) ss << templates[i].getAutoCode();

  Indentation::instance().increase();
  ss << Indentation::instance().indent() << "class DecoderMap {\n";

  Indentation::instance().increase();
  ss << Indentation::instance().indent() << "public:\n";

  ss << Indentation::instance().indent() << "static void initilize( std::map<int, FastDecoder*>& t_map) {\n";
  Indentation::instance().increase();
  for (unsigned int i = 0; i < templates.size(); ++i) {
    if (templates[i].id <= 0) continue;
    ss << Indentation::instance().indent() << "t_map[ " << templates[i].id << " ] = new " << templates[i].name
       << "(); \n";
  }
  Indentation::instance().decrease();
  ss << Indentation::instance().indent() << "}\n";

  ss << Indentation::instance().indent() << "static void cleanUpMem( std::map<int, FastDecoder*>& t_map) {\n";
  Indentation::instance().increase();
  for (unsigned int i = 0; i < templates.size(); ++i) {
    if (templates[i].id <= 0) continue;
    ss << Indentation::instance().indent() << "delete t_map[ " << templates[i].id << " ];\n";
  }
  Indentation::instance().decrease();
  ss << Indentation::instance().indent() << "}\n";

  Indentation::instance().decrease();
  ss << Indentation::instance().indent() << "};\n";
  ss << Indentation::instance().indent() << "};\n";

  std::cout << ss.str();

  return 0;
}
