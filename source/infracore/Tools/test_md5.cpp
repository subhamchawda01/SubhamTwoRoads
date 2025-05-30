
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <openssl/md5.h>
#include "dvccode/Utils/md5.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

// [180 for CM and 186 for FO]
#define DATA_SIZE 180

void generateRandomMessage(char* msg_ptr, int size) {
    for (int i = 0; i < size; ++i) {
        msg_ptr[i] = rand() % 256; // Generate random byte (0-255)
    }
}

void printMD5Hash(uint32_t* hash) {
    // Print the computed hash
    std::cout << "MD5 Hash Value: ";
    for (int i = 0; i < 4; ++i) { // MD5 produces a 128-bit hash, which consists of 4 32-bit integers
        std::cout << std::hex << std::setw(8) << std::setfill('0') << hash[i];
    }
    std::cout << std::endl;
}

void printMessage(char* msg_ptr, int size) {
    for (int i = 0; i < size; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(msg_ptr[i]) << " ";
        if ((i + 1) % 16 == 0) // Newline after every 16 bytes
            std::cout << std::endl;
    }
    std::cout << std::endl;
}

/*
int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <number of iterations> <MD5 implementation>" << std::endl;
        std::cerr << "MD5 implementations:" << std::endl;
        std::cerr << "1 - Boost" << std::endl;
        std::cerr << "2 - Optimized" << std::endl;
        std::cerr << "3 - OpenSSL" << std::endl;
        std::cerr << "4 - TwoRoads" << std::endl;
        return 1;
    }

    HFSAT::CpucycleProfiler::SetUniqueInstance(1);
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "MD5");

    int iterations = std::stoi(argv[1]);
    int md5_impl = std::stoi(argv[2]);
    // Seed random number generator
    std::srand(std::time(nullptr));

    // OpenSSL 
    MD5_CTX ctx;
    unsigned char openssl_md5_result[16];
    // msg_ptr to store data
    char msg_ptr[16 + DATA_SIZE]; // Message size 16 (MD5 checksum) + DATA_SIZE (data) [180 for CM and 186 for FO]
    std::memset(msg_ptr, 0, sizeof(msg_ptr));

    for (int iter = 0; iter < iterations; ++iter) {
        // Initialize msg_ptr to zero
        
        generateRandomMessage(msg_ptr + 16, DATA_SIZE); // Random data starts after the MD5 checksum
       
        switch(md5_impl) {
          case 1: // Boost	
            // Calculate MD5 checksum using Boost
	    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
            HFSAT::MD5::boostMD5(reinterpret_cast<unsigned char*>(msg_ptr), DATA_SIZE);
	    HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
            //std::cout << "Message after appending Boost MD5 checksum:" << std::endl;
            //std::cout << "MD5 Boost Hash Value: ";
    	    //printMD5Hash(reinterpret_cast<uint32_t*>(msg_ptr));
	    break;

          case 2: // Optimized
            // Calculate MD5 checksum using MD5 optimisation
	    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
            HFSAT::MD5::MD5_AVX512VL(reinterpret_cast<unsigned char*>(msg_ptr + 16), DATA_SIZE,
		 	             reinterpret_cast<unsigned int*>(msg_ptr));
	    HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
            //std::cout << "Message after appending MD5 optimization checksum:" << std::endl;
            //std::cout << "MD5 Optimisation Hash Value: ";
            //printMD5Hash(reinterpret_cast<uint32_t*>(msg_ptr));
	    break;
        
	  case 3: // OpenSSL
            // Calculate MD5 using OpenSSL lib
	    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
            MD5_Init(&ctx);
            MD5_Update(&ctx, reinterpret_cast<unsigned char*>(msg_ptr + 16), DATA_SIZE);
            MD5_Final(openssl_md5_result, &ctx);
            std::memcpy(msg_ptr, openssl_md5_result, 16);
	    HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
            //std::cout << "Message after appending OpenSSL MD5 checksum:" << std::endl;
            //std::cout << "MD5 OpenSSL Hash Value: ";
            //printMD5Hash(reinterpret_cast<uint32_t*>(msg_ptr));
	    break;

	  case 4: // TwoRoads
            // TwoRoads MD5 
	    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
            HFSAT::MD5::MD5(reinterpret_cast<unsigned char*>(msg_ptr + 16), DATA_SIZE,
                            reinterpret_cast<unsigned int*>(msg_ptr));
	    HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
            //std::cout << "Message after appending TwoRoads MD5 checksum:" << std::endl;
            //std::cout << "MD5 TwoRoads Hash Value: ";
            //printMD5Hash(reinterpret_cast<uint32_t*>(msg_ptr));
	    break;
	  default:
            throw std::invalid_argument("Invalid MD5 implementation choice");
        }
    }

    std::cout << "Summary " << HFSAT::CpucycleProfiler::GetUniqueInstance(1).GetCpucycleSummaryString() << std::endl;
    return 0;
}
*/

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <number of iterations>" << std::endl;
        return 1;
    }

    int iterations = std::stoi(argv[1]);

    // Set random number generator
    std::srand(std::time(nullptr));

    // Openssl
    MD5_CTX ctx;
    unsigned char openssl_md5_result[16];
    // data ptr
    char msg_ptr[16 + DATA_SIZE]; // Message size 16 (MD5 checksum) + DATA_SIZE (data) [180 for CM and 186 for FO]
    std::memset(msg_ptr, 0, sizeof(msg_ptr));

    for (int iter = 0; iter < iterations; ++iter) {
        // Initialize msg_ptr to zero
        std::cout << "MD5 Hash Values for iteration " << iter + 1 << std::endl;
        //std::memset(msg_ptr, 0, sizeof(msg_ptr));
        
        generateRandomMessage(msg_ptr + 16, DATA_SIZE); // Random data starts after the MD5 checksum
        
        // Calculate MD5 checksum using Boost
        HFSAT::MD5::boostMD5(reinterpret_cast<unsigned char*>(msg_ptr), DATA_SIZE);
        std::cout << "Message after appending Boost MD5 checksum:" << std::endl;
        std::cout << "MD5 Boost Hash Value: ";
	printMD5Hash(reinterpret_cast<uint32_t*>(msg_ptr));
        //printMessage(msg_ptr, sizeof(msg_ptr));
        
        // Reset checksum part to zero
        //std::memset(msg_ptr, 0, 16);
        
        // Calculate MD5 checksum using MD5 optimisation
        HFSAT::MD5::MD5_AVX512VL(reinterpret_cast<unsigned char*>(msg_ptr + 16), DATA_SIZE,
			         reinterpret_cast<unsigned int*>(msg_ptr));

        std::cout << "Message after appending MD5 optimization checksum:" << std::endl;
        std::cout << "MD5 Optimisation Hash Value: ";
        printMD5Hash(reinterpret_cast<uint32_t*>(msg_ptr));
        //printMessage(msg_ptr, sizeof(msg_ptr));
        
        // Reset checksum part to zero
        //std::memset(msg_ptr, 0, 16);
        
        // Calculate MD5 using OpenSSL lib
        MD5_Init(&ctx);
        MD5_Update(&ctx, reinterpret_cast<unsigned char*>(msg_ptr + 16), DATA_SIZE);
        MD5_Final(openssl_md5_result, &ctx);
        std::memcpy(msg_ptr, openssl_md5_result, 16);
        std::cout << "Message after appending OpenSSL MD5 checksum:" << std::endl;
        std::cout << "MD5 OpenSSL Hash Value: ";
        printMD5Hash(reinterpret_cast<uint32_t*>(msg_ptr));
        //printMessage(msg_ptr, sizeof(msg_ptr));
        
        // Reset checksum part to zero
        //std::memset(msg_ptr, 0, 16);

        // TwoRoads MD5 
        HFSAT::MD5::MD5(reinterpret_cast<unsigned char*>(msg_ptr + 16), DATA_SIZE,
                        reinterpret_cast<unsigned int*>(msg_ptr));
        std::cout << "Message after appending TwoRoads MD5 checksum:" << std::endl;
        std::cout << "MD5 TwoRoads Hash Value: ";
	printMD5Hash(reinterpret_cast<uint32_t*>(msg_ptr));
        //printMessage(msg_ptr, sizeof(msg_ptr));

        
        std::cout << std::endl;
    }

    return 0;
}


