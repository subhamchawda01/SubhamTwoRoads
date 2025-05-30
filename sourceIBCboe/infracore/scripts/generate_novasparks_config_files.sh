#!/bin/bash

yyyymmdd=$((`date +%Y%m%d`-1));
#yyyymmdd=20141030

GEN_CONFIG_FILES_SCRIPT=/home/dvcinfra/LiveExec/scripts/analyze-eobi-fixml-1-0.py;
RDF_DIR=/spare/local/files/EUREX/RDF/;
RDF_XML_ZIP=`ls "$RDF_DIR"90FILRDF01PUBLI"$yyyymmdd"XEURXEEE*000.XML.ZIP`;

EOBI_PROD_LIST=/spare/local/files/EUREX/eobi-refmode-product-list.txt;
PRODUCT_DICT=/spare/local/files/EUREX/eobi_product_dict.txt;
INSTRUMENT_DICT=/spare/local/files/EUREX/eobi_instrument_dict.txt;
TICK_TABLE=/spare/local/files/EUREX/eobi_tick_table.txt;

unzip $RDF_XML_ZIP -d $RDF_DIR;

RDF_XML_FILE=`ls "$RDF_DIR"90FILRDF01PUBLI"$yyyymmdd"XEURXEEE*000.XML`;

python $GEN_CONFIG_FILES_SCRIPT --fixml_file $RDF_XML_FILE --list_products_to_subscribe_file $EOBI_PROD_LIST --product_dictionary_file $PRODUCT_DICT --security_dictionary_file $INSTRUMENT_DICT --tick_table_file $TICK_TABLE

>eobi_instrument_dict_filtered.txt ; 

#for token in `cat /spare/local/files/EUREX/eobi-prod-codes.txt | awk '{print $1}'`; do if [ `grep "$token " /home/dvcinfra/LiveExec/fpga/eobi_instrument_dict.txt | wc -l` -gt 1 ] ; then grep "$token " /home/dvcinfra/LiveExec/fpga/eobi_instrument_dict.txt | grep -v " $token " >> eobi_instrument_dict_filtered.txt; fi; done
for token in `cat /spare/local/files/EUREX/eobi-prod-codes.txt | awk '{print $1}'`; do grep "^$token " $INSTRUMENT_DICT >> eobi_instrument_dict_filtered.txt ; done

if [ `wc -l eobi_instrument_dict_filtered.txt | awk '{print $1}'` -gt 0 ] 
then

    mv eobi_instrument_dict_filtered.txt $INSTRUMENT_DICT ; 

fi 

scp /spare/local/files/EUREX/eobi_* ivan@10.23.102.40:file_drop/
scp /spare/local/files/EUREX/eobi_* 10.23.102.52:/spare/local/files/EUREX/
scp /spare/local/files/EUREX/eobi-* 10.23.102.52:/spare/local/files/EUREX/
