LIB_PATH=../lib
API_PATH=../api

#GDB=-g
OPTIMIZATION = -O2

all: app manager 

app: main.cpp 
	/apps/gcc_versions/gcc-4_9_install/bin/g++ $(MAIN_DEFINES) -I$(API_PATH) $(OPTIMIZATION) $(GDB) -Wl,-rpath,$(LIB_PATH) -L$(LIB_PATH) -std=c++11  main.cpp -lsiliconmd_nse_event -o fpga_app_mopchip

manager: manager.cpp
	/apps/gcc_versions/gcc-4_9_install/bin/g++ $(MAIN_DEFINES) -I$(API_PATH) $(OPTIMIZATION) $(GDB) -Wl,-rpath,$(LIB_PATH) -L$(LIB_PATH) -std=c++11  manager.cpp -lsiliconmd_nse_event -o fpga_manager_mopchip

clean:
	rm -rf *.o app manager
