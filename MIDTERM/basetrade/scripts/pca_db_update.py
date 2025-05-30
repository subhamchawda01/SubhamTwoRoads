#!/usr/bin/python
import MySQLdb
import sys
import datetime
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart
import subprocess
import argparse

class PCA_update(object):
    def __init__(self,shortcode_stdev_file,portfolio_input_file=None,portfolio_pca_file=None,debug=False):
        self.connection = self.connect_to_sql_server()
        if self.connection==None:
            print ("PCA values not updated due to connection issues, exiting !!!")
            sys.exit(1)
        self.shortcode_stdev_file = shortcode_stdev_file
        self.portfolio_input_file = portfolio_input_file
        self.portfolio_pca_file = portfolio_pca_file
        self.debug=debug
        self.date = datetime.datetime.today().strftime('%Y%m%d')
        self.cursor = self.connection.cursor()
        self.mail_address = "nseall@tworoads.co.in"

    def connect_to_sql_server(self):
        try:
            connection = MySQLdb.connect(host="10.0.0.31", user="dvcroot", passwd="dvcwiki",
                                            db="pca")
            print(connection)
            return connection
        except:
            print ("Cannot connect to SQL server")
            return None

    def update_shortcode_stdev_table(self):
        '''
        input:
            shortcode_stdev_file : string
                                The file that has the shortcode stdev mappping
        output
            update_successful : boolean
                                A boolean flag that tells success of the update   


        '''
        shortcode_stdev_file = self.shortcode_stdev_file
        self.shortcode_stdev_dict={}
        self.large_stdev_change_dict={}
        self.failed_shortcode_stdev_dict={}
        shortcode_stdev_list = []
        try:
            with open(shortcode_stdev_file) as file_handle:
                shortcode_stdev_list = file_handle.read().splitlines()
        except:
            print ("Cant read the shortcode stdev file Exiting")

        #read the list in a dictionary
        for shortcode_stdev_line in shortcode_stdev_list:
            _,shortcode,stdev = shortcode_stdev_line.split()
            self.shortcode_stdev_dict[shortcode]=stdev


        #update the shortcode stdev table

        for shortcode in self.shortcode_stdev_dict.keys():
            self.insert_into_shortcode_stdev_table(shortcode)


    
    def insert_into_shortcode_stdev_table(self,shortcode):
        stdev = self.shortcode_stdev_dict[shortcode]
        sql_fetch_cmd = ("select * from shortcode_stdev where shortcode=\"%s\" order by date desc limit 1") % (shortcode)
        if self.debug:
            print (sql_fetch_cmd)

        try:
            self.cursor.execute(sql_fetch_cmd)
            result = self.cursor.fetchone()
        except:
            print ("Cannot run the sql command")
            print (sql_fetch_cmd)
            return -1

        if result==None:
            #the shortcode is not in table hence add it
            try:
                sql_insert_cmd = "INSERT INTO shortcode_stdev (shortcode,stdev,large_diff,date) VALUES (\"%s\",\"%s\",\"%s\",\"%s\")"%(str(shortcode),str(stdev),"0",str(self.date))
                self.cursor.execute(sql_insert_cmd)
                self.connection.commit()
            except:
                self.connection.rollback()
                print ("Cannot run the command: ")
                print (sql_insert_cmd)
                return -1
        else:
            db_shortcode,db_stdev,db_large_diff,db_date = result
            if str(self.date) == str(db_date):
                print("There is already an entry for shortcode : ",shortcode," for date: ",date)
                return -1
            db_stdev = float(db_stdev)
            #check for large diff
            if self.debug:
                print ("db stdev",float(db_stdev))
                print ("desired stdev",float(stdev))
                print (abs(float(db_stdev) - float(stdev))) 
            if abs(float(db_stdev) - float(stdev)) > 0.4 * float(db_stdev) :
                old_stdev = db_stdev
                new_stdev = stdev
                self.large_stdev_change_dict[shortcode]=[old_stdev,new_stdev]
                db_large_diff = "1"
            sql_insert_cmd = "INSERT INTO shortcode_stdev (shortcode,stdev,large_diff,date) VALUES (\"%s\",\"%s\",\"%s\",\"%s\")"%(str(shortcode),str(stdev),str(db_large_diff),str(self.date))
            if self.debug:
                print (sql_insert_cmd)
            try:
                self.cursor.execute(sql_insert_cmd)
                self.connection.commit()
            except:
                self.connection.rollback()
                print ("Cannot run the command: ")
                print (sql_insert_cmd)
                return -1
            
    def update_portfolio_input_table(self):
        '''
        
        Each line in the portfolio file has the format
        
        PLINE 6ACMNEQ 6M_0 6A_0 6C_0 6N_0 ES_0
        
        
        '''
        #read the portfolio input file
        portfolio_input_list = []
        try:
            with open(self.portfolio_input_file) as file_handle:
                portfolio_input_list = file_handle.read().splitlines()
        except:
            print ("Cant read the portfolio input file Exiting")
            return -1
    
        self.portfolio_input_dict={}
        self.failed_portfolio_input_dict={}
        for portfolio_input_line in portfolio_input_list:
            try:
                temp_lst = portfolio_input_line.split()
                portfolio_name = temp_lst[1]
                portfolio_inputs = " ".join(temp_lst[2:])
                self.portfolio_input_dict[portfolio_name] = portfolio_inputs
            except:
                print ("The portfolio cant be read: ",portfolio_name)
                self.failed_portfolio_input_dict[portfolio_name] = 1
        #update the portfolio input table
        for portfolio_name in self.portfolio_input_dict.keys():
            self.insert_into_portfolio_input_table(portfolio_name)
        
    def insert_into_portfolio_input_table(self,portfolio_name):
        try:
            p_name = portfolio_name
            p_string = self.portfolio_input_dict[portfolio_name]
            #check if there exists a portfolio with the name in the table
            sql_fetch_cmd = ("select * from portfolio_input where portfolio_name=\"%s\"") % (portfolio_name)
            if self.debug:
                print (sql_fetch_cmd)
            self.cursor.execute(sql_fetch_cmd)
            result = self.cursor.fetchone()
            if result==None:
                sql_insert_cmd = "INSERT INTO portfolio_input values(\""+p_name+"\",\""+p_string+"\""+")"
                if self.debug:
                    print (sql_insert_cmd)
                self.cursor.execute(sql_insert_cmd)
                self.connection.commit()
            else:
                print ("The portfolio with name : ",portfolio_name," already exists in the table")
        except:
            self.connection.rollback()
            print ("The portfolio name: ",portfolio_name," cannot be inserted into table")
            self.failed_portfolio_input_dict[portfolio_name] = 1
                
    def update_portfolio_principal_component_table(self):
        '''
        Each portfolio has three lines
        
        stdev:    PORTFOLIO_STDEV BAXFLY5 0.00212372 0.002072821 0.002098879 
        PC1:      PORTFOLIO_EIGEN BAXFLY5 1 0.80 0.57 0.57 0.57 
        PC2:      PORTFOLIO_EIGEN BAXFLY5 2 0.20 0.70 0.10 -0.70 

        
        
        '''
        #read the portfolio pca file in list
        portfolio_pca_list = []
        try:
            with open(self.portfolio_pca_file) as file_handle:
                portfolio_pca_list = file_handle.read().splitlines()
        except:
            print ("Cant read the shortcode stdev file Exiting")
            return -1
        
        
        #remove the stdev contaning lines from the list
        portfolio_pca_list_filtered = []
        for portfolio_pca_line in portfolio_pca_list:
            if "STDEV" not in portfolio_pca_line:
                portfolio_pca_list_filtered.append(portfolio_pca_line)
        
        self.portfolio_pca_dict={}
        self.failed_portfolio_pca_dict={}
        
        for portfolio_pca_line in portfolio_pca_list_filtered:
            try:
                temp_lst = portfolio_pca_line.split()
                p_name = temp_lst[1]
                p_string = " ".join(temp_lst[3:])
                if p_name not in self.portfolio_pca_dict.keys():
                    self.portfolio_pca_dict[p_name]=[p_string]
                else:
                    temp_lst = self.portfolio_pca_dict[p_name]
                    self.portfolio_pca_dict[p_name].append(p_string)
            except:
                self.failed_portfolio_pca_dict[p_name]=1
        for portfolio_name in self.portfolio_pca_dict.keys():
            #check that the portfolio dict has two principla component for each portfolio
            if len(self.portfolio_pca_dict[p_name]) < 1:
                self.failed_portfolio_pca_dict[portfolio_name] = 1
                print ("The portfolio with name: "+portfolio_name+" doesnot have two principal components")
            else:
                self.insert_into_portfolio_pca_table(portfolio_name)
        
    def insert_into_portfolio_pca_table(self,portfolio_name):
        p_name = portfolio_name
        print(self.portfolio_pca_dict[p_name])
        try:
            p_string_pc1 = self.portfolio_pca_dict[p_name][0]
            p_string_pc2 = self.portfolio_pca_dict[p_name][1]
        except:
            self.failed_portfolio_pca_dict[portfolio_name] = 1
            return 

        #check to see if pca for the portfolio_name already exists in the table
        sql_fetch_cmd = ("select * from portfolio_principal_component where portfolio_name=\"%s\" and date=\"%s\"") % (portfolio_name,self.date)
        try:
            self.cursor.execute(sql_fetch_cmd)
            result = self.cursor.fetchone()
        except:
            print ("Cannot run the sql command")
            print (sql_fetch_cmd)
            return -1
        
        if result==None:
            try:
                sql_insert_cmd = "INSERT INTO portfolio_principal_component  values(\""+p_name+"\",\""+p_string_pc1+"\""+",\""+p_string_pc2+"\",\""+self.date+"\")"
                if self.debug:
                    print (sql_insert_cmd)
                self.cursor.execute(sql_insert_cmd)
                self.connection.commit()
                self.connection.rollback()
                self.failed_portfolio_input_dict[portfolio_name] = 1
            except:
                print ("The pca weight update for failed for portfolio : ",portfolio_name)
                pass
        else:
            print ("The portfolio with name: ",portfolio_name," aleady has pc in the table for date :",self.date)

    def mail_large_stdev_change_shortcodes_(self):
        mail_string="Shortcode\tOld_Stdev\tNew_Stdev\n\n"
        for shortcode in self.large_stdev_change_dict.keys():
            mail_string+=shortcode
            mail_string+="\t"
            mail_string+=self.large_stdev_change_dict[shortcode][0]
            mail_string+="\t"
            mail_string+=self.large_stdev_change_dict[shortcode][1]
            mail_string+="\n"
        self.send_email(self.mail_address,mail_string)
    
    def send_email(self,mail_address, mail_body):
        msg = MIMEMultipart()
        msg["To"] = mail_address
        msg["From"] = mail_address
        msg["Subject"] = "Large Stdev change shortcode"
        msg.attach(MIMEText(mail_body, 'html'))
        mail_process = subprocess.Popen(["/usr/sbin/sendmail", "-t", "-oi"],
                                        stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE)
        out, err = mail_process.communicate(str.encode(msg.as_string()))
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')
        errcode = mail_process.returncode
        
    def dump_stdev_to_file(self,stdev_out_file):
        sql_fetch_cmd = ("select * from shortcode_stdev where date= \"%s\"")%self.date
        self.cursor.execute(sql_fetch_cmd)
        results_lst = []
        for row in self.cursor:
            shortcode,stdev,large_diff,date  =  row
            stdev = str(stdev)
            large_diff = str(large_diff)
            results_lst.append([shortcode,stdev,large_diff,date])
            #write this in file
        with open(stdev_out_file,"a") as file_handle:
            for result_line in results_lst:
                #taking only the first two columns ie shortcode and stdev.
                #ignoring the large_diff and date column 
                string_to_write_ = " ".join(["SHORTCODE_STDEV"]+result_line[:2])
                file_handle.write(string_to_write_)
                file_handle.write("\n")
        print ("New stdev value dumped in file system : ",stdev_out_file)
        



if __name__ == "__main__":
        parser = argparse.ArgumentParser()
        parser.add_argument('-update_portfolio_input', dest='update_port_input', help="update the portfolio input table", type=str, required=True)
        parser.add_argument('-update_portfolio_pca', dest='update_port_pca', help="update the portfolio pca", type=str, required=True)
        parser.add_argument('-update_stdev', dest='update_stdev', help="update the portfolio input table", type=str, required=True)
        parser.add_argument('-shortcode_stdev_file', dest='shortcode_stdev_file', help="Shortcode stdev file", type=str, required=False,default=None)
        parser.add_argument('-portfolio_input_file', dest='portfolio_input_file', help="Portfolio input file", type=str, required=False,default=None)
        parser.add_argument('-portfolio_pca_weight_file', dest='portfolio_pca_weight_file', help="update the portfolio input table", type=str, required=False,default=None)
        parser.add_argument('-stdev_out_file', dest='stdev_out_file', help="update the portfolio input table", type=str, required=False,default=None)
        args = parser.parse_args()
        PCA_object = PCA_update(shortcode_stdev_file = args.shortcode_stdev_file,
                                portfolio_input_file = args.portfolio_input_file,
                                portfolio_pca_file = args.portfolio_pca_weight_file,
                                debug =True)

        if args.shortcode_stdev_file!=None and args.update_stdev=="1":
                PCA_object.update_shortcode_stdev_table()
                #PCA_object.mail_large_stdev_change_shortcodes_()

        if args.portfolio_input_file!=None and args.update_port_input=="1":
                PCA_object.update_portfolio_input_table()

        if args.portfolio_pca_weight_file!=None and args.update_port_pca=="1":
                PCA_object.update_portfolio_principal_component_table()


        if args.stdev_out_file!=None:
                PCA_object.dump_stdev_to_file(args.stdev_out_file)



