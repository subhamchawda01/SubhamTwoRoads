/*
 * test.cpp
 *
 *  Created on: Nov 11, 2011
 *      Author: piyush
 */

#include <string.h>
#include <iostream>
//#include "infracore/rapidxml.hpp"
#include "infracore/rapidxml/rapidxml_utils.hpp"

void pp(rapidxml::xml_node<char>* n) {
  if (!n) return;
  std::cout << "processing node " << n->name() << ", type " << n->type() << "\n";
  switch (n->type()) {
    case rapidxml::node_element:
      for (rapidxml::xml_attribute<char>* attr = n->first_attribute(); attr; attr = attr->next_attribute())
        std::cout << "attribute-> " << attr->name() << " : " << attr->value() << "\n";

      for (rapidxml::xml_node<char>* cn = n->first_node(); cn; cn = cn->next_sibling()) pp(cn);
      break;
    case rapidxml::node_data:
      std::cout << n->value() << "\n";
      break;
    default:
      break;
  }
}

int main(int argc, char** argv) {
  //  std::string xmlText = "<rootNode isRoot=\"true\" color=\"pink\" > \n\t";
  //  xmlText += "<child > bad Child </child> \n\t";
  //  xmlText += "<selfClosing arr=\"myArr\" attr2=\"2\" />\n\t";
  //  xmlText += "</root>";
  //  char* c = new char [xmlText.length() + 1];
  //  strncpy(c, xmlText.c_str(), xmlText.length() + 1);

  rapidxml::file<char> fl = rapidxml::file<char>("/home/piyush/infracore/files/EUREX/eurex_template.xml");
  rapidxml::xml_document<> doc;  // character type defaults to char
  doc.parse<0>(fl.data());       // 0 means default parse flags
  pp(doc.first_node());

  return 0;
}
