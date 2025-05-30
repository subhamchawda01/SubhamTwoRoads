/**
   \file BasicOrderRoutingServer/order_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_ORDERMANAGER_H
#define BASE_BASICORDERROUTINGSERVER_ORDERMANAGER_H

//#include <iostream>
#include <sstream>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/CommonDataStructures/simple_mempool.hpp"

#define USING_STC 0
#define USING_STC_enable 1

#define EUREX_DELETE_MASX 0x3FFFFFFF

#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "infracore/BasicOrderRoutingServer/order.hpp"

namespace HFSAT {
namespace ORS {

/// This is the repository of all orders sent to the exchange
/// Currently we have one instance of order managers.
/// This causes a contention for procedures that involve the OrderManager
/// We should later analyse the callers of locked functions on OrderManager
/// and see if the OrderMnager can be broken up into multiple instances say 10 ( 0 to 9 ) and depending on
/// SAOS mod 10, the appropriate OrderManager instance can handle this order
/// unfortunately maintaining global position per security willbecome difficult
class OrderManager {
 public:
  /// Makes sure that there is only one instance of OrderManager
  static OrderManager &GetUniqueInstance() {
    static OrderManager uniqueinstance_;
    return uniqueinstance_;
  }

  void SetExchangeSource(const HFSAT::ExchSource_t &_exch_source_) { this_exchange_source_ = _exch_source_; }

  /// This will make sure that all future invocations of GetNewOrder
  /// will return NULL - No new orders can be placed --
  inline void DisableNewOrders() {
    // The mutual exclusion for the is_new_orders_disabled_ variable
    // is also provided by the order_mempool_lock_
    order_mempool_lock_.LockMutex();
    is_new_orders_disabled_ = true;
    order_mempool_lock_.UnlockMutex();
  }

  /// Only place to get new orders
  inline Order *GetNewOrder() {
    Order *p_new_active_order_ = NULL;

    // need to make this thread safe so that the same memory location is not returned to multiple callers
    order_mempool_lock_.LockMutex();

    // Before allocating a new order, we checked if we are allowing the client threads to place new orders --
    if (!is_new_orders_disabled_) {
      p_new_active_order_ = order_mempool_.Alloc();
    }  // else the p_new_active_order_ is NULL --

    order_mempool_lock_.UnlockMutex();
    return p_new_active_order_;
  }

  inline void DeallocateActiveOrder(Order *p_active_order_) {
    // write access to order_mempool_
    //    order_mempool_lock_.LockMutex();
    order_mempool_.DeAlloc(p_active_order_);
    //    order_mempool_lock_.UnlockMutex();
  }

  inline bool AddToCxlMap(const int _server_assigned_order_sequence_) {
    saos_cxl_map_lock_.LockMutex();
    if (saos_cxl_map_.find(_server_assigned_order_sequence_) != saos_cxl_map_.end()) {
      saos_cxl_map_lock_.UnlockMutex();
      return false;
    } else {
      saos_cxl_map_[_server_assigned_order_sequence_] = true;
      saos_cxl_map_lock_.UnlockMutex();
      return true;
    }
  }

  inline void RemoveFromCxlMap(const int _server_assigned_order_sequence_) {
    saos_cxl_map_lock_.LockMutex();
    if (saos_cxl_map_.find(_server_assigned_order_sequence_) != saos_cxl_map_.end())
      saos_cxl_map_.erase(_server_assigned_order_sequence_);
    saos_cxl_map_lock_.UnlockMutex();
  }

  inline int GetExchangeFillForThisSaos(int saos_) {
    int actual_size_executed_ = 0;

    exch_exec_lock_.LockMutex();

    if (pfc_saos_map_.find(saos_) != pfc_saos_map_.end()) {
      actual_size_executed_ = pfc_saos_map_[saos_];

    } else {
      actual_size_executed_ = 0;  // No value hence assume nothing executed
    }

    exch_exec_lock_.UnlockMutex();

    return actual_size_executed_;
  }

  /// this sets the map from SAOS to Order *
  inline void AddToActiveMap(Order *p_new_active_order_) {
    // lock saos_to_order_ptr_map_lock_
    saos_to_order_ptr_map_lock_.LockMutex();
    // add to saos_to_order_ptr_map_ map
    saos_to_order_ptr_map_[p_new_active_order_->server_assigned_order_sequence_] = p_new_active_order_;
    saos_to_order_all_ptr_map_[p_new_active_order_->server_assigned_order_sequence_] = *p_new_active_order_;
    // unlock saos_to_order_ptr_map_lock_
    saos_to_order_ptr_map_lock_.UnlockMutex();
  }

  inline bool RemoveOrderFromMaps(Order *p_active_order_) {
    // read lock on saos_to_order_ptr_map_
    saos_to_order_ptr_map_lock_.LockMutex();
    // remove from saos_to_order_ptr_map_ map
    // unsigned int _num_erased_ =
    bool order_deleted = false;
    int num_deleted = saos_to_order_ptr_map_.erase(p_active_order_->server_assigned_order_sequence_);

    if (num_deleted > 0) {
      order_deleted = true;
    }

    // unlocking read lock on saos_to_order_ptr_map_
    saos_to_order_ptr_map_lock_.UnlockMutex();

    return order_deleted;
  }

  inline Order *GetOrderByOrderSequence(const int _server_assigned_order_sequence_) const {
    Order *retval = NULL;

    // read lock on saos_to_order_ptr_map_
    saos_to_order_ptr_map_lock_.LockMutex();
    std::tr1::unordered_map<int, Order *>::const_iterator _citer_ =
        saos_to_order_ptr_map_.find(_server_assigned_order_sequence_);
    if (_citer_ != saos_to_order_ptr_map_.end()) {
      retval = _citer_->second;
    }

    // unlocking read lock on saos_to_order_ptr_map_
    saos_to_order_ptr_map_lock_.UnlockMutex();
    return retval;
  }

  inline void SetMirrorMetaData(int mirror_saos, int base_saos, uint64_t eng_ptr, int size_rem, int size_exec) {
    saos_to_saos_mirrormap_[mirror_saos] = MirrorMetaData(base_saos, eng_ptr, size_rem, size_exec);
  }

  inline Order *GetMirrorOrderBySAOS(int mirror_saos) {
    Order *ord_ptr = NULL;
    // check for mirror order map, if found
    // find in active map, if found -> replicate order struct and delete entry form mirror order map
    // if not found in active map, check in inactive vector -> replicate order struct and delete entry form mirror order
    // map
    // saos_to_saos_mirrormap_lock_.LockMutex();
    std::tr1::unordered_map<int, MirrorMetaData>::iterator iter = saos_to_saos_mirrormap_.find(mirror_saos);
    if (iter != saos_to_saos_mirrormap_.end()) {
      Order *base_ord_ptr = GetOrderByOrderSequence(iter->second.base_saos_);
      // saos_to_saos_mirrormap_lock_.UnlockMutex();
      if (base_ord_ptr != NULL) {
        ord_ptr = order_mempool_.Alloc();
        memcpy(ord_ptr, base_ord_ptr, sizeof(Order));
        ord_ptr->server_assigned_order_sequence_ = mirror_saos;
        ord_ptr->engine_ptr_ = iter->second.eng_ptr_;
        ord_ptr->size_executed_ = iter->second.size_executed_;
        ord_ptr->size_remaining_ = iter->second.size_remaining_;
        //! push the mirror order to saos_to_order_ptr_map_
        AddToActiveMap(ord_ptr);
      } else {
        Order *base_ord_ptr = GetInactiveOrderByOrderSequence(iter->second.base_saos_);

        // saos_to_saos_mirrormap_lock_.UnlockMutex();
        if (base_ord_ptr != NULL) {
          ord_ptr = order_mempool_.Alloc();
          memcpy(ord_ptr, base_ord_ptr, sizeof(Order));
          ord_ptr->server_assigned_order_sequence_ = mirror_saos;
          ord_ptr->engine_ptr_ = iter->second.eng_ptr_;
          ord_ptr->size_executed_ = iter->second.size_executed_;
          ord_ptr->size_remaining_ = iter->second.size_remaining_;
          //! push the mirror order to saos_to_order_ptr_map_
          AddToActiveMap(ord_ptr);
        }
      }
      saos_to_saos_mirrormap_.erase(iter);
    }
    return ord_ptr;
  }

  /// Attempt to find this order among inactive orders by exch_ord_id
  inline Order *GetInactiveOrderByExchOrderId(const char *_exch_ord_id_) const {
    Order *retval = NULL;
    for (unsigned int i = 0; i < inactive_order_vec_.size(); i++) {
      if (memcmp(inactive_order_vec_[i]->exch_assigned_order_sequence_, _exch_ord_id_,
                 EXCH_ASSIGNED_ORDER_SEQUENCE_LEN) == 0) {
        retval = inactive_order_vec_[i];
      }
    }
    return retval;
  }

  /// Attempt to find this order among active orders by exch_ord_id
  inline Order *GetOrderByExchOrderId(const char *_exch_ord_id_) const {
    Order *retval = NULL;

    // read lock on saos_to_order_ptr_map_
    saos_to_order_ptr_map_lock_.LockMutex();

    std::tr1::unordered_map<int, Order *>::const_iterator _citer_ = saos_to_order_ptr_map_.begin();
    while (_citer_ != saos_to_order_ptr_map_.end()) {
      Order *p_this_order_ = _citer_->second;
      if (memcmp(p_this_order_->exch_assigned_order_sequence_, _exch_ord_id_, EXCH_ASSIGNED_ORDER_SEQUENCE_LEN) == 0) {
        retval = p_this_order_;
        break;
      }
      _citer_++;
    }

    // unlocking read lock on saos_to_order_ptr_map_
    saos_to_order_ptr_map_lock_.UnlockMutex();

    return retval;
  }

  /// Attempt to find this order among inactive orders by SAOS
  inline Order *GetInactiveOrderByOrderSequence(const int _server_assigned_order_sequence_) const {
    Order *retval = NULL;
    for (unsigned int i = 0; i < inactive_order_vec_.size(); i++) {
      if (inactive_order_vec_[i]->server_assigned_order_sequence_ == _server_assigned_order_sequence_) {
        retval = inactive_order_vec_[i];
      }
    }
    return retval;
  }

  /// First allocates an inactive order
  /// copies given order to it, and pushes it into inactive_order_vec_
  /// removes p_active_order_ from saos_to_order_ptr_map_ map
  /// DeallocateActiveOrder
  inline void DeactivateOrder(Order *p_active_order_) {
    order_mempool_lock_.LockMutex();

    bool order_found_to_delete = RemoveOrderFromMaps(p_active_order_);

    if (order_found_to_delete) {
      Order *p_new_inactive_order_ = GetNewInactiveOrderPtr();

      memcpy(p_new_inactive_order_, p_active_order_, sizeof(Order));

      // should this be locked ? .. this is after all writing to inactive_order_vec_.
      // Could this be being read in another thread ( read called GetInactiveOrderByOrderSequence or
      // GetInactiveOrderByExchOrderId ) ?
      // At present the only functions not called from AccountThread are from ClientThread which are :
      // GetNewOrder, GetOrderByOrderSequence,
      inactive_order_vec_.push_back(p_new_inactive_order_);
      DeallocateActiveOrder(p_active_order_);
    } else {
      std::cout << __func__ << "Order Not found to delete. Not calling DeallocateActiveOrder for SAOS: "
                << p_active_order_->server_assigned_order_sequence_
                << " Pointer: " << (unsigned long long)p_active_order_ << std::endl;
    }

    order_mempool_lock_.UnlockMutex();
  }

  std::string DumpOMState() {
    std::ostringstream temp_oss;
    std::tr1::unordered_map<int, Order *>::iterator mapiter;
    int t_counter_ = 0;

    // lock saos_to_order_ptr_map_lock_
    saos_to_order_ptr_map_lock_.LockMutex();

    for (mapiter = saos_to_order_ptr_map_.begin(); mapiter != saos_to_order_ptr_map_.end() && t_counter_ < 50;
         mapiter++, t_counter_++) {
      temp_oss << "SAOS:\t" << mapiter->first << "\t Size_Remaining\t" << mapiter->second->size_remaining_
               << "\tSize_Exec:\t" << mapiter->second->size_executed_ << "\tPrice:\t" << mapiter->second->price_
               << "\tSide:\t" << mapiter->second->buysell_ << "\tProduct:\t" << mapiter->second->symbol_
               << "\tStatus:\t" << (mapiter->second->is_confirmed_ ? "CONF" : "SEQ") << "\n";
    }

    // unlock saos_to_order_ptr_map_lock_
    saos_to_order_ptr_map_lock_.UnlockMutex();

    return temp_oss.str();
  }

  // Returns a vector containing all pending orders.
  std::vector<Order *> GetAllOrders() {
    std::vector<Order *> orders_vector;

    // lock saos_to_order_ptr_map_lock_
    saos_to_order_ptr_map_lock_.LockMutex();

    for (std::tr1::unordered_map<int, Order *>::iterator _iter_ = saos_to_order_ptr_map_.begin();
         _iter_ != saos_to_order_ptr_map_.end(); ++_iter_) {
      orders_vector.push_back(_iter_->second);
    }

    // unlock saos_to_order_ptr_map_lock_
    saos_to_order_ptr_map_lock_.UnlockMutex();

    return orders_vector;
  }

    // Returns a vector containing all pending orders.
  std::vector<Order *> GetFullDayOrders() {
    std::vector<Order *> orders_vector;
    // lock saos_to_order_ptr_map_lock_
    saos_to_order_ptr_map_lock_.LockMutex();
    
    for (std::tr1::unordered_map<int, Order >::iterator _iter_ = saos_to_order_all_ptr_map_.begin();
         _iter_ != saos_to_order_all_ptr_map_.end(); ++_iter_) {
      orders_vector.push_back(&_iter_->second);
    }
     
    // unlock saos_to_order_ptr_map_lock_
    saos_to_order_ptr_map_lock_.UnlockMutex();
    return orders_vector;
  }

  // This function is used to get SAOS of the order corresponding to the sequence number
  // sent to the exchange. This is done by iterating over all the order structs and checking for a match
  // since we are storing both seq_num and SAOS in the struct.
  std::pair<int, int> GetSeqnumSaosPairUsingExchangeSeqNumber(const unsigned int &sequence_number) {
    int saos_val = -1;  //-1 is the default return value
    unsigned int seq_num = 0;
    std::pair<unsigned int, int> ret_val = make_pair(seq_num, saos_val);
    saos_to_order_ptr_map_lock_.LockMutex();
    for (std::tr1::unordered_map<int, Order *>::iterator _iter_ = saos_to_order_ptr_map_.begin();
         _iter_ != saos_to_order_ptr_map_.end(); ++_iter_) {
      if ((EUREX_DELETE_MASX & _iter_->second->message_sequence_number_) == sequence_number) {
        saos_val = _iter_->second->server_assigned_order_sequence_;
        seq_num = _iter_->second->message_sequence_number_;
        ret_val = make_pair(seq_num, saos_val);
        saos_to_order_ptr_map_lock_.UnlockMutex();
        return ret_val;
        break;
      }
    }
    saos_to_order_ptr_map_lock_.UnlockMutex();
    return ret_val;  // returns -1 when there is no order with the given sequence_number
  }

 protected:
  OrderManager() : order_mempool_(), order_mempool_lock_(), is_new_orders_disabled_(false) {}

  inline Order *GetNewInactiveOrderPtr() {
    // need to make this thread safe so that the same memory location is not returned to multiple callers
    inactive_order_mempool_lock_.LockMutex();
    Order *p_new_inactive_order_ = inactive_order_mempool_.Alloc();
    inactive_order_mempool_lock_.UnlockMutex();
    return p_new_inactive_order_;
  }

 private:
  struct MirrorMetaData {
    int base_saos_;
    uint64_t eng_ptr_;
    int size_remaining_;
    int size_executed_;

    MirrorMetaData(int saos, uint64_t eng_ptr, int size_rem, int size_exec) {
      base_saos_ = saos;
      eng_ptr_ = eng_ptr;
      size_remaining_ = size_rem;
      size_executed_ = size_exec;
    }

    MirrorMetaData() {
      base_saos_ = -1;
      eng_ptr_ = 0;
      size_remaining_ = size_executed_ = -1;
    }
  };
  /// Using SimpleMempool instead of calling "new" everytime to reduce the number of system calls.
  /// Not sure how all this will hold up if needed to be extended to a different class like say EUREX ( ETS ).
  /// As in if the Order for EUREX and CME have different fields
  /// we'll probably need a SimpleMempool < EUREXORSOrder > and SimpleMempool < CMEORSOrder >,
  /// with EUREXORSOrder and CMEORSOrder child classes of Order.
  /// perhaps we could make this a template class on typename Order ?
  SimpleMempool<Order> order_mempool_;
  Lock order_mempool_lock_;  ///< Used in function GetNewOrder to maintain thread safe access to allocation of new Order
  /// objects

  /// Memory allocator for order objects.
  /// As the name says whenever we get an intimation from the exchange that an order object has been deactivated (
  /// canceled / executed but NOT REJECTED_ON_SEND )
  /// we push the order to the inactive_order_vec_, but in doing so we allocate memory from inactive_order_mempool_ and
  /// DeAllocate the Order * from order_mempool_
  SimpleMempool<Order> inactive_order_mempool_;
  Lock inactive_order_mempool_lock_;  ///< Used in function DeactivateOrder to maintain thread safe access to allocation
  /// of new Order objects
  /// Storage of all orders that were sent through this OrderManager but aren't active any more
  std::vector<Order *> inactive_order_vec_;

  std::tr1::unordered_map<int, Order *> saos_to_order_ptr_map_;
  std::tr1::unordered_map<int, Order > saos_to_order_all_ptr_map_;

  std::tr1::unordered_map<int, MirrorMetaData> saos_to_saos_mirrormap_;
  // mutable Lock saos_to_saos_mirrormap_lock_;

  mutable Lock saos_to_order_ptr_map_lock_;  ///< made mutable to be locked and unlocked in a function that otherwise
  /// does not change anything GetOrderByOrderSequence
  mutable Lock saos_cxl_map_lock_;
  mutable Lock exch_exec_lock_;
  // --
  // When we cleanup on a crash or exit, we need to try to cancel all orders.
  // At that time, we disable all client threads from placing any new orders.
  // This variable causes the GetNewOrder method to stop returning new orders to send.
  bool is_new_orders_disabled_;
  std::map<int, int> pfc_saos_map_;
  std::map<int, bool> saos_cxl_map_;
  HFSAT::ExchSource_t this_exchange_source_;
};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_ORDERMANAGER_H
