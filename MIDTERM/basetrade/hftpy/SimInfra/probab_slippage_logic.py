from random import randint
import os
import sys
import getpass
sys.path.append(os.path.join("/home", getpass.getuser(), "basetrade", "hftpy"))
from hftpy.SimInfra.base_slippage_model_ import BaseSlippageModel
class ProbabSlippageLogic(BaseSlippageModel):
    def __init__(self,name,fill_probab):
        self.name = name
        self.fill_probab = fill_probab

    def is_order_slip(self,smv,mkt_update_info):
        '''
        INPUT:
        smv: custom_class 
            The smv(book info) of the shortcode under consideration
        mkt_update_info: custom class
            The current market update info 
        
        RETURN
        
        boolean
        '''
        return self.order_slippage_logic(smv,mkt_update_info)
    def order_slippage_logic(self,smv,mkt_update_info):
        return True if randint(0,1000)>self.fill_probab else False
