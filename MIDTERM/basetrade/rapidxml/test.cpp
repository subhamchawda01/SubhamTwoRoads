/*
 * test.cpp
 *
 *  Created on: Nov 11, 2011
 *      Author: piyush
 */

#include <string.h>
#include <iostream>
//#include "basetrade/rapidxml/rapidxml.hpp"
#include "basetrade/rapidxml/rapidxml_utils.hpp"

char* prod;
bool fprod = true;

void goFor(rapidxml::xml_node<char>* n) {
  int count_nodes = 0;

  if (!n) return;

  rapidxml::xml_node<char>* l1 = n->first_node("tc810Grp");
  if (!l1) {
    std::cout << "failed at level 2\n";
    return;
  }
  for (rapidxml::xml_node<char>* l2 = l1->first_node("tc810Grp1"); l2;
       l2 = l2->next_sibling("tc810Grp1"))  // for each trader UT001/002/003
  {
    for (rapidxml::xml_node<char>* l3 = l2->first_node("tc810Grp2"); l3;
         l3 = l3->next_sibling("tc810Grp2"))  // for each product traded by the trader FESX/FGBM/FGBL/FGBS
    {
      rapidxml::xml_node<char>* l4 = l3->first_node("tc810Grp3");  // product + records
      if (!l4) {
        std::cout << "failed at level 4\n";
        return;
      }

      rapidxml::xml_node<char>* l5 = l4->first_node("tc810KeyGrp3");  // product key PRODMMYYYY
      if (!l5) {
        std::cout << "failed at level 5\n";
        return;
      }

      rapidxml::xml_node<char>* l6 = l5->first_node("cntrIdGrp");
      if (!l6) {
        std::cout << "failed at level 6\n";
        return;
      }

      rapidxml::xml_node<char>* prod = l6->first_node("prodId");
      if (!prod) {
        std::cout << "failed at level prod\n";
        return;
      }

      std::cout << "\nPRODMMYYYY|" << prod->value() << "|";

      rapidxml::xml_node<char>* exp = l6->first_node("cntrDtlGrp");
      if (!exp) {
        std::cout << "failed at level exp\n";
        return;
      }

      for (rapidxml::xml_node<char>* mmyyyy = exp->first_node(); mmyyyy; mmyyyy = mmyyyy->next_sibling()) {
        std::cout << mmyyyy->value() << "|";
      }

      for (rapidxml::xml_node<char>* record = l4->first_node("tc810Rec"); record;
           record = record->next_sibling("tc810Rec"))  // records
      {
        std::cout << "\n";
        for (rapidxml::xml_node<char>* col = record->first_node(); col; col = col->next_sibling()) {
          std::cout << col->value() << "|";
        }
      }
    }
  }
}

int main(int argc, char** argv) {
  prod = argv[1];

  rapidxml::file<char> xml_file_ = rapidxml::file<char>("/home/kputta/00RPTTC810DVCNJ20120515FIMFR.XML");
  rapidxml::xml_document<> doc_;    // character type defaults to char
  doc_.parse<0>(xml_file_.data());  // 0 means default parse flags
  goFor(doc_.first_node());
  return 0;
}
