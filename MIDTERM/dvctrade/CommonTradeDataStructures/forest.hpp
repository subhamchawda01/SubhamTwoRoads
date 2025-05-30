/*
 * forest.hpp
 *
 *  Created on: Aug 12, 2015
 *      Author: archit
 */

#ifndef DVCCODE_COMMONDATASTRUCTURES_FOREST_HPP_
#define DVCCODE_COMMONDATASTRUCTURES_FOREST_HPP_

#define NONLINEARCOMPONENT_IND_IDX_OFFSET 3

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/CommonInterfaces/indicator_set.hpp"
#include <sstream>
#include <queue>

namespace HFSAT {

enum ForestModelFilePhase { kPreIndStart, kInd, kPostIndEnd, kPreTreeStart, KTree, KPostTreeEnd };

class Node {
 public:
  std::string node_pre_text_;
  int left_child_idx_;
  int right_child_idx_;
  const Indicator* p_ind_;
  // double splitting_val_;
  // using string instead of double to avoid differences due to floating points while printing the model
  // if needed we can change this to double as practically small floating point difference wont matter
  std::string splitting_val_;
  bool is_leaf_;
  // double pred_val_;
  std::string pred_val_;

  Node(const std::vector<const char*>& _tokens_, const std::vector<Indicator*>& _p_ind_vec_);
  int GetNextChildIdx(double _val_);
  std::string ToString(int _left_idx_, int _right_idx_) const;
};

class Tree : public Stringable {
  std::string pre_treestart_text_;
  std::string post_treeend_text_;
  std::vector<Node> node_vec_;
  unsigned int root_node_idx_;
  std::string comments_;

 public:
  Tree(const std::string& _pre_text_);
  void AddPostText(const std::string& _post_text_) { post_treeend_text_ += _post_text_; }
  void AddComment(const std::string& _comment_);
  void AddNode(const std::vector<const char*>& _tokens_, const std::vector<Indicator*>& _p_ind_vec_);
  void RemoveIndicator(int ind_idx_);
  bool IsEmptyTree() const { return (node_vec_.size() <= root_node_idx_); }
  std::string ToString() const;
};

class Forest : public IndicatorSet {
 private:
  std::string forestfile_name_;
  std::string pre_indstart_text_;
  std::string post_indend_text_;
  std::vector<Tree> tree_vec_;
  std::string comments_;

 public:
  Forest(const std::string& _modelfilename_);
  void RemoveIndicatorByIdx(int _ind_idx_);
  std::string ToString() const;
  void AddComment(const std::string& _comment_);
};

class NonLinearComp : public ModelLine {
 public:
  NonLinearComp(const std::string& _comp_str_, const Indicator* _p_ind_);
  std::string ToString() const;
  static int GetIdxFromStr(const std::string& _comp_str_);
};

class ModelLineSet : public IndicatorSet {
  std::vector<ModelLine*> p_model_lines_vec_;
  std::string modelfilename_;

 public:
  ModelLineSet(std::string& _modelfilename_);
  ~ModelLineSet() {
    for (auto i = 0u; i < p_model_lines_vec_.size(); i++) {
      if (p_model_lines_vec_[i]) {
        delete p_model_lines_vec_[i];
      }
    }
  }
  virtual void RemoveIndicatorByIdx(int _ind_idx_);
  virtual std::string ToString() const;
};
} /* namespace HFSAT */

#endif /* DVCCODE_COMMONDATASTRUCTURES_FOREST_HPP_ */
