#!/usr/bin/python

import numpy as np
import pandas as pd
import os
import sys
import subprocess
import time
import datetime
import shutil
from sklearn.linear_model import LinearRegression
from sklearn.linear_model import Ridge
pd.set_option('display.float_format', lambda x: '%.6f' % x)


class StratAnalyzer(object):
    def __init__(self, strat_file_name, product_name, end_date, lookback_days, pred_duration, price_increase_tick, price_decrease_tick, print_correlation, out_sample_end_date, out_sample_lookback_days, new_model_path, sim_result, skip_dates_file):
        STRAT_TO_ANALYZE = subprocess.check_output(
            "~/basetrade/scripts/print_strat_from_base.sh " + strat_file_name, shell=True)
        STRAT_TO_ANALYZE = STRAT_TO_ANALYZE.rstrip()
        self.strat_file = STRAT_TO_ANALYZE
        self.product_name = product_name
        self.end_date = end_date
        self.lookback_days = lookback_days
        self.pred_duration = pred_duration

        # creating temp directory
        user_name = os.getlogin()
        millis = int(round(time.time() * 1000))
        self.TEMP_DIR = "/home/dvctrader/animesh/" + user_name + "/strat_analyzer." + str(millis)
        os.makedirs(self.TEMP_DIR)

        # getting the min price increment and the date file
        if skip_dates_file != 0:
            dates_list = subprocess.check_output("/home/dvctrader/basetrade/scripts/get_list_of_dates_for_shortcode.pl " +
                                                 self.product_name + " " + self.end_date + " " + self.lookback_days + " " + skip_dates_file, shell=True)
            dates_list = dates_list.rstrip().split()
            self.dates_list = dates_list
        else:
            dates_list = subprocess.check_output("/home/dvctrader/basetrade/scripts/get_list_of_dates_for_shortcode.pl " +
                                                 self.product_name + " " + self.end_date + " " + self.lookback_days, shell=True)
            dates_list = dates_list.rstrip().split()
            self.dates_list = dates_list
        min_price_increment = subprocess.check_output(
            "/home/dvctrader/basetrade_install/bin/get_min_price_increment " + " " + self.product_name + " " + self.end_date, shell=True)
        self.min_price_increment = float(min_price_increment)
        self.pred_duration = int(pred_duration)
        self.price_increase_tick = float(price_increase_tick) * self.min_price_increment
        self.price_decrease_tick = float(price_decrease_tick) * self.min_price_increment
        self.new_model_path = new_model_path
        self.sim_result = sim_result
        # generating the final df
        all_df = []

        for dt in self.dates_list:
            try:
                temp_df = self.daily_df(dt)
                all_df.append(temp_df)
            except Exception as e:
                print("Error for date", dt, str(e))
                continue
        final_df = pd.concat(all_df)
        self.final_df = final_df

        if print_correlation == "1":
            insample_correlation_delta_y = self.get_correlation(self.final_df, "dynamic_delta_y_ticks")

        if out_sample_end_date is not None and out_sample_lookback_days is not None:
            out_sample_df = []
            out_sample_dates_list = subprocess.check_output("/home/dvctrader/basetrade/scripts/get_list_of_dates_for_shortcode.pl " +
                                                            self.product_name + " " + out_sample_end_date + " " + out_sample_lookback_days, shell=True)
            out_sample_dates_list = out_sample_dates_list.rstrip().split()
            self.out_sample_dates = out_sample_dates_list
            for dt in out_sample_dates_list:
                try:
                    temp_df = self.daily_df(dt)
                    out_sample_df.append(temp_df)
                except Exception as e:
                    print("Error for date", dt, str(e))
                    continue
            out_sample_final_df = pd.concat(out_sample_df)
            out_sample_correlation_delta_y = self.get_correlation(out_sample_final_df, "dynamic_delta_y_ticks")

            print("----------------------Delta y correlation-------------------------")
            out_sample_correlation = []
            for index in insample_correlation_delta_y.index.values:
                out_sample_correlation.append(out_sample_correlation_delta_y[index])

            corr_data_delta_y = pd.DataFrame(np.column_stack(
                (insample_correlation_delta_y.as_matrix(), np.array(out_sample_correlation))))
            corr_data_delta_y.index = insample_correlation_delta_y.index
            corr_data_delta_y.columns = ["In Sample", "Out Sample"]
            print(corr_data_delta_y.head(corr_data_delta_y.shape[0]))
            self.positive_correlated_column = insample_correlation_delta_y.index[insample_correlation_delta_y.values > 0].tolist(
            )
            if "SUMVARS" in self.positive_correlated_column:
                self.positive_correlated_column.remove("SUMVARS")

            # learn a new model on the trade data
            if new_model_path is not None:
                self.learn_model(new_model_path)

    def daily_df(self, date):
        os.chdir(self.TEMP_DIR)
        # creating datagen df
        datagen_df = self.create_datagen_df(self.strat_file, date)
        # generate send and trade order files    trade order file name tmp_orderexec_file tmp_sendtrade_file
        os.system("/home/dvctrader/basetrade/scripts/generate_sendtrade_orderexec_file.sh " +
                  self.strat_file + " " + date + " " + self.TEMP_DIR + " > /dev/null 2>&1")
        trade_exec_df = self.create_trade_order_file(
            self.TEMP_DIR + "/tmp_sendtrade_file", self.TEMP_DIR + "/tmp_orderexec_file")
        final_df = self.combine_datagen_trade_(
            datagen_df, trade_exec_df, self.min_price_increment, self.pred_duration, self.price_increase_tick, self.price_decrease_tick)
        return final_df

    def create_datagen_df(self, strat_file_, date_):
        od = os.getcwd()
        os.chdir(self.TEMP_DIR)
        args_model_ = ["awk", r'{print $4}', strat_file_]
        args_param_ = ["awk", r'{print $5}', strat_file_]
        args_start_time_ = ["awk", r'{print $6}', strat_file_]
        args_end_time_ = ["awk", r'{print $7}', strat_file_]
        out_model_ = subprocess.check_output(args_model_)
        out_param_ = subprocess.check_output(args_param_)
        start_time_ = subprocess.check_output(args_start_time_)
        end_time_ = subprocess.check_output(args_end_time_)

        out_model_ = out_model_.rstrip()
        out_param_ = out_param_.rstrip()
        start_time_ = start_time_.rstrip()
        end_time_ = end_time_.rstrip()

        # copying the model in present working directory with the name of temp_model
        shutil.copyfile(out_model_, "temp_model")

        # reading ilist file
        column_names = self.get_datagen_column_name("temp_model")

        # reading the datagen file
        model_file_ = out_model_
        prog_id_ = "349012"
        out_file_ = self.TEMP_DIR + "/tmp_datagen_out"
        datagen_args_ = "1000 c1 0 0".split()
        datagen_cmd = ["/home/dvctrader/LiveExec/bin/datagen", model_file_,
                       date_, start_time_, end_time_, prog_id_, out_file_] + datagen_args_
        out = subprocess.check_output(datagen_cmd)
        datagen_data = pd.read_csv(out_file_, sep=" ", header=None)
        datagen_data.columns = column_names
        # converting datagen msec column to seconds
        datagen_data["msec"] = datagen_data["msec"] / 1000
        datagen_data = datagen_data.rename(columns={"msec": "Datagen sec"})

        # reading the model coefficient
        model_coeff_args = ["awk", r'{if($1=="INDICATOR") print $2}', "temp_model"]
        p = subprocess.Popen(model_coeff_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        model_coeff_ = np.array([float(elem) for elem in p.stdout.read().splitlines()])

        # generating indicator sumvars df
        indicator_values_df = datagen_data.ix[:, 4:] * model_coeff_
        indicator_column_names = datagen_data.columns[4:].values
        indicator_column_value = [elem + "-VALUE" for elem in indicator_column_names]
        indicator_values_df.columns = indicator_column_value

        # getting SUMVARS for all indicators
        indicator_values_df["SUMVARS"] = indicator_values_df.sum(axis=1)

        # column bind of datagen data and indicator sumvars
        datagen_data = pd.concat([datagen_data, indicator_values_df], axis=1)
        os.chdir(od)
        return datagen_data

    def get_datagen_column_name(self, model_file_):
        args_indicator_ = ["awk", r'{if ($1=="INDICATOR") print $0}', model_file_]
        a = subprocess.check_output(args_indicator_).split("\n")[:-1]
        a1 = [elem.split("#")[0] for elem in a]
        indicator_column_names = ["-".join(elem1.split(" ")[2:]) for elem1 in [elem.split("#")[0] for elem in a]]
        return ["msec", "event", "target price", "base price"] + indicator_column_names

    def create_trade_order_file(self, trade_file, order_exec_file):
        # reading send trade file
        send_trade_file = pd.read_csv(trade_file, sep=" ", names=["time_send", "id"])
        order_exec_file = pd.read_csv(order_exec_file, sep=" ", names=["time_execute", "id"])
        # converting epoch time to seconds from midnight
        send_trade_file["time_send"] = np.array(
            list(map(self.convert_epoch_to_seconds, send_trade_file["time_send"].values.tolist())))
        order_exec_file["time_execute"] = np.array(
            list(map(self.convert_epoch_to_seconds, order_exec_file["time_execute"].values.tolist())))

        # inner join on id column
        final_df = pd.merge(send_trade_file, order_exec_file, on="id")
        return final_df

    def create_reg_data(self, datagen_df_matrix, pred_duration):
        datagen_df_time_column = datagen_df_matrix[:, 0]
        delta_y = []
        pred_duration = float(pred_duration)
        for i in range(datagen_df_matrix.shape[0]):
            present_price = datagen_df_matrix[i, 2]
            time_1 = int(pred_duration / 3)
            time_2 = int(2 * pred_duration / 3)
            time_3 = pred_duration
            all_price = []
            for intermediate_time in [time_1, time_2, time_3]:
                intermediate_price_index = self.find_nearest_idx(
                    datagen_df_time_column, datagen_df_time_column[i] + intermediate_time)
                intermediate_price_change = datagen_df_matrix[intermediate_price_index, 2] - present_price
                all_price.append(intermediate_price_change)
            temp_delta_y = sum([price * scale for price, scale in zip(all_price,
                                                                      [3**0.5, 1.5**0.5, 1])]) / (sum([3**0.5, 1.5**0.5, 1]))
            delta_y.append(temp_delta_y)
        return np.array(delta_y)

    def find_nearest_idx(self, array, value):
        temp_array = array[(array - value) < 0]
        idx = temp_array.argmax() if temp_array.shape[0] > 0 else -1
        return idx

    def convert_epoch_to_seconds(self, epoch_time):
        time_hh_mm_ss = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(epoch_time))[-8:]
        x = time.strptime(time_hh_mm_ss.split(',')[0], '%H:%M:%S')
        return datetime.timedelta(hours=x.tm_hour, minutes=x.tm_min, seconds=x.tm_sec).total_seconds()

    def combine_datagen_trade_(self, datagen_df, trade_df, min_price_increment, pred_duration, price_increase_tick, price_decrease_tick):
        datagen_df_matrix = datagen_df.as_matrix()
        y = self.create_reg_data(datagen_df_matrix, self.pred_duration)
        # adding the reg data column to datagen df
        datagen_df_matrix = np.column_stack((datagen_df_matrix, y))
        datagen_df_time_column = datagen_df_matrix[:, 0]
        trade_df_matrix = trade_df.as_matrix()
        selected_datagen_matrix = np.zeros((trade_df_matrix.shape[0], datagen_df_matrix.shape[1]))
        delta_y = []
        good_data = []
        dynamic_delta_y = []
        dynamic_pred_duration = []
        for i in range(trade_df.shape[0]):
            nearest_index = self.find_nearest_idx(datagen_df_time_column, trade_df_matrix[i, 0])
            if nearest_index > 0:
                selected_datagen_matrix[i, :] = datagen_df_matrix[nearest_index, :]
            else:
                continue
            # getting the points where the price has increase more than 1 tick before going down by half tick
            send_order_price = datagen_df_matrix[nearest_index, 3]  # base price
            send_order_time = datagen_df_matrix[nearest_index, 0]

            order_exec_price_index = self.find_nearest_idx(datagen_df_time_column, trade_df_matrix[i, 2])
            if order_exec_price_index > 0:
                order_exec_price = datagen_df_matrix[order_exec_price_index, 2]
                future_price_array = datagen_df_matrix[order_exec_price_index:, 2]
            else:
                continue
            # checking for price increase

            price_one_tick_increase_index = np.argmax((future_price_array - order_exec_price) > price_increase_tick)
            price_one_tick_decrease_index = np.argmax((order_exec_price - future_price_array) > price_increase_tick)

            # checking for intermediate price

            price_half_tick_decrease_index = np.argmax(order_exec_price - future_price_array > price_decrease_tick)
            price_half_tick_increase_index = np.argmax(future_price_array - order_exec_price > price_decrease_tick)

            if price_one_tick_increase_index < price_half_tick_decrease_index:
                good_data.append("Increase")
                dynamic_delta_y.append(future_price_array[price_one_tick_increase_index] - send_order_price)
                dynamic_pred_duration.append(
                    datagen_df_matrix[order_exec_price_index:, 0][price_one_tick_increase_index] - send_order_time)
            elif price_one_tick_decrease_index < price_half_tick_increase_index:
                good_data.append("Decrease")
                dynamic_delta_y.append(future_price_array[price_one_tick_decrease_index] - send_order_price)
                dynamic_pred_duration.append(
                    datagen_df_matrix[order_exec_price_index:, 0][price_one_tick_decrease_index] - send_order_time)
            elif price_one_tick_increase_index > price_half_tick_decrease_index:
                good_data.append("None")
                dynamic_delta_y.append(future_price_array[price_half_tick_decrease_index] - send_order_price)
                dynamic_pred_duration.append(
                    datagen_df_matrix[order_exec_price_index:, 0][price_half_tick_decrease_index] - send_order_time)
            elif price_one_tick_decrease_index > price_half_tick_increase_index:
                good_data.append("None")
                dynamic_delta_y.append(future_price_array[price_half_tick_increase_index] - send_order_price)
                dynamic_pred_duration.append(
                    datagen_df_matrix[order_exec_price_index:, 0][price_half_tick_increase_index] - send_order_time)
            else:
                good_data.append("None")
                dynamic_delta_y.append(0)
                dynamic_pred_duration.append("None")

        # removing rows that have 0's in the first column
        mask = (selected_datagen_matrix[:, 0] != 0)
        selected_datagen_matrix = selected_datagen_matrix[mask, :]
        selected_datagen_dataframe = pd.DataFrame(selected_datagen_matrix)
        datagen_df_columns = list(datagen_df.columns.values)
        datagen_df_columns.append("y")
        selected_datagen_dataframe.columns = datagen_df_columns
        selected_datagen_dataframe["good_data"] = np.array(good_data)
        selected_datagen_dataframe["dynamic_pred_duration"] = np.array(dynamic_pred_duration)
        selected_datagen_dataframe["dynamic_delta_y_ticks"] = np.array(dynamic_delta_y) / self.min_price_increment
        final = pd.concat([trade_df, selected_datagen_dataframe], axis=1)
        return final

    def get_final_df(self):
        return self.final_df

    def get_correlation(self, df, output_column="dynamic_delta_y_ticks"):
        args_model = ["awk", r'{print $4}', self.strat_file]
        out_model = subprocess.check_output(args_model)
        # getting model file path
        out_model = out_model.rstrip()
        f_handle = open(out_model)
        model_coefficient = []
        for line in f_handle.readlines():
            if line.split(" ")[0] == "INDICATOR":
                temp_data = line.rstrip().split()
                model_coefficient.append(float(temp_data[1]))
        model_coefficient.insert(0, 1)
        model_coefficient.insert(0, 1)
        wt_sign = np.sign(np.array(model_coefficient))
        df = df.loc[(df["good_data"] == "Increase") | (df["good_data"] == "Decrease")]
        col_names = ["-".join(col.split("-")[:-1]) for col in df.columns.values.tolist() if "VALUE" in col]
        col_names.insert(0, "SUMVARS")
        col_names.insert(0, output_column)
        data_frame = df[col_names]
        indicator_corr = data_frame.corr().ix[0, :]
        indicator_corr = indicator_corr * wt_sign
        indicator_corr.sort(ascending=False)

        return indicator_corr

    def learn_model(self, new_model_path):
        df = self.final_df
        strat_file = self.strat_file
        product_name = self.product_name
        args_model = ["awk", r'{print $4}', strat_file]
        out_model = subprocess.check_output(args_model)
        # getting model file path
        out_model_ = out_model.rstrip()
        old_model_path = self.TEMP_DIR + "/old_model"
        # copying the model in present working directory with the name of temp_model
        shutil.copyfile(out_model_, old_model_path)

        # filter on good data
        df = df.loc[(df["good_data"] == "Increase") | (df["good_data"] == "Decrease")]
        data_frame_model = df[self.positive_correlated_column]
        X = data_frame_model.ix[:, 1:].as_matrix()
        y = data_frame_model["dynamic_delta_y_ticks"].as_matrix()
        reg_coeff = 0.75 * X.shape[0]
        new_model = Ridge(alpha=reg_coeff, fit_intercept=False, normalize=True)
        new_model.fit(X, y)
        sumvars = new_model.predict(X)

        args_model_ = ["awk", r'{print $4}', strat_file]
        args_param_ = ["awk", r'{print $5}', strat_file]
        args_start_time_ = ["awk", r'{print $6}', strat_file]
        args_end_time_ = ["awk", r'{print $7}', strat_file]
        out_model_ = subprocess.check_output(args_model_)
        out_param_ = subprocess.check_output(args_param_)
        start_time_ = subprocess.check_output(args_start_time_)
        end_time_ = subprocess.check_output(args_end_time_)

        out_model_ = out_model_.rstrip()
        out_param_ = out_param_.rstrip()
        start_time_ = start_time_.rstrip()
        end_time_ = end_time_.rstrip()

        regdata_cmd = ["/home/dvctrader/basetrade/scripts/get_regdata.py", self.product_name, old_model_path, self.end_date,
                       self.lookback_days, start_time_, end_time_, "1000", "e1", "ts1", "0", "100", "na_e3", "fsg1", self.TEMP_DIR + "/"]
        out = subprocess.check_output(regdata_cmd)

        regdata_column_names = self.get_datagen_column_name(old_model_path)
        regdata_column_names = regdata_column_names[4:]
        regdata_column_names.insert(0, "y")
        reg_data = pd.read_csv(self.TEMP_DIR + "/" + "filtered_regdata_filename", sep=" ", names=regdata_column_names)
        # print "reg data columns:",reg_data.columns
        regdata_y_column = reg_data.as_matrix()[:, 0]
        regdata_X_column_old = reg_data.as_matrix()[:, 1:]
        regdata_X_column_new = reg_data[self.positive_correlated_column[1:]]

        # reading the orignal model
        f_handle = open(old_model_path)
        read_model = []
        orignal_model = []
        for line in f_handle.readlines():
            read_model.append(line.rstrip().split("#")[0])
            if line.split(" ")[0] == "INDICATOR":
                orignal_model.append(float(line.split(" ")[1]))
        orignal_model = np.array(orignal_model)
        orignal_model_sumvars = regdata_X_column_old * orignal_model
        new_model_sumvars = regdata_X_column_new * new_model.coef_
        stdev_scaling_factor = np.percentile(orignal_model_sumvars, 80) / np.percentile(new_model_sumvars, 80)
        model_wt_list = (new_model.coef_ * stdev_scaling_factor).tolist()
        indicator_string_list = list(self.positive_correlated_column[1:])
        i = 0
        for index, model_line in enumerate(read_model):
            temp_data = model_line.split(" ")
            # only edit the lines having the first elem as indicator
            if temp_data[0] == "INDICATOR":

                if i < len(indicator_string_list) and indicator_string_list[i] in regdata_column_names:
                    present_coef = model_wt_list[i]
                    temp_indicator_string = indicator_string_list[i]
                    present_indicator_string = " ".join(temp_indicator_string.split("-"))
                    read_model[index] = "INDICATOR " + str(present_coef) + " " + present_indicator_string
                    i = i + 1
                else:
                    read_model[index] = "INDICATOR " + str(0) + " " + " ".join(model_line.split(" ")[2:])

        # writing the model to a new file
        with open(self.new_model_path, mode="wb") as outfile:
            for line in read_model:
                outfile.write(line)
                outfile.write("\n")

        print("Old Model: ", old_model_path)
        print("New Model: ", self.new_model_path)

    def get_sim_result(self):
        dir_name = self.TEMP_DIR
        strat_name = self.strat_file
        # replacing the temp_strat file to include the new mode file
        temp_strat_name = dir_name + "/temp_strat"
        shutil.copyfile(strat_name, temp_strat_name)
        f_handle = open(temp_strat_name)
        read_strat = []
        for line in f_handle.readlines():
            strat_line = line.rstrip()
            strat_line_list = strat_line.split(" ")
            strat_line_list[3] = self.new_model_path
            string_to_write = " ".join(strat_line_list)
            read_strat.append(string_to_write)
        # writing the strat in a new strat file
        new_strat_name = dir_name + "/new_strat_file"
        with open(new_strat_name, mode="wb") as outfile:
            for line in read_strat:
                outfile.write(line)
                outfile.write("\n")

        # writing the strategies in the run_sim_file
        run_sim_strat_file = self.TEMP_DIR + "/run_sim"
        f1 = open(run_sim_strat_file, 'w+')
        f1.write(new_strat_name)
        f1.write("\n")
        f1.write(strat_name)
        f1.write("\n")

        run_sim_directory = self.TEMP_DIR + "/run_sim_results_dir/"
        run_sim_args = ['/home/dvctrader/basetrade/ModelScripts/run_simulations.pl',
                        self.product_name,
                        run_sim_strat_file,
                        self.out_sample_dates[-1],
                        self.out_sample_dates[0],
                        run_sim_directory,
                        '-d',
                        '0']
        return run_sim_args


if __name__ == "__main__":
    if len(sys.argv) < 11:
        print("USAGE : <strat_file>  <end_date> <lookback_days> <pred_duration> <upper_threshold> <lower_threshold_factor> <print_correlation> <out_sample_end_date> <out_sample_lookback_days> <new_model_file_path> <skip_dates_file=None>")
    else:
        strat_file = sys.argv[1]
        end_date = sys.argv[2]
        lookback_days = sys.argv[3]
        pred_duration = sys.argv[4]
        price_increase_tick = sys.argv[5]
        price_decrease_tick = sys.argv[6]
        print_correlation = sys.argv[7]
        out_sample_end_date = sys.argv[8]
        out_sample_lookback_days = sys.argv[9]
        new_model_path = sys.argv[10]
        strat_file_path_args = ["/home/dvctrader/basetrade/scripts/print_strat_from_base.sh", strat_file]
        strat_file_path = subprocess.check_output(strat_file_path_args)
        strat_file_path = strat_file_path.rstrip()
        args_shortcode_ = ["awk", r'{print $2}', strat_file_path]
        out_shortcode_ = subprocess.check_output(args_shortcode_)
        shortcode = out_shortcode_.rstrip()
        if len(sys.argv) == 11:
            skip_dates_file = 0
        elif len(sys.argv) == 12:
            skip_dates_file = sys.argv[11]
        # creating a strategy analyzer object
        strat_object = StratAnalyzer(strat_file, shortcode, end_date, lookback_days, pred_duration, price_increase_tick, price_decrease_tick,
                                     print_correlation, out_sample_end_date, out_sample_lookback_days, new_model_path, "1", skip_dates_file)

        cmd = strat_object.get_sim_result()
        cmd[5] = strat_object.TEMP_DIR + "/run_sim_dir"
        run_sim_command = " ".join(cmd)
        print("Run simulation command ", run_sim_command)
        print("Summarize Strategy Result: ", "~/basetrade_install/bin/summarize_local_results_dir_and_choose_by_algo kCNAPnlSharpe 4000 4000 -1 1 5000 100000 " +
              strat_object.TEMP_DIR + "/run_sim_dir")
