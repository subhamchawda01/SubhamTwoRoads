#!/usr/bin/env python


#!/home/psarthy/anaconda2/bin/python
#!/home/dvctrader/anaconda2/bin/python


import os


import pandas as pd
import numpy as np

from functools import partial
import datetime as dt
from string import split
from .update import update


if 1:
    home_directory = os.getenv("HOME")
else:
    home_directory = "/home/dvctrader/"


print(home_directory + "/basetrade/walkforward/sample_configs/simple.config")
obj = update(home_directory + "/basetrade/walkforward/sample_configs/type3.config")


# obj.show()

# print obj.get_attribute("SHORTCODE")

# print obj.get_attribute("PREDALGO")
# ll=obj.list_()

# obj.show()
# obj.update(20170101)
# obj.show()

# obj.update(20160104)
