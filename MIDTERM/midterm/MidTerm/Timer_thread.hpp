#include <thread>
#include <chrono>
#include <ctime>
#include "midterm/GeneralizedLogic/constants.hpp"

class Timer_thread {
public:
  Timer_thread(WatchUpdateListener *object_) {
    // detach would make this thread run independently
    std::thread(timer_thread, object_).detach();
  }
  Timer_thread(Timer_thread const &disabled_copy_constructor) =
      delete; // don't want a copy constructor

  static Timer_thread &GetUniqueInstance(WatchUpdateListener *object_) {
    static Timer_thread unique_instance(object_);
    return unique_instance;
  }
  static void timer_thread(WatchUpdateListener *object_) {
    while (1) {
      // Sleep for 30 seconds
      std::this_thread::sleep_for(std::chrono::seconds(ALERT_TIME));
      std::cout << "From thread : Last received time is : "
                << object_->GetLastDataReceivedTime() << "\n";
      int current_system_time = std::time(0);
      if (current_system_time - object_->GetLastDataReceivedTime() > ALERT_TIME)
        std::cout << "ALERT FROM TRADE_BAR_GENERATOR: UNABLE TO OBTAIN MARKET "
                     "UPDATES.... FATAL"
                  << std::endl;
    }
    return;
  }
};
