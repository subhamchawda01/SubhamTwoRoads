#pragma once
#include <iostream>
#include <cstring>
#include <algorithm>
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"

#define MAX_BUCKET_SIZE 256000
#define SEND_NOTIF_OFFSET 16
#define VECTOR_BUCKET_LOGGING 0

/*
 * This class is like a map where key in query will be in some small range of last entered key
 * and keys are continuous.
 * Insertion and query are in O(1) time.
 * */
template <class T>
class vector_bucket {
  T buckets[2][MAX_BUCKET_SIZE];
  unsigned int start_bucket[2];
  unsigned int bucket_size[2];
  unsigned int current_bucket = 0;
  T default_value_;
  std::string id_ = "";
  bool send_email_ = false;
  bool LOG = false;
  HFSAT::DebugLogger &dbglogger_;

 public:
  vector_bucket(HFSAT::DebugLogger &dbglogger) : dbglogger_(dbglogger) {
    start_bucket[0] = 0;
    start_bucket[1] = 0;
    bucket_size[0] = 0;
    bucket_size[1] = 0;
  }

  vector_bucket(int size, HFSAT::DebugLogger &dbglogger) : dbglogger_(dbglogger) {
    start_bucket[0] = 0;
    start_bucket[1] = 0;
    bucket_size[0] = 0;
    bucket_size[1] = 0;
  }

  void insert(T n, int position = -1) {
    if (position == -1) {
      buckets[current_bucket][bucket_size[current_bucket]] = n;
      bucket_size[current_bucket]++;
      if (bucket_size[current_bucket] == MAX_BUCKET_SIZE) {
        start_bucket[current_bucket ^ 1] = start_bucket[current_bucket] + MAX_BUCKET_SIZE;
        current_bucket = current_bucket ^ 1;
        // delete_data(current_bucket);
        reset_bucket(current_bucket);
      }
    } else {
      while ((int32_t)(MAX_BUCKET_SIZE + start_bucket[current_bucket]) < position) {
        start_bucket[current_bucket ^ 1] = start_bucket[current_bucket] + MAX_BUCKET_SIZE;
        current_bucket = current_bucket ^ 1;
        // delete_data(current_bucket);
        reset_bucket(current_bucket);
      }
      bucket_size[current_bucket] = position - start_bucket[current_bucket] - 1;
      buckets[current_bucket][bucket_size[current_bucket]] = n;
      bucket_size[current_bucket]++;
      if (bucket_size[current_bucket] == MAX_BUCKET_SIZE) {
        start_bucket[current_bucket ^ 1] = start_bucket[current_bucket] + MAX_BUCKET_SIZE;
        current_bucket = current_bucket ^ 1;
        // delete_data(current_bucket);
        reset_bucket(current_bucket);
      }
    }
  }

  // to avoid copying T
  void insertPointer(T *n, int position = -1) {
    if (position == -1) {
      buckets[current_bucket][bucket_size[current_bucket]] = *n;
      bucket_size[current_bucket]++;
      if (bucket_size[current_bucket] == MAX_BUCKET_SIZE) {
        start_bucket[current_bucket ^ 1] = start_bucket[current_bucket] + MAX_BUCKET_SIZE;
        current_bucket = current_bucket ^ 1;
        //   delete_data(current_bucket);
        reset_bucket(current_bucket);
      }
    } else {
      while ((int32_t)(MAX_BUCKET_SIZE + start_bucket[current_bucket]) < position) {
        start_bucket[current_bucket ^ 1] = start_bucket[current_bucket] + MAX_BUCKET_SIZE;
        current_bucket = current_bucket ^ 1;
        //     delete_data(current_bucket);
        reset_bucket(current_bucket);
      }
      bucket_size[current_bucket] = position - start_bucket[current_bucket] - 1;
      buckets[current_bucket][bucket_size[current_bucket]] = *n;
      bucket_size[current_bucket]++;
      if (bucket_size[current_bucket] == MAX_BUCKET_SIZE) {
        start_bucket[current_bucket ^ 1] = start_bucket[current_bucket] + MAX_BUCKET_SIZE;
        current_bucket = current_bucket ^ 1;
        //     delete_data(current_bucket);
        reset_bucket(current_bucket);
      }
    }
  }

  T query(unsigned int n) {
    n = n - 1;  // first sequence number is 1
    if (n >= start_bucket[current_bucket] + bucket_size[current_bucket] || n < start_bucket[current_bucket ^ 1]) {
#if VECTOR_BUCKET_LOGGING
      std::cout << "Query out of limits. Query: " << n - 1 << std::endl;
      std::cout << " seq range: " << start_bucket[current_bucket ^ 1] << " - "
                << start_bucket[current_bucket] + bucket_size[current_bucket];
#endif
      if (std::is_same<T, pair<int, char> >::value) {
        return std::make_pair(-1, '0');
      }

      sendEmailNotification(n);
      return default_value_;
    }

    if (n >= start_bucket[current_bucket]) {
      return buckets[current_bucket][n - start_bucket[current_bucket]];
    }

    else {
      return buckets[current_bucket ^ 1][n - start_bucket[current_bucket ^ 1]];
    }
  }

  // for editing some particular value in vector bucket
  T *queryForEdit(unsigned int n) {
    n = n - 1;  // first sequence number is 1
    if (n >= start_bucket[current_bucket] + bucket_size[current_bucket] || n < start_bucket[current_bucket ^ 1]) {
#if VECTOR_BUCKET_LOGGING
      std::cout << "Query out of limits. Query: " << n - 1 << std::endl;
      std::cout << " seq range: " << start_bucket[current_bucket ^ 1] << " - "
                << start_bucket[current_bucket] + bucket_size[current_bucket];
#endif
      sendEmailNotification(n);
      return &default_value_;
    }

    if (n >= start_bucket[current_bucket]) {
      return &(buckets[current_bucket][n - start_bucket[current_bucket]]);
    }

    else {
      return &(buckets[current_bucket ^ 1][n - start_bucket[current_bucket ^ 1]]);
    }
  }

  int size() { return (start_bucket[current_bucket] + bucket_size[current_bucket]); }

  void delete_data(int bucket_id) {
    memset(&buckets[bucket_id], 0, sizeof(buckets[bucket_id]));
    bucket_size[bucket_id] = 0;
  }

  void reset_bucket(int bucket_id) {
    if (LOG && start_bucket[bucket_id] >= MAX_BUCKET_SIZE) {
      dbglogger_ << "Switching buckets: ";
      dbglogger_ << "Deleting metadata of orders from saos: " << start_bucket[bucket_id] - MAX_BUCKET_SIZE + 1 << " to "
                 << start_bucket[bucket_id] << "\n";
      if (send_email_) {
        HFSAT::Email e;
        e.content_stream << "Switching buckets:  \n";
        e.content_stream << "Deleting metadata of orders from saos: " << start_bucket[bucket_id] - MAX_BUCKET_SIZE + 1
                         << " to " << start_bucket[bucket_id] << "\n";
        e.setSubject("Subject: deleting order data: " + id_);

        e.addRecepient("nseall@tworoads.co.in");
        e.addSender("nseall@tworoads.co.in");
        e.sendMail();
      }
    }
    bucket_size[bucket_id] = 0;
  }

  void set_default_value(T default_value, std::string id = "", bool log = false) {
    default_value_ = default_value;
    id_ = id;
    LOG = log;
  }

  /*In case where the first key pushed is not 1*/
  void set_start_seq(unsigned int start) {
    start_bucket[0] = start - 1;
    start_bucket[1] = start - 1;
    bucket_size[0] = 0;
    bucket_size[1] = 0;
  }

  void send_email_for_error() { send_email_ = true; }

  void sendEmailNotification(int n = 0) {
#if VECTOR_BUCKET_LOGGING
    dbglogger_ << "Requesting data for key out of range, Queried value: " << n
               << "Current range: " << start_bucket[current_bucket ^ 1] << "-"
               << start_bucket[current_bucket] + bucket_size[current_bucket] << "\n";
    dbglogger_.DumpCurrentBuffer();
#endif

    if (send_email_) {
      if (((n + SEND_NOTIF_OFFSET) < (int32_t)start_bucket[current_bucket ^ 1]) ||
          ((n - SEND_NOTIF_OFFSET) > (int32_t)(start_bucket[current_bucket] + bucket_size[current_bucket]))) {
        HFSAT::Email e;
        e.content_stream << "Requesting data for key out of range. \n";
        e.content_stream << "Queried value: " << n << "\n";
        e.content_stream << "Current range: " << start_bucket[current_bucket ^ 1] << "-"
                         << start_bucket[current_bucket] + bucket_size[current_bucket] << "\n";
        e.setSubject("Subject: Query out of range: " + id_);
        e.addRecepient("nseall@tworoads.co.in");
        e.addSender("nseall@tworoads.co.in");
        e.sendMail();
      }
    }
  }
};
