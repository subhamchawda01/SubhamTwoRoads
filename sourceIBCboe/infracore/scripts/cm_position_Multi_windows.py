import tkinter
import tkinter.filedialog
import pandas as pd
def main():
	root = tkinter.Tk()
	filez = tkinter.filedialog.askopenfilenames(parent=root,title='Choose a Trades File To process')
	filesSelected = root.tk.splitlist(filez)
	pos_dic={}
	vol_dic={}
	for fileSelected in filesSelected:
		print (fileSelected)
		filename = str(fileSelected);
	#	filename = filename[26:(len(filename)-2)];
	#	print (filename)
		df = pd.read_csv(filename, header=None, delimiter=',', skiprows=1);
		df = df[[4,6,6,9,12]]
		df.columns = ['Symbol','Pos','Volume','BS','USERID'];
		df = df[ df['USERID'] != 43545 ];
		df['Pos'] = df.apply(lambda x: (-1 * x['Pos'] if (x['BS'] == 'S') else x['Pos']),axis=1)
		del df['BS']
		del df['USERID']
		df = df.groupby(['Symbol']).agg({'Pos' : sum, 'Volume' : sum}).reset_index();
		df = df[df['Pos']!=0]
		for index,row in df.iterrows():
			if row['Symbol'] in pos_dic.keys():
				pos_dic[row['Symbol']]+=int(row['Pos'])
				vol_dic[row['Symbol']]+=int(row['Volume'])
			else:
				pos_dic[row['Symbol']]=int(row['Pos'])
				vol_dic[row['Symbol']]=int(row['Volume'])
	filePos=open("Product_Position.txt","w");
	filePosInv=open("Product_PositionInverted.txt","w");
	fileVol=open("Product_Volume.txt","w");
	filePos.write("Symbol\tPositions\n");
	filePosInv.write("Symbol\tPositions\n");
	fileVol.write("Symbol\tVolume\n");	
	for key in sorted (pos_dic.keys()):
		filePos.write("%s\t%d\n" % (key, pos_dic[key]))
		filePosInv.write("%s\t%d\n" % (key, -pos_dic[key]))
		fileVol.write("%s\t%d\n" % (key, vol_dic[key]))
	filePos.close()
	filePosInv.close()
	fileVol.close()
if __name__ == '__main__':
	main()
