#ifndef BASE_MODELMATH_DECISION_TREE_H
#define BASE_MODELMATH_DECISION_TREE_H

namespace HFSAT {
struct TreeLine {
  int tree_index_;
  int left_daughter_;
  int right_daughter_;
  int split_indicator_index_;
  double split_indicator_value_;

  bool is_leaf_;

  double prediction_;
  bool node_answer_;

  TreeLine()
      : left_daughter_(-1),
        right_daughter_(-1),
        split_indicator_index_(-1),
        split_indicator_value_(0.0),
        is_leaf_(false),
        prediction_(0.0),
        node_answer_(true) {}

  TreeLine(const TreeLine& _new_tree_line_)
      : left_daughter_(_new_tree_line_.left_daughter_),
        right_daughter_(_new_tree_line_.right_daughter_),
        split_indicator_index_(_new_tree_line_.split_indicator_index_),
        split_indicator_value_(_new_tree_line_.split_indicator_value_),
        is_leaf_(_new_tree_line_.is_leaf_),
        prediction_(_new_tree_line_.prediction_),
        node_answer_(_new_tree_line_.node_answer_) {}

  inline std::string toString() const {
    std::ostringstream t_temp_oss_;

    t_temp_oss_ << "TreeLine" << ' ' << left_daughter_ << ' ' << right_daughter_ << ' ' << split_indicator_index_ << ' '
                << split_indicator_value_ << ' ' << is_leaf_ << ' ' << prediction_ << ' ' << node_answer_ << ' '
                << "\n";

    return t_temp_oss_.str();
  }
};

bool TreeNodeComparision(TreeLine* t1, TreeLine* t2) {
  if (t1->split_indicator_value_ < t2->split_indicator_value_) {
    return true;
  } else {
    return false;
  }
}
}

#endif
