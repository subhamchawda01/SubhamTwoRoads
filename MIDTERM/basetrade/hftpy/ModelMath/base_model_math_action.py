from ModelMath.base_model_math import BaseModelMath


class BaseModelMathAction(BaseModelMath):

    def __init__(self,IndicatorListener, SecurityMarketViewListener):
        BaseModelMath.__init__(self,IndicatorListener, SecurityMarketViewListener)

    def predict(self):
        pass

    def propagate_result(self):
        pass
