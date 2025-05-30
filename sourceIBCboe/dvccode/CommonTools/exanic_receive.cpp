#include <stdio.h>
#include <exanic/exanic.h>
#include <exanic/fifo_rx.h>
#include<iostream>
#define MAX_NSE_DATA_BUFFER 65536

#define NSE_TBT_DATA_START_OFFSET 0

#define NSE_BCAST_TRADE_RANGE_TRANS_CODE 7220

// MESSAGE TYPES

// HEADER RELATED INFO

#define NSE_TBT_DATA_HEADER_MSGLENGTH_OFFSET NSE_TBT_DATA_START_OFFSET
#define NSE_TBT_DATA_HEADER_MSGLENGTH_LENGTH sizeof(int16_t)

#define NSE_TBT_DATA_HEADER_STREAMID_OFFSET \
  (NSE_TBT_DATA_HEADER_MSGLENGTH_OFFSET + NSE_TBT_DATA_HEADER_MSGLENGTH_LENGTH)
#define NSE_TBT_DATA_HEADER_STREAMID_LENGTH sizeof(int16_t)

#define NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET \
  (NSE_TBT_DATA_HEADER_STREAMID_OFFSET + NSE_TBT_DATA_HEADER_STREAMID_LENGTH)
#define NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_LENGTH sizeof(int32_t)





int main(void)
{
    const char *device = "exanic0";
    int port = 1;
    std::cout<< NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET <<std::endl;
    exanic_t *exanic = exanic_acquire_handle(device);
    if (!exanic)
    {
        fprintf(stderr, "exanic_acquire_handle: %s\n", exanic_get_last_error());
        return 1;
    }

    exanic_rx_t *rx = exanic_acquire_rx_buffer(exanic, port, 0);
    if (!rx)
    {
        fprintf(stderr, "exanic_acquire_rx_buffer: %s\n", exanic_get_last_error());
        return 1;
    }

    char buf[MAX_NSE_DATA_BUFFER];
    exanic_cycles32_t timestamp;

    while (1)
    {
        ssize_t sz = exanic_receive_frame(rx, buf, MAX_NSE_DATA_BUFFER, &timestamp);
        if (sz > 0)
        {   char* msg_ptr = buf;
	    uint32_t msg_seq_no = *((uint32_t*)((char*)(msg_ptr + NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET)));
	    std::cout<< "Got a valid frame " << sz << " Seq " << msg_seq_no <<std::endl;
        }
    }

    exanic_release_rx_buffer(rx);
    exanic_release_handle(exanic);
    return 0;
}
