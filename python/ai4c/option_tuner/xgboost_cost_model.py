from typing import List
import xgboost as xgb

# Fit a final xgboost model on the housing dataset and make a prediction.
from numpy import asarray
from xgboost import XGBRegressor


# Use high-level api XGBRegressor, which might be slower than xgb.train.
class XGBoostCostModel(object):

    def __init__(self):
        self.params = {
            'booster': 'gbtree',  # The default, outperforms gblinear
            'objective': 'reg:squarederror',  # 'reg:linear' is deprecated
            'learning_rate': 0.3,
            'n_estimators': 1000,
            'eval_metric': 'rmse'
        }
        self.bst = xgb.XGBRegressor(**self.params)

    def fit(self, xs: List[List[int]], ys: List[float]):
        self.bst.fit(xs, ys)

    def predict(self, xs: List[List[int]]):
        ret = []
        for x in xs:
            ret.append(self.bst.predict(np.asarray([x]))[0])
        return ret


if __name__ == '__main__':
    pass
