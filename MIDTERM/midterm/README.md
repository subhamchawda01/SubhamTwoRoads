# midterm

To compile ->
1. cd dvccode; bjam release
2. cd baseinfra; bjam release
3. cd dvctrade; bjam release
4. cd midterm; bjam release

Currently there are three executables ->
1. nse_given_notional_tradeinit.cpp: For execution on IND11
2. midterm_data_server.cpp: For subscribing to data per ticker. Run on IND12
3. option_data_and_order_server.cpp: For subscribing to data per ticker as well as execution of legacy systems. Has support for stoploss orders too. Run on IND12

## cmake build

* Installs the libraries in `midterm/../qplum_install/lib` directory.
* Requires all dependent libraries to be present in the `midterm/../qplum_install/lib` directory before building the midterm execs. Make sure to build and install `midterm-infra`.
* Installs the binaries in `midterm/../qplum_install/bin` directory.

Steps for a debug build:
```
mkdir build
cd build
rm -rf * && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j8 && make install
```

Steps for a release build:
```
mkdir build
cd build
rm -rf * && cmake .. -DCMAKE_BUILD_TYPE=Rebug && make -j8 && make install
```
