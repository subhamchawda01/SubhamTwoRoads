#include <iostream>
#include <stdio.h>
#include <fstream>
#include "infracore/rapidxml/rapidxml_utils.hpp"

// Uses the input file from ftp://ftp.cmegroup.com/SBEFix/Production/Configuration/config.xml
// Picks multicast channel information for all channels.

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << " USAGE: argv[0] Input-xml-file" << std::endl;
    exit(0);
  }

  std::string exch = argv[1];

  rapidxml::file<> xmlFile(argv[1]);  // Default template is char
  rapidxml::xml_document<> doc;
  // Parse the buffer using the xml file parsing library into doc
  doc.parse<0>(xmlFile.data());

  // Find our root node
  rapidxml::xml_node<>* root_node;
  root_node = doc.first_node("configuration");
  // Iterate over the channels
  for (rapidxml::xml_node<>* channels = root_node->first_node("channel"); channels;
       channels = channels->next_sibling()) {
    int channel = atoi(channels->first_attribute("id")->value());
    rapidxml::xml_node<>* all_connections = channels->first_node("connections");
    // Interate over all connection nodes
    for (rapidxml::xml_node<>* connection_node = all_connections->first_node("connection"); connection_node;
         connection_node = connection_node->next_sibling()) {
      std::string feed_type = connection_node->first_node("type")->first_attribute("feed-type")->value();
      if (feed_type == "SMBO") feed_type = "O";
      // Historical Replay channels has no ip field
      if (!connection_node->first_node("ip")) continue;
      std::string ip = connection_node->first_node("ip")->value();
      int port = atoi(connection_node->first_node("port")->value());
      std::string feed = connection_node->first_node("feed")->value();
      std::cout << channel << " \t" << feed_type << " \t" << feed << " \t" << ip << " \t" << port << std::endl;
    }
  }

  return 0;
}
