# from ModelMath.base_model_math import BaseModelMath
from abc import abstractmethod
import os



class BaseModelMath(object):

    def __init__(self,_list_of_indicator_objects_):
        self.indicator_object_list_= _list_of_indicator_objects_

        #read all the indicators
#         if not os.path.exists(self.indicator_list_file_):
#             print ("The indicator file doesnot exists")
#         else:
#             with open(self.indicator_list_file_) as ilist_file_handle:
#                 indicator_list = ilist_file_handle.read().splitlines()

        #Make this object listen to all the indicators
        for indicator_object in self.indicator_object_list_:
            indicator_object.AddIndicatorListener(0,self)
        self.ModelListenerList=[]
        #initialize the indicator value list with 0
        self.indicator_value_list_ = [0.0]*len(self.indicator_object_list_)
        self.indicator_update_count_ = 0
        self.model_value_ = 0


    def on_trade_print(self, _trade_print_info_, _market_update_info_):
        pass


    def OnIndicatorUpdate(self, _indicator_index_, _indicator_value_):
        #check that the indicator_index_ is less than the list of
        self.indicator_value_list_[_indicator_index_] = _indicator_value_
        if self.indicator_update_count_ < len(self.indicator_object_list_):
            self.indicator_update_count_+=1
        self.ComputeValue()

    #renundant because indicator object are being called in the constrtuctor
    # def add_indicator(self, _indicator_):
    #     pass

    def ComputeValue(self):
        self.model_value_ = sum(self.indicator_value_list_)
        if self.indicator_update_count_ == len(self.indicator_object_list_):
            self.NotifyModelListener(self.model_value_)
            self.indicator_update_count_ = 0

    def AddModelListener(self,_listener_):
        self.ModelListenerList.append(_listener_)


    def NotifyModelListener(self,_new_val_):
        for _listener_object_ in self.ModelListenerList:
            _listener_object_.OnModelUpdate(_new_val_)

