/*
 * forest.cpp
 *
 *  Created on: Aug 12, 2015
 *      Author: archit
 */

#include "dvctrade/CommonTradeDataStructures/forest.hpp"

namespace HFSAT {

Node::Node(const std::vector<const char*>& _tokens_, const std::vector<Indicator*>& _p_ind_vec_) {
  if (_tokens_.size() < 7) {
    std::cerr << "ERROR! Invalid args for node init : " << VectorUtils::Join(_tokens_, " ") << "\n";
    exit(1);
  }
  node_pre_text_ = std::string(_tokens_[0]);
  is_leaf_ = strcmp(_tokens_[5], "Y") == 0;
  if (!is_leaf_) {
    left_child_idx_ = atoi(_tokens_[1]);
    right_child_idx_ = atoi(_tokens_[2]);
    int ind_idx_ = atoi(_tokens_[3]);

    if (ind_idx_ < 0 || ind_idx_ >= int(_p_ind_vec_.size())) {
      std::cerr << "ERROR! Invalid ind_idx: " << ind_idx_ << " in line: " << VectorUtils::Join(_tokens_, " ")
                << " for node init\n";
      exit(1);
    }
    p_ind_ = _p_ind_vec_[ind_idx_];
    // splitting_val_ = atof(_tokens_[4]);
    splitting_val_ = std::string(_tokens_[4]);
  } else {
    left_child_idx_ = -1;
    right_child_idx_ = -1;
    p_ind_ = NULL;
    splitting_val_ = "0";
  }
  // pred_val_ = atof(_tokens_[6]);
  pred_val_ = std::string(_tokens_[6]);
}

int Node::GetNextChildIdx(double _val_) {
  if (is_leaf_) {
    return -1;
  }
  if (_val_ < atof(splitting_val_.c_str())) {
    return left_child_idx_;
  } else {
    return right_child_idx_;
  }
}

std::string Node::ToString(int _left_idx_, int _right_idx_) const {
  std::ostringstream temp_oss_;
  temp_oss_ << node_pre_text_ << " " << _left_idx_ << " " << _right_idx_ << " " << (is_leaf_ ? -1 : p_ind_->ind_idx_)
            << " " << splitting_val_ << " " << (is_leaf_ ? "Y" : "N") << " " << pred_val_;

  return temp_oss_.str();
}

Tree::Tree(const std::string& _pre_text_)
    : pre_treestart_text_(_pre_text_), post_treeend_text_(""), node_vec_(), root_node_idx_(0), comments_("") {}

void Tree::AddNode(const std::vector<const char*>& _tokens_, const std::vector<Indicator*>& _p_ind_vec_) {
  node_vec_.emplace_back(_tokens_, _p_ind_vec_);
}

void Tree::AddComment(const std::string& _comment_) {
  if (_comment_[0] != '#') {
    comments_ += "#";
  }
  comments_ += _comment_;
  comments_ += "\n";
}

void Forest::AddComment(const std::string& _comment_) {
  if (_comment_[0] != '#') {
    comments_ += "#";
  }
  comments_ += _comment_;
  comments_ += "\n";
}

void Tree::RemoveIndicator(int _ind_idx_) {
  if (IsEmptyTree()) {
    return;
  }

  while (!node_vec_[root_node_idx_].is_leaf_ && node_vec_[root_node_idx_].p_ind_->ind_idx_ == _ind_idx_) {
    Node& this_node_ = node_vec_[root_node_idx_];
    root_node_idx_ = this_node_.GetNextChildIdx(0.0);
  }

  std::queue<int> nodes_to_traverse_;
  nodes_to_traverse_.push(root_node_idx_);
  while (!nodes_to_traverse_.empty()) {
    int node_idx_ = nodes_to_traverse_.front();
    if (node_idx_ < 0 || node_idx_ >= int(node_vec_.size())) {
      std::cerr << "ERROR! invalid node_idx: " << node_idx_ << " while traversing tree\n";
      exit(1);
    }

    Node& this_node_ = node_vec_[node_idx_];
    nodes_to_traverse_.pop();
    if (!this_node_.is_leaf_) {
      while (!node_vec_[this_node_.left_child_idx_].is_leaf_ &&
             node_vec_[this_node_.left_child_idx_].p_ind_->ind_idx_ == _ind_idx_) {
        this_node_.left_child_idx_ = node_vec_[this_node_.left_child_idx_].GetNextChildIdx(0.0);
      }
      nodes_to_traverse_.push(this_node_.left_child_idx_);

      while (!node_vec_[this_node_.right_child_idx_].is_leaf_ &&
             node_vec_[this_node_.right_child_idx_].p_ind_->ind_idx_ == _ind_idx_) {
        this_node_.right_child_idx_ = node_vec_[this_node_.right_child_idx_].GetNextChildIdx(0.0);
      }
      nodes_to_traverse_.push(this_node_.right_child_idx_);
    }
  }
}

std::string Tree::ToString() const {
  std::ostringstream temp_oss_;
  temp_oss_ << pre_treestart_text_;
  temp_oss_ << comments_;
  if (!IsEmptyTree()) {
    int node_idx_counter_ = 0;
    std::queue<int> nodes_to_traverse_;
    nodes_to_traverse_.push(root_node_idx_);
    while (!nodes_to_traverse_.empty()) {
      int node_idx_ = nodes_to_traverse_.front();
      if (node_idx_ < 0 || node_idx_ >= int(node_vec_.size())) {
        std::cerr << "ERROR! invalid node_idx: " << node_idx_ << " while traversing tree\n";
        exit(1);
      }

      const Node& this_node_ = node_vec_[node_idx_];
      nodes_to_traverse_.pop();
      if (!this_node_.is_leaf_) {
        temp_oss_ << this_node_.ToString(node_idx_counter_ + 1, node_idx_counter_ + 2) << "\n";
        nodes_to_traverse_.push(this_node_.left_child_idx_);
        nodes_to_traverse_.push(this_node_.right_child_idx_);
        node_idx_counter_ += 2;
      } else {
        temp_oss_ << this_node_.ToString(-1, -1) << "\n";
      }
    }
  }
  temp_oss_ << post_treeend_text_;
  return temp_oss_.str();
}

Forest::Forest(const std::string& _modelfilename_)
    : IndicatorSet(),
      forestfile_name_(_modelfilename_),
      pre_indstart_text_(""),
      post_indend_text_(""),
      tree_vec_(),
      comments_("") {
  std::ifstream ifs_(_modelfilename_, std::ifstream::in);
  if (ifs_.is_open()) {
    const int kFileLineBufferLen = 1024;
    char readline_buffer_[kFileLineBufferLen];
    ForestModelFilePhase phase_ = kPreIndStart;
    std::string curr_tree_pre_text_ = "";

    while (ifs_.good()) {
      bzero(readline_buffer_, kFileLineBufferLen);
      ifs_.getline(readline_buffer_, kFileLineBufferLen);
      std::string line_ = std::string(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() < 1) {
        continue;
      }

      bool is_comment_ = (tokens_[0][0] == '#');
      // passing comments further to maintain right position of comments
      // comments b/w INDICATOR and TREELINE will loose the position

      if (kPreIndStart == phase_) {
        // [ , INDICATORSTART ]
        if (strcmp(tokens_[0], "INDICATORSTART") == 0) {
          phase_ = kInd;
        }
        pre_indstart_text_ += line_;
        pre_indstart_text_ += "\n";
      } else if (kInd == phase_) {
        // ( INDICATORSTART , INDICATOREND ]
        if (strcmp(tokens_[0], "INDICATOREND") == 0) {
          post_indend_text_ += line_;
          post_indend_text_ += "\n";
          phase_ = kPostIndEnd;
        } else if (strcmp(tokens_[0], "INDICATOR") == 0) {
          // ( INDICATORSTART , INDICATOREND )
          p_ind_vec_.push_back(new Indicator(line_, p_ind_vec_.size()));
        } else {
          if (!is_comment_) {
            std::cerr << "WARNING! NONINDICATOR line : " << line_
                      << " b/w INDICATORSTART & INDICATOREND in forestfile_: " << forestfile_name_
                      << " . Adding as a comment\n";
          }
          AddComment(line_);
        }

      } else if (kPostIndEnd == phase_) {
        // ( INDICATOREND, TREESTART ]
        if (strcmp(tokens_[0], "TREESTART") == 0) {
          curr_tree_pre_text_ = line_;
          curr_tree_pre_text_ += "\n";
          phase_ = kPreTreeStart;
        } else {
          // ( INDICATOREND, TREESTART )
          post_indend_text_ += line_;
          post_indend_text_ += "\n";
        }
      } else if (kPreTreeStart == phase_) {
        // ( TREESTART, TREELINE ]
        if (strcmp(tokens_[0], "TREELINE") == 0) {
          tree_vec_.emplace_back(curr_tree_pre_text_);
          tree_vec_.back().AddNode(tokens_, p_ind_vec_);
          phase_ = KTree;
        } else {
          // ( TREESTART, TREELINE )
          curr_tree_pre_text_ += line_;
          curr_tree_pre_text_ += "\n";
        }
      } else if (KTree == phase_) {
        // [ TREELINE , TREESTART ]
        if (strcmp(tokens_[0], "TREESTART") == 0) {
          curr_tree_pre_text_ = line_;
          curr_tree_pre_text_ += "\n";
          phase_ = kPreTreeStart;
        } else if (strcmp(tokens_[0], "TREELINE") == 0) {
          tree_vec_.back().AddNode(tokens_, p_ind_vec_);
        } else if (is_comment_) {
          tree_vec_.back().AddComment(line_);
        } else {
          tree_vec_.back().AddPostText(line_ + "\n");
          phase_ = KPostTreeEnd;
        }
      } else if (KPostTreeEnd == phase_) {
        if (strcmp(tokens_[0], "TREESTART") == 0) {
          curr_tree_pre_text_ = line_;
          curr_tree_pre_text_ += "\n";
          phase_ = kPreTreeStart;
        } else {
          tree_vec_.back().AddPostText(line_ + "\n");
        }
      }
    }
    ifs_.close();
  } else {
    std::cerr << "Can't open forestfile: " << forestfile_name_ << " for reading\n";
    exit(1);
  }
}

void Forest::RemoveIndicatorByIdx(int _ind_idx_) {
  if (_ind_idx_ < 0 || _ind_idx_ >= int(p_ind_vec_.size()) || !p_ind_vec_[_ind_idx_] ||
      p_ind_vec_[_ind_idx_]->ind_idx_ < 0) {
    return;  // invalid ind_idx_ or already removed
  }
  for (auto i = 0u; i < tree_vec_.size(); i++) {
    tree_vec_[i].RemoveIndicator(p_ind_vec_[_ind_idx_]->ind_idx_);
  }
  p_ind_vec_[_ind_idx_]->ind_idx_ = -1;  // mark this as invalid
  for (unsigned int i = _ind_idx_ + 1; i < p_ind_vec_.size(); i++) {
    if (p_ind_vec_[i]) {
      p_ind_vec_[i]->ind_idx_--;  // adjust ind_indices
    }
  }
}

std::string Forest::ToString() const {
  std::ostringstream temp_oss_;
  temp_oss_ << pre_indstart_text_;
  for (auto i = 0u; i < p_ind_vec_.size(); i++) {
    if (p_ind_vec_[i] && p_ind_vec_[i]->ind_idx_ >= 0) {  // only valid inds
      temp_oss_ << p_ind_vec_[i]->line_ << "\n";
    }
  }
  temp_oss_ << post_indend_text_;
  temp_oss_ << comments_;
  for (auto i = 0u; i < tree_vec_.size(); i++) {
    if (tree_vec_[i].IsEmptyTree()) {
      std::cerr << "WARNING! forest(" << forestfile_name_ << ") should not have an empty tree, skipping\n";
    }
    temp_oss_ << tree_vec_[i].ToString();
  }
  return temp_oss_.str();
}

NonLinearComp::NonLinearComp(const std::string& _comp_str_, const Indicator* _p_ind_)
    : ModelLine(_comp_str_, _p_ind_) {}

std::string NonLinearComp::ToString() const {
  std::ostringstream temp_oss_;
  const size_t t_len_ = strlen(line_.c_str()) + 1;
  char temp_str_[t_len_];
  strncpy(temp_str_, line_.c_str(), t_len_);
  temp_str_[t_len_] = '\0';
  std::vector<char*> tokens_;
  PerishableStringTokenizer::NonConstStringTokenizer(temp_str_, " ", tokens_);
  for (auto i = 0u; i < tokens_.size(); i++) {
    if (NONLINEARCOMPONENT_IND_IDX_OFFSET == i) {
      temp_oss_ << (p_ind_ ? p_ind_->ind_idx_ : -1) << " ";
    } else {
      temp_oss_ << tokens_[i] << " ";
    }
  }
  return temp_oss_.str();
}

int NonLinearComp::GetIdxFromStr(const std::string& _comp_str_) {
  const size_t t_len_ = strlen(_comp_str_.c_str()) + 1;
  char temp_str_[t_len_];
  strncpy(temp_str_, _comp_str_.c_str(), t_len_);
  temp_str_[t_len_] = '\0';
  PerishableStringTokenizer st_(temp_str_, t_len_);
  std::vector<const char*> tokens_ = st_.GetTokens();
  if (NONLINEARCOMPONENT_IND_IDX_OFFSET >= tokens_.size()) {
    std::cerr << "WARNING! NonLinearComp with less than " << NONLINEARCOMPONENT_IND_IDX_OFFSET + 1 << "tokens\n";
    return -1;
  } else {
    return atoi(tokens_[NONLINEARCOMPONENT_IND_IDX_OFFSET]);
  }
}

ModelLineSet::ModelLineSet(std::string& _modelfilename_)
    : IndicatorSet(), p_model_lines_vec_(), modelfilename_(_modelfilename_) {
  std::ifstream ifs_(_modelfilename_, std::ifstream::in);
  if (ifs_.is_open()) {
    const int kFileLineBufferLen = 1024;
    char readline_buffer_[kFileLineBufferLen];

    while (ifs_.good()) {
      bzero(readline_buffer_, kFileLineBufferLen);
      ifs_.getline(readline_buffer_, kFileLineBufferLen);
      std::string line_ = std::string(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() < 1) {
        continue;
      }

      if (strcmp(tokens_[0], "INDICATOR") == 0) {
        // TODO: fix this
        p_ind_vec_.push_back(new Indicator(line_, p_ind_vec_.size()));
        p_model_lines_vec_.push_back(p_ind_vec_.back());
      } else if (strcmp(tokens_[0], "NONLINEARCOMPONENT") == 0) {
        Indicator* p_ind_ = NULL;
        int ind_idx_ = NonLinearComp::GetIdxFromStr(line_);
        if (ind_idx_ >= 0 && ind_idx_ < int(p_ind_vec_.size())) {
          p_ind_ = p_ind_vec_[ind_idx_];
        }
        p_model_lines_vec_.push_back(new NonLinearComp(line_, p_ind_));
      } else {
        p_model_lines_vec_.push_back(new ModelLine(line_, NULL));
      }
    }
    ifs_.close();
  } else {
    std::cerr << "Can't open modelfile: " << _modelfilename_ << " for reading\n";
    exit(1);
  }
}
std::string ModelLineSet::ToString() const {
  std::ostringstream temp_oss_;
  for (auto i = 0u; i < p_model_lines_vec_.size(); i++) {
    if (!p_model_lines_vec_[i]) {
      std::cerr << "WARNING! skipping null/corrupt ModelLine[" << i << "] in modelfile: " << modelfilename_ << "\n";
      continue;
    }
    if (!p_model_lines_vec_[i]->p_ind_ || p_model_lines_vec_[i]->p_ind_->ind_idx_ >= 0) {
      temp_oss_ << p_model_lines_vec_[i]->ToString() << std::endl;
    }
  }
  return temp_oss_.str();
}

void ModelLineSet::RemoveIndicatorByIdx(int _ind_idx_) {
  if (_ind_idx_ < 0 || _ind_idx_ >= int(p_ind_vec_.size()) || !p_ind_vec_[_ind_idx_] ||
      p_ind_vec_[_ind_idx_]->ind_idx_ < 0) {
    return;  // invalid ind_idx_ or already removed
  }
  p_ind_vec_[_ind_idx_]->ind_idx_ = -1;  // mark this as invalid
  for (unsigned int i = _ind_idx_ + 1; i < p_ind_vec_.size(); i++) {
    if (p_ind_vec_[i]) {
      p_ind_vec_[i]->ind_idx_--;  // adjust ind_indices
    }
  }
}
} /* namespace HFSAT */
