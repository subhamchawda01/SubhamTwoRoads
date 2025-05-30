
import abc
class BaseSlippageModel(object):
    def __init__(self):
        pass
    @abc.abstractmethod
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
    @abc.abstractmethod
    def order_slippage_logic(self,smv,mkt_update_info):
        '''
         The main order slippage logic
        
        '''
        return False