#include <thread>  // For std::this_thread::sleep_for
#include <chrono>  // For std::chrono::seconds
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/IBUtils/ControlCommandClient.hpp"

ControlCommandClient::ControlCommandClient(const std::string& ip, int port)
    : server_ip_(ip), server_port_(port) {}
    
std::string ControlCommandClient::JoinText(const std::string& separator_text_, const std::vector<std::string>& r_control_command_text_) {
  std::string retval = "";
  if (r_control_command_text_.size() > 0) {
    retval = r_control_command_text_[0];
  }
  for (unsigned int i = 1; i < r_control_command_text_.size(); i++) {
    retval += separator_text_ + r_control_command_text_[i];
  }
  return retval;
}

bool ControlCommandClient::sendControlCommand(const std::vector<std::string>& command, std::string& response, int retries) {
    if (command.empty()) {
        std::cerr << "No command provided to send." << std::endl;
        return false;
    }

    // Join command parts into a single string
    std::string send_text = "CONTROLCOMMAND " + ControlCommandClient::JoinText(" ", command);

    // Initialize the TCP client socket
    HFSAT::TCPClientSocket tcp_client_socket_;
    tcp_client_socket_.Connect(server_ip_, server_port_);

    while (retries > 0) {

        // Send the command
        if (tcp_client_socket_.WriteN(send_text.length(), send_text.c_str()) > 0) {
            // If the command expects a response
            
            char response_buffer[1024];
            size_t bytes_read = tcp_client_socket_.ReadN(sizeof(response_buffer) - 1, response_buffer);

            if (bytes_read > 0) {
                response_buffer[bytes_read] = '\0'; // Null-terminate the response
                response = response_buffer;
                tcp_client_socket_.Close();
                return true;
            } else {
                std::cerr << "Failed to read response or no response received." << std::endl;
            }
            
        } else {
            std::cerr << "Failed to send command to server. Retries left: " << (retries - 1) << std::endl;
        }

        tcp_client_socket_.Close();
        retries--;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Add a delay before retrying
    }

    std::cerr << "All retries exhausted. Command failed." << std::endl;
    return false;
}
bool ControlCommandClient::sendControlCommandNoRes(const std::vector<std::string>& command, int retries) {
    if (command.empty()) {
        std::cerr << "No command provided to send." << std::endl;
        return false;
    }

    // Join command parts into a single string
    std::string send_text = "CONTROLCOMMAND " + ControlCommandClient::JoinText(" ", command);

    // Initialize the TCP client socket
    HFSAT::TCPClientSocket tcp_client_socket_;
    tcp_client_socket_.Connect(server_ip_, server_port_);

    while (retries > 0) {

        // Send the command
        if (tcp_client_socket_.WriteN(send_text.length(), send_text.c_str()) > 0) {
            // No response expected
            tcp_client_socket_.Close();
            return true;
        } else {
            std::cerr << "Failed to send command to server. Retries left: " << (retries - 1) << std::endl;
        }

        tcp_client_socket_.Close();
        retries--;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Add a delay before retrying
    }

    std::cerr << "All retries exhausted. Command failed." << std::endl;
    return false;
}
