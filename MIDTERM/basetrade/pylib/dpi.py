#!/usr/bin/env python

"""

"""

import os
import datetime

import pandas as pd
import numpy as np

import plotly.graph_objs as go
from pylib.setup_profile_plots import get_shortcode_for_query_id
from pylib.query_id_dpi import existing_dpi_factors

from pylib.profile_stats_utils import get_dpi_filepath

from pylib.shortcode_dpi import ShortcodeDPI

from walkforward.definitions.execs import paths
from walkforward.utils.date_utils import calc_next_week_day


class DPIPlot:
    shortcode_to_tradingdate_to_dpi_map = {}
    date_dpi_mapping = pd.DataFrame(columns=['DPI'])

    def __init__(self):
        """

        :param startdate:
        :param enddate:
        :param shortcode:
        :param query_id:
        """

    def compute_for(self, start_date, end_date, shortcode=None, query_id=None):
        """

        :param start_date:
        :param end_date:
        :param shortcode:
        :param query_id:
        :return:
        """
        if not shortcode:
            shortcode = get_shortcode_for_query_id(query_id)

        if shortcode not in self.shortcode_to_tradingdate_to_dpi_map.keys():
            self.shortcode_to_tradingdate_to_dpi_map[shortcode] = {}

        compute_stacked_bar = True
        if compute_stacked_bar:
            temp_dpi_mapping = pd.DataFrame(columns=['DPI'] + existing_dpi_factors)
        else:
            temp_dpi_mapping = pd.DataFrame(columns=['DPI'])

        # iterate through the dates and compute the dpi values for all instances where it's not generated
        tradingdate = start_date
        while tradingdate <= end_date:
            format_date = datetime.datetime.strptime(str(tradingdate), '%Y%m%d')
            print('FOR TRADINGDATE', tradingdate, " and query: ", query_id)
            if format_date not in self.shortcode_to_tradingdate_to_dpi_map[shortcode].keys():
                shc_dpi = ShortcodeDPI(shortcode, str(tradingdate))
                # shortcode tradingdate map
                self.shortcode_to_tradingdate_to_dpi_map[shortcode][format_date] = shc_dpi
            else:
                shc_dpi = self.shortcode_to_tradingdate_to_dpi_map[shortcode][format_date]

            if query_id:
                print('SHOULD NOT CALL', query_id)
                shc_dpi.add_query_id(query_id)
            else:
                # compute for all query id's for that product
                shc_dpi.set_compute_data_for_all_query_id(True)

            # compute the data for required query ids
            shc_dpi.compute_data()

            if format_date in self.date_dpi_mapping:
                temp_dpi_mapping.loc[format_date] = [self.date_dpi_mapping[format_date]]
                tradingdate = calc_next_week_day(tradingdate)
                continue

            if query_id:
                dpi_value = shc_dpi.get_dpi_value_for_query_id(query_id, True)
            else:
                dpi_value = shc_dpi.get_dpi_value(True)

            print('PRINT DPI LIST', temp_dpi_mapping.columns, [dpi_value])
            temp_dpi_mapping.loc[datetime.datetime.strptime(
                str(tradingdate), '%Y%m%d')] = [item for sublist in [dpi_value] for item in sublist]

            tradingdate = calc_next_week_day(tradingdate)

        self.date_dpi_mapping = temp_dpi_mapping
        self.date_dpi_mapping.sort_index(inplace=True)

    def get_dpi_plot(self):
        """

        :return:
        """
        df = self.date_dpi_mapping
        print('DATES', df.index)
        print('DPI', df['DPI'])

        if len(df.columns) > 1:

            layout = go.Layout(
                # barmode='group', title=' Constituents Constituents'
                barmode='stack', title='Constituents'
            )

            return {
            'data': go.Figure(data=[go.Bar(x=df.index, y=df[this_col], name=this_col) for this_col in df.columns if this_col != 'DPI'], layout=layout)
            }
        else:
            # print('DPI actual', df[int(df[0].replace("-", "")) <= self.enddate])
            return {
                'data': [go.Scatter(x=df.index, y=df['DPI'])]
            }
