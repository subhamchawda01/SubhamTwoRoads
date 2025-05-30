// HTMLGenerator.cpp
#include "dvccode/Utils/html_generator.hpp"
#include <fstream>
#include <sstream>

namespace HFSAT
{
namespace Utils {
    
    // Static member definitions
    std::map<int, std::string> HTMLGenerator::contractRows;
    std::map<long, std::string> HTMLGenerator::executionRows;
    std::string HTMLGenerator::file_path = "positions.html";
    // Add new static members for time and PnL data
    std::string HTMLGenerator::time = "0";  // Default time
    double HTMLGenerator::dailyPnL = 0.0;
    double HTMLGenerator::unrealizedPnL = 0.0;
    double HTMLGenerator::realizedPnL = 0.0;
    // Margin values
    std::string HTMLGenerator::initMarginReq = "";  // Initial Margin Requirement
    std::string HTMLGenerator::maintMarginReq = ""; // Maintenance Margin Requirement
    std::string HTMLGenerator::netLiquidation = ""; // Net Liquidation Value
    std::string HTMLGenerator::availableFunds = "";  // Available Funds
    std::string HTMLGenerator::grossPositionValue = "";  // Gross Position Value

    std::string HTMLGenerator::html_header = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Positions Monitor</title>
            <style>
                table {
                    width: 100%;
                    border-collapse: collapse;
                }
                th, td {
                    border: 1px solid black;
                    padding: 8px;
                    text-align: left;
                }
                th {
                    background-color: #f2f2f2;
                }
                .summary {
                    margin-bottom: 20px;
                }
                .table-gap {
                    margin-top: 40px; /* Add space between the tables */
                }
                .search-container {
                    margin-top: 10px;
                }
            </style>
        </head>
        <body>
            <h1>Positions Monitor</h1>
            <div class="summary">
                <!-- Time and PnL details will be inserted dynamically -->
            </div>
            <div class="search-container">
                <label for="searchInput">Search by Short Code: </label>
                <input type="text" id="searchInput" onkeyup=\"searchTable()\" placeholder="Enter Short Code">
            </div>
    )";

    std::string HTMLGenerator::html_footer = R"(
        <script>
            document.addEventListener('DOMContentLoaded', function () {
                function searchTable() {
                    const input = document.getElementById('searchInput').value.toLowerCase();
                    const filterTables = ['positionsTable', 'executionTable'];
                    
                    filterTables.forEach(tableId => {
                        const table = document.getElementById(tableId);
                        if (table) {
                            const rows = table.querySelectorAll('tbody tr');
                            rows.forEach(row => {
                                const shortCodeCell = row.querySelector('td:last-child');
                                if (shortCodeCell) {
                                    const text = shortCodeCell.textContent || shortCodeCell.innerText;
                                    row.style.display = text.toLowerCase().includes(input) ? '' : 'none';
                                }
                            });
                        }
                    });
                }

                function sortTable(tableId, columnIndex) {
                    const table = document.getElementById(tableId);
                    const tbody = table.querySelector('tbody');
                    const rows = Array.from(tbody.querySelectorAll('tr'));
                    const isAscending = table.dataset.sortOrder === 'asc';
                    
                    rows.sort((rowA, rowB) => {
                        const cellA = rowA.cells[columnIndex].innerText.toLowerCase();
                        const cellB = rowB.cells[columnIndex].innerText.toLowerCase();
                        if (cellA < cellB) return isAscending ? -1 : 1;
                        if (cellA > cellB) return isAscending ? 1 : -1;
                        return 0;
                    });

                    table.dataset.sortOrder = isAscending ? 'desc' : 'asc';
                    tbody.innerHTML = '';
                    rows.forEach(row => tbody.appendChild(row));
                }

                document.getElementById('searchInput').addEventListener('keyup', searchTable);
                document.querySelectorAll('.sortable').forEach(header => {
                    header.addEventListener('click', function () {
                        const tableId = this.dataset.table;
                        const columnIndex = this.cellIndex;
                        sortTable(tableId, columnIndex);
                    });
                });
            });
        </script>
    </body>
    </html>
    )";


    // Method to update PnL and time
    void HTMLGenerator::updatePnl(double dailyPnL, double unrealizedPnL, double realizedPnL, const std::string& newTime) {
        HTMLGenerator::dailyPnL = dailyPnL;
        HTMLGenerator::unrealizedPnL = unrealizedPnL;
        HTMLGenerator::realizedPnL = realizedPnL;
        HTMLGenerator::time = newTime;
    }
    // Method to update margin details
    void HTMLGenerator::updInitMarginReq(const std::string& initMargin) {
        HTMLGenerator::initMarginReq = initMargin;
    }
    void HTMLGenerator::updMaintMarginReq(const std::string& maintMargin) {
        HTMLGenerator::maintMarginReq = maintMargin;
    }
    void HTMLGenerator::updNetLiquidation(const std::string& netLiquid) {
        HTMLGenerator::netLiquidation = netLiquid;
    }
    void HTMLGenerator::updAvailableFunds(const std::string& availableFunds) {
        HTMLGenerator::availableFunds = availableFunds;
    }
    void HTMLGenerator::updGrossPositionValue(const std::string& grossPosition) {
        HTMLGenerator::grossPositionValue = grossPosition;
    }

    void HTMLGenerator::addOrUpdateExecutionRow(long orderId, const std::string& exec_time,int reqId, const std::string& symbol,
                                                const std::string& secType, const std::string& currency,
                                                const std::string& execId, const std::string& shares,
                                                const std::string& cumQty, const std::string& lastLiquidity,
                                                const double& fillPrice, const std::string& short_code) {
        std::ostringstream row;
        row << "<tr>"
            << "<td>" << orderId << "</td>"
            << "<td>" << exec_time << "</td>"
            << "<td>" << reqId << "</td>"
            << "<td>" << symbol << "</td>"
            << "<td>" << secType << "</td>"
            << "<td>" << currency << "</td>"
            << "<td>" << execId << "</td>"
            << "<td>" << shares << "</td>"
            << "<td>" << cumQty << "</td>"
            << "<td>" << lastLiquidity << "</td>"
            << "<td>" << fillPrice << "</td>"
            << "<td>" << short_code << "</td>"
            << "</tr>";
        
        // Add or update the row in the map
        executionRows[orderId] = row.str();
    }
    void HTMLGenerator::setFilePath(const std::string& path) {
        file_path = path;
    }

    void HTMLGenerator::addOrUpdateRow(int contractId, const std::string& account, const std::string& symbol, 
                                    const std::string& secType, const std::string& currency, 
                                    const std::string& position, const std::string& avgCost, 
                                    const std::string& exchange_symbol, const std::string& short_code,
                                    const std::string& marketPrice,const std::string& marketValue,
                                    const std::string& unRealizedPnl,const std::string& realizedPnl) {
        std::ostringstream row;
        row << "<tr>"
            << "<td>" << account << "</td>"
            << "<td>" << symbol << "</td>"
            << "<td>" << secType << "</td>"
            << "<td>" << currency << "</td>"
            << "<td>" << position << "</td>"
            << "<td>" << marketPrice << "</td>"
            << "<td>" << marketValue << "</td>"
            << "<td>" << avgCost << "</td>"
            << "<td>" << unRealizedPnl << "</td>"
            << "<td>" << realizedPnl << "</td>"
            << "<td>" << exchange_symbol << "</td>"
            << "<td>" << short_code << "</td>"
            << "</tr>";
        
        // Add or update the row in the map
        contractRows[contractId] = row.str();
    }

    void HTMLGenerator::clear() {
        // Clear the rows map but keep the headers
        contractRows.clear();
        executionRows.clear();
        // Open the file and write only the headers
        std::ofstream file(file_path);
        if (file.is_open()) {
            file << html_header;  // Write the HTML header and table structure
            file << html_footer;  // Write the closing HTML tags
            file.close();
        } else {
            throw std::ios_base::failure("Failed to open the file for clearing");
        }
    }

    void HTMLGenerator::writeToFile() {
        std::ofstream file(file_path);
        if (file.is_open()) {
            // Construct the HTML body dynamically
            std::stringstream content;
            content << html_header;

            // Add the Summary section at the top
            content << "<div class='summary'>"
                    << "<p>Time: " << time << "</p>"
                    << "<p>Daily PnL: " << std::to_string(dailyPnL) << "</p>"
                    << "<p>Unrealized PnL: " << std::to_string(unrealizedPnL) << "</p>"
                    << "<p>Realized PnL: " << std::to_string(realizedPnL) << "</p>"
                    << "</div>";

            // Add Margin Usage Table
            content << "<h2>Margin Usage</h2>"
                    << "<table>"
                    << "<thead><tr>"
                    << "<th>Initial Margin Requirement</th>"
                    << "<th>Maintenance Margin Requirement</th>"
                    << "<th>Net Liquidation Value</th>"
                    << "<th>Available Funds</th>"
                    << "<th>Gross Position Value</th>"
                    << "</tr></thead><tbody>"
                    << "<tr>"
                    << "<td>" << initMarginReq << "</td>"
                    << "<td>" << maintMarginReq << "</td>"
                    << "<td>" << netLiquidation << "</td>"
                    << "<td>" << availableFunds << "</td>"
                    << "<td>" << grossPositionValue << "</td>"
                    << "</tr>"
                    << "</tbody></table>";

            // Add a gap between the two tables
            content << "<div class='table-gap'></div>";

            // Add Positions Table header
            content << "<h2>Positions</h2>";

            // Add the Positions Table
            content << "<table>"
                    << "<table id=\"positionsTable\">"
                    << "<thead><tr>"
                    << "<th>Account</th>"
                    << "<th>Symbol</th>"
                    << "<th>SecType</th>"
                    << "<th>Currency</th>"
                    << "<th>Position</th>"
                    << "<th>MarketPrice</th>"
                    << "<th>MarketValue</th>"
                    << "<th>Avg Cost</th>"
                    << "<th>Unrealized Pnl</th>"
                    << "<th>Realized Pnl</th>"
                    << "<th>Exchange Symbol</th>"
                    << "<th class=\"sortable\" data-table=\"positionsTable\">Short Code <span style='font-size: 10px;'>&#9650;&#9660;</span></th>"
                    << "</tr></thead><tbody>";

            // Write each position row
            for (const auto& pair : contractRows) {
                content << pair.second;
            }

            content << "</tbody></table>";

            // Add a gap between the tables
            content << "<div class='table-gap'></div>";

            // Add Order Execution Table header
            content << "<h2>Order Execution Details</h2>";

            // Add the Order Execution Table
            content << "<table>"
                    << "<table id=\"executionTable\">"
                    << "<thead><tr>"
                    << "<th>Order ID</th>"
                    << "<th>Execution Time</th>"
                    << "<th>Request ID</th>"
                    << "<th>Symbol</th>"
                    << "<th>SecType</th>"
                    << "<th>Currency</th>"
                    << "<th>Execution ID</th>"
                    << "<th>Shares</th>"
                    << "<th>Cumulative Quantity</th>"
                    << "<th>Last Liquidity</th>"
                    << "<th>Fill Price</th>"
                    << "<th class=\"sortable\" data-table=\"executionTable\">Short Code <span style='font-size: 10px;'>&#9650;&#9660;</span></th>"
                    << "</tr></thead><tbody>";

            // Add all rows from the executionRows map
            for (const auto& orderid_row : executionRows) {
                content << orderid_row.second;
            }

            content << "</tbody></table>";

            content << html_footer;

            // Write the content to the file
            file << content.str();
            file.close();
        } else {
            throw std::ios_base::failure("Failed to open the file for writing");
        }
    }

} // namespace Utils
    
} // namespace HFSAT
