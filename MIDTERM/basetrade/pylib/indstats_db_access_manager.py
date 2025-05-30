#!/usr/bin/env python
import MySQLdb
import sys
import argparse

from walkforward.definitions import execs
from walkforward.utils.run_exec import exec_function
import numpy as np

# this script creates a class of IndStatsDBAcessManager to access the IndStats DB


class IndStatsDBAcessManager:
    cache_shc = {}
    cache_tp = {}
    cache_price = {}
    cache_filter = {}
    cache_predalgo = {}
    cache_indicator = {}

    def open_conn(self):
        self.conn = MySQLdb.connect(host="52.87.81.158", user='dvcwriter', passwd="f33du5rB", db="IndStats")
        self.cursor = self.conn.cursor()

    def insert_value(self, value, type):
        if type == "shortcode":
            table = "Shortcodes"
            column = "shortcode"
        elif type == "timeperiod":
            table = "Timeperiod"
            column = "timeperiod"
        elif type == "filter":
            table = "Filter"
            column = "filter"
        elif type == "predalgo":
            table = "Predalgo"
            column = "predalgo"
        elif type == "pricetype":
            table = "Pricetype"
            column = "pricetype"

        query = "INSERT IGNORE INTO " + table + " (" + column + ") VALUES (  '" + value + "');"
        self.cursor.execute(query)
        self.conn.commit()

    def get_shc_id(self, shc):
        if shc in list(self.cache_shc.keys()):
            return self.cache_shc[shc]
        else:
            query = "SELECT shc_id from Shortcodes where shortcode = '" + shc + "';"
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            if len(data) == 0:
                return -1
            shc_id = str(data[0][0])
            self.cache_shc[shc] = shc_id
            return shc_id

    def get_tp_id(self, tp):
        if tp in list(self.cache_tp.keys()):
            return self.cache_tp[tp]
        else:
            query = "SELECT tp_id from Timeperiod where timeperiod = '" + tp + "';"
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            if len(data) == 0:
                return -1
            tp_id = str(data[0][0])
            self.cache_tp[tp] = tp_id
            return tp_id

    def get_price_id(self, pricetype):
        if pricetype in list(self.cache_price.keys()):
            return self.cache_price[pricetype]
        else:
            query = "SELECT pricetype_id from Pricetype where pricetype = '" + pricetype + "';"
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            if len(data) == 0:
                return -1
            pricetype_id = str(data[0][0])
            self.cache_price[pricetype] = pricetype_id
            return pricetype_id

    def get_filter_id(self, filter_):
        if filter_ in list(self.cache_filter.keys()):
            return self.cache_filter[filter_]
        else:
            query = "SELECT filter_id from Filter where filter = '" + filter_ + "';"
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            if len(data) == 0:
                return -1
            filter_id = str(data[0][0])
            self.cache_filter[filter_] = filter_id
            return filter_id

    def get_predalgo_id(self, predalgo):
        if predalgo in list(self.cache_predalgo.keys()):
            return self.cache_predalgo[predalgo]
        else:
            query = "SELECT predalgo_id from Predalgo where predalgo = '" + predalgo + "';"
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            if len(data) == 0:
                return -1
            predalgo_id = str(data[0][0])
            self.cache_predalgo[predalgo] = predalgo_id
            return predalgo_id

    def get_indicator_id(self, indicator):
        if indicator in list(self.cache_indicator.keys()):
            return self.cache_indicator[indicator]
        else:
            query = "SELECT indicator_id from Indicator where indicator = '" + indicator + "';"
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            if len(data) == 0:
                return -1
            indicator_id = str(data[0][0])
            self.cache_indicator[indicator] = indicator_id
            return indicator_id

    def get_shc(self, shc_id):
        shc_id = str(shc_id)
        if shc_id in list(self.cache_shc.values()):
            for shc in list(self.cache_shc.keys()):
                if self.cache_shc[shc] == shc_id:
                    return shc
        else:
            query = "SELECT shortcode from Shortcodes where shc_id = " + shc_id + " ;"
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            shc = str(data[0][0])
            self.cache_shc[shc] = shc_id
            return shc

    def get_tp(self, tp_id):
        tp_id = str(tp_id)
        if tp_id in list(self.cache_tp.values()):
            for tp in list(self.cache_tp.keys()):
                if self.cache_tp[tp] == tp_id:
                    return tp
        else:
            query = "SELECT timeperiod from Timeperiod where tp_id = " + tp_id + " ;"
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            tp = str(data[0][0])
            self.cache_tp[tp] = tp_id
            return tp

    def get_predalgo(self, predalgo_id):
        predalgo_id = str(predalgo_id)
        if predalgo_id in list(self.cache_predalgo.values()):
            for predalgo in list(self.cache_predalgo.keys()):
                if self.cache_predalgo[predalgo] == predalgo_id:
                    return predalgo
        else:
            query = "SELECT predalgo from Predalgo where predalgo_id = " + predalgo_id
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            predalgo = str(data[0][0])
            self.cache_predalgo[predalgo] = predalgo_id
            return predalgo

    def get_filter(self, filter_id):
        filter_id = str(filter_id)
        if filter_id in list(self.cache_filter.values()):
            for filter_ in list(self.cache_filter.keys()):
                if self.cache_filter[filter_] == filter_id:
                    return filter_
        else:
            query = "SELECT filter from Filter where filter_id = " + filter_id
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            filter_ = str(data[0][0])
            self.cache_filter[filter_] = filter_id
        return filter_

    def get_price(self, price_id):
        price_id = str(price_id)
        if price_id in list(self.cache_price.values()):
            for pricetype in list(self.cache_price.keys()):
                if self.cache_price[pricetype] == price_id:
                    return pricetype
        else:
            query = "SELECT pricetype from Pricetype where pricetype_id = " + price_id
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            pricetype = str(data[0][0])
            self.cache_price[pricetype] = price_id
        return pricetype

    def get_indicator(self, ind_id):
        ind_id = str(ind_id)
        if ind_id in list(self.cache_indicator.values()):
            for ind in list(self.cache_indicator.keys()):
                if self.cache_indicator[ind] == ind_id:
                    return ind
        else:
            query = "SELECT indicator from Indicator where indicator_id = " + ind_id
            self.cursor.execute(query)
            data = self.cursor.fetchall()
            ind = str(data[0][0])
            self.cache_indicator[ind] = ind_id
        return ind

    # returns the map of product to prod_indc_id + "^" + indicator for shc
    def get_map_prod_prod_indc_n_indc_id_for_shc(self, shc):
        shc_id = self.get_shc_id(shc)
        query = "SELECT PI.shc_id, Dt.start_end_time, PI.pred_dur, PI.predalgo_id, PI.filter_id, PI.basepx_id, PI.futpx_id, PI.indicator_id, PI.prod_indc_id from ProdIndicators PI, Datagentimings Dt where shc_id = '" + \
            shc_id + "' and Dt.prod_indc_id = PI.prod_indc_id"
        self.cursor.execute(query)

        data = self.cursor.fetchall()
        product_prod_indc_n_indc_id = {}
        for row in data:
            start_end_time_ = row[1]
            pred_dur_ = str(row[2])
            predalgo_id_ = str(row[3])
            filter_id_ = str(row[4])
            basepx_id = str(row[5])
            futpx_id = str(row[6])
            indicator_id = str(row[7])
            prod_indc_id = str(row[8])

            predalgo = self.get_predalgo(predalgo_id_)
            filter_ = self.get_filter(filter_id_)
            basepx = self.get_price(basepx_id)
            futpx = self.get_price(futpx_id)
            indicator = self.get_indicator(indicator_id)

            product = shc + "^" + start_end_time_ + "^" + pred_dur_ + "^" + predalgo + "^" + filter_ + "^" + basepx + "^" + futpx
            prod_indc_id_n_indc_id = prod_indc_id + "^" + indicator
            if product not in list(product_prod_indc_n_indc_id.keys()):
                product_prod_indc_n_indc_id[product] = [prod_indc_id_n_indc_id]
            else:
                product_prod_indc_n_indc_id[product].append(prod_indc_id_n_indc_id)

        return product_prod_indc_n_indc_id

    # returns the map of product to prod_indc_id + "^" + indicator for shc and tp
    def get_map_prod_prod_indc_n_indc_id_for_shc_n_tp(self, shc, tp):
        shc_id = self.get_shc_id(shc)
        tp_id = self.get_tp_id(tp)
        query = "SELECT PI.shc_id, Dt.start_end_time, PI.pred_dur, PI.predalgo_id, PI.filter_id, PI.basepx_id, PI.futpx_id,\
                PI.indicator_id, PI.prod_indc_id from ProdIndicators PI, Datagentimings Dt where shc_id = '" + shc_id + \
                "' and tp_id = " + tp_id + " and Dt.prod_indc_id = PI.prod_indc_id"
        self.cursor.execute(query)

        data = self.cursor.fetchall()
        product_prod_indc_n_indc_id = {}
        for row in data:
            start_end_time_ = row[1]
            pred_dur_ = str(row[2])
            predalgo_id_ = str(row[3])
            filter_id_ = str(row[4])
            basepx_id = str(row[5])
            futpx_id = str(row[6])
            indicator_id = str(row[7])
            prod_indc_id = str(row[8])

            predalgo = self.get_predalgo(predalgo_id_)
            filter_ = self.get_filter(filter_id_)
            basepx = self.get_price(basepx_id)
            futpx = self.get_price(futpx_id)
            indicator = self.get_indicator(indicator_id)

            product = shc + "^" + start_end_time_ + "^" + pred_dur_ + "^" + predalgo + "^" + filter_ + "^" + basepx + "^" + futpx
            prod_indc_id_n_indc_id = prod_indc_id + "^" + indicator
            if product not in list(product_prod_indc_n_indc_id.keys()):
                product_prod_indc_n_indc_id[product] = [prod_indc_id_n_indc_id]
            else:
                product_prod_indc_n_indc_id[product].append(prod_indc_id_n_indc_id)

        return product_prod_indc_n_indc_id

    # returns the map of product to prod_indc_id + "^" + indicator for shc , tp, preddur, predalgo, filter_ , basepx and futpx
    def get_map_prod_prod_indc_n_indc_id(self, shc, tp, preddur, predalgo, filter_, basepx, futpx):
        shc_id = self.get_shc_id(shc)
        tp_id = self.get_tp_id(tp)
        predalgo_id = self.get_predalgo_id(predalgo)
        filter_id = self.get_filter_id(filter_)
        basepx_id = self.get_price_id(basepx)
        futpx_id = self.get_price_id(futpx)
        query = "SELECT PI.shc_id, Dt.start_end_time, PI.pred_dur, PI.predalgo_id, PI.filter_id, PI.basepx_id, PI.futpx_id,\
         PI.indicator_id, PI.prod_indc_id from ProdIndicators PI, Datagentimings Dt where shc_id = " + shc_id + \
                " and tp_id = " + tp_id + " and pred_dur = " + \
                preddur + " and predalgo_id = " + predalgo_id + " and filter_id = " + filter_id + " and basepx_id = " + \
                basepx_id + " and futpx_id = " + futpx_id + " and Dt.prod_indc_id = PI.prod_indc_id"
        self.cursor.execute(query)

        data = self.cursor.fetchall()
        product_prod_indc_n_indc_id = {}
        for row in data:
            start_end_time_ = row[1]
            pred_dur_ = str(row[2])
            predalgo_id_ = str(row[3])
            filter_id_ = str(row[4])
            basepx_id = str(row[5])
            futpx_id = str(row[6])
            indicator_id = str(row[7])
            prod_indc_id = str(row[8])

            predalgo = self.get_predalgo(predalgo_id_)
            filter_ = self.get_filter(filter_id_)
            basepx = self.get_price(basepx_id)
            futpx = self.get_price(futpx_id)
            indicator = self.get_indicator(indicator_id)

            product = shc + "^" + start_end_time_ + "^" + pred_dur_ + "^" + predalgo + "^" + filter_ + "^" + basepx + "^" + futpx
            prod_indc_id_n_indc_id = prod_indc_id + "^" + indicator
            if product not in list(product_prod_indc_n_indc_id.keys()):
                product_prod_indc_n_indc_id[product] = [prod_indc_id_n_indc_id]
            else:
                product_prod_indc_n_indc_id[product].append(prod_indc_id_n_indc_id)

        return product_prod_indc_n_indc_id

    # returns the map od date to prod_indc_ids to skip as per recompute flag
    def GetProdIndcidDtMapForShc(self, shc, recompute, start_date, end_date):
        shc_id = self.get_shcid(shc)
        prod_indc_id_to_skip_for_date_map = {}
        if recompute == 1:
            query = "SELECT prod_indc_id,date from ProdIndicators where shc_id = " + shc_id + \
                " and date > " + start_date + "and date < " + end_date + " and correlation is not null"
        elif recompute == 0:
            query = "SELECT prod_indc_id,date from ProdIndicators where shc_id = " + \
                shc_id + " and date > " + start_date + "and date < " + end_date
        else:
            print("Expected 0 or 1 as recompute for GetProdIndcidDtMapForShc \nExiting")
            sys.exit()
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        for row in data:
            prod_indc_id = row[0]
            date = row[1]
            if date not in list(prod_indc_id_to_skip_for_date_map.keys()):
                prod_indc_id_to_skip_for_date_map[date] = [prod_indc_id]
            else:
                prod_indc_id_to_skip_for_date_map[date].append(prod_indc_id)

        return prod_indc_id_to_skip_for_date_map

    # returns the prod_indc_ids to skip for shc and date as per the recompute flag
    def GetProdIndcidtoSkip(self, shc, recompute, date):
        shc_id = self.get_shc_id(shc)
        prod_indc_id_to_skip = []
        if recompute == "1":
            query = "SELECT prod_indc_id from IndicatorStats where date = " + date + \
                " and correlation IS NOT NULL and tail_correlation is NOT NULL and prod_indc_id in ( select prod_indc_id from ProdIndicators where shc_id = " + shc_id + ")"
        elif recompute == "0":
            query = "SELECT prod_indc_id from IndicatorStats where date = " + date + \
                " and prod_indc_id in ( select prod_indc_id from ProdIndicators where shc_id = " + shc_id + ")"
        else:
            print("Expected 0 or 1 as recompute for GetProdIndcidDtMapForShc \nExiting")
            sys.exit()
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        for row in data:
            prod_indc_id = str(row[0])
            prod_indc_id_to_skip.append(prod_indc_id)

        return prod_indc_id_to_skip

    # returns the indicators for a product
    def get_indicators_for_product(self, shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id, start_end_time):
        query = "SELECT indicator FROM Indicator where indicator_id in (select indicator_id from ProdIndicators PI, \
            Datagentimings Dt where  PI.shc_id = " + shc_id + " and PI.tp_id = " + tp_id + " and PI.pred_dur = " + \
            pred_dur + " and PI.predalgo_id = " + predalgo_id + " and PI.filter_id = " + filter_id + " and PI.basepx_id = " + \
            basepx_id + " and PI.futpx_id = " + futpx_id + " and PI.prod_indc_id = Dt.prod_indc_id and Dt.start_end_time = '" + \
            start_end_time + "');"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        indicators_list = []
        for row in data:
            indicator = row[0]
            indicators_list.append(indicator)
        return indicators_list

    # returns the indicators for a product
    def get_num_of_prodindicators_for_shc_tp(self, shc_id, tp_id):
        query = "SELECT  count(prod_indc_id) from ProdIndicators PI where  PI.shc_id = " + \
            shc_id + " and PI.tp_id = " + tp_id + ";"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        if len(data) == 0:
            return 0
        num_indicators = int(data[0][0])

        return num_indicators

    # to insert multiple stats to Indicator table
    def insert_indicators_multiple(self, indicators_list):
        query = "INSERT IGNORE INTO Indicator (indicator) values ('"
        query += "'),('".join(indicators_list) + "');"
        self.cursor.execute(query)
        self.conn.commit()

    # to insert multiple shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id, indicators_ids to ProdIndicators table
    def insert_prod_indicators_multiple(self, shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id, indicators_ids_to_insert):
        query = "INSERT IGNORE INTO ProdIndicators (shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id, indicator_id) \
                               VALUES"

        for i in range(len(indicators_ids_to_insert) - 1):
            query += "(" + shc_id + "," + tp_id + "," + pred_dur + "," + predalgo_id + "," + filter_id + "," + basepx_id + "," + futpx_id + "," \
                + indicators_ids_to_insert[i] + "),"

        query += "(" + shc_id + "," + tp_id + "," + pred_dur + "," + predalgo_id + "," + filter_id + "," + basepx_id + "," + futpx_id + "," \
            + indicators_ids_to_insert[-1] + ")"

        self.cursor.execute(query)

        self.conn.commit()

    # to delete multiple shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id, indicators_ids from ProdIndicators table
    def delete_prod_indicators_multiple(self, shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id, indicator_ids_to_delete):
        query = "DELETE FROM ProdIndicators where shc_id = " + shc_id + " and tp_id = " + tp_id + " and pred_dur = " + \
                pred_dur + " and predalgo_id = " + predalgo_id + " and filter_id = " + filter_id + " and basepx_id = " + \
                basepx_id + " and futpx_id = " + futpx_id + \
            " and indicator_id in (" + ",".join(indicator_ids_to_delete) + ");"

        self.cursor.execute(query)
        self.conn.commit()

    # get indicator ids of multiple indicators
    def get_indicator_id_multiple(self, indicators_list):
        query = "SELECT indicator_id from Indicator where indicator in ('" + "','".join(indicators_list) + "');"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        indicator_ids_list = []
        for row in data:
            indicator_id = str(row[0])
            indicator_ids_list.append(indicator_id)
        return indicator_ids_list

    # get indicator ids of indicators already present for shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id and futpx_id
    def get_indicator_ids_already_present(self, shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id):
        indicator_ids_already_present = []
        query = "SELECT indicator_id from Indicator where shc_id = " + shc_id + " and tp_id = " + tp_id + " and pred_dur = " + \
            pred_dur + " and predalgo_id = " + predalgo_id + " and filter_id = " + filter_id + " and basepx_id = " + \
            basepx_id + " and futpx_id = " + futpx_id + ";"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        for row in data:
            indc_id = row[0]
            indicator_ids_already_present.append(indc_id)
        return indicator_ids_already_present

    # get prod_indc_ids for product and indicator pair
    def get_prod_indc_ids_for_product_and_indicator(self, shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id,
                                                    indicator_ids_list):
        prod_indc_ids_list = []
        query = "SELECT prod_indc_id from ProdIndicators where shc_id = " + shc_id + " and tp_id = " + tp_id + " and pred_dur = " + \
                pred_dur + " and predalgo_id = " + predalgo_id + " and filter_id = " + filter_id + " and basepx_id = " + \
                basepx_id + " and futpx_id = " + futpx_id + \
            " and indicator_id in (" + ",".join(indicator_ids_list) + ");"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        for row in data:
            prod_indc_id = str(row[0])
            prod_indc_ids_list.append(prod_indc_id)

        return prod_indc_ids_list

    # Insert the timings for multiple prod_indc_ids to Datagentimings table
    def insert_timings_for_prod_indc_id_multiple(self, prod_indc_ids_list, start_end_time):
        for prod_indc_id in prod_indc_ids_list:
            query = "INSERT INTO Datagentimings (prod_indc_id, start_end_time) VALUES( " + prod_indc_id + ", '" + start_end_time + "')\
                           ON DUPLICATE KEY UPDATE prod_indc_id = VALUES(prod_indc_id), start_end_time = VALUES(start_end_time);"
            self.cursor.execute(query)
            self.conn.commit()

    # Insert into Indicator Stats corr and tail correlation for a prod_indc_id and date
    def InsertIndStats(self, prod_indc_id, date, corr, tail_corr):
        query = "INSERT INTO IndicatorStats VALUES(" + prod_indc_id + "," + date + "," + corr + "," + tail_corr + ") ON DUPLICATE KEY\
                 UPDATE correlation = VALUES(correlation), tail_correlation = VALUES(tail_correlation) ;"
        try:
            self.cursor.execute(query)
            self.conn.commit()
            return 1
        except MySQLdb.Error as e:
            return 0

    # get products for shc tp
    def get_products_for_shc_tp(self, shc_id, tp_id):
        products_for_shc_tp = []
        query = "select shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id from ProdIndicators where shc_id = " \
                + shc_id + " and tp_id = " + tp_id + " group by shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        for row in data:
            shc_id = str(row[0])
            tp_id = str(row[1])
            pred_dur = str(row[2])
            predalgo_id = str(row[3])
            filter_id = str(row[4])
            basepx_id = str(row[5])
            futpx_id = str(row[6])
            products_for_shc_tp.append([shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id])
        return products_for_shc_tp

    def get_product_for_shc_id(self, shc_id):
        products_for_shc = []
        query = "select shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id from ProdIndicators where shc_id = " \
                + shc_id + " group by shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        for row in data:
            shc_id = str(row[0])
            tp_id = str(row[1])
            pred_dur = str(row[2])
            predalgo_id = str(row[3])
            filter_id = str(row[4])
            basepx_id = str(row[5])
            futpx_id = str(row[6])
            products_for_shc.append([shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id])
        return products_for_shc

    def get_start_end_times_for_product(self, shc_id, tp_id, pred_dur, predalgo_id, filter_id, basepx_id, futpx_id):
        start_end_times = []
        query = "select distinct(start_end_time) from Datagentimings where prod_indc_id in (select prod_indc_id from ProdIndicators " + \
                " where shc_id = " + shc_id + " and tp_id = " + tp_id + " and pred_dur = " + \
                pred_dur + " and predalgo_id = " + predalgo_id + " and filter_id = " + filter_id + " and basepx_id = " + \
                basepx_id + " and futpx_id = " + futpx_id + ")"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        if len(data) != 0:
            for row in data:
                start_end_times.append(row[0])
        return start_end_times

    def get_tp_for_shc_id(self, shc_id):
        tp = []
        query = "select distinct(timeperiod) from Timeperiod TP, ProdIndicators PI where PI.shc_id = '" + shc_id + \
                "' and TP.tp_id = PI.tp_id"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        if len(data) != 0:
            for row in data:
                tp.append(row[0])
        return tp

    def get_shortcodes_in_prod(self):
        shortcodes = []
        query = "select shortcode from Shortcodes where shc_id in (select distinct(shc_id) from ProdIndicators)"
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        if len(data) != 0:
            for row in data:
                shortcodes.append(row[0])
        return shortcodes

    def get_corr_prod_indc_id_date(self, prod_indc_id, date):
        query = "select correlation from IndicatorStats where prod_indc_id = {} and date = {}".format(prod_indc_id, date)
        self.cursor.execute(query)
        data = self.cursor.fetchall()
        if len(data) == 0 or data[0][0] == None:
            return None
        return float(data[0][0])

    def get_ind_stats_for_ilist_dates(self, shc, start_time, end_time, predalgo, pred_dur, filter, ilist, dates):
        ## Since in update_ilist_for_products, we had hardcoded price types, here we can't support
        ## to get price types from ilist file.
        # basepx_type, futpx_type= self.read_price_types(ilist)
        basepx_type, futpx_type = "OfflineMixMMS", "OfflineMixMMS"
        indicators, _ = self.read_master_ilist(ilist)
        basepx_id = self.get_price_id(basepx_type)
        futpx_id = self.get_price_id(futpx_type)
        shc_id = self.get_shc_id(shc)
        predalgo_id = self.get_predalgo_id(predalgo)
        filter_id = self.get_filter_id(filter)
        session = self.get_session(start_time+'-'+end_time)
        tp_id = self.get_tp_id(session)
        pred_dur = str(pred_dur)

        # ind_ids = self.get_indicator_id_multiple(indicators)
        # prod_indc_ids = self.get_prod_indc_ids_for_product_and_indicator(shc_id, tp_id, pred_dur, predalgo_id,
        #                                                                  filter_id, basepx_id, futpx_id, ind_ids)

        ind_ids = []
        prod_indc_ids = []
        for indicator in indicators:
            ind_id = self.get_indicator_id(indicator)
            ind_ids.append(ind_id)
            if ind_id == -1:
                prod_indc_ids.append(-1)
            else:
                prod_indc_id = self.get_prod_indc_ids_for_product_and_indicator(shc_id, tp_id, pred_dur, predalgo_id,
                                                                                filter_id, basepx_id, futpx_id, [ind_id])
                if len(prod_indc_id) == 0:
                    prod_indc_ids.append(-1)
                else:
                    prod_indc_ids.append(prod_indc_id[0])

        prod_ind_corr = np.zeros(len(indicators))
        for i in range(len(prod_indc_ids)):
            prod_indc_id = prod_indc_ids[i]
            ## prod_indc_id == -1, there is no correlation stored for this product, indicator pair.
            if prod_indc_id == -1:
                continue
            valid_dates = 0
            for date in dates:
                corr = self.get_corr_prod_indc_id_date(prod_indc_id, date)
                if corr is not None:
                    prod_ind_corr[i] = prod_ind_corr[i]*1.0*valid_dates/(1+valid_dates) + corr*1.0/(valid_dates+1)
                    valid_dates += 1
            if valid_dates < int(0.7*len(dates)):
                return None
        return prod_ind_corr


    # close connection
    def close_conn(self,):
        self.cursor.close()
        self.conn.close()


    @staticmethod
    def get_session(pool_timing):
        pool_endtime = int(exec_function(execs.execs().get_utc_hhmm + ' ' + pool_timing.split("-")[1])[0].strip())
        as_endtime = int(exec_function(execs.execs().get_utc_hhmm + ' ' + "EST_300")[0].strip())
        eu_endtime = int(exec_function(execs.execs().get_utc_hhmm + ' ' + "EST_1000")[0].strip())
        if pool_endtime <= as_endtime:
            session = "AS"
        elif pool_endtime <= eu_endtime:
            session = "EU"
        else:
            session = "US"
        return session


    @staticmethod
    def read_master_ilist(ilist_filename):
        """
        Reads a ilist, and returns a tuple of indicators and their weights
        :param ilist_filename: (str)
        :return: (list(str),list(float))
        """
        indicators = []
        weights = []
        f = open(ilist_filename, 'r')
        for line in f:
            tokens = line.split()
            if len(tokens) > 0 and tokens[0] == "INDICATOR":
                end_indx = len(tokens)
                for j in range(len(tokens)):
                    if tokens[j][0] == '#':
                        end_indx = j
                        break
                indicator = ' '.join(x for x in tokens[2:end_indx])
                weights.append(float(tokens[1]))
                indicators.append(indicator)

        f.close()
        return indicators, weights

    @staticmethod
    def read_price_types(ilist_filename):
        basepx = futpx = None
        f = open(ilist_filename, 'r')
        for line in f:
            tokens = line.strip().split()
            if len(tokens) > 0 and tokens[0] == "MODELINIT":
                basepx, futpx = tokens[3], tokens[4]
                break
        f.close()
        return basepx, futpx


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--query", "-q", help="Query to execute", default="")
    parser.add_argument("--outputfile", "-of", help="Output File", default="")

    args = parser.parse_args()
    query = args.query
    outputfile = args.outputfile

    if query != "":
        obj = IndStatsDBAcessManager()
        obj.open_conn()
        obj.cursor.execute(query)
        data_list = obj.cursor.fetchall()

        if len(data_list) == 0:
            print("No Data\n")
            obj.close_conn()
            sys.exit()

        field_names = [i[0] for i in obj.cursor.description]
        f = open(outputfile, 'w')
        for data in data_list:
            data = list(data)
            for i in range(len(field_names)):
                if field_names[i] == "shc_id":
                    data[i] = obj.get_shc(data[i])
                if field_names[i] == "predalgo_id":
                    data[i] = obj.get_predalgo(data[i])
                if field_names[i] == "filter_id":
                    data[i] = obj.get_filter(data[i])
                if field_names[i] == "basepx_id":
                    data[i] = obj.get_price(data[i])
                if field_names[i] == "futpx_id":
                    data[i] = obj.get_price(data[i])
                if field_names[i] == "indicator_id":
                    data[i] = obj.get_indicator(data[i])
            f.write(','.join(map(str, data)) + "\n")
        f.close()
        obj.close_conn()
