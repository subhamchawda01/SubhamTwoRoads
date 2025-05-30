#include <string.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include <iostream>

#include "basetrade/rapidxml/rapidxml_utils.hpp"

void goFor(rapidxml::xml_node<char>* n, const char * given_shc ) {
  if (!n) return;
  for (rapidxml::xml_node<char>* l1 = n->first_node("ta111Grp"); l1; l1 = l1->next_sibling("ta111Grp")) {
    if (!l1) {
      std::cout << "failed at level 2\n";
      return;
    }
    for (rapidxml::xml_node<char>* l2 = l1->first_node("ta111CntrRec"); l2; l2 = l2->next_sibling("ta111CntrRec")) {
      if (std::strcmp(given_shc, l2->first_node("cntrIdGrp")->first_node("prodId")->value()) == 0) {
        std::cout << l2->first_node("cntrIdGrp")->first_node("prodId")->value() << " ";
        std::cout << l2->first_node("cntrIdGrp")->first_node("cntrDtlGrp")->first_node("cntrExpMthDat")->value();
        std::cout << l2->first_node("cntrIdGrp")->first_node("cntrDtlGrp")->first_node("cntrExpYrDat")->value() << " ";
        std::cout << l2->first_node("expDat")->value() << std::endl;
      }
    }
  }
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "give filename and product code\n";
    return 0;
  }

  const char* basefile_ = argv[1];

  const char * shc = argv[2];

  rapidxml::file<char> xml_file_ = rapidxml::file<char>(basefile_);

  rapidxml::xml_document<> doc_;  // character type defaults to char

  doc_.parse<0>(xml_file_.data());  // 0 means default parse flags

  goFor(doc_.first_node(), shc);

  return 1;
}
