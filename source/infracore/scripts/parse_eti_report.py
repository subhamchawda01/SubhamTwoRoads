from xml.dom import minidom
from xml.etree import ElementTree as ET
import sys, re
def main () :
  if (len(sys.argv) < 2 ):
    print "Usage: XML file [SUM]"
    return
  xml_file_ = sys.argv[1]
  to_print_sum_ = False
  if ( len( sys.argv) > 2 ) :
    if sys.argv[2] :
      to_print_sum_ = True

  xmlstring_ = open(xml_file_).readlines()
  xmlstring_ = '\n'.join(xmlstring_)
  xmlstring_ = re.sub(' xmlns="[^"]+"', '', xmlstring_, count=1 )
  xmldoc = ET.ElementTree(ET.fromstring( xmlstring_ )).getroot()
  cb069grp_ = xmldoc.find('cb069Grp')
  cb069_prod_grps_ = cb069grp_.findall('cb069ProdGrp')
  to_print_ = True 
  total_tx_ = 0
  total_vol_ = 0
  total_ovol_ = 0
  total_str_ = "ALL    "
  for product_grp_ in cb069_prod_grps_ :
    prod_rec_ = product_grp_.find('cb069ProdRec')
    text_= ""
    if prod_rec_.find('prodId') is not None :
      text_ = text_ + prod_rec_.find('prodId').text + " "
    else :
      to_print_ = False
      text_ = text_ + " 0 "
  
    if prod_rec_.find('txnCnt')  is not None and  prod_rec_.find('txnCnt').text.strip().isdigit() :
      text_ = text_ + prod_rec_.find('txnCnt').text + " "
      total_tx_ = total_tx_ + int (prod_rec_.find('txnCnt').text)
    else :
      text_ = text_ + " 0 "

    if ( prod_rec_.find('orderVol') is not None ) and ( prod_rec_.find('orderVol').text.strip().isdigit() ) :
      text_ = text_ + prod_rec_.find('orderVol').text + " "
      total_ovol_ = total_ovol_ + int( prod_rec_.find('orderVol').text )
    else :
      text_ = text_ + " 0 "

    if prod_rec_.find('trdVol')  is not None and prod_rec_.find('trdVol').text.strip().isdigit() :
      text_ = text_ + prod_rec_.find('trdVol').text + " "
      total_vol_ = total_vol_ + int ( prod_rec_.find('trdVol').text )
    else :
      text_ = text_ + " 0 "

    if to_print_ :
      print text_;
  if to_print_sum_ :
    print total_str_ + str( total_tx_ ) + "   " + str ( total_ovol_) + "   " + str( total_vol_)
 #   print prod_rec_.find('prodId').text +" " + prod_rec_.find('txnCnt').text + " " + prod_rec_.find('orderVol').text +" " +  prod_rec_.find('trdVol').text 

if __name__ == "__main__" :
    main()
