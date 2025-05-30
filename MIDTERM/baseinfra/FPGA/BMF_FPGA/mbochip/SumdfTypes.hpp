/*--------------------------------------------------------------------------------
--
-- This file is owned and controlled by MBOChip and must be used solely
-- for design, simulation, implementation and creation of design files
-- limited to MBOChip products. Do not distribute to third parties. 
--
--            *************************************************
--            ** Copyright (C) 2013, MBOChip Private Limited **
--            ** All Rights Reserved.                        **
--            *************************************************
--
--------------------------------------------------------------------------------
-- Filename: SumdfTypes.hpp
--
-- Description: 
--    Declares data types used on the SiliconUmdf Event API 
--
*/
#ifndef SUMDFTYPES_HPP
#define SUMDFTYPES_HPP

/*! \file SumdfTypes.hpp
    \brief SiliconUMDF EventAPI types.
*/

///@cond
#include <string.h>
#include <string>
#include <stdint.h>

typedef uint64_t Uint64 ;

#define BID '0'
#define OFFER '1'
#define TRADE '2'
#define INDEX_VALUE '3'
#define OPENING_PRICE '4'
#define CLOSING_PRICE '5'
#define SETTLEMENT_PRICE '6'
#define TRADING_SESSION_HIGH_PRICE '7'
#define TRADING_SESSION_LOW_PRICE '8'
#define TRADING_SESSION_VWAP_PRICE '9'
#define IMBALANCE 'A'
#define TRADE_VOLUME 'B'
#define OPEN_INTEREST 'C'
#define EMPTY_BOOK 'J'
#define PRICE_BAND 'g'
#define QUANTITY_BAND 'h'
///@endcond

namespace SiliconUmdf {     
namespace EventAPI {
    
    typedef struct DeviceHandler_t{  //! Handler struct.
		int* handler;		

		DeviceHandler_t(){
			handler = NULL;
		}
	} DeviceHandler;
        
    enum WaitEventsReturn { 
        SUCCEEDED=0,        ///< \brief Event is valid
        TIMED_OUT=1,        ///< \brief No event returned
        FAILED=2            ///< \brief General failure. Close the handler, open it and rejoin the channels again.
    };
    
    enum JoinChannelReturn {
        JOIN_SUCCEEDED=0,                   ///< \brief JoinChannel succeeded. The channel will start to synchronize
        JOIN_FAILED=2,                      ///< \brief JoinChannel failed. Check the log for more information.
        INSTRUMENT_FILTER_NOT_AVAILABLE = 3,///< \brief Instrument filter configuration was not accepted. Check the log for more information.
        CONFIGURATION_NOT_SUPORTED=4        ///< \brief Configuration not supported to joinChannel. Check the log for more information.
    };

	enum SubscribeInstrumentReturn{		
        SUBSCRIBE_INSTRUMENT_NOT_FOUND=0,   ///< \brief Instrument not found on list.
        SUBSCRIBE_SUCCEEDED=1,              ///< \brief SubscribeInstrument succeeded.
        SUBSCRIBE_FAILED=2,                 ///< \brief SubscribeInstrument failed. Check the log for more information.
	};

	enum UnsubscribeInstrumentReturn{
        UNSUBSCRIBE_INSTRUMENT_NOT_FOUND = 0,   ///< \brief Instrument not found on list.
        UNSUBSCRIBE_SUCCEEDED = 1,              ///< \brief UnsubscribeInstrument succeeded.
        UNSUBSCRIBE_FAILED=2                    ///< \brief UnsubscribeInstrument failed. Check the log for more information.
	};

    enum StreamType {
        INSTRUMENTAL=0,             ///< \brief Instruments stream
        SNAPSHOT=1,                 ///< \brief Snapshot stream
        INCREMENTAL=2               ///< \brief Incremental stream
    };
    
    enum ExchangePlatform {
        BVMF_PUMA_1_6=0,    ///< \brief PUMA 1.6 market data platform
        BVMF_PUMA_2_0=1     ///< \brief PUMA 2.0 market data platform
    };  

    enum MarketSegment {
        BMF=0,              ///< \brief BMF segment (Derivatives)
        BOVESPA=1           ///< \brief BOVESPA segment (Equities)
    };
        
    enum ChannelStatusType {
        DISCONNECTED=0,                     ///< \brief Channel is disconnected.
        RECEIVING_INSTRUMENTAL_MESSAGES=1,  ///< \brief Waiting or receiving instrumental messages.
        SYNCHRONIZING=2,                    ///< \brief Waiting synchronization to complete.
        SYNCHRONIZED=3,                     ///< \brief API returns book and MDEntries events.
    };

    enum ReturnEventsConfiguration {
        RETURN_BOOK_AND_MDENTRIES=0,    ///< \brief API returns book and MDEntries events.
        RETURN_MDENTRIES_ONLY=1,        ///< \brief API returns MDEntries events.
        RETURN_BOOK_ONLY=2              ///< \brief API returns book events.
    };
		
	enum SubscriptionType{
        S_QUOTES=0,             ///< \brief Subscribe to receive MDEntries and book events.
        S_INSTRUMENT_STATUS=1,  ///< \brief Subscribe to receive INSTRUMENT_STATUS and GROUP_STATUS events.
        S_INSTRUMENT_UPDATES=2, ///< \brief Subscribe to receive INSTRUMENT_UPDATES events.
        S_NEWS=3,               ///< \brief Subscribe to receive NEWS events.
	};

    enum LogNameFormat {        
        APPEND_DATE_AND_TIME=0, ///< \brief Append date and time to log file name.
        APPEND_DATE=1           ///< \brief Append date to log file name.
    };
    
    enum EventsType {       
        MDENTRIES_INIT=0,                     ///< \brief  Returned when returnEventsConfiguration is set to MDENTRIES_ONLY.
        BOOK_AND_MDENTRIES_INIT=1,            ///< \brief  Returned when returnEventsConfiguration is set to BOOK_AND_MDENTRIES.
        MDENTRIES_ONLY_BVMF_PUMA_1_6=2,       ///< \brief  Returned when returnEventsConfiguration is set to MDENTRIES_ONLY.
        MDENTRIES_ONLY_BVMF_PUMA_2_0=3,       ///< \brief  Returned when returnEventsConfiguration is set to MDENTRIES_ONLY.
        BOOK_AND_MDENTRIES_BVMF_PUMA_1_6=4,   ///< \brief  Returned when returnEventsConfiguration is set to BOOK_AND_MDENTRIES.
        BOOK_AND_MDENTRIES_BVMF_PUMA_2_0=5,   ///< \brief  Returned when returnEventsConfiguration is set to BOOK_AND_MDENTRIES.
        BOOK_ONLY=6,                          ///< \brief  Returned when returnEventsConfiguration is set to BOOK_AND_MDENTRIES or BOOK_ONLY.
        EMPTY_ALL_BOOKS=7,                    ///< \brief  Notifies an empty book.
        INSTRUMENT_LIST=8,                    ///< \brief  Returns instrument and group lists.
        INSTRUMENT_UPDATES=9,                 ///< \brief  Returns instrument updates.
        INSTRUMENT_STATUS=10,                 ///< \brief  Returns instrument status updates.
        GROUP_STATUS=11,                      ///< \brief  Returns group status updates.
        NEWS=12,                              ///< \brief  Returns news.
        CHANNEL_TIMEOUT=13,                   ///< \brief  Channel timeout. Channel has not received messages for the time configured on STREAMS_TIMEOUT parameter in \a device.cfg file.
        ERROR_EVENT=14,                       ///< \brief  An error occured on the channel. You must rejoin the channel.
        ALL_CHANNELS_SYNCHRONIZED=15,         ///< \brief  All joined channels have finished synchronization.
        TCP_RECOVERY_STARTED=16,              ///< \brief  API detected a missing message on the channel and will request it on TCP Recovery.
        TCP_RECOVERY_ENDED=17,                ///< \brief  The TCP Recovery has finished the requisitions for the channel.
    };  
    

    enum Error {        
        MISSING_MESSAGE=0,                  ///< \brief A missing message was detected on the channel and the API is unable to retrieve by TCP Recovery
        SECURITY_ID_NOT_FOUND=1,            ///< \brief Message with SecurityID not found on the list (deprecated). This case is now reported as warning on log
        DIFFERENT_SECURITY_IDS=2,           ///< \brief Incremental message with different SecurityIDs
        RPTSEQ_ERROR=3,                     ///< \brief RptSeq number checking failed
        BOOK_INSERTION_ERROR=4,             ///< \brief Book insertion error
        BOOK_DELETE_ERROR=5,                ///< \brief Book delete error
        BOOK_CHANGE_ERROR=6,                ///< \brief Book change error
        MDUPDATEACTION_UNKNOWN=7,           ///< \brief Received an unknown MDUpdateAction
        MDENTRYTYPE_UNKNOWN=8,              ///< \brief Received an unknown MDEntryType
        MESSAGE_WITHOUT_SECURITY_GROUP=9,   ///< \brief Received instrumental message without SecurityGroup
        INVALID_MESSAGE_TYPE=10,            ///< \brief Received invalid MsgType
        SYNCHRONIZATION_FAILED=11,          ///< \brief Synchronization failed
        INSTRUMENT_CREATION_ERROR=12,       ///< \brief Failed to create new instrument on list (deprecated)
        INSTRUMENT_DELETE_ERROR=13,         ///< \brief Failed to delete instrument on list (deprecated)
        INSTRUMENT_UPDATE_ACTION_ERROR=14   ///< \brief Failed to update instrument on list
        
    };  

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)

    
    struct ReturnEventsConfiguration_t {
        ReturnEventsConfiguration   returnEvents;	///< \brief  Returns events configuration.
        unsigned waitEventTimeout;                  ///< \brief  Timeout for WaitEvents in microseconds. Default is \e 50ms.
		ReturnEventsConfiguration_t(){
			waitEventTimeout = 50; //50ms
		}
    };
            
    struct Stream_t {        
        char ip[16];        ///< \brief  Stream IP address.
        unsigned port;      ///< \brief  Stream port.
	}; 
    
    struct Channel_t {      		
        ExchangePlatform    exchangePlatform;       ///< \brief  Exchange platform.
        MarketSegment       marketSegment;          ///< \brief  Market segment.
        char                exchangeChannelId[4];   ///< \brief  Three-digit number used by exchange to identify the channel.
        Stream_t            instrumentalStream;     ///< \brief  Instrumental stream configuration for the channel.
        Stream_t            incrementalStream;      ///< \brief  Incremental stream configuration for the channel.
        Stream_t            snapshotStream;         ///< \brief  Snapshot stream configuration for the channel.
    };  
        
    
    struct TcpRecoveryConfiguration_t {
      bool enabled;                 ///< \brief  Enables TCP Recovery
      char  senderCompID[51];       ///< \brief  SenderCompID of TCP Recovery session
      char  targetCompID[51];       ///< \brief  TargetCompID of TCP Recovery session
      char ip[16];                  ///< \brief  IP address of TCP Recovery session as char array string. \e e.g.: "192.168.10.10".
      unsigned port;                ///< \brief  Port address of TCP Recovery session
      unsigned requisitionTimeout;  ///< \brief  Time to wait before sending a requisition (in milliseconds). Default is \e 20ms.
      unsigned nRetrials;           ///< \brief  Number of retrials for the requisition before returning error.
      unsigned retryWaitTime;       ///< \brief  Time to wait between failed requisitions.
      
      TcpRecoveryConfiguration_t(){
          requisitionTimeout=200;
          enabled = false;
          nRetrials = 3;
          retryWaitTime = 1;
          senderCompID[0] = '\0';
          targetCompID[0] = '\0';
      }   
    };  

    
    struct sumdfConfiguration_t {

      char siliconUMDF_path[260];                                   ///< \brief Path to SiliconUMDF folder. Default folder is \e /usr/siliconUmdf/.
      char siliconUMDF_logFile[260];                                ///< \brief Base name of log file generated by API. Default is \e SiliconUMDF.log.
      LogNameFormat siliconUMDF_logNameFormat;                      ///< \brief Format of log name. Appends date or date and time to log base name set on \ref sumdfConfiguration_t.siliconUMDF_logFile.


      TcpRecoveryConfiguration_t        tcpRecoveryBMF;             ///< \brief TCP Recovery configuration for BMF segment
      TcpRecoveryConfiguration_t        tcpRecoveryBOVESPA;         ///< \brief TCP Recovery configuration for BOVESPA segment
      Channel_t                         channelArray[30];           ///< \brief Channel configuration array.
      ReturnEventsConfiguration_t       returnEventsConfiguration;  ///< \brief Return events configuration.
      bool                              generateMbpBookFromMbo;     ///< \brief Generate Market-by-Price books from Market-by-Order channels
      
      sumdfConfiguration_t(){
        strcpy(siliconUMDF_logFile,"SiliconUMDF.log");
        strcpy(siliconUMDF_path,"/usr/siliconUmdf/");
        memset(channelArray,0,sizeof(Channel_t)*30);
		siliconUMDF_logNameFormat = APPEND_DATE_AND_TIME;
        generateMbpBookFromMbo = false;
      }
    };  
            
    
/// Configures which MDEntryTypes are returned on \ref MDEntries array.
///
struct MDEntriesAllowed_t{
    bool bid;                       ///< \brief Allows \b Bid (MDEntryType '0')
    bool offer;                     ///< \brief Allows \b Offer (MDEntryType '1')
    bool trade;                     ///< \brief Allows \b Trade (MDEntryType '2')
    bool openingPrice;              ///< \brief Allows \b OpeningPrice (MDEntryType '4'). Check if your API version supports this MDEntryType.
    bool imbalance;                 ///< \brief Allows \b Imbalance (MDEntryType 'A'). Check if your API version supports this MDEntryType.
    MDEntriesAllowed_t();
};
    
    /// Channel synchronization status information
    ///
    struct ChannelStatus_t {
        ChannelStatusType status;           ///< \brief Current channel synchronization status.
        int messagesOnInstrumentalBuffer;   ///< \brief Current number of messages on instrumental buffer
        int messagesOnSnapshotBuffer;       ///< \brief Current number of messages on snapshot buffer
        int messagesOnIncrementalBuffer;    ///< \brief Current number of messages on incremental buffer
        int synchronizationAttempts;        ///< \brief How many synchronization attempts it has tried (Number of snapshot loops received)
        char description[200];              ///< \brief String describing the synchronization status
    };

#pragma pack(pop)   /* restore original alignment from stack */	
    
} // namespace EventAPI
} // namespace SiliconUmdf
#endif
