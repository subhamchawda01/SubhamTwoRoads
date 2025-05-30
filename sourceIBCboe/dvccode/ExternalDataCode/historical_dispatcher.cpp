/**
    \file ExternalDataCode/historical_dispatcher.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include <algorithm>
#include "dvccode/ExternalData/historical_dispatcher.hpp"

namespace HFSAT {

#define USING_HEAP 1  // using STL heap algorithm to get the earliest tmestamp source in historical

HistoricalDispatcher::~HistoricalDispatcher() { DeleteSources(); }

void HistoricalDispatcher::SeekHistFileSourcesTo(const ttime_t r_endtime_) {
  if (!first_event_enqueued_) {
    // In historical all sources already have all the events they will have that day
    for (ExternalDataListenerPtrVecIter _iter_ = external_data_listener_vec_.begin();
         _iter_ != external_data_listener_vec_.end();) {
      bool t_hasevents_ = true;
      (*_iter_)->SeekToFirstEventAfter(r_endtime_, t_hasevents_);
      if (!t_hasevents_) {
        prev_external_data_listener_vec_.push_back(*_iter_);
        _iter_ = external_data_listener_vec_.erase(_iter_);
      } else {
        _iter_++;
      }
    }

    first_event_enqueued_ = true;
  }
}

void HistoricalDispatcher::DeleteSources() {
  for (auto i = 0u; i < external_data_listener_vec_.size(); i++) {
    if (external_data_listener_vec_[i] != NULL) {
      delete external_data_listener_vec_[i];
    }
  }
  external_data_listener_vec_.clear();
  for (auto i = 0u; i < prev_external_data_listener_vec_.size(); i++) {
    if (prev_external_data_listener_vec_[i] != NULL) {
      delete prev_external_data_listener_vec_[i];
    }
  }
  prev_external_data_listener_vec_.clear();
}

void HistoricalDispatcher::AddExternalDataListener(ExternalDataListener* _new_listener_, bool smm_filesource) {
  // higher priority for updates for SMM filesource
  if (smm_filesource) {
    _new_listener_->SetHighPriority();
  }
  VectorUtils::UniqueVectorAdd(external_data_listener_vec_, _new_listener_);
}

void HistoricalDispatcher::RunHist(const ttime_t _end_time_ /*= DateTime::GetTimeUTC(20301231, 0000)*/) {
  // std::cout<<"Size of external_data_listener_vec_"<<external_data_listener_vec_.size()<<"\n";

  if (!first_event_enqueued_) {  // we need to get the sources to a stage that we
   
    // can do sorting or creating heaps on them
    // that requires the sources to process their respective
    // data files and queue the first event that is
    // after permissible time stamp. In case no such time stamp
    // has been specified, or SeekHistFileSourcesTo has not
    // been called the following code will be run

    // In historical all sources already have all the events they will have that day
    for (ExternalDataListenerPtrVecIter _iter_ = external_data_listener_vec_.begin();
         _iter_ != external_data_listener_vec_.end();) {
      bool _hasevents_ = true;
      (*_iter_)->ComputeEarliestDataTimestamp(_hasevents_);
      if (!_hasevents_)  // only in historical ... if there is no event remove this source
      {
        prev_external_data_listener_vec_.push_back(*_iter_);
        _iter_ = external_data_listener_vec_.erase(_iter_);
      } else {
        _iter_++;
      }
    }
    first_event_enqueued_ = true;
  }
  // std::cout<<"Size of external_data_listener_vec_"<<external_data_listener_vec_.size()<<"\n";
  if (external_data_listener_vec_.size() ==
      1) {  // if only one source is left, then no need to sort sources etc, just process all of it's events
    // Since ComputeEarliestDataTimestamp () has been called the time stamp will be valid
    external_data_listener_vec_.front()->ProcessEventsTill(_end_time_);
    prev_external_data_listener_vec_.push_back(external_data_listener_vec_.front());
    external_data_listener_vec_.pop_back();  // remove that source .. now none left
    return;
  }
  if (external_data_listener_vec_.empty()) {
    return;
  }

#ifdef USING_HEAP

  make_heap(external_data_listener_vec_.begin(), external_data_listener_vec_.end(), ExternalDataListenerPtrGreater);

  while (1)  // only way to get out of this loop is when all sources have been removed. This is the only while loop in
             // the program
  {
    // data source with earliest timestamp event
    ExternalDataListenerPtr _top_edl_ = external_data_listener_vec_.front();

    // remove from heap top .. to get a new top
    pop_heap(external_data_listener_vec_.begin(), external_data_listener_vec_.end(), ExternalDataListenerPtrGreater);
    // std::cout << "Before Heap next timestmp : " << _top_edl_->next_event_timestamp() << std::endl;

    // process all events in this source till the next event is older the one on the new top
    _top_edl_->ProcessEventsTill(external_data_listener_vec_.front()->next_event_timestamp());

    // the next event timestamp of erstwhile top
    const ttime_t next_event_timestamp_from_edl = _top_edl_->next_event_timestamp();
    // std::cout << "After Heap next timestmp : " << next_event_timestamp_from_edl << std::endl;
    // if 0 then no events .. and in historical this means this source can be removed, since it has finished reading the
    // file it was reading from
    bool _hasevents_ =
        (next_event_timestamp_from_edl.tv_sec != 0) && (next_event_timestamp_from_edl.tv_sec <= _end_time_.tv_sec);

    if (_hasevents_) {
      // reinsert into heap
      push_heap(external_data_listener_vec_.begin(), external_data_listener_vec_.end(), ExternalDataListenerPtrGreater);
    } else {
      prev_external_data_listener_vec_.push_back(external_data_listener_vec_.back());
      external_data_listener_vec_.pop_back();
      if (external_data_listener_vec_.size() == 1) {
        // Since ComputeEarliestDataTimestamp () has been called, or ProcessEventsTill has been called, the
        // next_event_timestamp will be valid
        external_data_listener_vec_.front()->ProcessEventsTill(_end_time_);
        prev_external_data_listener_vec_.push_back(external_data_listener_vec_.back());
        external_data_listener_vec_.pop_back();
        return;
      }
      if (external_data_listener_vec_.empty()) {
        return;
      }
    }
  }

#else

  // sorting the external data listener vec ... overkill since all we need to do is bring the smallest to top
  // that's why heap is more intuitive
  std::sort(external_data_listener_vec_.begin(), external_data_listener_vec_.end(), ExternalDataListenerPtrLess);

  // only way to get out of this loop is when all sources have been removed. This is the only while loop in the program
  while (1) {
    // assume external_data_listener_vec_ is sorted coming in
    ExternalDataListener* _top_edl_ = external_data_listener_vec_.front();
    // remove top listener since it is certain to be not the earliest timestamp source any more
    external_data_listener_vec_.erase(external_data_listener_vec_.begin());
    _top_edl_->ProcessEventsTill(external_data_listener_vec_.front()->next_event_timestamp());

    // ask the source to get the next_event_timestamp ( the first event that is older than the specified endtime )
    const ttime_t next_event_timestamp_from_edl = _top_edl_->next_event_timestamp();

    // if 0 then no events .. and in historical this means this source can be removed, since it has finished reading the
    // file it was reading from
    bool _hasevents_ =
        (next_event_timestamp_from_edl.tv_sec != 0) && (next_event_timestamp_from_edl.tv_sec <= _end_time_.tv_sec);

    if (_hasevents_) {
      // if this source still has events ... insert it in the place where this_source->next_event_timestamp <= the
      // next_iter_source->next_event_timestamp
      ExternalDataListenerPtrVecIter _iter_ = external_data_listener_vec_.begin();
      for (; (_iter_ != external_data_listener_vec_.end()) &&
             ((*_iter_)->next_event_timestamp() < next_event_timestamp_from_edl);
           _iter_++) {
      }
      external_data_listener_vec_.insert(_iter_, _top_edl_);
    } else {
      prev_external_data_listener_vec_.push_back(_top_edl_);
      // this source does not have any more events
      if (external_data_listener_vec_.size() == 1) {
        // Since ComputeEarliestDataTimestamp () has been called, or ProcessEventsTill has been called, the
        // next_event_timestamp will be valid
        external_data_listener_vec_.front()->ProcessEventsTill(_end_time_);
        prev_external_data_listener_vec_.push_back(external_data_listener_vec_.back());
        external_data_listener_vec_.pop_back();
        return;
      }
      if (external_data_listener_vec_.size() == 0) {
        return;
      }
    }
  }

#endif  // USING_HEAP
}
#undef USING_HEAP
}
