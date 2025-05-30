/**
   \file MarketAdapterCode/sparse_index_book_hash_extended_data_structure.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2017
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/sparse_index_book_hash_extended_data_structure.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#define NSE_HASH_BOOK_SIZE 13
#define NSE_HASH_BLOCK_SIZE 5

#define NSE_HASH_USE_BITMASK 1

namespace HFSAT {
SparseIndexBookHashExtendedDataStructure::SparseIndexBookHashExtendedDataStructure(
    DebugLogger& dbglogger, const Watch& watch, int num_sec, SimpleMempool<MarketUpdateInfoLevelStruct>& p_mempool,
    TradeType_t side)
    : BaseSparseIndexDataStructure(dbglogger, watch, num_sec, p_mempool),
      level_node_mempool_(),
      sid_to_price_hashed_map_(num_sec, std::vector<LevelNode>(pow(2, NSE_HASH_BOOK_SIZE))),
      book_sz_((1 << NSE_HASH_BOOK_SIZE) - 1),
      side_(side),
      block_sz_(pow(2, NSE_HASH_BLOCK_SIZE)),
      sid_to_block_marker_(num_sec, std::vector<uint32_t>(pow(2, NSE_HASH_BOOK_SIZE - NSE_HASH_BLOCK_SIZE), 0)) {}

MarketUpdateInfoLevelStruct* SparseIndexBookHashExtendedDataStructure::AddOrderToPriceLevel(int security_id_,
                                                                                            int int_price_,
                                                                                            double price_,
                                                                                            unsigned int size) {
  MarketUpdateInfoLevelStruct* px_level_ = nullptr;

  // Get the hash index for the price
  int index = GetPriceHashIndex(int_price_);

  // Level node at the hash index for the sec_id
  LevelNode* head_node_level = &sid_to_price_hashed_map_[security_id_][index];

  // If px_level in null, then there's no level here, simply update px_level
  if (head_node_level->px_level == nullptr) {
    px_level_ = one_pxlevel_mempool_.Alloc();
    px_level_->limit_ordercount_ = 1;
    px_level_->limit_size_ = size;
    px_level_->limit_int_price_level_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
    px_level_->limit_int_price_ = int_price_;
    px_level_->limit_price_ = price_;
    px_level_->mod_time_ = watch_.tv();
    head_node_level->px_level = px_level_;
    num_levels_vec_[security_id_]++;
  }
  // Insert at price sorted position in the list
  else {
    LevelNode* prev = nullptr;
    LevelNode* curr = head_node_level;

    while (curr != nullptr) {
      // Found an existing price level with same price
      if (curr->px_level->limit_int_price_ == int_price_) {
        curr->px_level->limit_size_ += size;
        curr->px_level->limit_ordercount_ += 1;
        px_level_ = curr->px_level;
        px_level_->mod_time_ = watch_.tv();
        break;
      }
      // Found a place to insert ( ascending sort for bid / descending for ask )
      else if ((side_ == HFSAT::kTradeTypeBuy && curr->px_level->limit_int_price_ > int_price_) ||
               (side_ == HFSAT::kTradeTypeSell && curr->px_level->limit_int_price_ < int_price_)) {
        px_level_ = one_pxlevel_mempool_.Alloc();
        px_level_->limit_ordercount_ = 1;
        px_level_->limit_size_ = size;
        px_level_->limit_int_price_level_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
        px_level_->limit_int_price_ = int_price_;
        px_level_->limit_price_ = price_;
        px_level_->mod_time_ = watch_.tv();
        LevelNode* new_node = level_node_mempool_.Alloc();

        // If inserted in between two existing nodes
        if (prev != nullptr) {
          new_node->px_level = px_level_;
          prev->next_level = new_node;
          new_node->next_level = curr;
        }
        // If new node is to be inserted at the beginning
        else {
          new_node->px_level = head_node_level->px_level;
          new_node->next_level = head_node_level->next_level;
          head_node_level->px_level = px_level_;
          head_node_level->next_level = new_node;
        }
        num_levels_vec_[security_id_]++;
        break;
      }

      prev = curr;
      curr = curr->next_level;

      // Reached the end, will have to insert here ( at the end )
      if (curr == nullptr) {
        px_level_ = one_pxlevel_mempool_.Alloc();
        px_level_->limit_ordercount_ = 1;
        px_level_->limit_size_ = size;
        px_level_->limit_int_price_level_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
        px_level_->limit_int_price_ = int_price_;
        px_level_->limit_price_ = price_;
        px_level_->mod_time_ = watch_.tv();
        LevelNode* new_node = level_node_mempool_.Alloc();

        prev->next_level = new_node;
        new_node->next_level = nullptr;
        new_node->px_level = px_level_;
        num_levels_vec_[security_id_]++;
        break;
      }
    }
  }

#if NSE_HASH_USE_BITMASK
  // mark the bit as 1, as new level has been added
  sid_to_block_marker_[security_id_][index / block_sz_] |= 1 << (block_sz_ - (index % block_sz_) - 1);
#endif

  return px_level_;
}

MarketUpdateInfoLevelStruct* SparseIndexBookHashExtendedDataStructure::ChangeSizeAtPriceLevel(
    int security_id_, int int_price_, int size, bool decrement_ordercount) {
  // Get the hash index for the price
  int index = GetPriceHashIndex(int_price_);

  // Level node at the has index for the sec_id
  LevelNode* head_node_level = &sid_to_price_hashed_map_[security_id_][index];

  // If px_level in null, then there's no level here, return NULL and log error
  if (head_node_level->px_level == nullptr) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Invalid Usage: ChangeSizeAtPriceLevel called for non-existent level @ "
                                << int_price_ << " Symbol: " << HFSAT::SecurityNameIndexer::GetUniqueInstance().GetSecurityNameFromId(security_id_) 
				<<" SecID: " << security_id_ << DBGLOG_ENDL_FLUSH;
    return NULL;  // exception state
  }
  // This level has some existing nodes, try to find the price node we are interested in
  else {
    LevelNode* prev = nullptr;
    LevelNode* curr = head_node_level;

    MarketUpdateInfoLevelStruct* retval = NULL;

    // Loop over the node list
    while (curr != nullptr) {
      // Found an existing price level with same price, update the px_level node
      if (curr->px_level->limit_int_price_ == int_price_) {
        curr->px_level->limit_size_ += size;
        if (size > 0) {
          curr->px_level->mod_time_ = watch_.tv();
        }
        if (decrement_ordercount) {
          curr->px_level->limit_ordercount_ -= 1;
        }
        retval = curr->px_level;

        // If ordercount is zero, remove the node from the list
        if (curr->px_level->limit_ordercount_ == 0) {
          // debug statements - remove later : TODO
          if (curr->px_level->limit_size_ != 0) {
            DBGLOG_TIME_CLASS_FUNC_LINE << " Inconsistent state empty pxlevel has nonempty size "
                                        << curr->px_level->limit_size_ << DBGLOG_ENDL_FLUSH;
          }

          // delete a node if it is not the head node
          if (prev != nullptr) {
            prev->next_level = curr->next_level;
            curr->next_level = nullptr;
            curr->px_level = nullptr;
            level_node_mempool_.DeAlloc(curr);
          }
          // delete the head node
          else {
            // if head node is the only node present
            if (head_node_level->next_level == nullptr) {
              head_node_level->px_level = nullptr;
#if NSE_HASH_USE_BITMASK
              // mark the bit as zero as level has been entirely deleted
              sid_to_block_marker_[security_id_][index / block_sz_] &= ~(1 << (block_sz_ - (index % block_sz_) - 1));
#endif
            }
            // if there are other nodes apart from head node ( and head node is being deleted )
            else {
              curr = head_node_level->next_level;
              head_node_level->px_level = head_node_level->next_level->px_level;
              head_node_level->next_level = head_node_level->next_level->next_level;
              curr->next_level = nullptr;
              curr->px_level = nullptr;
              level_node_mempool_.DeAlloc(curr);
            }
          }
          num_levels_vec_[security_id_]--;
        }
        return retval;
      }

      prev = curr;
      curr = curr->next_level;

      // Reached the end, will have to insert here
      if (curr == nullptr) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Invalid Usage: ChangeSizeAtPriceLevel called for non-existent level @ "
                                    << int_price_ << " Symbol: " << HFSAT::SecurityNameIndexer::GetUniqueInstance().GetSecurityNameFromId(security_id_) 
                                    << " SecID: " << security_id_<< DBGLOG_ENDL_FLUSH;
        return NULL;  // exception state
      }
    }
  }

  return NULL;
}

/*
 * Start with ( int_price_ + 1 ),try to find if such a level exists else keep looping with higher price (int_price_++)
 * If we find such a level, simply return as that must be the next higher price level. Meanwhile, keep track of
 * min_price which is greater than the int_price_. If we have looped over all the hashed levels and haven't found the
 * price level which equals iter px, return the min_price we had been tracking.
 */
MarketUpdateInfoLevelStruct* SparseIndexBookHashExtendedDataStructure::GetHigherPriceLevel(int security_id_,
                                                                                           int int_price_) {
  // Get the hash index for the price
  int iter_px = int_price_ + 1;

  int iter_idx = GetPriceHashIndex(iter_px);
  int start_idx = iter_idx;

#if NSE_HASH_USE_BITMASK
  std::vector<uint32_t> block_marker_ = sid_to_block_marker_[security_id_];
  int start_block = iter_idx / block_sz_;
#endif

  MarketUpdateInfoLevelStruct* retval = nullptr;

  do {
    // Level node at the has index for the sec_id
    LevelNode* head_node_level = &sid_to_price_hashed_map_[security_id_][iter_idx];

    if (head_node_level->px_level != nullptr) {
      LevelNode* curr = head_node_level;

      while (curr != nullptr) {
        // Found an existing price level with same price
        if (curr->px_level->limit_int_price_ == iter_px) {
          retval = curr->px_level;
          return retval;
        } else if (curr->px_level->limit_int_price_ > int_price_) {
          if (retval == nullptr) {
            retval = curr->px_level;
          } else {
            if (curr->px_level->limit_int_price_ < retval->limit_int_price_) {
              retval = curr->px_level;
            }
          }
        }
        curr = curr->next_level;
      }
    }

#if NSE_HASH_USE_BITMASK
    int curr_block = iter_idx / block_sz_;

    if (curr_block != start_block && block_marker_[curr_block] == 0) {
      iter_px += block_sz_;
    } else {
      iter_px++;
    }
#else
    iter_px++;
#endif

    iter_idx = GetPriceHashIndex(iter_px);
  } while (iter_idx != start_idx);

  return retval;
}

/*
 * Start with (int_price_ - 1), try to find if such a level exists else keep looping with lower price (int_price_--)
 * If we find such a level, simply return as that must be the next lower price level. Meanwhile, keep track of
 * max_price which is lower than the int_price_. If we have looped over all the hashed levels and haven't found the
 * price level which equals iter px, return the max_price we had been tracking.
 */
MarketUpdateInfoLevelStruct* SparseIndexBookHashExtendedDataStructure::GetLowerPriceLevel(int security_id_,
                                                                                          int int_price_) {
  // Get the hash index for the price
  int iter_px = int_price_ - 1;

  int iter_idx = GetPriceHashIndex(iter_px);
  int start_idx = iter_idx;

#if NSE_HASH_USE_BITMASK
  std::vector<uint32_t> block_marker_ = sid_to_block_marker_[security_id_];
  int start_block = iter_idx / block_sz_;
#endif

  MarketUpdateInfoLevelStruct* retval = NULL;

  do {
    // Level node at the has index for the sec_id
    LevelNode* head_node_level = &sid_to_price_hashed_map_[security_id_][iter_idx];

    if (head_node_level->px_level != nullptr) {
      LevelNode* curr = head_node_level;

      while (curr != nullptr) {
        // Found an existing price level with same price
        if (curr->px_level->limit_int_price_ == iter_px) {
          retval = curr->px_level;
          return retval;
        } else if (curr->px_level->limit_int_price_ < int_price_) {
          if (retval == nullptr) {
            retval = curr->px_level;
          } else {
            if (curr->px_level->limit_int_price_ > retval->limit_int_price_) {
              retval = curr->px_level;
            }
          }
        }
        curr = curr->next_level;
      }
    }

#if NSE_HASH_USE_BITMASK
    int curr_block = iter_idx / block_sz_;

    if (curr_block != start_block && block_marker_[curr_block] == 0) {
      iter_px -= block_sz_;
    } else {
      iter_px--;
    }
#else
    iter_px--;
#endif

    iter_idx = GetPriceHashIndex(iter_px);
  } while (iter_idx != start_idx);

  return retval;
}

MarketUpdateInfoLevelStruct* SparseIndexBookHashExtendedDataStructure::GetPLAtIntPrice(int security_id_,
                                                                                       int int_price_) {
  // Get the hash index for the price
  int index = GetPriceHashIndex(int_price_);

  // Level node at the has index for the sec_id
  LevelNode* curr = &sid_to_price_hashed_map_[security_id_][index];

  // If px_level in null, then there's no level here, return null
  if (curr->px_level == nullptr) {
    return NULL;
  } else {
    while (curr != nullptr) {
      // Found an existing price level with same price, return px_level
      if (curr->px_level->limit_int_price_ == int_price_) {
        return curr->px_level;
      }
      curr = curr->next_level;
    }
  }
  return NULL;
}

int SparseIndexBookHashExtendedDataStructure::GetPriceHashIndex(int price) {
  int hash_index = price & book_sz_;
  return hash_index;
}
}
