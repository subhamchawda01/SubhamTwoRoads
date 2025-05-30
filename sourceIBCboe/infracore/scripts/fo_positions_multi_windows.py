import tkinter
import tkinter.filedialog
import pandas as pd
#month_dict = {"JAN":1,"FEB":2,"MAR":3,"APR":4, "MAY":5, "JUN":6, "JUL":7,"AUG":8,"SEP":9,"OCT":10,"NOV":11,"DEC":12}
#def to_dict(name):
#	return month_dict[name]


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

def GenerateAvgPositions():
	f = open("fo_positions.txt","r+");
	lines = f.readlines()[1:];
	result = {}
	for x in lines:
		x=x.rstrip("\n");
		name=x.split('\t' )[0];
		if name.split(' ')[0] == "FUTSTK":
			if name.split(' ')[1] in result.keys():
				result[name.split(' ')[1]]=int(x.split('\t' )[1])+result[name.split(' ')[1]];
			else:
				result[name.split(' ')[1]]=int(x.split('\t' )[1]);
	f.close()
	for key in result:
		fileAvg=open("fo_positions_FUT0.txt","w");
		fileAvgInv=open("fo_positions_FUT0_Inverted.txt","w");
		fileAvg.write("Symbol\tPositions\n");
		fileAvgInv.write("Symbol\tPositions\n");
	for key in sorted (result.keys()):
		fileAvg.write("%s\t%d\n" % (key, result[key]))
		fileAvgInv.write("%s\t%d\n" % (key, -result[key]))
	fileAvg.close()
	fileAvgInv.close()

if __name__ == '__main__':
	root = tkinter.Tk()
	filez = tkinter.filedialog.askopenfilenames(parent=root,title='Choose a Trades File To process')
	filesSelected = root.tk.splitlist(filez)
	symbol_position={}
	for fileSelected in filesSelected:
		filename=str(fileSelected)
		#end=len(filename)-2;
		fo_pos_df = GetFoPositionsdf(filename);
		with pd.option_context('display.max_rows', None, 'display.max_columns', None):
			print(fo_pos_df);
		for index,row in fo_pos_df.iterrows():
			if index in symbol_position.keys():
				symbol_position[index]+=int(row['Positions']); 
			else:
				symbol_position[index]=int(row['Positions']);
	filePosition=open("fo_positions.txt","w");
	filePosition.write("Symbol\tPositions\n");	
	for key in sorted (symbol_position.keys()):
		filePosition.write("%s\t%d\n" % (key, symbol_position[key]))
	GenerateAvgPositions();
