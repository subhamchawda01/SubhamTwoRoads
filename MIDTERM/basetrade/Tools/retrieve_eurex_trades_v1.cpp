#include <string.h>
#include <iostream>
#include "basetrade/rapidxml/rapidxml_utils.hpp"

int monthchar2int (std::string MMM ) {
  if(MMM.compare("JAN")==0) { return 1; }
  if(MMM.compare("FEB")==0) { return 2; }
  if(MMM.compare("MAR")==0) { return 3; }
  if(MMM.compare("APR")==0) { return 4; }
  if(MMM.compare("MAY")==0) { return 5; }
  if(MMM.compare("JUN")==0) { return 6; }
  if(MMM.compare("JUL")==0) { return 7; }
  if(MMM.compare("AUG")==0) { return 8; }
  if(MMM.compare("SEP")==0) { return 9; }
  if(MMM.compare("OCT")==0) { return 10; }
  if(MMM.compare("NOV")==0) { return 11; }
  if(MMM.compare("DEC")==0) { return 12; }
  else { exit(0);}
};



char* shc;

void goFor(rapidxml::xml_node<char>* n) {
  if (!n) return;

  rapidxml::xml_node<char>* header = n->first_node("rptHdr");
  char* trade_date_ = header->first_node("rptPrntEffDat")->value();

  rapidxml::xml_node<char>* l1 = n->first_node("te810Grp");
  if (!l1) {
    std::cout << "failed at level 2\n";
    return;
  }

    for (rapidxml::xml_node<char>* l2 = l1->first_node("te810Grp1"); l2;
       l2 = l2->next_sibling("te810Grp1"))  
  {
    std::cout << "failed at level 3\n";
    for (rapidxml::xml_node<char>* l3 = l2->first_node("te810Grp2"); l3;
         l3 = l3->next_sibling("te810Grp2")) // for each trader UT001/002/003
    {
      std::cout << "failed at level 4\n";
      for (rapidxml::xml_node<char>* l4 = l3->first_node("te810Grp3"); l4;
           l4 = l4->next_sibling("te810Grp3"))   //    for each product traded by the trader FESX/FGBM/FGBL/FGBS
      {
	std::cout << "failed at level 50\n";
      for (rapidxml::xml_node<char>* l5 = l4->first_node("te810Grp4"); l5;
           l5 = l5->next_sibling("te810Grp4"))   //    for each product traded by the trader FESX/FGBM/FGBL/FGBS
      {	
        rapidxml::xml_node<char>* l6 = l5->first_node("te810KeyGrp4");  // for each contract mmyyyy // product key PRODMMYYYY
        if (!l5) {
          std::cout << "failed at level 5\n";
          return;
        }

        rapidxml::xml_node<char>* l7 = l6->first_node("instrumentGrp");
        if (!l6) {
          std::cout << "failed at level 6\n";
          return;
        }

        rapidxml::xml_node<char>* prod_MMMYY = l7->first_node("instrumentMnemonic");
	char* prod_ = strtok(prod_MMMYY->value(), " ");
	std::string MMMYY = std::string(strtok(NULL, " "));
	int month_ = monthchar2int(MMMYY.substr(0,3));
	std::string yyyy_ = "20" + MMMYY.substr(3);

	std::cout << prod_ << "\n";

        if(strcmp(prod_, shc) != 0) continue;
	std::cout << "\nPRODMMYYYY|" << prod_ << "|" << month_ << "|" << yyyy_ << "|";
        if (!prod_MMMYY) {
          std::cout << "failed at level prod\n";
          return;
        }

        for (rapidxml::xml_node<char>* record = l5->first_node("te810Rec"); record;
             record = record->next_sibling("te810Rec"))  // records
        {
          std::cout << "\n";
	  std::cout << record->first_node("time18")->value() << "|||"; // 0 
	  std::cout << record->first_node("buyCod")->value() << "||"; // 3
	  std::cout << record->first_node("execQty")->value() << "||"; // 5
	  std::cout << record->first_node("execPrc")->value() << "|||||||"; // 7
	  std::cout << trade_date_ << "|||||||"; // 14
          std::cout << month_ << "|"; // 21
          std::cout << yyyy_ << "|"; // 22
        }
      }
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

  shc = argv[2];

  rapidxml::file<char> xml_file_ = rapidxml::file<char>(basefile_);

  rapidxml::xml_document<> doc_;  // character type defaults to char

  doc_.parse<0>(xml_file_.data());  // 0 means default parse flags

  goFor(doc_.first_node());

  return 1;
}
