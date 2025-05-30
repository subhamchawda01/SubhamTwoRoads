// HTMLGenerator.hpp
#ifndef HTMLGENERATOR_HPP
#define HTMLGENERATOR_HPP

#include <map>
#include <string>

namespace HFSAT{

namespace Utils{

    class HTMLGenerator {
    private:
        static std::map<int, std::string> contractRows; // Map to store unique rows based on contract ID
        static std::map<long, std::string> executionRows;

        static std::string file_path;
        static std::string html_header;
        static std::string html_footer;
        static std::string time;  // Default time
        static double dailyPnL;
        static double unrealizedPnL;
        static double realizedPnL;
        static std::string initMarginReq;
        static std::string maintMarginReq; 
        static std::string netLiquidation;
        static std::string availableFunds;
        static std::string grossPositionValue;
    public:
        static void updatePnl(double dailyPnL, double unrealizedPnL, double realizedPnL, const std::string& newTime);
        static void updInitMarginReq(const std::string& initMargin);
        static void updMaintMarginReq(const std::string& maintMargin);
        static void updNetLiquidation(const std::string& netLiquid);
        static void updAvailableFunds(const std::string& availableFunds);
        static void updGrossPositionValue(const std::string& grossPosition);
        static void addOrUpdateExecutionRow(long orderId, const std::string& exec_time, int reqId, const std::string& symbol,
                                                const std::string& secType, const std::string& currency,
                                                const std::string& execId, const std::string& shares,
                                                const std::string& cumQty, const std::string& lastLiquidity,
                                                const double& fillPrice, const std::string& short_code);
        // Set the file path for the HTML
        static void setFilePath(const std::string& path);

        // Add or update a row for a given contract ID
        static void addOrUpdateRow(int contractId, const std::string& account, const std::string& symbol, 
                                    const std::string& secType, const std::string& currency, 
                                    const std::string& position, const std::string& avgCost, 
                                    const std::string& exchange_symbol, const std::string& short_code,
                                    const std::string& marketPrice="null",const std::string& marketValue="null",
                                    const std::string& unRealizedPnl="null",const std::string& realizedPnl="null");

        // Clear all rows from the HTML (but keep headers)
        static void clear();

        // Write the HTML to the file
        static void writeToFile();
    };

}
}


#endif // HTMLGENERATOR_HPP