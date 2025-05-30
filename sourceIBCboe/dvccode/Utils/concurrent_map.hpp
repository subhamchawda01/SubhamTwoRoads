#pragma once

#include <iostream>
#include <inttypes.h>
#include "dvccode/Utils/lock.hpp"
template <class K, class V, V default_>
class BSTNode {
 private:
  K key;
  V value;
  BSTNode* left;
  BSTNode* right;

 public:
  HFSAT::Lock mutex;

  BSTNode(K key, V value) {
    this->key = key;
    this->value = value;
    left = NULL;
    right = NULL;
    // is_leaf_=true;
  }

  bool add(K key, V value) {
    if (key == this->key)

      return false;

    else if (key < this->key) {
      if (left == NULL) {
        (this->mutex).LockMutex();
        if (left != NULL) return left->add(key, value);
        left = new BSTNode<K, V, default_>(key, value);
        (this->mutex).UnlockMutex();

        return true;

      }

      else

        return left->add(key, value);

    } else if (key > this->key) {
      if (right == NULL) {
        (this->mutex).LockMutex();
        if (right != NULL) return right->add(key, value);
        right = new BSTNode<K, V, default_>(key, value);
        (this->mutex).UnlockMutex();

        return true;

      }

      else
        return right->add(key, value);
    }

    return false;
  }

  V lookup(K key) {
    if (key == this->key) {
      return this->value;
    } else if (key < this->key) {
      if (left == NULL) {
        return default_;

      }

      else

        return left->lookup(key);

    } else if (key > this->key) {
      if (right == NULL)

        return default_;

      else
        return right->lookup(key);
    }

    return default_;
  }
};

template <class K, class V, V default_>
class thread_map {
 private:
  BSTNode<K, V, default_>* root;

 public:
  thread_map() { root = NULL; }

  bool add(K key, V value) {
    if (root == NULL) {
      (root->mutex).LockMutex();
      if (root != NULL) return root->add(key, value);
      root = new BSTNode<K, V, default_>(key, value);
      (root->mutex).UnlockMutex();

      return true;

    } else {
      return root->add(key, value);
    }
  }
  V lookup(K key) {
    if (root == NULL)
      return default_;

    else
      return root->lookup(key);
  }

  // bool find
};

/*
bool thread_map::remove(K key) {

      if (root == NULL)

            return false;

      else {

            if (root->getValue() == key) {

                  BSTNode auxRoot(0);

                  auxRoot.setLeftChild(root);

                  BSTNode* removedNode = root->remove(key, &auxRoot);

                  root = auxRoot.getLeft();

                  if (removedNode != NULL) {

                        delete removedNode;

                        return true;

                  } else

                        return false;

            } else {

                  BSTNode* removedNode = root->remove(key, NULL);

                  if (removedNode != NULL) {

                        delete removedNode;

                        return true;

                  } else

                        return false;

            }

      }

}



BSTNode* BSTNode::remove(K key, BSTNode *parent) {

      if (key < this->key) {

            if (left != NULL)

                  return left->remove(key, this);

            else

                  return NULL;

      } else if (key > this->key) {

            if (right != NULL)

                  return right->remove(key, this);

            else

                  return NULL;

      } else {

            if (left != NULL && right != NULL) {

                  this->key = right->minValue();

                  return right->remove(this->key, this);

            } else if (parent->left == this) {

                  parent->left = (left != NULL) ? left : right;

                  return this;

            } else if (parent->right == this) {

                  parent->right = (left != NULL) ? left : right;

                  return this;

            }

      }

}



int BSTNode::minValue() {

      if (left == NULL)

            return value;

      else

            return left->minValue();

}





*/
