## Fetch:
Filename: fetch_strat_from_config_and_date.py
For a config and tradedate, gets the model and param from database to generate the stratfile

## Update:
Filename: update_strat_for_config_and_date.py
- For a config and tradedate, insert/update the walk-forward database row corresponding to config and tradedate with the model and param for that trade date. 
- If entries already present, unless they have been marked needs-updating we don't update them.
- If entries don't exist or exist but have been marked needs-updating, then check the config to see if we need to update or will it be the same as the trade_date before.
- If we don't need to update and this row will be the same as the row before, then copy from the row before.
- Else depending on the updation algo type call the appropriate method and insert resulting model and param into this row.

Pseudocode: 
- gets the model and param for last tradedate prior to the given tradedate
- gets the algo-type for updating
- if algo-type is 3
  gets the list of modelfiles and paramfiles to choose from
  if the list of models is empty then there is a config error. Move config out of pool.
  else choose the first model
  if the list of params is empty then there is a config error. Move config out of pool.
  else choose the first param.
  insert model, param for config and the given tradedate, into database

## Pickstrats:
Only change to be made is to use the Fetch command above before installing a stratfile.

## Compute results of a list of configs on a tradedate:
Call Fetch on each config
For all configs that returned a model and param, make a stratfile and run-simulation.
For all configs that failed ( which means there was no model and param in the DB for that config ) write them in an output file // error condition.

## Compute results of a list of configs on a range of dates:
For loop over startdate to enddate to get a list of valid tradedates and call the function above.
Note that this is the only function where we need an existing util that we have in Perl, to see what is a valid tradedate.

## Projct document
https://docs.google.com/document/d/1Y1hBrYwPV7w8YFJ4_qcHpAbUK1Ki-ogB_1JWopkucSY/edit?ts=5877e911
