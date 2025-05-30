#include "dvccode/Utils/get_todays_date_utc.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string DateUtility::getTodayDateUTC() {
    // Get the current time as a system clock point
    auto now = std::chrono::system_clock::now();

    // Convert to time_t for UTC time
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    // Convert to UTC
    std::tm utc_tm = *std::gmtime(&now_time);

    // Format the date as a string
    std::ostringstream date_stream;
    date_stream << std::put_time(&utc_tm, "%Y%m%d");
    return date_stream.str();
}
