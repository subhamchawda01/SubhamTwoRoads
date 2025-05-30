#
#NOTE: Bar data should be in csv format

#

import plotly.graph_objects as go
import pandas as pd
import sys

if ( len(sys.argv) != 2 ) :
  print("Usage EXEC CSV_FILE")
  exit()

df = pd.read_csv(sys.argv[1])

fig = go.Figure(data=go.Ohlc(x=df['Date'],
                open=df['AAPL.Open'],
                high=df['AAPL.High'],
                low=df['AAPL.Low'],
                close=df['AAPL.Close']))
fig.update(layout_xaxis_rangeslider_visible=False)
fig.show()
