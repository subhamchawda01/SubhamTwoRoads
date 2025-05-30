// =====================================================================================
// 
//       Filename:  openssl_tsl_client_socket.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  Wednesday 22 November 2023 04:28:08  GMT
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551 
// 
// =====================================================================================

#pragma once

#include <stdlib.h>  // Include this for exit()
#include <stdio.h>
#include <unistd.h>
#include <cstring>  // Include this for bzero()
#include <sys/time.h>
#include <sys/types.h>

#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <fcntl.h>
#include <string>
#include <assert.h>
#include <errno.h>
#include <vector>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#include "dvccode/CDef/fwd_decl.hpp"

namespace HFSAT{ 
namespace Utils{

  class OpenSSLTLSClientSocket {

    private:
      SSL_CTX *ssl_context_;
      SSL *ssl_;
      int socket_file_descriptor_;
      unsigned int offset_;

    public:
      OpenSSLTLSClientSocket(const std::string &cert_file):
        ssl_context_(nullptr),
        ssl_(nullptr),
        socket_file_descriptor_(-1),
        offset_(0){

        //Initialize OpenSSL library
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSLeay_add_ssl_algorithms();
        SSL_load_error_strings();

        //Create an SSL context
        ssl_context_ = SSL_CTX_new(TLS_client_method());
        if(!ssl_context_){
          std::cout << "<FATAL> SSL_CTX_new failed" << std::endl;
          std::exit(-1);
        }

        SSL_CTX_set_min_proto_version(ssl_context_, TLS1_3_VERSION);
        SSL_CTX_set_max_proto_version(ssl_context_, TLS1_3_VERSION);

        //Load Certificate
        if(SSL_CTX_use_certificate_file(ssl_context_, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0){
          std::cout << "<FATAL> Failed to Load Certificate File : "<< cert_file << " Error Stack : " << std::strerror(errno) << std::endl;
          std::exit(-1);
        }

        socket_file_descriptor_ = socket(AF_INET, SOCK_STREAM, 0);
        if(socket_file_descriptor_ < 0){
          std::cout << "<FATAL> Failed to Create SSL TLS Socket, Error Stack : "<< std::strerror(errno) << std::endl;
          std::exit(-1);
        }
      }

      void Connect(const std::string ip, const int port){
        struct sockaddr_in sock_Addr_;
        bzero(&sock_Addr_, sizeof(sock_Addr_));
        sock_Addr_.sin_family = AF_INET; 
        sock_Addr_.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &(sock_Addr_.sin_addr));

        if(connect(socket_file_descriptor_, (struct sockaddr *)&sock_Addr_, sizeof(struct sockaddr_in)) < 0){
          std::cout << "Connect Failed On : " << ip << " X " << port << std::endl;
          close(socket_file_descriptor_);
          socket_file_descriptor_ = -1;
        }

        ssl_ = SSL_new(ssl_context_);
        if(!ssl_){
          std::cout << "<FATAL> Failed to Create SSL" << std::endl;
          close(socket_file_descriptor_);
          std::exit(-1);
        }

        SSL_set_fd(ssl_, socket_file_descriptor_);
        if(SSL_connect(ssl_) != 1) {
          std::cout << "<FATAL> Failed to Open SSL Connection" << std::endl;
          std::exit(-1);
        }
      }

      int WriteN(const unsigned int len, void *src) const {
        if(ssl_){
          return SSL_write(ssl_, src, len);
        }
        return -1;
      }

      int ReadN(const unsigned int len, void *dest) const {
        if(ssl_){
          return SSL_read(ssl_, dest, len);
        }
        return -1;
      }

      void DisConnect(){
        if(ssl_){
          SSL_shutdown(ssl_);
          SSL_free(ssl_);
        }

        if(socket_file_descriptor_ != -1){
          close(socket_file_descriptor_);
        }

        if(ssl_context_){
          SSL_CTX_free(ssl_context_);
        }

        ERR_free_strings();   //cleanup, meemory for error strings
      }

      ~OpenSSLTLSClientSocket(){
        DisConnect();
      }

  };
}}

