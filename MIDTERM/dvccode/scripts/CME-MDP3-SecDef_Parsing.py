import time
import sys

# Constant
input_file_path=sys.argv[1]
output_file_path=sys.argv[1]



# Output Configuration

# Channels <tag 1180> to be extracted, '' item is mandatory
channels=('310', '312', '314', '318', '320', '340', '342', '344', '346', '360', '382');  
#channels=('310', '311', '312', '313', '314', '315', '3116', '317', '318', '319', '320', '321', '340', '341', '342', '343', '344', '345', '346', '360', '361', '380', '381', '382', '383', '384', '385', '386', '387', '410', '430', '431', '440', '441', '450', '460', '461', '')  


Max_Dico_Size = 32000000
Max_Instru_Per_Channel = 105000
max_depth=10 # Max diffusing depth. Use 1 for BBO on all instrument. Use 999 for market published depth

dispatcher='11111111' #Output dispatcher configurationerage 
output_list_label=('ApplID','SecurityGroup','Symbol','SecurityID','SecurityDesc','CFICode','SecuritySubType','ImpliedMarketIndicator','MarketDepth','MarketDepth', 'DisplayFactor', 'TradinRefPrice')
output_list_index=('1180','1151','55','48','107','461','762','1144','264','264-0', '9787', '1150')




# Identify and open CME Security Definition file
secdef_file_name=input_file_path + '/' + "secdef.dat"
secdef_file=open(secdef_file_name)


# Variable initialization

nb_instru_per_channel=[]
for i in range (0,10):
    nb_instru_per_channel.append([0])
print nb_instru_per_channel   

    
i=0
j=1
p=0
depth=0
ns_dico=""
grp_dico=""
symbols=[]
cme_instruments=""
from time import gmtime, strftime
current_time=strftime("%Y%b%d-%H%M%S", gmtime())
nb_instru_per_channel = [0]*1000   # account for up to 1000 channels



# l_index=[]   # Used only once to compute all possible indexes


# Create and open output files, both dico and list
tmp_str=""
for items in channels:
    tmp_str=tmp_str+ '-' + items
if len(tmp_str)>20:
    tmp_str=tmp_str[:20] + "_and_more_"
    
output_dico_file_name = output_file_path + '/' + "cme_instrument_dict_raw.txt"
output_dico_file=open(output_dico_file_name, "w")
output_grp_file_name = output_file_path + '/' + "cme_instrument_group_raw.txt"
output_grp_file=open(output_grp_file_name, "w")
output_list_file_name = output_file_path + '/' + "cme_instruments-channels" + tmp_str + current_time + ".csv"
output_list_file=open(output_list_file_name, "w")


# Write header values in cme_intruments file
for items in output_list_label:                
    cme_instruments=cme_instruments + items + ","
cme_instruments=cme_instruments + "\n"
output_list_file.write(cme_instruments)
cme_instruments=""
for items in output_list_index:                
    cme_instruments=cme_instruments + items + ","
cme_instruments=cme_instruments + "\n"
output_list_file.write(cme_instruments)
cme_instruments=""



###################
#### MAIN LOOP ####
###################
for line in secdef_file:

    # Test to show progress as a percentage of all instrument scanned  
    if p==1000:
        print str(i/3800) + "% completed. Number of Instruments included = " + str(j-1)
        p=0
    p=p+1
    i=i+1

    # Extract data line by line and convert to list data structure
    l = line.replace(chr(1),",")

    # Test to speed up loop and increment line if line is in channels that is not of interest. Can be removed.
    if channels.count(l[l.find('1180=')+5:l.find(',',l.find('1180='))])==0:
        continue
    tmpp=l[l.find('107=')+5:l.find(',',l.find('107='))]

    #if tmpp=='0EJU2':
    #    print tmpp
    #    print l[l.find('1180=')+5:l.find(',',l.find('1180='))]

    l = l.replace("=",",")
    l = l.replace(",\n","")
    l=l.split(',')

    # Make index unique by appending a '-x' sufix to avoid duplicate indexes
    ll=[]
    tmp_l=[]
    for items in l[::2]:
        if ll.count(items)==0:
            ll.append(items)
            #if l_index.count(items)==0:
            #    l_index.append(items)
        else:
            ll.append(items+'-'+str(tmp_l.count(items)))
            #if l_index.count(items+'-'+str(tmp_l.count(items)))== 0:
            #    l_index.append(items+'-'+str(tmp_l.count(items)))
            tmp_l.append(items)

    # Convert to dictionary data structure
    d = dict([(k, v) for k,v in zip (ll, l[1::2])])

    # Build output 
    if channels.count(d.get('1180'))>=1:
        if int(d.get('264'))>max_depth:
            depth=max_depth
        else:
            depth=d.get('264')
        # Build requested Novasparks Dictionary 
        if (j <= Max_Dico_Size) and (nb_instru_per_channel[int(d.get('1180'))]<Max_Instru_Per_Channel):
            ns_dico = ns_dico + d.get('1151') + ' ' + d.get('48') + ' ' + str(j) + ' ' + d.get('264') + ' ' + str(depth) + ' ' + dispatcher + '\n'
            symbols.append(d.get('1151'))
            if (d.get('48')==116984):
                print "ID found symbol" + str(d.get('55'))

        
        # Build requested instrument list
        for items in output_list_index:
            if d.get(items) == None:
                tmp_str=""
            else:
                tmp_str=d.get(items)                
            cme_instruments=cme_instruments + tmp_str + ","
        cme_instruments=cme_instruments + "\n"
        if (j <= Max_Dico_Size) and (nb_instru_per_channel[int(d.get('1180'))]<Max_Instru_Per_Channel):
            output_list_file.write(cme_instruments)
            nb_instru_per_channel[int(d.get('1180'))]=nb_instru_per_channel[int(d.get('1180'))]+1
            j=j+1
        cme_instruments=""

        if (j == Max_Dico_Size):
            break
        
# extract the list of groups without duplicate
grp_index=1
for grp in set(symbols):
    #print grp + str(grp_index)
    grp_dico = grp_dico + grp + ' ' + str(grp_index) + ' ' + dispatcher + '\n' 
    grp_index=grp_index+1
        
output_grp_file.write(grp_dico)
output_dico_file.write(ns_dico)
#print ns_dico


secdef_file.close()
output_dico_file.close()
output_grp_file.close()
output_list_file.close()


