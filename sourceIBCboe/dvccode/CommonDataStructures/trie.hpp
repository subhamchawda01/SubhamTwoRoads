/**
    \file trie.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef TRIE_HPP_
#define TRIE_HPP_

#define NUM_CHILDS 256

namespace HFSAT {

/**
 * An implementation of trie. Observed to be faster for small set of strings and can be used in SecurityNameIndexer
 */
class SimpleTrie {
  class trie_node {
    trie_node* children[NUM_CHILDS];
    int value;

   public:
    void add(char* ch, int& curr_val) {
      if (ch == NULL) {
        return;
      }
      if (*ch == 0) {
        if (value == -1) value = ++curr_val;
        return;
      }

      if (children[*ch] != NULL) {
        children[*ch]->add(ch + 1, curr_val);
      } else {
        children[*ch] = new trie_node(ch + 1, curr_val);
      }
    }

    int find(char* ch) {
      for (trie_node* p = this; p != NULL; p = p->children[*(ch++)]) {
        if (*ch == 0) return p->value;
      }
      return -1;
    }

    trie_node(char* ch, int& curr_val) {
      if (ch == NULL) return;

      for (int i = 0; i < NUM_CHILDS; ++i) children[i] = NULL;

      if (*ch == 0) {
        value = ++curr_val;
        return;
      }

      value = -1;
      children[*ch] = new trie_node(ch + 1, curr_val);
    }
  };

  int curr_val_;
  trie_node* children[NUM_CHILDS];

 public:
  void add(char* ch) {
    if (children[*ch] == NULL)
      children[*ch] = new trie_node(ch + 1, curr_val_);
    else
      children[*ch]->add(ch + 1, curr_val_);
  }
  int find(char* ch) {
    if (children[*ch] == NULL)
      return -1;
    else
      return children[*ch]->find(ch + 1);
  }
  SimpleTrie() : curr_val_(0) {}
};

/**
 * An improvement of previous SimpleTrie in terms of memory. Performance is similar. The cost is more complicated logic
 */

class CompactTrie {
  class trie_node {
    trie_node* children[NUM_CHILDS];
    char* substr;
    uint8_t len;
    int value;
    bool isLast;

    void split(int split_len) {
      if (split_len == 0 || split_len >= len) return;
      trie_node* newNode = new trie_node();
      newNode->substr = substr + split_len;
      newNode->len = len - split_len;
      newNode->isLast = isLast;
      newNode->value = value;
      for (int i = 0; i < NUM_CHILDS; ++i) newNode->children[i] = children[i];

      len = split_len;
      children[*(substr + split_len)] = newNode;
    }

   public:
    void setVal(const int val) { value = val; }
    int find(char* ch) {
      for (trie_node* p = this; p != NULL;) {
        for (int i = 0; i < p->len; ++i)
          if (p->substr[i] != ch[i]) return -1;

        if (ch[p->len] == 0) {
          if (p->isLast) return p->value;
          return -1;
        }
        ch += p->len;
        p = p->children[*ch];
      }
      return -1;
    }

    void add(char* c, int& curr_val) {
      for (int i = 0; i < len; ++i) {
        if (c[i] != substr[i]) {
          split(i);

          if (c[i] == 0) {
            isLast = true;
            value = ++curr_val;
            return;
          } else {
            children[c[i]] = new trie_node(c + i, curr_val);
            return;
          }
        }
      }

      if (*(c + len) == 0) {
        isLast = true;
        if (value == -1) value = ++curr_val;
        return;
      }

      if (children[*(c + len)] == NULL)
        children[*(c + len)] = new trie_node(c + len, curr_val);
      else
        children[*(c + len)]->add(c + len, curr_val);
    }

    trie_node() {
      substr = NULL;
      len = 0;
      isLast = true;
      value = -1;
    }

    trie_node(char* ch, int& curr_val) {
      if (ch == NULL) return;
      substr = ch;
      len = 0;
      while (*(ch + len++))
        ;
      --len;

      value = ++curr_val;
      isLast = true;

      for (int i = 0; i < NUM_CHILDS; ++i) children[i] = NULL;
    }
  };

  trie_node* children[NUM_CHILDS];
  int curr_val_;

 public:
  void add(char* ch) {
    if (ch == NULL) return;
    if (*ch == 0) {
      if (children[0] == NULL) {
        children[0] = new trie_node();
        children[0]->setVal(++curr_val_);
      }
      return;
    }
    if (children[*ch] == NULL) {
      children[*ch] = new trie_node(ch, curr_val_);
    } else {
      children[*ch]->add(ch, curr_val_);
    }
  }

  int find(char* ch) {
    if (children[*ch] == NULL)
      return -1;
    else
      return children[*ch]->find(ch);
  }
  CompactTrie() : curr_val_(0) {}
};
}

#endif /* TRIE_HPP_ */
