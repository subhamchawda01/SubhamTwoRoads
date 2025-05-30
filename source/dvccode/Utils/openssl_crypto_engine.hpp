// =====================================================================================
// 
//       Filename:  openssl_crypto_engine.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  Wednesday 22 November 2023 04:56:35  GMT
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
#include <string.h>  // Include this for bzero()
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

#define MAX_CRYPTO_BUFFER_LENGTH 8192

namespace HFSAT{
namespace Utils{

class OpenSSLCrypto{

  public :

    EVP_CIPHER_CTX *encrypt_ctx;
    unsigned char *encrypt_text;
    int encrypt_len;

    EVP_CIPHER_CTX *decrypt_ctx;
    unsigned char *decrypt_data;
    int decrypt_len;

  public :

    OpenSSLCrypto():
      encrypt_ctx(nullptr),
      encrypt_text(new unsigned char[MAX_CRYPTO_BUFFER_LENGTH]),
      encrypt_len(0),
      decrypt_ctx(nullptr),
      decrypt_data(new unsigned char[MAX_CRYPTO_BUFFER_LENGTH]),
      decrypt_len(0){
        std::cout << "OPENSSL VERSION : " << OpenSSL_version(OPENSSL_VERSION) << std::endl;  
    }

    ~OpenSSLCrypto(){}

    //BSE openssl3 encryption with fixed IV length
    void encrypt_EVP_aes_256_gcm_init_BSE_with_fixed_iv_length(unsigned char*key, unsigned char *iv){
      if(!(encrypt_ctx = EVP_CIPHER_CTX_new())){
        std::cout << "FAILED TO GET NEW CRYPTO CTX OBJ " << std::endl;
      }

      if(1 != EVP_EncryptInit_ex(encrypt_ctx,EVP_aes_256_gcm(), NULL, NULL, NULL)){
        std::cout << "FAILED TO INITIALIZE CRYPTO AES 256" << std::endl;
      }

      if(1 != EVP_CIPHER_CTX_ctrl(encrypt_ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL)){
        std::cout << "FAILED TO SET IV LENGTH" << std::endl;
      }

      if(1 != EVP_EncryptInit_ex(encrypt_ctx,NULL, NULL, key, iv)){
        std::cout << "FAILED TO INITIALIZE CRYPTO AES 256" << std::endl;
      }
    }

    void encrypt_EVP_aes_256_gcm_init(unsigned char*key, unsigned char *iv){
      if(!(encrypt_ctx = EVP_CIPHER_CTX_new())){
        std::cout << "FAILED TO GET NEW CRYPTO CTX OBJ " << std::endl;
      }

      if(1 != EVP_EncryptInit_ex(encrypt_ctx, EVP_aes_256_gcm(), NULL, key, iv)){
        std::cout << "FAILED TO INITIALIZE CRYPTO AES 256" << std::endl;
      }
    }

    //BSE openssl3 encryption with fixed IV length
    void decrypt_EVP_aes_256_gcm_init_BSE_with_fixed_iv_length(unsigned char*key, unsigned char *iv){
      if(!(decrypt_ctx = EVP_CIPHER_CTX_new())){
        std::cout << "FAILED TO GET NEW DECRYPT CRYPTO CTX OBJ " << std::endl;
      }

      if(1 != EVP_DecryptInit_ex(decrypt_ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)){
        std::cout << "FAILED TO INITIALIZE CRYPTO AES 256" << std::endl;
      }

      if(1 != EVP_CIPHER_CTX_ctrl(decrypt_ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL)){
        std::cout << "FAILED TO INITIALIZE CRYPTO AES 256" << std::endl;
      }

      if(1 != EVP_DecryptInit_ex(decrypt_ctx, NULL, NULL, key, iv)){
        std::cout << "FAILED TO INITIALIZE CRYPTO AES 256" << std::endl;
      }
    }

    void decrypt_EVP_aes_256_gcm_init(unsigned char*key, unsigned char *iv){
      if(!(decrypt_ctx = EVP_CIPHER_CTX_new())){
        std::cout << "FAILED TO GET NEW DECRYPT CRYPTO CTX OBJ " << std::endl;
      }

      if(1 != EVP_DecryptInit_ex(decrypt_ctx, EVP_aes_256_gcm(), NULL, key, iv)){
        std::cout << "FAILED TO INITIALIZE CRYPTO AES 256" << std::endl;
      }
    }

    void aes_encrypt(unsigned char *data, int data_len){
      encrypt_len = 0;

      if(1 != EVP_EncryptUpdate(encrypt_ctx, encrypt_text, &encrypt_len, data, data_len)){
        std::cout << "FAILED TO ENCRYPT DATA INTO AES 256" << std::endl;
      }
//      std::cout << "ENCRYPTED MSG LENGTH : " << encrypt_len << " DATA PACKET : "<< data_len << std::endl;
    }

    void aes_decrypt(unsigned char *encrypted_data, int encrypted_data_len){

//      std::cout << "DECRYPT DATA : " << encrypted_data_len << std::endl;
      decrypt_len = 0;
      if(1 != EVP_DecryptUpdate(decrypt_ctx, decrypt_data, &decrypt_len, encrypted_data, encrypted_data_len)){
        std::cout << "FAILED TO DECRYPT DATA : " << std::endl;
      }

      //std::cout << "DECRYPT DATA LENGTH : " << decrypt_len << std::endl;
    }
};

}}
