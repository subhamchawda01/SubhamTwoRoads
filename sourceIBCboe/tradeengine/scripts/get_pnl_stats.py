import argparse
import sys
import numpy


def create_stats_from_content(content_, mode_, pnl_column_=8, pos_column_=6):

    num_trades_ = 0
    final_volume_ = 0
    final_pnl_ = 0
    seen_first_trade_ = 0
    min_pnl_ = 0
    max_pnl_ = 0
    max_drawdown_ = 0

    # trade not open right now
    trade_on_ = 0

    # total time that we have a non zero position
    time_position_ = 0

    this_trade_open_sfm_ = 0
    last_pos_sfm_ = 0
    sum_abs_pos_time_ = 0
    last_abs_pos_ = 0
    last_pos_ = 0
    last_closed_trade_pnl_ = 0

    total_positive_closed_trades_ = 0
    total_closed_trades_ = 0

    closed_trade_pnls_ = []
    times_to_close_trades_ = []
    normalized_time_to_close_trades_ = []
    normalization_weights_ = []

    for trades in content_:
        if len(trades) >= 15 and trades[0] != "PNLSAMPLES":
            num_trades_ += 1
            time_stamp_ = float(trades[0])
            t_size_ = float(trades[4])
            t_pos_ = float(trades[pos_column_])
            total_pnl_ = float(trades[pnl_column_])

            final_volume_ += t_size_
            final_pnl_ = total_pnl_

            if seen_first_trade_ == 0:
                max_pnl_ = total_pnl_
                min_pnl_ = total_pnl_
                seen_first_trade_ = 1
                max_drawdown_ = total_pnl_
            else:
                min_pnl_ = min(total_pnl_, min_pnl_)
                max_pnl_ = max(total_pnl_, max_pnl_)
                max_drawdown_ = max(max_drawdown_, (max_pnl_ - total_pnl_))

            if trade_on_ == 0:
                this_trade_open_sfm_ = time_stamp_
                trade_on_ = 1
                last_pos_ = t_pos_
                last_abs_pos_ = abs(t_pos_)
                last_pos_sfm_ = time_stamp_
            else:
                if t_pos_ == 0:
                    if last_abs_pos_ != 0:
                        trade_on_ = 0
                        time_position_ += (time_stamp_ - last_pos_sfm_)
                        sum_abs_pos_time_ += (time_stamp_ - last_pos_sfm_) * last_abs_pos_
                        last_abs_pos_ = 0
                        last_pos_ = 0
                        total_closed_trades_ += 1

                        closed_trade_pnls_.append(total_pnl_ - last_closed_trade_pnl_)
                        if (total_pnl_ - last_closed_trade_pnl_) > 0:
                            total_positive_closed_trades_ += 1

                        last_closed_trade_pnl_ = total_pnl_
                        last_pos_sfm_ = time_stamp_

                        times_to_close_trades_.append(time_stamp_ - this_trade_open_sfm_)

                        # Normalization support is not present
                        normalized_time_to_close_trades_.append(time_stamp_ - this_trade_open_sfm_)
                        normalization_weights_.append(1)

                else:
                    if last_pos_ * t_pos_ < 0:
                        time_position_ += (time_stamp_ - last_pos_sfm_)
                        sum_abs_pos_time_ += (time_stamp_ - last_pos_sfm_) * last_abs_pos_
                        last_pos_ = t_pos_
                        last_abs_pos_ = abs(t_pos_)
                        total_closed_trades_ += 1
                        if (total_pnl_ - last_closed_trade_pnl_) > 0:
                            total_positive_closed_trades_ += 1

                        closed_trade_pnls_.append(total_pnl_ - last_closed_trade_pnl_)
                        last_closed_trade_pnl_ = total_pnl_
                        last_pos_sfm_ = time_stamp_

                        times_to_close_trades_.append(time_stamp_ - this_trade_open_sfm_)

                        # Normalization support is not present
                        normalized_time_to_close_trades_.append(time_stamp_ - this_trade_open_sfm_)
                        normalization_weights_.append(1)

                        this_trade_open_sfm_ = time_stamp_

                    else:
                        time_position_ += (time_stamp_ - last_pos_sfm_)
                        sum_abs_pos_time_ += (time_stamp_ - last_pos_sfm_) * last_abs_pos_
                        last_pos_ = t_pos_
                        last_abs_pos_ = abs(t_pos_)
                        last_pos_sfm_ = time_stamp_

    average_abs_position_ = 1
    median_time_to_close_trades_ = 1000
    average_time_to_close_trades_ = 1000
    max_time_to_close_trades_ = 1000
    normalized_average_time_to_close_trades_ = 1000
    median_closed_trade_pnls_ = 0
    average_closed_trade_pnls_ = 0
    stdev_closed_trade_pnls_ = 1
    sharpe_closed_trade_pnls_ = 0
    fracpos_closed_trade_pnls_ = 0

    if time_position_ > 0:
        average_abs_position_ = sum_abs_pos_time_ / time_position_

    if len(times_to_close_trades_) > 0:
        median_time_to_close_trades_ = numpy.median(numpy.array(times_to_close_trades_))
        average_time_to_close_trades_ = sum(times_to_close_trades_) / len(times_to_close_trades_)
        max_time_to_close_trades_ = max(times_to_close_trades_)

    if len(normalized_time_to_close_trades_) > 0:
        try:
            normalized_average_time_to_close_trades_ = sum(normalized_time_to_close_trades_) / sum(
                normalization_weights_)
        except ValueError:
            normalized_average_time_to_close_trades_ = average_time_to_close_trades_

    if len(closed_trade_pnls_) > 0:
        median_closed_trade_pnls_ = numpy.median(numpy.array(closed_trade_pnls_))
        average_closed_trade_pnls_ = sum(closed_trade_pnls_) / len(closed_trade_pnls_)
        stdev_closed_trade_pnls_ = numpy.std(numpy.array(closed_trade_pnls_))
        if stdev_closed_trade_pnls_ > 0:
            sharpe_closed_trade_pnls_ = average_closed_trade_pnls_ / stdev_closed_trade_pnls_
        fracpos_closed_trade_pnls_ = total_positive_closed_trades_ / total_closed_trades_
    if mode_ == 1:
        s = ("%.1f %d %d %d %d %d %.2f %.2f %d %d %d %d %d %f %d %d %d" %
             (average_abs_position_, median_time_to_close_trades_, average_time_to_close_trades_,
              median_closed_trade_pnls_, average_closed_trade_pnls_, stdev_closed_trade_pnls_,
              sharpe_closed_trade_pnls_, fracpos_closed_trade_pnls_, min_pnl_, max_pnl_, max_drawdown_,
              max_time_to_close_trades_, normalized_average_time_to_close_trades_, last_abs_pos_,
              total_positive_closed_trades_, total_closed_trades_, num_trades_))
    else:
        s = ("Average Abs position: %.1f\n" % (average_abs_position_))
        s += ("Median trade-close-time ( secs ): %d\n" % (median_time_to_close_trades_))
        s += ("Average trade-close-time ( secs ): %d\n" % (average_time_to_close_trades_))
        s += ("Max trade-close-time ( secs ): %d\n" % (max_time_to_close_trades_))
        s += ("TradePNL stats: Median: %d Avg: %d Stdev: %d Sharpe: %.2f Fracpos: %.2f\n" % (
            median_closed_trade_pnls_, average_closed_trade_pnls_, stdev_closed_trade_pnls_, sharpe_closed_trade_pnls_,
            fracpos_closed_trade_pnls_))
        s += ("PNL-min-max-draw: %d %d %d\n" % (min_pnl_, max_pnl_, max_drawdown_))
        s += ("NumTradeLines : %d\n" % (num_trades_))
        s += ("FinalVolume: %d\n" % (final_volume_))
        s += ("FinalPNL: %d\n" % (final_pnl_))
        pnl_per_contract_ = 0
        if final_volume_ > 0:
            pnl_per_contract_ = final_pnl_ / final_volume_
        s += ("PNL_per_contract : %.2f\n" % (pnl_per_contract_))
        s += ("Normalized averagetrade-close-time ( secs ): %d\n" % (normalized_average_time_to_close_trades_))
        s += ("Last Abs position: %f\n" % (last_abs_pos_))
        s += ("Total Positive Closed Trades: %d\n" % (total_positive_closed_trades_))
        s += ("Total Closed Trades: %d" % (total_closed_trades_))

    return s

if __name__ == "__main__":

    mode = 1

    parser = argparse.ArgumentParser()
    parser.add_argument('trades_file', help='Trades files for multiple product')
    parser.add_argument('--mode', help='Mode of printing')

    args = parser.parse_args()

    if args.trades_file:
        trades_file = args.trades_file
    else:
        sys.exit('Please provide input strat folder')

    if args.mode:
        mode = int(args.mode)

    with open(trades_file) as f:
        content = f.readlines()
    # you may also want to remove whitespace characters like `\n` at the end of each line
    content = [x.strip().split() for x in content]

    print(create_stats_from_content(content,mode))
