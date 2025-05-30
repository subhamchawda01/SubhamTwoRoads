class ParamSet(object):
    def __init__(self,param_file_name):
        self.param_file_name = param_file_name
        self.keep_threshold=0
        self.place_threshold=0

        self.parse()

    def parse(self):
        pass