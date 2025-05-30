#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include "infracore/rapidxml/rapidxml_utils.hpp"

struct LiffeRef {
  std::string SecGrp;
  std::string Sym;
  std::string LogSym;
  std::string Desc;
  std::string Exch;
  std::string RndLot;
  std::string MinPxIncr;
  std::vector<std::string> series_futures_symbol_;

  std::string NestedAttrType25;
  std::string NestedAttrType101;
  std::string NestedAttrType102;
  std::string NestedAttrType120;
  std::string NestedAttrType121;

  void toStr() {
    for (unsigned int i = 0; i < series_futures_symbol_.size(); i++) {
      std::cout << SecGrp << "~" << Sym << "~" << LogSym << "~" << MinPxIncr << "~" << series_futures_symbol_[i] << "~"
                << NestedAttrType25 << "~" << NestedAttrType101 << "~" << NestedAttrType102 << "~" << NestedAttrType121
                << "\n";
    }
  }

  LiffeRef() {
    SecGrp = "";
    Sym = "";
    LogSym = "";
    Desc = "";
    Exch = "";
    RndLot = "";
    MinPxIncr = "";
    NestedAttrType25 = "";
    NestedAttrType101 = "";
    NestedAttrType102 = "";
    NestedAttrType120 = "";
    NestedAttrType121 = "";

    series_futures_symbol_.clear();
  }

  ~LiffeRef() {}
};

FILE* spread_file_;

void parseSeries(rapidxml::xml_node<char>* n, LiffeRef& toRet) {
  std::string id_ = "";

  for (rapidxml::xml_attribute<char>* attr = n->first_attribute(); attr; attr = attr->next_attribute()) {
    std::string attr_name_ = attr->name();

    if (attr_name_ == "ID") {
      id_ = attr->value();

      toRet.series_futures_symbol_.push_back(id_);
    }
  }
}

void parseExpiry(rapidxml::xml_node<char>* n, LiffeRef& toRet) {
  std::string node_name_;

  // process children of interest
  for (rapidxml::xml_node<char>* cn = n->first_node(); cn; cn = cn->next_sibling()) {
    std::string node_name = cn->name();
    if (node_name == "Series") {
      parseSeries(cn, toRet);
    }
  }
}

void parseStrategy(rapidxml::xml_node<char>* n, LiffeRef& toRet) {
  std::string id_ = "";

  for (rapidxml::xml_attribute<char>* attr = n->first_attribute(); attr; attr = attr->next_attribute()) {
    std::string attr_name_ = attr->name();
    if (attr_name_ == "ID") {
      id_ = attr->value();
      toRet.series_futures_symbol_.push_back(id_);
    }
  }
  std::vector<std::string> legs_;
  for (rapidxml::xml_node<char>* leg = n->first_node(); leg; leg = leg->next_sibling())
    for (rapidxml::xml_attribute<char>* attr = leg->first_attribute(); attr; attr = attr->next_attribute()) {
      std::string attr_name_ = attr->name();
      if (attr_name_ == "ID") legs_.push_back(attr->value());
    }

  std::stringstream ss;
  ss << id_ << "~";
  for (unsigned int i = 0; i < legs_.size() - 1; i++) {
    ss << legs_[i] << "~";
  }
  ss << legs_[legs_.size() - 1];
  fprintf(spread_file_, "%s\n", ss.str().c_str());
}

void prarseNestedAttr(rapidxml::xml_node<char>* n, LiffeRef& toRet) {
  std::string type;
  std::string value = "";
  for (rapidxml::xml_attribute<char>* attr = n->first_attribute(); attr; attr = attr->next_attribute()) {
    std::string at_name = attr->name();
    if (at_name == "Typ") {
      type = attr->value();

    } else if (at_name == "Val") {
      value = attr->value();
    }
  }
  if (type == "25") {
    toRet.NestedAttrType25 = value;
  } else if (type == "101") {
    toRet.NestedAttrType101 = value;
  } else if (type == "102") {
    toRet.NestedAttrType102 = value;
  } else if (type == "120") {
    toRet.NestedAttrType120 = value;
  } else if (type == "121") {
    toRet.NestedAttrType121 = value;
  }
}

LiffeRef parseContract(rapidxml::xml_node<char>* n) {
  LiffeRef toRet;

  for (rapidxml::xml_attribute<char>* attr = n->first_attribute(); attr; attr = attr->next_attribute()) {
    std::string at_name = attr->name();
    if (at_name == "SecGrp")
      toRet.SecGrp = attr->value();
    else if (at_name == "Sym")
      toRet.Sym = attr->value();
    else if (at_name == "LogSym")
      toRet.LogSym = attr->value();
    else if (at_name == "Desc")
      toRet.Desc = attr->value();
    else if (at_name == "Exch")
      toRet.Exch = attr->value();
    else if (at_name == "RndLot")
      toRet.RndLot = attr->value();
    else if (at_name == "MinPxIncr")
      toRet.MinPxIncr = attr->value();
  }
  // process children of interest
  for (rapidxml::xml_node<char>* cn = n->first_node(); cn; cn = cn->next_sibling()) {
    std::string node_name = cn->name();
    if (node_name == "NestedAttr") {
      prarseNestedAttr(cn, toRet);
    } else if (node_name == "Expiry") {
      parseExpiry(cn, toRet);

    } else if (node_name == "Strategy") {
      parseStrategy(cn, toRet);
    }
  }
  return toRet;
}

void parseFIXML(rapidxml::xml_node<char>* n) {
  for (rapidxml::xml_node<char>* cn = n->first_node()->first_node(); cn; cn = cn->next_sibling()) {
    std::string node_name = cn->name();
    if (node_name == "Contract") {
      LiffeRef lr = parseContract(cn);
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << " Usage : <exec> <liffe_fixml> <liff-spreads-symbol-mapping-output-file>\n";
    exit(1);
  }

  spread_file_ = fopen(argv[2], "w");
  if (!spread_file_) {
    std::cerr << "Unable to open file " << argv[2] << std::endl;
  }
  rapidxml::file<char> fl = rapidxml::file<char>(argv[1]);
  rapidxml::xml_document<> doc;  // character type defaults to char
  doc.parse<0>(fl.data());       // 0 means default parse flags
  parseFIXML(doc.first_node());

  fclose(spread_file_);

  return 0;
}
