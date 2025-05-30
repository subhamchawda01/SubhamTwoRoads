#!/usr/bin/env python

"""

ORS STRUCT CLASS

"""


ors_header_unique = ['saos', 'px', 'buysell', 'saci', 'sz_remaining', 'sz_executed', 'orderid', 'q_send', 'q_conf',
                     'q_cxl_seq', 'q_cxld', 'q_exec', 'q_modify']
ors_header_rep = ['send_time', 'status']
ors_header = ors_header_unique + ors_header_rep

class ORSStruct():
    """
    Information about struct
    """

    def __init__(self):
        self.saos_ = 0
        self.caos_ = 0
        self.price_ = 0
        self.int_price_ = 0
        self.buysell_ = 'O'
        self.send_time_vec_ = []
        self.data_time_vec_ = []
        self.status_vec_ = []
        self.pos_vec_ = []
        self.price_vec_ = []
        self.saci_ = 0
        self.size_remaining_ = 0
        self.size_executed_ = 0
        self.order_id_ = 0
        self.cancel_reject_reason_ = None
        self.reject_reason_ = None
        self.queue_at_send_ = -1
        self.queue_at_conf_ = -1
        self.queue_at_cxl_seq_ = -1
        self.queue_at_cxld_ = -1
        self.queue_at_exec_ = -1
        self.queue_at_modify_ = -1

    def to_string(self):
        print_string = '%5d %0.05f %1s %5d %4d %4d ' \
                       '%15d [ %6d %6d %6d %6d %6d %6d ] ' % (self.saos_, self.price_,
                                                              self.buysell_, self.saci_, self.size_remaining_,
                                                              self.size_executed_, self.order_id_, self.queue_at_send_,
                                                              self.queue_at_conf_, self.queue_at_cxl_seq_,
                                                              self.queue_at_cxld_, self.queue_at_exec_,
                                                              self.queue_at_modify_)
        for idx in  range(len(self.send_time_vec_)):
            # print_string += '%0.6f %0.6f %s ' % (self.send_time_vec_[idx], self.data_time_vec_[idx], self.status_vec_[idx])
            print_string += '%0.6f %s ' % ( self.send_time_vec_[idx], self.status_vec_[idx])
        return print_string