#!/usr/bin/env python

"""

This is main script which runs on server, the contents ect would get automatically refreshed.

"""

import os
import sys
import argparse

import datetime as dt

import dash
import dash_core_components as dcc
import dash_html_components as html
from dash.dependencies import Input, Output, Event, State


import plotly.graph_objs as go
import numpy as np

sys.path.append(os.path.expanduser('~/trash/praful/basetrade'))
from pylib.setup_profile_plots import plot_queue_pos_for_shortcode
from pylib.setup_profile_plots import plot_queue_pos_diff_histogram_for_shortcode
from pylib.setup_profile_plots import plot_queue_pos_histogram_for_shortcode
from pylib.setup_profile_plots import plot_cancel_queue_pos_histogram_for_shortcode
from pylib.setup_profile_plots import get_cxl_rej_stats_for_shortcode
from pylib.dpi import DPIPlot


app = dash.Dash(__name__)
dpi_plot = DPIPlot()
shortcode = "FGBL_0"

def run_dash_server(query, startdate, enddate, query_type='shortcode'):
    global dpi_plot
    global shortcode
    """

    :return:
    """
    startdate = startdate.replace('-', '')
    enddate = enddate.replace('-', '')
    print(startdate + "-" + enddate)

    startdate = int(startdate)
    enddate = int(enddate)

    if query_type == 'shortcode':
        shortcode = query
        dpi_plot.compute_for(startdate, enddate, shortcode=query)
    else:
        dpi_plot.compute_for(startdate, enddate, query_id=query)

    return_dict = dpi_plot.get_dpi_plot()

    layout = go.Layout(
        xaxis=dict(
            title='Date',
        ),
        yaxis=dict(
            title='DPI Value',
        ),
        title='DPI Graph'
    )
    return_dict['layout'] = layout
    return_dict['mode'] = 'lines+markers'
    return return_dict


def update_histogram(query_id, date):
    global shortcode
    """
    data = plot_queue_pos_for_shortcode("FGBL_0", "20180103")
    :param query_id:
    :param date:
    :return:
    """
    date = date.replace("-", "")
    data = plot_queue_pos_diff_histogram_for_shortcode(shortcode, date)
    data_x = data['data'][0]['x']
    data_y = data['data'][0]['y']

    if not data_x or not data_y:
        return

    bar_data = [go.Bar(
        x=data['data'][0]['x'],
        y=data['data'][0]['y']
    )]
    print(data)
    return bar_data


"""
app.layout = html.Div([
    html.Div('Temporary layout'),
    html.Div(dcc.Input(placeholder='FGBM_0', type='text', value='FGBM_0', id='shortcode-input')),
    dcc.Input(placeholder='20171201', type='text', value='20171201', id='tradingdate-input'),
    dcc.Graph(id='live-graph', figure=run_dash_server('FGBM_0', '20171201'))

])
"""


class CxlRejectHistogram(object):
    bar_data = None
    def initialize(self, dateval, granularity=0):
        global shortcode
        date = dateval.replace("-", "")
        cxl_data = get_cxl_rej_stats_for_shortcode(shortcode, date)
        diff_col = cxl_data['y_axis'] - cxl_data['x_axis']
        # Test with instead diff_col.hist(column='CXL Rejects', bins=15)
        # when HS1 mounts starts working
        # below is a code that worked before that failure.
        diff_min, diff_max = diff_col.min(), diff_col.max()
        if granularity == 0:
            granularity = (diff_max - diff_min) / 10
        stats_range = np.arange(diff_min, diff_max + granularity - 0.01, granularity)
        stats_count = diff_col.groupby(pd.cut(diff_col, stats_range)).count()
        bar_data = [go.Bar(x=str(stats_count),y=stats_count)]
        self.bar_data = bar_data

    def get_histogram(self):
        return dcc.Graph(id="cxl_histogram" + date,figure=go.Figure(data=self.bar_data))

class QueuePosDiffHistogram(object):
    bar_data = None
    def initialize(self, dateval):
        global shortcode
        date = dateval.replace("-", "")
        data = plot_queue_pos_diff_histogram_for_shortcode(shortcode, date)
        data_x = data['data'][0]['x']
        data_y = data['data'][0]['y']

        if not data_x or not data_y:
            return

        bar_data = [go.Bar(
            x=data['data'][0]['x'],
            y=data['data'][0]['y']
        )]

        self.bar_data = bar_data

    def get_histogram(self):
        return dcc.Graph(id="queue_pos_cxl_histogram" + date,figure=go.Figure(data=self.bar_data))

class QueuePosCxlHistogram(object):
    bar_data = None
    def initialize(self, dateval):
        global shortcode
        date = dateval.replace("-", "")
        data = plot_queue_pos_cancel_histogram_for_shortcode(shortcode, date)
        data_x = data['data'][0]['x']
        data_y = data['data'][0]['y']

        if not data_x or not data_y:
            return

        bar_data = [go.Bar(
            x=data['data'][0]['x'],
            y=data['data'][0]['y']
        )]

        self.bar_data = bar_data

    def get_histogram(self):
        return dcc.Graph(id="queue_pos_cxl_histogram" + date,figure=go.Figure(data=self.bar_data))

class QueuePosSeqHistogram(object):
    bar_data = None
    def initialize(self, dateval):
        global shortcode
        date = dateval.replace("-", "")
        data = plot_queue_pos_histogram_for_shortcode(shortcode, date)
        data_x = data['data'][0]['x']
        data_y = data['data'][0]['y']

        if not data_x or not data_y:
            return

        bar_data = [go.Bar(
            x=data['data'][0]['x'],
            y=data['data'][0]['y']
        )]

        self.bar_data = bar_data

    def get_histogram(self):
        return dcc.Graph(id="queue_pos_seq_histogram" + date,figure=go.Figure(data=self.bar_data))


class HistogramFactory(object):
    hist_map = {}
    histogram_handler = {
      'queue_pos_diff': QueuePosDiffHistogram,
      'queue_pos_seq': QueuePosSeqHistogram,
      'queue_pos_cxl': QueuePosCxlHistogram,
      'cxl_reject': CxlRejectHistogram,
    }

    @staticmethod
    def add_histogram_for_date(date, hist_type):
        if hist_type not in HistogramFactory.hist_map:
            HistogramFactory.hist_map[hist_type] = {}

        if date not in HistogramFactory.hist_map[hist_type]:
            handler_class = histogram_handler[hist_type]
            handler_class().initialize(date)
            if not handler_class.bar_data:
                return
            hist = handler_class().get_histogram()
            HistogramFactory.hist_map[hist_type][date] = hist
        hist = HistogramFactory.hist_map[hist_type][date]
        return hist

    @staticmethod
    def get_histogram_list(hist_type):
        hist_array = []
        for date in sorted(HistogramFactory.hist_map[hist_type]):
            hist_array += [HistogramFactory.hist_map[hist_type][date]]

        return hist_array

    @staticmethod
    def clear_histogram_list(hist_type):
        HistogramFactory.hist_map[hist_type] = {}


app.layout = html.Div([
    html.Div('DPI Plot'),
    html.Div(
        dcc.Input(
            placeholder='FGBM_0',
            type='text',
            value='FGBM_0',
            id='queryid-input')),
    dcc.RadioItems(
        options=[
            {'label': 'Query ID', 'value': 'query'},
            {'label': 'Shortcode', 'value': 'shortcode'},
            #{'label': 'Exchange', 'value': 'exchange'}
        ],
        id='query_type',
        value='shortcode'
    ),

    dcc.Input(
        id='tradingstartdate-input',
        type='Date',
        value=dt.date.today() -
        dt.timedelta(
            days=10)),
    dcc.Input(id='tradingenddate-input', type='Date', value=dt.date.today()),

    html.Button("Show DPI Graph", id="show_dpi"),

    dcc.Graph(id='live-graph', figure=run_dash_server('FGBM_0', (dt.date.today() - dt.timedelta(days=10)).strftime('%Y%m%d'),
                                                      dt.date.today().strftime('%Y%m%d'), query_type='shortcode')),
    html.Div([
        html.Button("Add Queue-pos diff histogram", id="queue_diff_hist"),
        dcc.Input(
            id='queue-diff-histdate-input',
            type='Date',
            value=dt.date.today() -
            dt.timedelta(
                days=10)),
        html.Div(id="queue-diff-hist-list")
    ]),
    html.Div([
        html.Button("Add Queue-pos seq histogram", id="queue_seq_hist"),
        dcc.Input(
            id='queue-seq-histdate-input',
            type='Date',
            value=dt.date.today() -
            dt.timedelta(
                days=10)),
        html.Div(id="queue-seq-hist-list")
    ]),
    html.Div([
        html.Button("Add Queue-pos before cancel histogram", id="queue_cancel_hist"),
        dcc.Input(
            id='queue-cancel-histdate-input',
            type='Date',
            value=dt.date.today() -
            dt.timedelta(
                days=10)),
        html.Div(id="queue-cancel-hist-list")
    ]),
    html.Div([
        html.Button("Add Cancel-reject histogram", id="cancelrej_hist"),
        dcc.Input(
            id='cancel-histdate-input',
            type='Date',
            value=dt.date.today() -
            dt.timedelta(
                days=10)),
        dcc.Dropdown(
            id='cancel-granularity-input',
            options=[
                {'label': '10', 'value': '10'},
                {'label': '20', 'value': '20'},
                {'label': 'Auto', 'value': '0'}
            ],
            value=0),
        html.Div(id="cancel-hist-list")
    ])
])


@app.callback(Output('queue-diff-hist-list', 'children'), [],
              [State('queue-diff-histdate-input', 'value')], [Event('queue_diff_hist', 'click')])
def add_queue_diff_histogram_on_click(dateval):
    HistogramFactory.add_histogram_for_date(dateval, "queue_pos_diff")
    return HistogramFactory.get_histogram_list("queue_pos_diff")

@app.callback(Output('queue-seq-hist-list', 'children'), [],
              [State('queue-seq-histdate-input', 'value')], [Event('queue_seq_hist', 'click')])
def add_queue_seq_histogram_on_click(dateval):
    HistogramFactory.add_histogram_for_date(dateval, "queue_pos_seq")
    return HistogramFactory.get_histogram_list("queue_pos_seq")

@app.callback(Output('queue-cancel-hist-list', 'children'), [],
              [State('queue-cancel-histdate-input', 'value')], [Event('queue_cancel_hist', 'click')])
def add_queue_cancel_histogram_on_click(dateval):
    HistogramFactory.add_histogram_for_date(dateval, "queue_pos_cxl")
    return HistogramFactory.get_histogram_list("queue_pos_cxl")


@app.callback(Output('cancel-hist-list', 'children'), [],
              [State('cancel-histdate-input', 'value'), State('cancel-granularity-input', 'value')], [Event('cancelrej_hist', 'click')])
def add_cxl_histogram_on_click(dateval, granularity):
    HistogramFactory.add_histogram_for_date(dateval, "cxl_reject")
    return HistogramFactory.get_histogram_list("cxl_reject")

'''
@app.callback(Output('hist-list', 'children'),[], [], [Event('clear_hist', 'click')])
def clear_histogram_list_on_click(dateval):
    HistogramFactory.clear_histogram_list()
    return HistogramFactory.get_histogram_list()
'''


@app.callback(Output('live-graph', 'figure'), [], [State('queryid-input', 'value'),
                                                   State('tradingstartdate-input', 'value'), State('tradingenddate-input', 'value'), State('query_type', 'value')], [Event('show_dpi', 'click')])
def update_plot(val1, val2, val3, val4):
    """
    one call for all inputs in dpi
    :param val1:
    :param val2:
    :param val3:
    :return:
    """
    return run_dash_server(val1, val2, val3, val4)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-host', dest='host', help="host Name", type=str, default='')
    parser.add_argument('-p', dest='port', help='Desired Port', type=str, default=8050)

    args = parser.parse_args()

    if args.host:
        app.run_server(host=args.host, port=args.port)
    else:
        app.run_server(host="0.0.0.0", port=args.port)
