/**
 \file Indentation.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 162, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */

#pragma once

#include <iostream>
#include <sstream>

class Indentation {
  int depth;
  Indentation() : depth(0) {}
  Indentation(const Indentation& i) {}

 public:
  static Indentation& instance() {
    static Indentation* instance = NULL;
    if (instance == NULL) instance = new Indentation();
    return *instance;
  }

  std::string indent() {
    char buf[90];
    for (int i = 0; i < 2 * depth; ++i) buf[i] = ' ';
    buf[2 * depth] = 0;
    return buf;
  }

  void increase() {
    ++depth;
    if (depth > 40) depth = 40;
  }

  void decrease() {
    --depth;
    if (depth < 0) depth = 0;
  }
};

class PMapNameManager {
  int depth;
  PMapNameManager() : depth(0) {}
  PMapNameManager(const PMapNameManager& i) {}

 public:
  static PMapNameManager& instance() {
    static PMapNameManager* instance = NULL;
    if (instance == NULL) instance = new PMapNameManager();
    return *instance;
  }

  std::string get_name() {
    static std::stringstream ss;
    ss.str("");
    ss << "pmap" << (depth);
    return ss.str();
  }

  void create_new() { ++depth; }

  /**
   * returns false if fails to remove last
   */
  bool remove_old() {
    if (depth > 0) --depth;
    if (depth >= 0) return true;
    return false;
  }
};
