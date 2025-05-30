/**
    \file TemplateParser.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#pragma once

#include "infracore/rapidxml/rapidxml_utils.hpp"

#include "infracore/codegen/Field.hpp"
#include "infracore/codegen/Template.hpp"

namespace FFCodeGen {

class TemplateParser {
  typedef rapidxml::xml_node<char>* XMLNode;
  typedef rapidxml::xml_attribute<char>* XMLAttribute;
  XMLAttribute getAttribute(XMLNode e, std::string attributeName) {
    for (XMLAttribute a = e->first_attribute(); a; a = a->next_attribute()) {
      if (attributeName == a->name()) return a;
    }
    return NULL;
  }

  std::string getAttributeValue(XMLNode e, std::string attributeName, const std::string defaultValue) {
    for (XMLAttribute a = e->first_attribute(); a; a = a->next_attribute()) {
      if (attributeName == a->name()) {
        return a->value();
      }
    }
    return defaultValue;
  }

  Template parseTemplate(XMLNode templateNode) {
    Template t;
    t.name = getAttributeValue(templateNode, "name", "");
    t.id = atoi(getAttributeValue(templateNode, "id", "-1").c_str());
    t.fieldArr = std::vector<Field*>();
    for (XMLNode child = templateNode->first_node(); child; child = child->next_sibling()) {
      parseFieldNode(child, t.fieldArr);
    }
    return t;
  }

  void parseFieldNode(XMLNode fieldNode, std::vector<Field*>& fieldArr) {
    std::string fieldType = fieldNode->name();

    FieldType type = FFCodeGen::Utils::getFieldType(fieldType);
    switch (type) {
      case Grp: {
        fieldArr.push_back(new Group());
        fieldArr.back()->name_attr = getAttributeValue(fieldNode, "name", "");
        fieldArr.back()->field_id = atoi(getAttributeValue(fieldNode, "id", "-1").c_str());
        ;
        fieldArr.back()->optional = getAttributeValue(fieldNode, "presence", "") == "optional";
        fieldArr.back()->field_operator = NoOp;
        fieldArr.back()->field_type = Grp;
        fieldArr.back()->requires_pmap_ = false;

        for (XMLNode child = fieldNode->first_node(); child; child = child->next_sibling()) {
          parseFieldNode(child, ((Group*)fieldArr.back())->fieldArr);
          if (((Group*)fieldArr.back())->fieldArr.size() > 0 &&
              ((Group*)fieldArr.back())->fieldArr.back()->requires_pmap_)
            fieldArr.back()->requires_pmap_ = true;
        }
        return;
      }
      case Seq: {
        std::string name_attr = getAttributeValue(fieldNode, "name", "");
        bool optional = getAttributeValue(fieldNode, "presence", "") == "optional";
        int field_id = atoi(getAttributeValue(fieldNode, "id", "-1").c_str());
        fieldArr.push_back(new FFCodeGen::Sequence(name_attr, field_id, NoOp, optional));
        fieldArr.back()->requires_pmap_ = false;

        int cnt = 0;
        for (XMLNode child = fieldNode->first_node(); child; child = child->next_sibling()) {
          cnt++;
          parseFieldNode(child, ((FFCodeGen::Sequence*)fieldArr.back())->fieldArr);
          // the first field is length, hence we dont use it to set requires_pmap_ for the sequence field
          if (cnt > 1 && ((Sequence*)fieldArr.back())->fieldArr.size() > 0 &&
              ((Sequence*)fieldArr.back())->fieldArr.back()->requires_pmap_)
            fieldArr.back()->requires_pmap_ = true;
        }
        ((FFCodeGen::Sequence*)fieldArr.back())->fieldArr.front()->optional =
            optional;  // length field is optional if sequence is optional
        return;
      }
      case TemplateRef: {
        std::string templateRef = getAttributeValue(fieldNode, "name", "");
        for (unsigned int i = 0; i < allTemplates.size(); ++i) {
          if (allTemplates[i].name == templateRef) {
            for (unsigned int j = 0; j < allTemplates[i].fieldArr.size(); ++j)
              fieldArr.push_back(allTemplates[i].fieldArr[j]);
            break;
          }
        }
      } break;
      case TypeRef:
        // If the parent template has name attribute empty, it can be set from the name attribute of this node
        // However for now we ignore it
        break;

      case Decimal: {
        DecimalField* d = new DecimalField();
        d->name_attr = getAttributeValue(fieldNode, "name", "");
        d->optional = getAttributeValue(fieldNode, "presence", "") == "optional";
        d->field_id = atoi(getAttributeValue(fieldNode, "id", "-1").c_str());
        d->field_type = Decimal;

        d->scalar = !isDecimalComponentsPresent(fieldNode);
        if (d->scalar) {
          bool hasDefVal = false;
          std::string defVal;
          d->field_operator = getFieldOperator(fieldNode, hasDefVal, defVal);
          d->requires_pmap_ = Utils::isPmapRequired(d->field_operator, d->optional);
          d->has_def_value = hasDefVal;
          d->default_value = Utils::getDefaultValue(Decimal, defVal);
        } else {
          d->requires_pmap_ = d->optional;  // TODO figure this out

          std::vector<Field*> decComp;
          parseFieldNode(fieldNode->first_node(), decComp);
          // if the xml file does not have a mantissa node following exponent
          // next line will result in segfault cause isDecimalComponentsPresent checks that "one" node is there.
          // although this should not be the case with valid XML.
          parseFieldNode(fieldNode->first_node()->next_sibling(), decComp);
          d->exponent = decComp[0];
          if (d->optional) d->exponent->optional = d->optional;
          d->mantissa = decComp[1];

          d->exponent->name_attr = d->name_attr + "_exponent";
          d->mantissa->name_attr = d->name_attr + "_mantissa";
        }
        fieldArr.push_back(d);
        return;
      }
      default: {
        std::string name_attr = getAttributeValue(fieldNode, "name", "");
        bool optional = getAttributeValue(fieldNode, "presence", "") == "optional";
        int field_id = atoi(getAttributeValue(fieldNode, "id", "-1").c_str());
        bool hasDefVal = false;
        std::string defVal = "";
        FFCodeGen::FieldOp fieldOp = getFieldOperator(fieldNode, hasDefVal, defVal);
        bool pmap_required = Utils::isPmapRequired(fieldOp, optional);
        FieldType fldType = Utils::getFieldType(fieldType);
        fieldArr.push_back(new Field(name_attr, field_id, fieldOp, fldType, optional, hasDefVal,
                                     Utils::getDefaultValue(fldType, defVal), pmap_required));
        return;
      }
    }
  }

  FFCodeGen::FieldOp getFieldOperator(XMLNode fieldNode, bool& hasDefValue, std::string& defVal) {
    std::string field_operator_ = "";

    if (fieldNode->first_node()) {
      field_operator_ = fieldNode->first_node()->name();
      defVal = getAttributeValue(fieldNode->first_node(), "value", "");
      hasDefValue = (defVal == "") ? false : true;
    }
    return Utils::getFieldOperatorType(field_operator_);
  }

  // check to see if decimal's having sub field or not
  bool isDecimalComponentsPresent(XMLNode fieldNode) {
    std::string decimal_field_type_ = "";
    if (fieldNode->first_node()) {
      decimal_field_type_ = fieldNode->first_node()->name();
      if (decimal_field_type_ == "exponent" || decimal_field_type_ == "mantissa") return true;
      return false;
    }
    return false;
  }

  std::vector<Template> allTemplates;

 public:
  std::vector<FFCodeGen::Template> parseXMLFile(std::string file) {
    rapidxml::file<char> fl = rapidxml::file<char>(file.c_str());
    rapidxml::xml_document<> doc;  // character type defaults to char
    doc.parse<0>(fl.data());       // 0 means default parse flags

    allTemplates = std::vector<Template>();
    allTemplates.clear();
    for (XMLNode child = doc.first_node()->first_node(); child; child = child->next_sibling()) {
      if (strncmp(child->name(), "template", 8) != 0) continue;
      allTemplates.push_back(parseTemplate(child));
    }
    return allTemplates;
  }
};
}
