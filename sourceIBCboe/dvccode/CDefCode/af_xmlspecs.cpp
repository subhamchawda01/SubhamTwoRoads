// =====================================================================================
//
//       Filename:  af_xmlspecs.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Thursday 11 December 2014 08:14:26  IST
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvccode/CDef/af_xmlspecs.hpp"

namespace AF_MSGSPECS {

MsgType parseMsgType(std::string name_str_) {
  if (name_str_.compare("release") == 0) {
    return kRelease;
  } else if (name_str_.compare("estimate") == 0) {
    return kEstimate;
  } else {
    return kUndefined;
  }
}

FieldType parseFieldType(std::string name_str_) {
  if (name_str_.compare("float") == 0) {
    return kFloat;
  } else if (name_str_.compare("float-range") == 0) {
    return kFloat_range;
  } else if (name_str_.compare("float-range") == 0) {
    return kFloat_range;
  } else if (name_str_.compare("short") == 0) {
    return kShort;
  } else if (name_str_.compare("long") == 0) {
    return kLong;
  } else if (name_str_.compare("double") == 0) {
    return kDouble;
  } else if (name_str_.compare("date") == 0) {
    return kDate;
  } else if (name_str_.compare("boolean") == 0) {
    return kBoolean;
  } else if (name_str_.compare("yes_no_na") == 0) {
    return kYes_no_na;
  } else if (name_str_.compare("directional") == 0) {
    return kDirectional;
  } else if (name_str_.compare("int") == 0) {
    return kInt;
  } else if (name_str_.compare("time") == 0) {
    return kTime;
  } else if (name_str_.compare("timeframe") == 0) {
    return kTimeframe;
  } else if (name_str_.compare("enumeration") == 0) {
    return kEnumeration;
  } else if (name_str_.compare("iso_country_code") == 0) {
    return kIso_country_code;
  } else if (name_str_.compare("short_value_enumeration") == 0) {
    return kShort_value_enumeration;
  } else {
    return kInvalidFieldType;
  }
}

void parseXML(std::string xml_fname_, std::vector<Category *> &catg_list, HFSAT::DebugLogger *dbglogger_) {
  std::ifstream xml_file_;
  xml_file_.open(xml_fname_.c_str());

  if (!xml_file_.is_open()) {
    if (dbglogger_) {
      (*dbglogger_) << " Error: XML_spec_File: " << xml_fname_ << "does not exist.. Exiting\n";
    } else {
      std::cerr << " XML_spec_File: " << xml_fname_ << "does not exist.. Exiting " << std::endl;
    }
    exit(-1);
  }

  char line_buffer_[MAX_AF_LINE_SIZE];
  std::string line_read_ = "";
  Category *catg;

  /*  defines if we are in the particular element of the xml */
  bool in_catg = false;
  bool in_data = false;
  bool in_datum = false;
  bool in_msgs = false;
  bool in_msg = false;
  bool in_fields = false;
  bool in_field = false;

  int datum_index_ = -1;
  int msg_index_ = -1;
  int field_index_ = -1;

  while (xml_file_.good()) {
    memset(line_buffer_, 0, sizeof(line_buffer_));
    line_read_ = "";

    xml_file_.getline(line_buffer_, sizeof(line_buffer_));
    line_read_ = line_buffer_;

    int name_st_ = line_read_.find(">");
    int name_end_ = line_read_.find("<", name_st_ + 1);
    std::string name_str_ = line_read_.substr(name_st_ + 1, (name_end_ - name_st_ - 1));

    if (!in_catg) {
      if (line_read_.find("<category") == 0) {
        catg = new Category;
        catg_list.push_back(catg);
        int name_st_ = line_read_.find("\"");
        int name_end_ = line_read_.find("\"", name_st_ + 1);
        name_str_ = line_read_.substr(name_st_ + 1, (name_end_ - name_st_ - 1));
        catg->category_name_ = name_str_;
        in_catg = true;
        datum_index_ = -1;
        msg_index_ = -1;
      }
    }

    /* Inside Category element but outside of any Message or Data */
    else if (in_catg && !in_data && !in_msgs) {
      if (line_read_.find("<description>") == 0) {
        catg->description_ = name_str_;
      }

      if (line_read_.find("<category_id>") == 0) {
        catg->category_id_ = atoi(name_str_.c_str());
      }

      if (line_read_.find("<country>") == 0) {
        catg->country_ = name_str_;
      }

      if (line_read_.find("<data>") == 0) {
        in_data = true;  // Enters data
      }

      if (line_read_.find("<messages>") == 0) {
        in_msgs = true;  // Enters Messages
      }

      if (line_read_.find("</category>") == 0) {
        in_catg = false;  // Exits Category
      }
    }

    /*  Inside Data, outside any Datum */
    else if (in_data && !in_datum) {
      if (line_read_.find("<datum>") == 0) {
        in_datum = true;  // Enters Datum
        catg->data.push_back(new Datum);
        datum_index_++;
      }
      if (line_read_.find("</data>") == 0) {
        in_data = false;  // Exits Data
      }
    }

    /*  Inside Messages_list, Outside Msg */
    else if (in_msgs && !in_msg) {
      if (line_read_.find("<message>") == 0) {
        in_msg = true;  // Enters Message
        catg->msgs.push_back(new Message);
        msg_index_++;
        field_index_ = -1;
      }
      if (line_read_.find("</messages>") == 0) {
        in_msgs = false;  // Exits Messages
      }
    }

    /*  Inside Datum  */
    else if (in_data && in_datum) {
      if (line_read_.find("<short-description>") == 0) {
        catg->data[datum_index_]->short_desc_ = name_str_;
      }
      if (line_read_.find("<description>") == 0) {
        catg->data[datum_index_]->desc_ = name_str_;
      }
      if (line_read_.find("<datum_type>") == 0) {
        catg->data[datum_index_]->type_ = name_str_;
      }
      if (line_read_.find("<datum_scale>") == 0) {
        catg->data[datum_index_]->scale_ = name_str_;
      }
      if (line_read_.find("<datum_id>") == 0) {
        catg->data[datum_index_]->id_ = atoi(name_str_.c_str());
      }
      if (line_read_.find("</datum>") == 0) {
        in_datum = false;
      }
    }

    /*  Inside Message, Outside any of its Fields */
    else if (in_msgs && in_msg && !in_fields) {
      if (line_read_.find("<message_type>") == 0) {
        catg->msgs[msg_index_]->msg_type_ = parseMsgType(name_str_);
      }
      if (line_read_.find("<message_version>") == 0) {
        catg->msgs[msg_index_]->msg_version_ = atoi(name_str_.c_str());
      }
      if (line_read_.find("<multicast_group>") == 0) {
        catg->msgs[msg_index_]->mult_group_ = name_str_;
      }
      if (line_read_.find("<fields>") == 0) {
        in_fields = true;
      }
      if (line_read_.find("</message>") == 0) {
        in_msg = false;
      }
    }

    else if (in_msgs && in_msg && in_fields && !in_field) {
      if (line_read_.find("<field>") == 0) {
        in_field = true;
        catg->msgs[msg_index_]->fields.push_back(new Field);
        field_index_++;
      }
      if (line_read_.find("</fields>") == 0) {
        in_fields = false;
      }
    }

    /*  Inside Field */
    else if (in_msgs && in_msg && in_fields && in_field) {
      if (line_read_.find("<datum_id>") == 0) {
        catg->msgs[msg_index_]->fields[field_index_]->datum_id_ = atoi(name_str_.c_str());
      }
      if (line_read_.find("<field_type>") == 0) {
        catg->msgs[msg_index_]->fields[field_index_]->field_type_ = parseFieldType(name_str_);
      }
      if (line_read_.find("</field>") == 0) {
        in_field = false;
      }
    }
  }

  return;
}

std::string &trim_right_inplace(std::string &s, const std::string &delimiters) {
  return s.erase(s.find_last_not_of(delimiters) + 1);
}

std::string &trim_left_inplace(std::string &s, const std::string &delimiters) {
  return s.erase(0, s.find_first_not_of(delimiters));
}

std::string &trim_inplace(std::string &s, const std::string &delimiters) {
  return trim_left_inplace(trim_right_inplace(s, delimiters), delimiters);
}

void GetCatgXMLSpecs(std::vector<Category *> &catg_list, HFSAT::DebugLogger *dbglogger_) {
  std::ifstream xml_list_file_;
  xml_list_file_.open(XML_LIST_FNAME);

  if (!xml_list_file_.is_open()) {
    if (dbglogger_) {
      (*dbglogger_) << " Error: XML_List_File : " << XML_LIST_FNAME << "does not exist.. Exiting ";
    } else {
      std::cerr << " XML_List_File : " << XML_LIST_FNAME << "does not exist " << std::endl;
    }
    exit(-1);
  }

  char line_buffer_[MAX_AF_LINE_SIZE];
  std::string line_read_ = "";
  std::string xml_fname_ = "";

  while (xml_list_file_.good()) {
    memset(line_buffer_, 0, sizeof(line_buffer_));

    xml_list_file_.getline(line_buffer_, sizeof(line_buffer_));
    line_read_ = std::string(line_buffer_);
    line_read_ = trim_inplace(line_read_);

    if (line_read_.empty()) {
      continue;
    }
    xml_fname_ = XML_DIR + "/" + line_read_;

    std::vector<Category *> t_catg_list_;
    parseXML(xml_fname_, t_catg_list_, dbglogger_);
    catg_list.insert(catg_list.end(), t_catg_list_.begin(), t_catg_list_.end());
  }
}

std::string printCategory(Category *catg) {
  std::ostringstream catstr_;
  if (catg == NULL) {
    return "";
  }

  catstr_ << "Category:\n\tName: " << catg->category_name_ << std::endl;
  catstr_ << "\tDescription: " << catg->description_ << std::endl;
  catstr_ << "\tId: " << catg->category_id_ << std::endl;
  catstr_ << "\tCountry: " << catg->country_ << std::endl;
  for (std::vector<Datum *>::iterator it = catg->data.begin(); it != catg->data.end(); it++) {
    Datum *t_datum_ = *it;
    catstr_ << "\t\tDatum Id: " << t_datum_->id_ << std::endl;
    catstr_ << "\t\tDatum Short-desc: " << t_datum_->short_desc_ << std::endl;
    catstr_ << "\t\tDatum desc: " << t_datum_->desc_ << std::endl;
    catstr_ << "\t\tDatum type: " << t_datum_->type_ << std::endl;
    catstr_ << "\t\tDatum scale: " << t_datum_->scale_ << std::endl;
  }

  for (std::vector<Message *>::iterator it = catg->msgs.begin(); it != catg->msgs.end(); it++) {
    Message *t_msg_ = *it;
    catstr_ << "\t\tMsg type: " << t_msg_->msg_type_ << std::endl;
    catstr_ << "\t\tMsg version: " << t_msg_->msg_version_ << std::endl;
    catstr_ << "\t\tMsg multicast group: " << t_msg_->mult_group_ << std::endl;
    for (std::vector<Field *>::iterator it = t_msg_->fields.begin(); it != t_msg_->fields.end(); it++) {
      Field *t_field_ = *it;
      catstr_ << "\t\t\tField datum_id: " << t_field_->datum_id_ << std::endl;
      catstr_ << "\t\t\tField type: " << t_field_->field_type_ << std::endl;
    }
  }
  return catstr_.str();
}

void freeAll(std::vector<Category *> &catg_list) {
  for (std::vector<Category *>::iterator it = catg_list.begin(); it != catg_list.end(); it++) {
    Category *t_catg_ = *it;
    for (std::vector<Datum *>::iterator it = t_catg_->data.begin(); it != t_catg_->data.end(); it++) {
      Datum *t_datum_ = *it;
      delete t_datum_;
    }
    for (std::vector<Message *>::iterator it = t_catg_->msgs.begin(); it != t_catg_->msgs.end(); it++) {
      Message *t_msg_ = *it;
      for (std::vector<Field *>::iterator it = t_msg_->fields.begin(); it != t_msg_->fields.end(); it++) {
        Field *t_field_ = *it;
        delete t_field_;
      }
      delete t_msg_;
    }
    delete t_catg_;
  }
}
}
