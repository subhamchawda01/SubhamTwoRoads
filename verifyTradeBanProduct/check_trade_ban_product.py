import tkinter
import tkinter.filedialog
import pandas as pd
import requests
import urllib
def main():
        url = 'https://www1.nseindia.com/content/fo/fo_secban.csv'
        urllib.request.urlretrieve(url, "fo_secban.csv")
        banFile = 'fo_secban.csv'
        root = tkinter.Tk()
        filez = tkinter.filedialog.askopenfilenames(parent=root,title='Choose a Trades File To process')
        filesSelected = root.tk.splitlist(filez)
        banTraded=set()

        bf = pd.read_csv(banFile, header=None, delimiter=',', skiprows=1);
        bf = bf[[1]];
        bf.columns = ['Symbol'];
        banList = bf.Symbol.unique();
        banTradedFile = open("Ban_Traded.txt","w");
        banTradedFile.write("Ban_Product_List\n");
        for product in banList:
            banTradedFile.write("%s " % product);
        print(banList);

        for fileSelected in filesSelected:
                print (fileSelected)
                filename = str(fileSelected);
                df = pd.read_csv(filename, header=None, delimiter=',', skiprows=1);
                df = df[[4]]
                df.columns = ['Symbol'];
                df = df.Symbol.unique();
                for row in df:
                        if row in banList:
                                banTraded.add(row);
        banTradedFile.write("\nBan_Product_Traded\n");
        for product in sorted (banTraded):
            banTradedFile.write("%s " % product);
        print(banTraded);
        banTradedFile.close();

if __name__ == '__main__':
     print("Subham")
     main()
