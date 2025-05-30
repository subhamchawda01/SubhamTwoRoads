#!/usr/bin/env python

import json
import os
import sys
import math
import numpy as np
from scipy.spatial import distance
import subprocess
import shlex
import warnings


class ObtainWeightsOnDays:

    def __init__(self, _current_date_=None, _dates_=None, _feature_file_='/spare/local/tradeinfo/day_features/dailyfeatures.txt', _model_type_='ARIMA_DEF', _distance_metric_='Mahalanobis', _events_file_='IF', _weights_=''):
        if _current_date_ is not None:
            self.current_date_ = _current_date_
        else:
            raise ValueError('no date has been specified')
        if _dates_ is not None:
            self.dates_ = json.loads(_dates_)
        else:
            raise ValueError('date json object is empty')

        self.feature_file_ = _feature_file_
        self.model_type_ = _model_type_
        self.last_seen_date_ = _current_date_
        self.distance_metric_ = _distance_metric_
        self.features_ = []
        self.current_feature_ = []
        self.date_to_feature_idx_ = {}
        self.forecast_script_ = os.environ["HOME"] + '/basetrade/WKoDii/forecast_dayfeatures.R'
        self.events_file_ = _events_file_
        self.weights_ = np.fromstring(_weights_, sep=",")

    def LoadFeatures(self):
        all_dates = []
        with open(self.feature_file_) as feature_fp:
            idx_ = 0
            for line in feature_fp:
                line = line.split()
                self.features_.append(list(map(float, line[1:])))
                self.date_to_feature_idx_[line[0]] = idx_
                self.last_seen_date_ = line[0]
                idx_ = idx_ + 1
        self.features_ = np.array(self.features_)

    def ObtainFeatureForDate(self):
        if self.last_seen_date_ >= self.current_date_:
            warnings.warn(
                'the last date seen in the database equals or exceeds the date for which the features have to be computed')

        day_diff_process_script = os.environ["HOME"] + "/basetrade/scripts/get_difference_between_dates.sh"
        day_diff_process_cmd = [day_diff_process_script, self.last_seen_date_, self.current_date_]
        out = subprocess.Popen(day_diff_process_cmd, stdout=subprocess.PIPE)
        day_diff_ = out.communicate()[0].decode('utf-8').strip()

        arima_param_file_ = "ARIMA_DEF"  # if self.model_type_ == 'ARIMA' else ''
        forecast_cmd = [self.forecast_script_, self.current_date_, day_diff_, self.model_type_,
        self.feature_file_, arima_param_file_, self.events_file_]
        process = subprocess.Popen(forecast_cmd, stdout=subprocess.PIPE)
        self.current_feature_ = list(map(float, process.communicate()[0].decode('utf-8').strip().split('\n')[-1].split()))

    def weightedEuc(self, A, B, w):
        # a man needs to normalise the data
        q = A - B
        return np.sqrt((w * q * q).sum())

    def ObtainWeights(self):
        # a man needs to normalise the data

        features_mean = [np.nanmean(np.ma.masked_invalid(self.features_[:, fidx_]))
                         for fidx_ in range(self.features_.shape[1])]
        features_stdev = [np.nanstd(np.ma.masked_invalid(self.features_[:, fidx_]))
                          for fidx_ in range(self.features_.shape[1])]

        for i in range(self.features_.shape[1]):
            self.features_[:, i] = (self.features_[:, i] - features_mean[i]) / \
                features_stdev[i] if features_stdev[i] != 0 else self.features_[:, i]
            self.current_feature_[i] = (self.current_feature_[i] - features_mean[i]) / \
                features_stdev[i] if features_stdev[i] != 0 else 0

        if self.distance_metric_ == 'Mahalanobis':
            cov_mat = np.linalg.inv(np.cov(np.transpose(self.features_)))
            for date in self.dates_:
                try:
                    idx_ = self.date_to_feature_idx_[date]
                    self.dates_[date] = 1 / (1 + distance.mahalanobis(self.current_feature_,
                                                                      self.features_[idx_], cov_mat))
                except:
                    warnings.warn("some dates specified in the input do not appear in the training file")
        if self.distance_metric_ == 'MahalanobisExp':
            cov_mat = np.linalg.inv(np.cov(np.transpose(self.features_)))
            for date in self.dates_:
                try:
                    idx_ = self.date_to_feature_idx_[date]
                    self.dates_[
                        date] = math.exp(-1 * 0.5 * distance.mahalanobis(self.current_feature_, self.features_[idx_], cov_mat))
                except:
                    warnings.warn("some dates specified in the input do not appear in the training file")
        if self.distance_metric_ == 'Chebyshev':
            for date in self.dates_:
                try:
                    idx_ = self.date_to_feature_idx_[date]
                    self.dates_[date] = 1 / (1 + distance.chebyshev(self.current_feature_, self.features_[idx_]))
                except:
                    warnings.warn("some dates specified in the input do not appear in the training file")
        if self.distance_metric_ == 'Euclidean':
            for date in self.dates_:
                try:
                    idx_ = self.date_to_feature_idx_[date]
                    self.dates_[date] = 1 / (1 + distance.euclidean(self.current_feature_, self.features_[idx_]))
                except:
                    warnings.warn("some dates specified in the input do not appear in the training file")
        if self.distance_metric_ == 'WeightedEuclidean':
            for date in self.dates_:
                try:
                    idx_ = self.date_to_feature_idx_[date]
                    self.dates_[date] = 1 / (1 + self.weightedEuc(self.current_feature_,
                                                                  self.features_[idx_], self.weights_))
                except:
                    warnings.warn("some dates specified in the input do not appear in the training file")
        if self.distance_metric_ == 'WeightedEuclideanExp':
            for date in self.dates_:
                try:
                    idx_ = self.date_to_feature_idx_[date]
                    self.dates_[date] = math.exp(-1 * 0.5 * self.weightedEuc(self.current_feature_,
                                                                             self.features_[idx_], self.weights_))
                except:
                    warnings.warn("some dates specified in the input do not appear in the training file")
        if self.distance_metric_ == 'Manhattan':
            for date in self.dates_:
                try:
                    idx_ = self.date_to_feature_idx_[date]
                    self.dates_[date] = 1 / (1 + distance.cityblock(self.current_feature_, self.features_[idx_]))
                except:
                    warnings.warn("some dates specified in the input do not appear in the training file")

        self.dates_ = json.dumps(self.dates_)


def main():

    dates_ = None
    current_date_ = None
    feature_file_ = '/spare/local/tradeinfo/day_features/dailyfeatures.txt'
    forecast_file_orig_ = feature_file_
    model_type_ = 'ARIMA'
    distance_metric_ = 'Mahalanobis'
    events_file_ = 'IF'
    weight_str_ = ''
    pretty_print_ = 1

    sys.tracebacklimit = 0
    if len(sys.argv) < 2:
        raise ValueError(
            "USAGE:<script> <current_date> [<dates>(-1 for all)] [pretty_print(1)] [<feature_file>] [USE_LAST_DAY/ARIMA] [<distance_metric>] [<events_file>] [<weights_str>].")

    current_date_ = sys.argv[1]
    dates_ = "-1"

    if len(sys.argv) >= 3:
        dates_ = sys.argv[2]
    if len(sys.argv) >= 4:
        pretty_print_ = int(sys.argv[3])
    if len(sys.argv) >= 5:
        feature_file_ = sys.argv[4]
    if len(sys.argv) >= 6:
        model_type_ = sys.argv[5]
    if len(sys.argv) >= 7:
        distance_metric_ = sys.argv[6]
    if len(sys.argv) >= 8:
        events_file_ = sys.argv[7]
    if len(sys.argv) >= 9:
        weight_str_ = sys.argv[8]

    if dates_ == "-1":
        dates_map_ = {}
        with open(feature_file_) as infile:
            for line in infile:
                dates_map_[line.split()[0]] = 0
        dates_ = json.dumps(dates_map_)

    weight_obj = ObtainWeightsOnDays(current_date_, dates_, feature_file_, model_type_,
                                     distance_metric_, events_file_, weight_str_)
    weight_obj.LoadFeatures()
    weight_obj.ObtainFeatureForDate()
    weight_obj.ObtainWeights()

    # just print the value?
    if pretty_print_ == 0:
        print((weight_obj.dates_))
    else:
        weight_obj.dates_ = json.loads(weight_obj.dates_)
        for day in weight_obj.dates_:
            print((day, weight_obj.dates_[day]))


if __name__ == '__main__':
    main()
