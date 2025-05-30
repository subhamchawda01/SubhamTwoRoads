<h1>CBOE Fetch</h1>

<h3>About</h3>

Fetch script for a particular exchange refers to the process of getting details
about tradable instruments and storing them in such a way to accomodate for
security definition loading.

<h3>Files this script generates</h3>

* /spare/local/tradeinfo/CBOE_Files/datasource_exchsymbol.txt
* /spare/local/tradeinfo/CBOE_Files/BhavCopy/fo/${MM}${YY}/${DD}${MM}fo_0000.md
* /spare/local/tradeinfo/CBOE_Files/ContractFiles/cboe_contracts.${YYYYMMDD}
* /spare/local/tradeinfo/CBOE_Files/RefData/cboe_fo_${YYYYMMDD}_contracts.txt

NOTE: Bhavcopy file and Refdata file are same in case of CBOE

<h3>Date Selection</h3>

Uses this script for date selection.

* /home/pengine/prod/live_scripts/get_next_trading_day.sh

Takes date as input

* if input date is holiday, returns next worker day
* if input date is working, reutrns same
   
<h3>File Generation</h3>

<h4>Bhavcopy & Refdata</h4>

First file that gets generated is this, both these files are same, we just generate one and copy other.

```
def generate_bhav_and_ref_file():
```

* TWS should be up
* This fucntion requires `ibapi` installed
* Fucntion retrives all valid contract for next 2 months
* Dumps them into a file(bhavcopy file generated)
* Copy the same as refdata file

<h4>Datasource symbol</h4>

Generation of this file directly doesnt require the use of `ibapi`

* Loads old datasource file
* Get the latest offset
* Offset is the number after exchange ($EXCH$OFFSET)
* Reads the bhavcopy file
* Adds $EXCH_$OFFSET and $EXCH_$PROD_$OPT_TYPE_$EXPIRY_$STRIKE

<h4>Contract file</h4>

```
def get_spx_spot_yf_and_gen_con_file():
```

This fucntion requires SPOT value to decide ATM strike.

* Get the spot value from OEBU
* cols: [ opt_type | underlying | expiry | ATM | lot_size | min_tick | number_of_strikes | step_value ]

These files together help loading the sec_def for the particular exchange.


<h3>Hosting</h3>

* Script can be ran on worker 68 and 71
* Script requires installation of `ibapi` in a python venv
* Currently installed in `/home/dvcinfra/cboe_fetch/bin/activate`

<h4>Steps to install ibapi</h4>

* Check min python version
* Create python env: `python -m venv cboe_fetch`
* `source /home/dvcinfra/cboe_fetch/bin/activate`
* Downlaod: https://interactivebrokers.github.io/#
* `unzip twsapi_macunix.1035.zip`
* `cd IBJts/source/pythonclient`
* `python setup.py install`

<h4>Step to verify installation</h4>

*  `source /home/dvcinfra/cboe_fetch/bin/activate`
```
(cboe_fetch) [ dvcinfra@sdv-srv68 ~ ] python
Python 3.10.9 (main, Dec 30 2022, 15:58:27) [GCC 6.3.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> import ibapi
>>> print(ibapi.__version__)
10.19.4
>>> 
(cboe_fetch) [ dvcinfra@sdv-srv68 ~ ] 
```

<h4>Files verification</h4>

A script checks the md5sum of all required files and mail them

```
/home/pengine/prod/live_scripts/check_file.sh IBKR ${filepath} | tail -n 2 | head -n 1 | awk '{print $3}'
/home/pengine/prod/live_scripts/check_file.sh WORKER ${filepath} | tail -n 2 | head -n 1 | awk '{print $3}'
```

* script to check files: /home/pengine/prod/live_scripts/check_file.sh
* mail subject: CBOE Fetch Status: $YYYYMMDD

NOTE: We have python env setup for `ibapi` in 2 worker machines and on trading machine as well. 

* 10.23.5.68 (infra user)
* 10.23.5.10 (infra user)
* 10.23.5.102 (infra user)

<h3>Cron</h3>

```
##================================= FETCH =========================================================##
00 01 * * 1-5 /home/dvcinfra/cboe_fetch/bin/python3 /home/pengine/prod/live_scripts/cboe_fetch.py > /home/dvcinfra/trash/fetch.`date +\%Y\%m\%d`.logs 2>&1
```

<h3>Relevent Mail Subjects</h3>

* CBOE Fetch Status: $YYYYMMDD
