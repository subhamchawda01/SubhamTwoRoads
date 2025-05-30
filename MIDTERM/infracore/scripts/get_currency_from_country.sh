#!/bin/bash

if [ $# -lt 1 ] ; then echo "USAGE: <script> <country>"; exit ; fi;

case $1 in
  Australia)
    echo AUD;
  ;;
  Canada)
    echo CAD;
  ;;
  China)
    echo CNY;
  ;;
  European_Monetary_Union)
    echo EUR;
  ;;
  France)
    echo FRA;
  ;;
  Germany)
    echo GER;
  ;;
  Greece)
    echo GRC;
  ;;
  Italy)
    echo ITA;
  ;;
  Japan)
    echo JPY;
  ;;
  New_Zealand)
    echo NZD;
  ;;
  Portugal)
    echo POR;
  ;;
  Spain)
    echo SPA;
  ;;
  Switzerland)
    echo CHF;
  ;;
  Finland)
    echo FIN;
  ;;
  Brazil)
    echo BRL;
  ;;
  India)
    echo INR;
  ;;
  Ireland)
    echo IRE;
  ;;
  Russia)
    echo RUB;
  ;;
  United_Kingdom)
    echo GBP;
  ;;
  United_States)
    echo USD;
  ;;
  *)
    echo EXT;
  ;;
esac
