import os
import sys
import getpass
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade", "hftpy"))
class BaseOrder:
    def __init__(self):
        self.security_name_ = ''
        self.buysell_ = 0
        self.price_ = 0
        self.size_remaining_ = 0
        self.size_executed_ = 0
        self.size_requested_ = 0
        self.int_price_ = 0
        self.order_status_ = 'None'

        self.queue_size_ahead_ = 0
        self.queue_size_behind_ = 0
        self.num_events_seen_ = 0
        self.client_assigned_order_sequence_ = 0
        self.server_assigned_order_sequence_ = 0
        self.server_assigned_client_id_ = 0

        self.canceled_ = False

    def dump(self):
        print('(', self.buysell_, str(self.int_price_), str(self.size_requested_), str(self.size_remaining_),
              str(self.size_executed_), str(self.queue_size_ahead_), str(self.num_events_seen_), ')')

    def security_name(self):
        return self.security_name_

    def buysell(self):
        return self.buysell_

    def price(self):
        return self.price_

    def size_remaining(self):
        return self.size_remaining_

    def size_executed(self):
        return self.size_executed_

    def size_requested(self):
        return self.size_requested_

    def int_price(self):
        return self.int_price_

    def order_status(self):
        return self.order_status_

    def canceled(self):
        return self.canceled_

    def client_assigned_order_sequence(self):
        return self.client_assigned_order_sequence_

    def server_assigned_order_sequence(self):
        return self.server_assigned_order_sequence_

    def server_assigned_client_id(self):
        return self.server_assigned_client_id_

    def execute_remaining(self):
        t_size_remaining_ = self.size_remaining_
        self.size_remaining_ = 0
        self.size_executed_ += t_size_remaining_
        return t_size_remaining_

    def match_partial(self, _further_match_):
        t_size_possible_ = min(_further_match_, self.size_remaining_)
        self.size_executed_ += t_size_possible_
        self.size_remaining_ -= t_size_possible_
        return t_size_possible_

    def handle_crossing_trade(self, _trade_size_, _posttrade_size_at_price_):
        if (self.num_events_seen_ < 1):
            return 0
        if (_trade_size_ > self.queue_size_ahead_):
            _further_match_ = _trade_size_ - self.queue_size_ahead_
            t_size_executed_ = self.match_partial(_further_match_)
            return t_size_executed_
        else:
            self.queue_size_ahead_ -= _trade_size_
            self.enqueue(
                _posttrade_size_at_price_)  # since we have an estimate of the total_market_non_self_size at this level after this trade, we use it to adjust queue_size_ahead_ and queue_size_behind_
            return 0

    def confirm(self):
        self.order_status_ = 'Conf'
        self.num_events_seen_ = 0

    def confirm_new_size(self, _new_size_):
        self.size_executed_ = self.size_requested_ - _new_size_
        self.size_remaining_ = _new_size_

    def can_be_canceled(self):
        return not self.canceled_ and self.size_remaining_ > 0

    def is_confirm(self):
        return self.order_status_ == 'Conf'

    def enqueue(self, _size_):
        if (self.num_events_seen_ == 0):
            self.queue_size_behind_ = 0
            self.queue_size_ahead_ = _size_
            self.num_events_seen_ += 1
        else:
            if self.queue_size_ahead_ > _size_:
                self.queue_size_ahead_ = _size_
            self.queue_size_behind_ = _size_ - self.queue_size_ahead_
            self.num_events_seen_ += 1
