#pragma once
#include <string>
#include <vector>
#include <iostream>

class ControlCommandClient {
public:
    // Constructor
    ControlCommandClient()=delete;
    ControlCommandClient(const ControlCommandClient &obj1)=delete;
    ControlCommandClient(const std::string& ip, int port);

    // Destructor
    ~ControlCommandClient() = default;

    // Send a control command with retries and optionally update the response string
    bool sendControlCommand(const std::vector<std::string>& command, std::string& response, int retries = 3);
    bool sendControlCommandNoRes(const std::vector<std::string>& command, int retries = 3);

private:
    static std::string JoinText(const std::string& separator_text_, const std::vector<std::string>& r_control_command_text_);
    std::string server_ip_;
    int server_port_;
};
