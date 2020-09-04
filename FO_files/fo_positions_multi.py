import tkinter
import tkinter.filedialog
import pandas as pd
from datetime import datetime
from datetime import date
from dateutil.relativedelta import relativedelta
#month_dict = {"JAN":1,"FEB":2,"MAR":3,"APR":4, "MAY":5, "JUN":6, "JUL":7,"AUG":8,"SEP":9,"OCT":10,"NOV":11,"DEC":12}
#def to_dict(name):
#       return month_dict[name]
today = date.today();
next_month = today + relativedelta(months=+1)
next_next_month = today + relativedelta(months=+2)
current_month_name = today.strftime("%b");
next_month_name = next_month.strftime("%b");
next_next_month_name = next_next_month.strftime("%b");

def GetFoPositionsdf(filename):
        fo_pos_df = pd.read_csv(filename);
        fo_pos_df = fo_pos_df[ ['Instrument','Symbol','Expiry Date','Strike Price','OptionType','B/S'] ];
        fo_pos_df['Positions'] = fo_pos_df['B/S'].apply(lambda x: -1 if x == 'S' else 1);
        fo_pos_df.fillna('',inplace=True);
        fo_pos_df['Symbol'] = fo_pos_df.apply(lambda x: x['Instrument']+" "+x['Symbol']+" "+
                x['Expiry Date']+" "+str(x['Strike Price'])+" "+x['OptionType'],axis=1);
        fo_pos_df = fo_pos_df[['Symbol','Positions']];
        fo_pos_df.fillna('',inplace=True);
        fo_pos_df = fo_pos_df.groupby('Symbol').agg({'Positions':'sum'});
        return fo_pos_df; 

def GenerateAvgPositionsFO():
        f = open("fo_positions.txt","r+");
        lines = f.readlines()[1:];
        resultFUT = {}
        resultOPT = {}
        for x in lines:
                x=x.rstrip("\n");
                name=x.split('\t' )[0];
                if (name.split(' ')[0])[0:3] == "FUT":
                        flag = 0;
                        if (name.split(' ')[2])[2:5] == next_month_name.upper():
                                flag = 1;
                        elif (name.split(' ')[2])[2:5] == next_next_month_name.upper():
                                flag = 2;

                        symbol = name+"NSE_"+name.split(' ')[1]+"_"+(name.split(' ')[0])[0:3]+str(flag);
                        if symbol in resultFUT.keys():
                                resultFUT[symbol]=int(x.split('\t' )[1])+resultFUT[name.split(' ')[1]];
                        else:
                                resultFUT[symbol]=int(x.split('\t' )[1]);

                elif (name.split(' ')[0])[0:3] == "OPT":
                        date_str = name.split(' ')[2];
                        date_format = datetime.strptime(date_str,'%d%b%Y');
                        date_number_format = date_format.strftime("%Y%m%d");
                        strike_price = float(name.split(' ')[3]);
                        strike_price_rounded = '{0:.2f}'.format(strike_price);

                        symbol = name+" NSE_"+name.split(' ')[1]+"_"+name.split(' ')[4]+"_"+str(date_number_format)+"_"+str(strike_price_rounded);
                        if symbol in resultOPT.keys():
                                resultOPT[symbol]=int(x.split('\t' )[1])+resultOPT[name.split(' ')[1]];
                        else:
                                resultOPT[symbol]=int(x.split('\t' )[1]);
        f.close()

        for key in resultFUT:
                fileAvgFUT=open("fo_positions_FUT.txt","w");
                fileAvgInvFUT=open("fo_positions_FUT_Inverted.txt","w");
                fileAvgFUT.write("Symbol\tPositions\n");
                fileAvgInvFUT.write("Symbol\tPositions\n");
        for key in resultOPT:
                fileAvgOPT=open("fo_positions_OPT.txt","w");
                fileAvgInvOPT=open("fo_positions_OPT_Inverted.txt","w");
                fileAvgOPT.write("Symbol\tPositions\n");
                fileAvgInvOPT.write("Symbol\tPositions\n");
        for key in sorted (resultFUT.keys()):
                fileAvgFUT.write("%s\t%d\n" % (key, resultFUT[key]))
                fileAvgInvFUT.write("%s\t%d\n" % (key, -resultFUT[key]))
        for key in sorted (resultOPT.keys()):
                fileAvgOPT.write("%s\t%d\n" % (key, resultOPT[key]))
                fileAvgInvOPT.write("%s\t%d\n" % (key, -resultOPT[key]))
        fileAvgFUT.close()
        fileAvgInvFUT.close()
        fileAvgOPT.close()
        fileAvgInvOPT.close()

if __name__ == '__main__':
        root = tkinter.Tk()
        filez = tkinter.filedialog.askopenfilenames(parent=root,title='Choose a Trades File To process')
        filesSelected = root.tk.splitlist(filez)
        symbol_position={}
        for fileSelected in filesSelected:
                filename=str(fileSelected)
                #end=len(filename)-2;
                fo_pos_df = GetFoPositionsdf(filename);
                for index,row in fo_pos_df.iterrows():
                        if index in symbol_position.keys():
                                symbol_position[index]+=int(row['Positions']); 
                        else:
                                symbol_position[index]=int(row['Positions']);
        filePosition=open("fo_positions.txt","w");
        filePosition.write("Symbol\tPositions\n");      
        for key in sorted (symbol_position.keys()):
                filePosition.write("%s\t%d\n" % (key, symbol_position[key]))
        GenerateAvgPositionsFO();
