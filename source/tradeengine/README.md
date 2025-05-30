## Latency sensistive strategies developed for Indian markets

### Theo:
A `Theo` is basically an alpha or a signal. 
HFT strategies are RatioTheo, HedgeTheo, MispriceTheo.
Medium Frequency theos are in TheoCalculator directory: `MidTermTheo`, `MomentumTheo`, `MACDTheo` and `MasterTheo`
To reduce slippage, the midterm Theos have used the HFT executioners and ratiotheo to get a good fill.

### TODO:
1. Remove documented code
One will find a lot of that in MFT folder which is python sim and is not part of production. I keep commented code as they act as potential alphas/ideas which i routinely try to improve the current ideas. It also serves as things which didnt work.

### FAQ
1. Which editor to use: Please use VSCode. 
1. How to format: To format all C++ files use: `find ./ -type f -name \*.hpp | xargs clang-format -assume-filename=.clang-format -style=file -i` and `find ./ -type f -name \*.cpp | xargs clang-format -assume-filename=.clang-format -style=file -i`
