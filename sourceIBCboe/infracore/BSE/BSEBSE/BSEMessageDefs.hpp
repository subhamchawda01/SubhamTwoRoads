/*******************************************************************************
 * Permission to use this/these file(s) is subject to the Terms of Use set
 * forth in the file Terms_of_Use.txt accompanying this file.
 *******************************************************************************
 *
 *  FILE NAME: ETILayouts.h
 *
 *  INTERFACE VERSION:   2.3
 *
 *  BUILD NUMBER:        600.002.500-600002500.49
 *
 *  DESCRIPTION:
 *
 *    This header file documents the binary message format of ETI.
 *    - All integers are in little endian byte order.
 *    - Padding bytes in following structures (char PadX[...]) are not required to be initialized.
 *
 *  DISCLAIMER:
 *
 *    Supported on Linux/x64 platforms with gnu C/C++ version 4.1 and 4.4.
 *
 *    This header file is meant to be compatible (but not supported) with any C/C++
 *    compiler/architecture that defines C99 compliant integer types in stdint.h and
 *    corresponds with the following alignment and padding requirements:
 *
 *    Padding:
 *      The compiler does not add implicit padding bytes between any of the following
 *      structure members. All padding bytes required for the alignment rules below are
 *      already explicitly contained in the structures.
 *
 *    Alignment rules:
 *      1 byte alignment for  int8_t and  uint8_t types
 *      2 byte alignment for int16_t and uint16_t types
 *      4 byte alignment for int32_t and uint32_t types
 *      8 byte alignment for int64_t and uint64_t types
 *
 *******************************************************************************/

#ifndef __ETI_LAYOUTS__
#define __ETI_LAYOUTS__

#include <iostream>
#include <stdint.h>
#include <iomanip>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"
{
#endif

#define ETI_INTERFACE_VERSION "2.4"
#define ETI_BUILD_NUMBER      "600.002.500-600002500.49"

/*
 * No Value defines
 */
#define BYTE_ARRAY_OF_0_16 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define NO_VALUE_SLONG                                   ((int64_t) 0x8000000000000000L)
#define NO_VALUE_ULONG                                   ((uint64_t) 0xffffffffffffffffUL)
#define NO_VALUE_SINT                                    ((int32_t) 0x80000000)
#define NO_VALUE_UINT                                    ((uint32_t) 0xffffffff)
#define NO_VALUE_SSHORT                                  ((int16_t) 0x8000)
#define NO_VALUE_USHORT                                  ((uint16_t) 0xffff)
#define NO_VALUE_SCHAR                                   ((int8_t) 0x80)
#define NO_VALUE_UCHAR                                   ((uint8_t) 0xff)
#define NO_VALUE_STR                                     0
#define NO_VALUE_DATA_16                                 BYTE_ARRAY_OF_0

/*
 * Template IDs defines
 */
#define	TID_ADD_COMPLEX_INSTRUMENT_REQUEST               10301		// < AddComplexInstrumentRequest (Create Strategy)
#define	TID_ADD_COMPLEX_INSTRUMENT_RESPONSE              10302		// < AddComplexInstrumentResponse (Create Strategy Response)
#define	TID_BROADCAST_ERROR_NOTIFICATION                 10032		// < BroadcastErrorNotification (Gap Fill)
#define	TID_CROSS_REQUEST                                10118		// < CrossRequest (Cross Request)
#define	TID_CROSS_REQUEST_RESPONSE                       10119		// < CrossRequestResponse (Cross Request Response)
#define	TID_DEBT_INQUIRY_REQUEST                         10390		// < DebtInquiryRequest (Debt Inquiry Request)
#define	TID_DEBT_INQUIRY_RESPONSE                        10391		// < DebtInquiryResponse (Debt Inquiry Response)
#define	TID_DELETE_ALL_ORDER_BROADCAST                   10122		// < DeleteAllOrderBroadcast (Order Mass Cancellation Notification)
#define	TID_DELETE_ALL_ORDER_NR_RESPONSE                 10124		// < DeleteAllOrderNRResponse (Order Mass Cancellation Response No Hits)
#define	TID_DELETE_ALL_ORDER_QUOTE_EVENT_BROADCAST       10308		// < DeleteAllOrderQuoteEventBroadcast (Mass Cancellation Event)
#define	TID_DELETE_ALL_ORDER_REQUEST                     10120		// < DeleteAllOrderRequest (Order Mass Cancellation Request)
#define	TID_DELETE_ALL_ORDER_RESPONSE                    10121		// < DeleteAllOrderResponse (Order Mass Cancellation Response)
#define	TID_DELETE_ALL_QUOTE_BROADCAST                   10410		// < DeleteAllQuoteBroadcast (Quote Mass Cancellation Notification)
#define	TID_DELETE_ALL_QUOTE_REQUEST                     10408		// < DeleteAllQuoteRequest (Quote Mass Cancellation Request)
#define	TID_DELETE_ALL_QUOTE_RESPONSE                    10409		// < DeleteAllQuoteResponse (Quote Mass Cancellation Response)
#define	TID_DELETE_ORDER_BROADCAST                       10112		// < DeleteOrderBroadcast (Cancel Order Notification)
#define	TID_DELETE_ORDER_BY_ORDER_BROADCAST              10116		// < DeleteOrderByOrderBroadcast (Cancel Order by Order Notification)
#define	TID_DELETE_ORDER_COMPLEX_REQUEST                 10123		// < DeleteOrderComplexRequest (Cancel Order Complex)
#define	TID_DELETE_ORDER_NR_RESPONSE                     10111		// < DeleteOrderNRResponse (Cancel Order Response (Lean Order))
#define	TID_DELETE_ORDER_RESPONSE                        10110		// < DeleteOrderResponse (Cancel Order Response (Standard Order))
#define	TID_DELETE_ORDER_SINGLE_REQUEST                  10109		// < DeleteOrderSingleRequest (Cancel Order Single)
#define	TID_FORCED_LOGOUT_NOTIFICATION                   10012		// < ForcedLogoutNotification (Session Logout Notification)
#define	TID_GATEWAY_REQUEST                              10020		// < GatewayRequest (Connection Gateway Request)
#define	TID_GATEWAY_RESPONSE                             10022		// < GatewayResponse (Connection Gateway Response)
#define	TID_GROUP_LEVEL_LIMIT_BROADCAST                  10051		// < GroupLevelLimitBroadcast (Group Level Limit Trader)
#define	TID_GW_ORDER_ACKNOWLEDGEMENT                     10990		// < GwOrderAcknowledgement (Order Confirmation)
#define	TID_HEARTBEAT                                    10011		// < Heartbeat (Heartbeat)
#define	TID_HEARTBEAT_NOTIFICATION                       10023		// < HeartbeatNotification (Heartbeat Notification)
#define	TID_INQUIRE_ENRICHMENT_RULE_ID_LIST_REQUEST      10040		// < InquireEnrichmentRuleIDListRequest (Trade Enrichment List Inquire)
#define	TID_INQUIRE_ENRICHMENT_RULE_ID_LIST_RESPONSE     10041		// < InquireEnrichmentRuleIDListResponse (Trade Enrichment List Inquire Response)
#define	TID_INQUIRE_MM_PARAMETER_REQUEST                 10305		// < InquireMMParameterRequest (Inquire Market Maker Parameters)
#define	TID_INQUIRE_MM_PARAMETER_RESPONSE                10306		// < InquireMMParameterResponse (Inquire Market Maker Parameters Response)
#define	TID_INQUIRE_SESSION_LIST_REQUEST                 10035		// < InquireSessionListRequest (Session List Inquire)
#define	TID_INQUIRE_SESSION_LIST_RESPONSE                10036		// < InquireSessionListResponse (Session List Inquire Response)
#define	TID_INQUIRE_USER_REQUEST                         10038		// < InquireUserRequest (User List Inquire)
#define	TID_INQUIRE_USER_RESPONSE                        10039		// < InquireUserResponse (User List Inquire Response)
#define	TID_INSTRUMENT_LEVEL_LIMIT_BROADCAST             10052		// < InstrumentLevelLimitBroadcast (Instrument Level Limit Trader)
#define	TID_LEGAL_NOTIFICATION_BROADCAST                 10037		// < LegalNotificationBroadcast (Legal Notification)
#define	TID_LOGON_REQUEST                                10000		// < LogonRequest (Session Logon)
#define TID_SECURE_REGISTRATION                          10053
#define	TID_LOGON_RESPONSE                               10001		// < LogonResponse (Session Logon Response)
#define	TID_LOGOUT_REQUEST                               10002		// < LogoutRequest (Session Logout)
#define	TID_LOGOUT_RESPONSE                              10003		// < LogoutResponse (Session Logout Response)
#define	TID_MM_PARAMETER_DEFINITION_REQUEST              10303		// < MMParameterDefinitionRequest (Market Maker Parameter Definition)
#define	TID_MM_PARAMETER_DEFINITION_RESPONSE             10304		// < MMParameterDefinitionResponse (Market Maker Parameter Definition Response)
#define	TID_MARKET_PICTURE_INQUIRY_REQUEST               10046		// < MarketPictureInquiryRequest (Market Picture Query )
#define	TID_MARKET_PICTURE_INQUIRY_RESPONSE              10047		// < MarketPictureInquiryResponse (Market Picture Query (report))
#define	TID_MASS_QUOTE_REQUEST                           10405		// < MassQuoteRequest (Mass Quote Request)
#define	TID_MASS_QUOTE_RESPONSE                          10406		// < MassQuoteResponse (Mass Quote Response)
#define	TID_MODIFY_ORDER_COMPLEX_REQUEST                 10114		// < ModifyOrderComplexRequest (Replace Order Complex)
#define	TID_MODIFY_ORDER_NR_RESPONSE                     10108		// < ModifyOrderNRResponse (Replace Order Response (Lean Order))
#define	TID_MODIFY_ORDER_RESPONSE                        10107		// < ModifyOrderResponse (Replace Order Response (Standard Order))
#define	TID_MODIFY_ORDER_SINGLE_REQUEST                  10106		// < ModifyOrderSingleRequest (Replace Order Single)
#define	TID_MODIFY_ORDER_SINGLE_SHORT_REQUEST            10126		// < ModifyOrderSingleShortRequest (Replace Order Single (short layout))
#define	TID_MULTI_LEG_EXEC_REPORT_BROADCAST              10994		// < MultiLegExecReportBroadcast (Extended Order Information(MultiLeg))
#define	TID_MULTI_LEG_EXEC_RESPONSE                      10993		// < MultiLegExecResponse (Immediate Execution Response(MultiLeg))
#define	TID_MULTI_LEG_ORDER_REJECT                       10992		// < MultiLegOrderReject (Reject(MultiLeg))
#define	TID_MULTI_LEG_ORDER_REQUEST                      10991		// < MultiLegOrderRequest (New Order MultiLeg)
#define	TID_NEW_ORDER_COMPLEX_REQUEST                    10113		// < NewOrderComplexRequest (New Order Complex)
#define	TID_NEW_ORDER_NR_RESPONSE                        10102		// < NewOrderNRResponse (New Order Response (Lean Order))
#define	TID_NEW_ORDER_RESPONSE                           10101		// < NewOrderResponse (New Order Response (Standard Order))
#define	TID_NEW_ORDER_SINGLE_REQUEST                     10100		// < NewOrderSingleRequest (New Order Single)
#define	TID_NEW_ORDER_SINGLE_SHORT_REQUEST               10125		// < NewOrderSingleShortRequest (New Order Single (short layout))
#define	TID_NEWS_BROADCAST                               10031		// < NewsBroadcast (News)
#define	TID_ORDER_BOOK_INQUIRY_REQUEST                   10999		// < OrderBookInquiryRequest (Order book inquiry)
#define	TID_ORDER_BOOK_INQUIRY_RESPONSE                  10998		// < OrderBookInquiryResponse (Order book inquiry (report))
#define	TID_ORDER_EXEC_NOTIFICATION                      10104		// < OrderExecNotification (Book Order Execution)
#define	TID_ORDER_EXEC_REPORT_BROADCAST                  10117		// < OrderExecReportBroadcast (Extended Order Information)
#define	TID_ORDER_EXEC_RESPONSE                          10103		// < OrderExecResponse (Immediate Execution Response)
#define	TID_PARTY_ACTION_REPORT                          10042		// < PartyActionReport (Party Action Report)
#define	TID_PARTY_ENTITLEMENTS_UPDATE_REPORT             10034		// < PartyEntitlementsUpdateReport (Entitlement Notification)
#define	TID_QUOTE_ACTIVATION_NOTIFICATION                10411		// < QuoteActivationNotification (Quote Activation Notification)
#define	TID_QUOTE_ACTIVATION_REQUEST                     10403		// < QuoteActivationRequest (Quote Activation Request)
#define	TID_QUOTE_ACTIVATION_RESPONSE                    10404		// < QuoteActivationResponse (Quote Activation Response)
#define	TID_QUOTE_EXEC_REPORT_BROADCAST                  10412		// < QuoteExecReportBroadcast (Quote Cancellation Notification)
#define	TID_QUOTE_EXECUTION_REPORT                       10407		// < QuoteExecutionReport (Quote Execution Notification)
#define	TID_RFQ_REQUEST                                  10401		// < RFQRequest (Quote Request)
#define	TID_RFQ_RESPONSE                                 10402		// < RFQResponse (Quote Request Response)
#define	TID_REJECT                                       10010		// < Reject (Reject)
#define	TID_RETRANSMIT_ME_MESSAGE_REQUEST                10026		// < RetransmitMEMessageRequest (Retransmit (Order/Quote Event))
#define	TID_RETRANSMIT_ME_MESSAGE_RESPONSE               10027		// < RetransmitMEMessageResponse (Retransmit Response (Order/Quote Event))
#define	TID_RETRANSMIT_REQUEST                           10008		// < RetransmitRequest (Retransmit)
#define	TID_RETRANSMIT_RESPONSE                          10009		// < RetransmitResponse (Retransmit Response)
#define	TID_RISK_COLLATERAL_ALERT_ADMIN_BROADCAST        10048		// < RiskCollateralAlertAdminBroadcast (Risk Collateral Alert Admin)
#define	TID_RISK_COLLATERAL_ALERT_BROADCAST              10049		// < RiskCollateralAlertBroadcast (Risk Collateral Alert Trader)
#define	TID_RISK_NOTIFICATION_BROADCAST                  10033		// < RiskNotificationBroadcast (Risk Notification)
#define	TID_SERVICE_AVAILABILITY_BROADCAST               10030		// < ServiceAvailabilityBroadcast (Service Availability)
#define	TID_SESSION_PASSWORD_CHANGE_REQUEST              10997		// < SessionPasswordChangeRequest (Session Password Change)
#define	TID_SESSION_PASSWORD_CHANGE_RESPONSE             10995		// < SessionPasswordChangeResponse (Session Password Change Response)
#define	TID_SUBSCRIBE_REQUEST                            10025		// < SubscribeRequest (Subscribe)
#define	TID_SUBSCRIBE_RESPONSE                           10005		// < SubscribeResponse (Subscribe Response)
#define	TID_TM_TRADING_SESSION_STATUS_BROADCAST          10501		// < TMTradingSessionStatusBroadcast (Trade Notification Status)
#define	TID_THROTTLE_UPDATE_NOTIFICATION                 10028		// < ThrottleUpdateNotification (Throttle Update Notification)
#define	TID_TRADE_BROADCAST                              10500		// < TradeBroadcast (Trade Notification)
#define	TID_TRADE_ENHANCEMENT_BROADCAST                  10989		// < TradeEnhancementBroadcast (Trade Enhancement Notification)
#define	TID_TRADING_SESSION_STATUS_BROADCAST             10307		// < TradingSessionStatusBroadcast (Trading Session Event)
#define	TID_UNSUBSCRIBE_REQUEST                          10006		// < UnsubscribeRequest (Unsubscribe)
#define	TID_UNSUBSCRIBE_RESPONSE                         10007		// < UnsubscribeResponse (Unsubscribe Response)
#define	TID_USER_AND_SESSION_PASSWORD_CHANGE_REQUEST     10044		// < UserAndSessionPasswordChangeRequest (User and Session Password Change Request)
#define	TID_USER_AND_SESSION_PASSWORD_CHANGE_RESPONSE    10045		// < UserAndSessionPasswordChangeResponse (User and Session Password Change Response)
#define	TID_USER_LEVEL_LIMIT_BROADCAST                   10050		// < UserLevelLimitBroadcast (User Level Limit Trader)
#define	TID_USER_LOGIN_REQUEST                           10018		// < UserLoginRequest (User Logon)
#define	TID_USER_LOGIN_RESPONSE                          10019		// < UserLoginResponse (User Logon Response)
#define	TID_USER_LOGOUT_REQUEST                          10029		// < UserLogoutRequest (User Logout)
#define	TID_USER_LOGOUT_RESPONSE                         10024		// < UserLogoutResponse (User Logout Response)
#define	TID_USER_PASSWORD_CHANGE_REQUEST                 10996		// < UserPasswordChangeRequest (User Password Change)
#define	TID_USER_PASSWORD_CHANGE_RESPONSE                10043		// < UserPasswordChangeResponse (User Password Change Response)
#define TID_SESSION_REGISTRATION_RESPONSE                10054

//================================== Our defines ========================//

#define ETI_SESSION_LOGON_START_SEQUENCE 1
#define ETI_CONNECTION_GATEWAY_REQUEST_START_SEQUENCE 1
#define MAX_BSE_RESPONSE_BUFFER_SIZE 65536
#define MAX_BSE_REQUEST_BUFFER_SIZE 65536

#define ETI_SESSION_STATUS_ACTIVE 0
#define BSE_SESSION_STATUS_LOGOUT 4

#define ETI_FIXED_DECIMAL_OFFSET_MULTIPLIER_FOR_PRICE 100000000

#define ETI_NEW_ORDER_STATUS_ADDED '0'
#define ETI_NEW_ORDER_STATUS_CALCELLED '4'
#define ETI_NEW_ORDER_STATUS_SUSPENDED '9'

#define ETI_NEW_ORDER_EXEC_TYPE_NEW '0'
#define ETI_NEW_ORDER_EXEC_TYPE_CANCELLED '4'
#define ETI_NEW_ORDER_EXEC_TYPE_TRIGGERED 'L'

#define ETI_UNSIGNED_1_BYTE_INT_NOVALUE 0xFF
#define ETI_UNSIGNED_2_BYTE_INT_NOVALUE 0xFFFF
#define ETI_UNSIGNED_4_BYTE_INT_NOVALUE 0xFFFFFFFF
#define ETI_UNSIGNED_8_BYTE_INT_NOVALUE 0xFFFFFFFFFFFFFFFF

#define ETI_SIGNED_1_BYTE_INT_NOVALUE 0x80
#define ETI_SIGNED_2_BYTE_INT_NOVALUE 0x8000
#define ETI_SIGNED_4_BYTE_INT_NOVALUE 0x80000000
#define ETI_SIGNED_8_BYTE_INT_NOVALUE 0x8000000000000000

#define BSE_RESPONSE_HEADER_LENGTH 8
#define BSE_REQUEST_HEADER_LENGTH 15

/*
 * Max defines for sequences defines
 */
#define MAX_ADD_COMPLEX_INSTRUMENT_REQUEST_INSTRMT_LEG_GRP	5
#define MAX_ADD_COMPLEX_INSTRUMENT_RESPONSE_INSTRMT_LEG_GRP	5
#define MAX_DELETE_ALL_ORDER_BROADCAST_NOT_AFFECTED_ORDERS_GRP	500
#define MAX_DELETE_ALL_ORDER_RESPONSE_NOT_AFFECTED_ORDERS_GRP	500
#define MAX_DELETE_ALL_QUOTE_BROADCAST_NOT_AFFECTED_SECURITIES_GRP	500
#define MAX_DELETE_ALL_QUOTE_RESPONSE_NOT_AFFECTED_SECURITIES_GRP	500
#define MAX_GROUP_LEVEL_LIMIT_BROADCAST_GROUP_LEVEL_DATA	100
#define MAX_INQUIRE_ENRICHMENT_RULE_ID_LIST_RESPONSE_ENRICHMENT_RULES_GRP	400
#define MAX_INQUIRE_MM_PARAMETER_RESPONSE_MM_PARAMETER_GRP	6
#define MAX_INQUIRE_SESSION_LIST_RESPONSE_SESSIONS_GRP  	1000
#define MAX_INQUIRE_USER_RESPONSE_PARTY_DETAILS_GRP     	1000
#define MAX_INSTRUMENT_LEVEL_LIMIT_BROADCAST_INSTRUMENT_LEVEL_DATA	100
#define MAX_MARKET_PICTURE_INQUIRY_RESPONSE_BEST_RATE_GRP	5
#define MAX_MASS_QUOTE_REQUEST_QUOTE_ENTRY_GRP          	100
#define MAX_MASS_QUOTE_RESPONSE_QUOTE_ENTRY_ACK_GRP     	100
#define MAX_MODIFY_ORDER_COMPLEX_REQUEST_LEG_ORD_GRP    	5
#define MAX_MULTI_LEG_EXEC_REPORT_BROADCAST_MULTI_LEG_GRP	99
#define MAX_MULTI_LEG_EXEC_REPORT_BROADCAST_MULTI_LEG_FILL_GRP	100
#define MAX_MULTI_LEG_EXEC_REPORT_BROADCAST_INSTRMNT_LEG_EXEC_GRP	600
#define MAX_MULTI_LEG_EXEC_RESPONSE_MULTI_LEG_EXEC_GRP  	99
#define MAX_MULTI_LEG_EXEC_RESPONSE_MULTI_LEG_FILL_GRP  	100
#define MAX_MULTI_LEG_EXEC_RESPONSE_INSTRMNT_LEG_EXEC_GRP	600
#define MAX_MULTI_LEG_ORDER_REQUEST_MULTI_LEG_ORD_GRP   	99
#define MAX_NEW_ORDER_COMPLEX_REQUEST_LEG_ORD_GRP       	5
#define MAX_ORDER_BOOK_INQUIRY_RESPONSE_OB_INQUIRY_GRP  	24
#define MAX_ORDER_EXEC_NOTIFICATION_FILLS_GRP           	100
#define MAX_ORDER_EXEC_NOTIFICATION_INSTRMNT_LEG_EXEC_GRP	600
#define MAX_ORDER_EXEC_REPORT_BROADCAST_LEG_ORD_GRP     	5
#define MAX_ORDER_EXEC_REPORT_BROADCAST_FILLS_GRP       	100
#define MAX_ORDER_EXEC_REPORT_BROADCAST_INSTRMNT_LEG_EXEC_GRP	600
#define MAX_ORDER_EXEC_RESPONSE_FILLS_GRP               	100
#define MAX_ORDER_EXEC_RESPONSE_INSTRMNT_LEG_EXEC_GRP   	600
#define MAX_QUOTE_ACTIVATION_NOTIFICATION_NOT_AFFECTED_SECURITIES_GRP	500
#define MAX_QUOTE_ACTIVATION_RESPONSE_NOT_AFFECTED_SECURITIES_GRP	500
#define MAX_QUOTE_EXEC_REPORT_BROADCAST_QUOTE_EVENT_GRP 	100
#define MAX_QUOTE_EXECUTION_REPORT_QUOTE_EVENT_GRP      	100
#define MAX_QUOTE_EXECUTION_REPORT_QUOTE_LEG_EXEC_GRP   	600

/*
 * Data Type defines
 */

// DataType Account
#define LEN_ACCOUNT                                      2

// DataType AccountType
#define ENUM_ACCOUNT_TYPE_OWN                            20
#define ENUM_ACCOUNT_TYPE_CLIENT                         30
#define ENUM_ACCOUNT_TYPE_SPL_CLI                        40
#define ENUM_ACCOUNT_TYPE_INST                           90

// DataType AccruedInterestAmt

// DataType ActivityTime

// DataType AggressorIndicator
#define ENUM_AGGRESSOR_INDICATOR_PASSIVE                 0
#define ENUM_AGGRESSOR_INDICATOR_AGRESSOR                1

// DataType AggressorSide
#define ENUM_AGGRESSOR_SIDE_BUY                          1
#define ENUM_AGGRESSOR_SIDE_SELL                         2

// DataType AlgoID
#define LEN_ALGOID                                       16

// DataType AllOrNoneFlag
#define LEN_ALL_OR_NONE_FLAG                             1
#define ENUM_ALL_OR_NONE_FLAG_USE_ALL_OR_NONE            "Y"
#define ENUM_ALL_OR_NONE_FLAG_USE_ALL_OR_NONE_CHAR       'Y'
#define ENUM_ALL_OR_NONE_FLAG_USE_ALL_OR_NONE_NOT        "N"
#define ENUM_ALL_OR_NONE_FLAG_USE_ALL_OR_NONE_NOT_CHAR   'N'

// DataType ApplBegMsgID
#define LEN_APPL_BEG_MSGID                               16

// DataType ApplBegSeqNum

// DataType ApplEndMsgID
#define LEN_APPL_END_MSGID                               16

// DataType ApplEndSeqNum

// DataType ApplID
#define ENUM_APPLID_TRADE_ENHANCEMENT                    0
#define ENUM_APPLID_TRADE                                1
#define ENUM_APPLID_NEWS                                 2
#define ENUM_APPLID_SERVICE_AVAILABILITY                 3
#define ENUM_APPLID_SESSION_DATA                         4
#define ENUM_APPLID_LISTENER_DATA                        5
#define ENUM_APPLID_RISK_CONTROL                         6
#define ENUM_APPLID_ORDER_BY_ORDER                       7
#define ENUM_APPLID_RISK_ADMIN                           8
#define ENUM_APPLID_USER_LEVEL                           9
#define ENUM_APPLID_GROUP_LEVEL                          10
#define ENUM_APPLID_INSTRUMENT_LEVEL                     11

// DataType ApplIDStatus
#define ENUM_APPL_ID_STATUS_OUTBOUND_CONVERSION_ERROR    105

// DataType ApplMsgID
#define LEN_APPL_MSGID                                   16

// DataType ApplResendFlag
#define ENUM_APPL_RESEND_FLAG_FALSE                      0
#define ENUM_APPL_RESEND_FLAG_TRUE                       1

// DataType ApplSeqIndicator
#define ENUM_APPL_SEQ_INDICATOR_NO_RECOVERY_REQUIRED     0
#define ENUM_APPL_SEQ_INDICATOR_RECOVERY_REQUIRED        1

// DataType ApplSeqNum

// DataType ApplSeqStatus
#define ENUM_APPL_SEQ_STATUS_UNAVAILABLE                 0
#define ENUM_APPL_SEQ_STATUS_AVAILABLE                   1

// DataType ApplSeqTradeDate

// DataType ApplSubID

// DataType ApplTotalMessageCount

// DataType ApplUsageOrders
#define LEN_APPL_USAGE_ORDERS                            1
#define ENUM_APPL_USAGE_ORDERS_AUTOMATED                 "A"
#define ENUM_APPL_USAGE_ORDERS_AUTOMATED_CHAR            'A'
#define ENUM_APPL_USAGE_ORDERS_MANUAL                    "M"
#define ENUM_APPL_USAGE_ORDERS_MANUAL_CHAR               'M'
#define ENUM_APPL_USAGE_ORDERS_AUTO_SELECT               "B"
#define ENUM_APPL_USAGE_ORDERS_AUTO_SELECT_CHAR          'B'
#define ENUM_APPL_USAGE_ORDERS_NONE                      "N"
#define ENUM_APPL_USAGE_ORDERS_NONE_CHAR                 'N'

// DataType ApplUsageQuotes
#define LEN_APPL_USAGE_QUOTES                            1
#define ENUM_APPL_USAGE_QUOTES_AUTOMATED                 "A"
#define ENUM_APPL_USAGE_QUOTES_AUTOMATED_CHAR            'A'
#define ENUM_APPL_USAGE_QUOTES_MANUAL                    "M"
#define ENUM_APPL_USAGE_QUOTES_MANUAL_CHAR               'M'
#define ENUM_APPL_USAGE_QUOTES_AUTO_SELECT               "B"
#define ENUM_APPL_USAGE_QUOTES_AUTO_SELECT_CHAR          'B'
#define ENUM_APPL_USAGE_QUOTES_NONE                      "N"
#define ENUM_APPL_USAGE_QUOTES_NONE_CHAR                 'N'

// DataType ApplicationSystemName
#define LEN_APPLICATION_SYSTEM_NAME                      30

// DataType ApplicationSystemVendor
#define LEN_APPLICATION_SYSTEM_VENDOR                    30

// DataType ApplicationSystemVersion
#define LEN_APPLICATION_SYSTEM_VERSION                   30

// DataType AskPriceLevel

// DataType AutoAcceptIndicator
#define LEN_AUTO_ACCEPT_INDICATOR                        1
#define ENUM_AUTO_ACCEPT_INDICATOR_ACCEPTED              "Y"
#define ENUM_AUTO_ACCEPT_INDICATOR_ACCEPTED_CHAR         'Y'
#define ENUM_AUTO_ACCEPT_INDICATOR_REJECTED              "N"
#define ENUM_AUTO_ACCEPT_INDICATOR_REJECTED_CHAR         'N'

// DataType BidCxlSize

// DataType BidPriceLevel

// DataType BidPx

// DataType BidSize

// DataType BidYTC

// DataType BidYTM

// DataType BidYTP

// DataType BlockdealReferencePx

// DataType BodyLen

// DataType BusinessUnitSymbol
#define LEN_BUSINESS_UNIT_SYMBOL                         8

// DataType CPCode
#define LEN_CP_CODE                                      12

// DataType ClOrdID

// DataType ClearingTradePrice

// DataType ClearingTradeQty

// DataType ClosePx

// DataType ContraBroker
#define LEN_CONTRA_BROKER                                5

// DataType CumQty

// DataType CustOrderHandlingInst
#define LEN_CUST_ORDER_HANDLING_INST                     1

// DataType CxlQty

// DataType DaysLeftForPasswdExpiry

// DataType DefaultBuyQuantity

// DataType DefaultCstmApplVerID
#define LEN_DEFAULT_CSTM_APPL_VERID                      30

// DataType DefaultSellQuantity

// DataType Delta

// DataType DeltaQtyFlag
#define LEN_DELTA_QTY_FLAG                               1
#define ENUM_DELTA_QTY_FLAG_DIFFERENTIAL_QTY             "D"
#define ENUM_DELTA_QTY_FLAG_DIFFERENTIAL_QTY_CHAR        'D'

// DataType Duration
#define ENUM_DURATION_NEAR                               1
#define ENUM_DURATION_MID                                2
#define ENUM_DURATION_FAR                                3

// DataType EnrichmentRuleID

// DataType EquiPx

// DataType EquiSize

// DataType ExecID

// DataType ExecInst
#define ENUM_EXEC_INST_H                                 1
#define ENUM_EXEC_INST_Q                                 2
#define ENUM_EXEC_INST_H_Q                               3
#define ENUM_EXEC_INST_H_6                               5
#define ENUM_EXEC_INST_Q_6                               6

// DataType ExecRestatementReason
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_BOOK_RESTATEMENT 001
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_ADDED         101
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_MODIFIED      102
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_CANCELLED     103
#define ENUM_EXEC_RESTATEMENT_REASON_IOC_ORDER_CANCELLED 105
#define ENUM_EXEC_RESTATEMENT_REASON_BOOK_ORDER_EXECUTED 108
#define ENUM_EXEC_RESTATEMENT_REASON_MARKET_ORDER_TRIGGERED 135
#define ENUM_EXEC_RESTATEMENT_REASON_CAO_ORDER_ACTIVATED 149
#define ENUM_EXEC_RESTATEMENT_REASON_CAO_ORDER_INACTIVATED 150
#define ENUM_EXEC_RESTATEMENT_REASON_OCO_ORDER_TRIGGERED 164
#define ENUM_EXEC_RESTATEMENT_REASON_STOP_ORDER_TRIGGERED 172
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_CANCELLATION_PENDING 197
#define ENUM_EXEC_RESTATEMENT_REASON_PENDING_CANCELLATION_EXECUTED 199
#define ENUM_EXEC_RESTATEMENT_REASON_BOC_ORDER_CANCELLED 212
#define ENUM_EXEC_RESTATEMENT_REASON_PENDING_NORMAL_ORDERS_QUERIED 213
#define ENUM_EXEC_RESTATEMENT_REASON_PENDING_STOPLOSS_ORDERS_QUERIED 214
#define ENUM_EXEC_RESTATEMENT_REASON_RRM_ORDER_ADDED     215
#define ENUM_EXEC_RESTATEMENT_REASON_RRM_ORDER_ACCEPTED  216
#define ENUM_EXEC_RESTATEMENT_REASON_RRM_ORDER_UPDATION_REJECTED 217
#define ENUM_EXEC_RESTATEMENT_REASON_RRM_ORDER_UPDATED_SUCCESSFULLY 218
#define ENUM_EXEC_RESTATEMENT_REASON_RRM_ORDER_DELETED   219
#define ENUM_EXEC_RESTATEMENT_REASON_RRM_MARKET_ORDER_TRIGGERED 220
#define ENUM_EXEC_RESTATEMENT_REASON_PROV_ORDER_ADDED    221
#define ENUM_EXEC_RESTATEMENT_REASON_PROV_ORDER_ACCEPTED 222
#define ENUM_EXEC_RESTATEMENT_REASON_PROV_ORDER_UPDATION_REJECTED 223
#define ENUM_EXEC_RESTATEMENT_REASON_PROV_ORDER_UPDATED_SUCCESSFULLY 224
#define ENUM_EXEC_RESTATEMENT_REASON_PROV_ORDER_DELETED  225
#define ENUM_EXEC_RESTATEMENT_REASON_PROV_MARKET_ORDER_TRIGGERED 226
#define ENUM_EXEC_RESTATEMENT_REASON_CAL_LAUC_ORDER_ADDED 227
#define ENUM_EXEC_RESTATEMENT_REASON_CAL_LAUC_ORDER_ACCEPTED 228
#define ENUM_EXEC_RESTATEMENT_REASON_CAL_LAUC_ORDER_UPDATION_REJECTED 229
#define ENUM_EXEC_RESTATEMENT_REASON_CAL_LAUC_ORDER_UPDATED_SUCCESSFULLY 230
#define ENUM_EXEC_RESTATEMENT_REASON_CAL_LAUC_ORDER_DELETED 231
#define ENUM_EXEC_RESTATEMENT_REASON_GTCL_ORDER_DELETED  232
#define ENUM_EXEC_RESTATEMENT_REASON_EOD_ORDER_DELETED   233
#define ENUM_EXEC_RESTATEMENT_REASON_HALT_ORDER_DELETED  234
#define ENUM_EXEC_RESTATEMENT_REASON_BLOCK_DEAL_ORDER_TIMED_OUT 235
#define ENUM_EXEC_RESTATEMENT_REASON_OUT_OF_PRICE_BAND_ORDER 236
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_WORSE_THAN_CLOSE_PRICE 237
#define ENUM_EXEC_RESTATEMENT_REASON_AUCTION_MARKET_ORDER_TRIGGERED 238
#define ENUM_EXEC_RESTATEMENT_REASON_PENDING_BLOCKDEAL_ORDERS_QUERIED 239
#define ENUM_EXEC_RESTATEMENT_REASON_BU_SUSPENDED        240
#define ENUM_EXEC_RESTATEMENT_REASON_COOLING_OFF_RRM_ORDER_DELETED 241
#define ENUM_EXEC_RESTATEMENT_REASON_MWPL_RRM_ORDER_DELETED 242
#define ENUM_EXEC_RESTATEMENT_REASON_CALL_AUCTION_UNCROSS_ORDER_DELETED 243
#define ENUM_EXEC_RESTATEMENT_REASON_COLLATERAL_RRM      244
#define ENUM_EXEC_RESTATEMENT_REASON_PENDING_MARKET_ORDERS_QUERIED 245
#define ENUM_EXEC_RESTATEMENT_REASON_SELF_TRADE_ORDER_DELETED 246
#define ENUM_EXEC_RESTATEMENT_REASON_REVERSE_TRADE_ORDER_DELETED 247
#define ENUM_EXEC_RESTATEMENT_REASON_PENDING_OCO_ORDERS_QUERIED 248
#define ENUM_EXEC_RESTATEMENT_REASON_NLT_ORDER_TIMED_OUT 249
#define ENUM_EXEC_RESTATEMENT_REASON_CLIENT_RRM_FOR_PRODUCT 250
#define ENUM_EXEC_RESTATEMENT_REASON_CLIENT_SUSPENDED    251
#define ENUM_EXEC_RESTATEMENT_REASON_CLIENT_RRM_FOR_CONTRACT 252
#define ENUM_EXEC_RESTATEMENT_REASON_MEMBER_RRM_FOR_CONTRACT 253
#define ENUM_EXEC_RESTATEMENT_REASON_CLIENT_RRM          254

// DataType ExecType
#define LEN_EXEC_TYPE                                    1
#define ENUM_EXEC_TYPE_NEW                               "0"
#define ENUM_EXEC_TYPE_NEW_CHAR                          '0'
#define ENUM_EXEC_TYPE_CANCELED                          "4"
#define ENUM_EXEC_TYPE_CANCELED_CHAR                     '4'
#define ENUM_EXEC_TYPE_REPLACED                          "5"
#define ENUM_EXEC_TYPE_REPLACED_CHAR                     '5'
#define ENUM_EXEC_TYPE_PENDING_CANCEL_E                  "6"
#define ENUM_EXEC_TYPE_PENDING_CANCEL_E_CHAR             '6'
#define ENUM_EXEC_TYPE_SUSPENDED                         "9"
#define ENUM_EXEC_TYPE_SUSPENDED_CHAR                    '9'
#define ENUM_EXEC_TYPE_RESTATED                          "D"
#define ENUM_EXEC_TYPE_RESTATED_CHAR                     'D'
#define ENUM_EXEC_TYPE_TRIGGERED                         "L"
#define ENUM_EXEC_TYPE_TRIGGERED_CHAR                    'L'
#define ENUM_EXEC_TYPE_TRADE                             "F"
#define ENUM_EXEC_TYPE_TRADE_CHAR                        'F'
#define ENUM_EXEC_TYPE_RRM_ACCEPT                        "M"
#define ENUM_EXEC_TYPE_RRM_ACCEPT_CHAR                   'M'
#define ENUM_EXEC_TYPE_RRM_REJECT                        "N"
#define ENUM_EXEC_TYPE_RRM_REJECT_CHAR                   'N'
#define ENUM_EXEC_TYPE_PROV_ACCEPT                       "X"
#define ENUM_EXEC_TYPE_PROV_ACCEPT_CHAR                  'X'
#define ENUM_EXEC_TYPE_PROV_REJECT                       "Y"
#define ENUM_EXEC_TYPE_PROV_REJECT_CHAR                  'Y'

// DataType ExpireDate

// DataType ExposureDuration

// DataType FIXEngineName
#define LEN_FIX_ENGINE_NAME                              30

// DataType FIXEngineVendor
#define LEN_FIX_ENGINE_VENDOR                            30

// DataType FIXEngineVersion
#define LEN_FIX_ENGINE_VERSION                           30

// DataType FillDirtyPx

// DataType FillExecID

// DataType FillLiquidityInd
#define ENUM_FILL_LIQUIDITY_IND_ADDED_LIQUIDITY          1
#define ENUM_FILL_LIQUIDITY_IND_REMOVED_LIQUIDITY        2
#define ENUM_FILL_LIQUIDITY_IND_TRIGGERED_STOP_ORDER     5
#define ENUM_FILL_LIQUIDITY_IND_TRIGGERED_OCO_ORDER      6
#define ENUM_FILL_LIQUIDITY_IND_TRIGGERED_MARKET_ORDER   7

// DataType FillMatchID

// DataType FillPx

// DataType FillQty

// DataType FillYield

// DataType Filler1

// DataType Filler2

// DataType Filler3

// DataType Filler4

// DataType Filler5

// DataType FirstFragment
#define ENUM_FIRST_FRAGMENT_NOT_FIRST_MESSAGE            0
#define ENUM_FIRST_FRAGMENT_FIRST_MESSAGE                1

// DataType FreeText1
#define LEN_FREE_TEXT1                                   12

// DataType FreeText2
#define LEN_FREE_TEXT2                                   12

// DataType FreeText3
#define LEN_FREE_TEXT3                                   12

// DataType FuncCategory
#define LEN_FUNC_CATEGORY                                100
#define ENUM_FUNC_CATEGORY_ORDER_HANDLING                "Order Handling                                                                                      "
#define ENUM_FUNC_CATEGORY_SESSION_LAYER                 "Session Layer                                                                                       "
#define ENUM_FUNC_CATEGORY_QUOTE_HANDLING                "Quote Handling                                                                                      "
#define ENUM_FUNC_CATEGORY_QUOTE_AND_CROSS_REQUEST       "Quote and Cross Request                                                                             "
#define ENUM_FUNC_CATEGORY_STRATEGY_CREATION             "Strategy Creation                                                                                   "
#define ENUM_FUNC_CATEGORY_OTHER                         "Other                                                                                               "
#define ENUM_FUNC_CATEGORY_BROADCAST                     "Broadcast                                                                                           "

// DataType GatewayID

// DataType GatewaySubID

// DataType GraceLoginsLeft

// DataType GrossBuyLimit

// DataType GrossSellLimit

// DataType GrossTradeAmt

// DataType Group
#define LEN_GROUP                                        2

// DataType GroupLevelLimitFlag
#define LEN_GROUP_LEVEL_LIMIT_FLAG                       1
#define ENUM_GROUP_LEVEL_LIMIT_FLAG_USE_GROUP_LEVEL_LIMIT "Y"
#define ENUM_GROUP_LEVEL_LIMIT_FLAG_USE_GROUP_LEVEL_LIMIT_CHAR 'Y'
#define ENUM_GROUP_LEVEL_LIMIT_FLAG_USE_GROUP_LEVEL_LIMIT_NOT "N"
#define ENUM_GROUP_LEVEL_LIMIT_FLAG_USE_GROUP_LEVEL_LIMIT_NOT_CHAR 'N'

// DataType Headline
#define LEN_HEADLINE                                     256

// DataType HeartBtInt

// DataType HighLimitPrice

// DataType HighPx

// DataType ImpliedMarketIndicator
#define ENUM_IMPLIED_MARKET_INDICATOR_NOT_IMPLIED        0
#define ENUM_IMPLIED_MARKET_INDICATOR_IMPLIED_IN_OUT     3

// DataType IncrementDecrementStatus
#define ENUM_INCREMENT_DECREMENT_STATUS_INCREMENT        1
#define ENUM_INCREMENT_DECREMENT_STATUS_NO_CHANGE        0
#define ENUM_INCREMENT_DECREMENT_STATUS_DECREMENT        -1

// DataType InstrumentLevelLimitFlag
#define LEN_INSTRUMENT_LEVEL_LIMIT_FLAG                  1
#define ENUM_INSTRUMENT_LEVEL_LIMIT_FLAG_USE_INSTRUMENT_LEVEL_LIMIT "Y"
#define ENUM_INSTRUMENT_LEVEL_LIMIT_FLAG_USE_INSTRUMENT_LEVEL_LIMIT_CHAR 'Y'
#define ENUM_INSTRUMENT_LEVEL_LIMIT_FLAG_USE_INSTRUMENT_LEVEL_LIMIT_NOT "N"
#define ENUM_INSTRUMENT_LEVEL_LIMIT_FLAG_USE_INSTRUMENT_LEVEL_LIMIT_NOT_CHAR 'N'

// DataType LastEntityProcessed
#define LEN_LAST_ENTITY_PROCESSED                        16

// DataType LastFragment
#define ENUM_LAST_FRAGMENT_NOT_LAST_MESSAGE              0
#define ENUM_LAST_FRAGMENT_LAST_MESSAGE                  1

// DataType LastLoginIP

// DataType LastLoginTime

// DataType LastPx

// DataType LastQty

// DataType LastUpdateTime

// DataType LastUpdatedTime

// DataType LastYTC

// DataType LastYTM

// DataType LastYTP

// DataType LeavesQty

// DataType LegAccount
#define LEN_LEG_ACCOUNT                                  2

// DataType LegExecID

// DataType LegLastPx

// DataType LegLastQty

// DataType LegPositionEffect
#define LEN_LEG_POSITION_EFFECT                          1
#define ENUM_LEG_POSITION_EFFECT_CLOSE                   "C"
#define ENUM_LEG_POSITION_EFFECT_CLOSE_CHAR              'C'
#define ENUM_LEG_POSITION_EFFECT_OPEN                    "O"
#define ENUM_LEG_POSITION_EFFECT_OPEN_CHAR               'O'

// DataType LegPrice

// DataType LegRatioQty

// DataType LegSecurityID

// DataType LegSecurityType
#define ENUM_LEG_SECURITY_TYPE_MULTILEG_INSTRUMENT       1
#define ENUM_LEG_SECURITY_TYPE_UNDERLYING_LEG            2

// DataType LegSide
#define ENUM_LEG_SIDE_BUY                                1
#define ENUM_LEG_SIDE_SELL                               2

// DataType LegSymbol

// DataType ListUpdateAction
#define LEN_LIST_UPDATE_ACTION                           1
#define ENUM_LIST_UPDATE_ACTION_ADD                      "A"
#define ENUM_LIST_UPDATE_ACTION_ADD_CHAR                 'A'
#define ENUM_LIST_UPDATE_ACTION_DELETE                   "D"
#define ENUM_LIST_UPDATE_ACTION_DELETE_CHAR              'D'

// DataType LowLimitPrice

// DataType LowPx

// DataType LowerCktPx

// DataType MMParameterReportID

// DataType MarketID
#define ENUM_MARKETID_XEUR                               1
#define ENUM_MARKETID_XEEE                               2

// DataType MarketSegmentID

// DataType MarketType

// DataType MassActionReason
#define ENUM_MASS_ACTION_REASON_NO_SPECIAL_REASON        0
#define ENUM_MASS_ACTION_REASON_STOP_TRADING             1
#define ENUM_MASS_ACTION_REASON_EMERGENCY                2
#define ENUM_MASS_ACTION_REASON_MARKET_MAKER_PROTECTION  3
#define ENUM_MASS_ACTION_REASON_STOP_BUTTON_ACTIVATED    4
#define ENUM_MASS_ACTION_REASON_BUSSINESS_UNIT_SUSPENDED 5
#define ENUM_MASS_ACTION_REASON_SESSION_LOSS             6
#define ENUM_MASS_ACTION_REASON_COLLATERAL_RRM           7
#define ENUM_MASS_ACTION_REASON_PRICE_BAND_SHRINK        8
#define ENUM_MASS_ACTION_REASON_ORDERS_WORSE_THAN_CLOSE_PRICE 9
#define ENUM_MASS_ACTION_REASON_PRODUCT_STATE_HALT       105
#define ENUM_MASS_ACTION_REASON_PRODUCT_STATE_HOLIDAY    106
#define ENUM_MASS_ACTION_REASON_INSTRUMENT_SUSPENDED     107
#define ENUM_MASS_ACTION_REASON_COMPLEX_INSTRUMENT_DELETION 109
#define ENUM_MASS_ACTION_REASON_VOLATILITY_INTERRUPTION  110
#define ENUM_MASS_ACTION_REASON_PRODUCT_TEMPORARILY_NOT_TRADABLE 111
#define ENUM_MASS_ACTION_REASON_PRODUCT_STATE_CLOSING    114
#define ENUM_MASS_ACTION_REASON_PRODUCT_STATE_EOD        115
#define ENUM_MASS_ACTION_REASON_COOLING_OFF              116
#define ENUM_MASS_ACTION_REASON_MWPL_RRM                 117

// DataType MassActionReportID

// DataType MassActionType
#define ENUM_MASS_ACTION_TYPE_SUSPEND_QUOTES             1
#define ENUM_MASS_ACTION_TYPE_RELEASE_QUOTES             2

// DataType MatchDate

// DataType MatchSubType
#define ENUM_MATCH_SUB_TYPE_OPENING_AUCTION              1
#define ENUM_MATCH_SUB_TYPE_CLOSING_AUCTION              2
#define ENUM_MATCH_SUB_TYPE_INTRADAY_AUCTION             3
#define ENUM_MATCH_SUB_TYPE_CIRCUIT_BREAKER_AUCTION      4

// DataType MatchType
#define ENUM_MATCH_TYPE_CONFIRMED_TRADE_REPORT           3
#define ENUM_MATCH_TYPE_AUTO_MATCH_INCOMING              4
#define ENUM_MATCH_TYPE_CROSS_AUCTION                    5
#define ENUM_MATCH_TYPE_CALL_AUCTION                     7
#define ENUM_MATCH_TYPE_AUTO_MATCH_RESTING               11

// DataType MatchingEngineStatus
#define ENUM_MATCHING_ENGINE_STATUS_UNAVAILABLE          0
#define ENUM_MATCHING_ENGINE_STATUS_AVAILABLE            1

// DataType MatchingEngineTradeDate

// DataType MaxBuyQuantity

// DataType MaxBuyValue

// DataType MaxPricePercentage

// DataType MaxSellQuantity

// DataType MaxSellValue

// DataType MaxShow

// DataType MemberType
#define ENUM_MEMBER_TYPE_TRADING_MEMBER                  1
#define ENUM_MEMBER_TYPE_CLEARING_MEMBER                 2
#define ENUM_MEMBER_TYPE_PROPRIETARY                     3

// DataType MessageTag

// DataType MsgSeqNum

// DataType MsgType
#define LEN_MSG_TYPE                                     3
#define ENUM_MSG_TYPE_HEARTBEAT                          "0  "
#define ENUM_MSG_TYPE_REJECT                             "3  "
#define ENUM_MSG_TYPE_LOGOUT                             "5  "
#define ENUM_MSG_TYPE_EXECUTION_REPORT                   "8  "
#define ENUM_MSG_TYPE_LOGON                              "A  "
#define ENUM_MSG_TYPE_NEW_ORDER_MULTILEG                 "AB "
#define ENUM_MSG_TYPE_MULTILEG_ORDER_CANCEL_REPLACE      "AC "
#define ENUM_MSG_TYPE_TRADE_CAPTURE_REPORT               "AE "
#define ENUM_MSG_TYPE_TRADE_CAPTURE_REPORT_ACK           "AR "
#define ENUM_MSG_TYPE_NEWS                               "B  "
#define ENUM_MSG_TYPE_USER_REQUEST                       "BE "
#define ENUM_MSG_TYPE_USER_RESPONSE                      "BF "
#define ENUM_MSG_TYPE_APPLICATION_MESSAGE_REQUEST        "BW "
#define ENUM_MSG_TYPE_APPLICATION_MESSAGE_REQUEST_ACK    "BX "
#define ENUM_MSG_TYPE_APPLICATION_MESSAGE_REPORT         "BY "
#define ENUM_MSG_TYPE_ORDER_MASS_ACTION_REPORT           "BZ "
#define ENUM_MSG_TYPE_ORDER_MASS_ACTION_REQUEST          "CA "
#define ENUM_MSG_TYPE_USER_NOTIFICATION                  "CB "
#define ENUM_MSG_TYPE_PARTY_RISK_LIMITS_UPDATE_REPORT    "CR "
#define ENUM_MSG_TYPE_PARTY_ENTITLEMENTS_UPDATE_REPORT   "CZ "
#define ENUM_MSG_TYPE_NEW_ORDER_SINGLE                   "D  "
#define ENUM_MSG_TYPE_ORDER_CANCEL_REQUEST               "F  "
#define ENUM_MSG_TYPE_ORDER_CANCEL_REPLACE_REQUEST       "G  "
#define ENUM_MSG_TYPE_QUOTE_REQUEST                      "R  "
#define ENUM_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH  "W  "
#define ENUM_MSG_TYPE_MASS_QUOTE_ACKNOWLEDGEMENT         "b  "
#define ENUM_MSG_TYPE_SECURITY_DEFINITION_REQUEST        "c  "
#define ENUM_MSG_TYPE_SECURITY_DEFINITION                "d  "
#define ENUM_MSG_TYPE_TRADING_SESSION_STATUS             "h  "
#define ENUM_MSG_TYPE_MASS_QUOTE                         "i  "
#define ENUM_MSG_TYPE_REQUEST_ACKNOWLEDGE                "U1 "
#define ENUM_MSG_TYPE_SESSION_DETAILS_LIST_REQUEST       "U5 "
#define ENUM_MSG_TYPE_SESSION_DETAILS_LIST_RESPONSE      "U6 "
#define ENUM_MSG_TYPE_QUOTE_EXECUTION_REPORT             "U8 "
#define ENUM_MSG_TYPE_MM_PARAMETER_DEFINITION_REQUEST    "U14"
#define ENUM_MSG_TYPE_CROSS_REQUEST                      "U16"
#define ENUM_MSG_TYPE_MM_PARAMETER_REQUEST               "U17"
#define ENUM_MSG_TYPE_MM_PARAMETER_RESPONSE              "U18"
#define ENUM_MSG_TYPE_PARTY_DETAIL_LIST_REQUEST          "CF "
#define ENUM_MSG_TYPE_PARTY_DETAIL_LIST_REPORT           "CG "
#define ENUM_MSG_TYPE_TRADE_ENRICHMENT_LIST_REQUEST      "U7 "
#define ENUM_MSG_TYPE_TRADE_ENRICHMENT_LIST_REPORT       "U9 "
#define ENUM_MSG_TYPE_PARTY_ACTION_REPORT                "DI "
#define ENUM_MSG_TYPE_ORDER_BOOK_INQUIRY_REQUEST         "U19"
#define ENUM_MSG_TYPE_ORDER_BOOK_INQUIRY_RESPONSE        "U20"
#define ENUM_MSG_TYPE_MARKET_PICTURE_INQUIRY_REQUEST     "U21"
#define ENUM_MSG_TYPE_MARKET_PICTURE_INQUIRY_RESPONSE    "U22"
#define ENUM_MSG_TYPE_COLLATERAL_ALERT_ADMIN_REPORT      "U23"
#define ENUM_MSG_TYPE_COLLATERAL_ALERT_REPORT            "U24"
#define ENUM_MSG_TYPE_USER_LEVEL_LIMIT_REPORT            "U25"
#define ENUM_MSG_TYPE_GROUP_LEVEL_LIMIT_REPORT           "U26"
#define ENUM_MSG_TYPE_INSTRUMENT_LEVEL_LIMIT_REPORT      "U27"
#define ENUM_MSG_TYPE_MULTI_LEG_ORDER                    "U28"
#define ENUM_MSG_TYPE_MULTI_LEG_ORDER_ACKNOWLEDGEMENT    "U29"
#define ENUM_MSG_TYPE_MULTI_LEG_EXECUTION_REPORT         "U30"
#define ENUM_MSG_TYPE_TRADE_ENHANCEMENT_STATUS           "U31"
#define ENUM_MSG_TYPE_DEBT_INQUIRY_REQUEST               "U32"
#define ENUM_MSG_TYPE_DEBT_INQUIRY_RESPONSE              "U33"

// DataType MultiLegReportingType
#define ENUM_MULTI_LEG_REPORTING_TYPE_SINGLE_SECURITY    1
#define ENUM_MULTI_LEG_REPORTING_TYPE_INDIVIDUAL_LEG_OF_A_MULTILEG_SECURITY 2

// DataType MultilegModel
#define ENUM_MULTILEG_MODEL_PREDEFINED_MULTILEG_SECURITY 0
#define ENUM_MULTILEG_MODEL_USER_DEFINED_MULTLEG         1

// DataType NetLimit

// DataType NetworkMsgID
#define LEN_NETWORK_MSGID                                8

// DataType NewPassword
#define LEN_NEW_PASSWORD                                 32

// DataType NoEnrichmentRules

// DataType NoFills

// DataType NoFillsIndex

// DataType NoGroupLevelData

// DataType NoInstrumentLevelData

// DataType NoLegExecs

// DataType NoLegs

// DataType NoMMParameters

// DataType NoMPInquiry

// DataType NoNotAffectedOrders

// DataType NoNotAffectedSecurities

// DataType NoOBInquiry

// DataType NoOfMultiLeg

// DataType NoOfMultiLegExecs

// DataType NoOfPartition

// DataType NoPartyDetails

// DataType NoQuoteEntries

// DataType NoQuoteEvents

// DataType NoQuoteEventsIndex

// DataType NoSessions

// DataType NotAffOrigClOrdID

// DataType NotAffectedOrderID

// DataType NotAffectedSecurityID

// DataType NumberOfSecurities

// DataType NumberOfTrades

// DataType OfferCxlSize

// DataType OfferPx

// DataType OfferSize

// DataType OfferYTC

// DataType OfferYTM

// DataType OfferYTP

// DataType OpenPx

// DataType OrdStatus
#define LEN_ORD_STATUS                                   1
#define ENUM_ORD_STATUS_NEW                              "0"
#define ENUM_ORD_STATUS_NEW_CHAR                         '0'
#define ENUM_ORD_STATUS_PARTIALLY_FILLED                 "1"
#define ENUM_ORD_STATUS_PARTIALLY_FILLED_CHAR            '1'
#define ENUM_ORD_STATUS_FILLED                           "2"
#define ENUM_ORD_STATUS_FILLED_CHAR                      '2'
#define ENUM_ORD_STATUS_CANCELED                         "4"
#define ENUM_ORD_STATUS_CANCELED_CHAR                    '4'
#define ENUM_ORD_STATUS_PENDING_CANCEL                   "6"
#define ENUM_ORD_STATUS_PENDING_CANCEL_CHAR              '6'
#define ENUM_ORD_STATUS_SUSPENDED                        "9"
#define ENUM_ORD_STATUS_SUSPENDED_CHAR                   '9'

// DataType OrdType
#define ENUM_ORD_TYPE_MARKET                             1
#define ENUM_ORD_TYPE_LIMIT                              2
#define ENUM_ORD_TYPE_STOP                               3
#define ENUM_ORD_TYPE_STOP_LIMIT                         4
#define ENUM_ORD_TYPE_MARKET_TO_LIMIT                    5
#define ENUM_ORD_TYPE_BLOCK_DEAL                         6
#define ENUM_ORD_TYPE_QUOTE                              7
#define ENUM_ORD_TYPE_NLT                                8

// DataType OrderCategory
#define LEN_ORDER_CATEGORY                               1
#define ENUM_ORDER_CATEGORY_ORDER                        "1"
#define ENUM_ORDER_CATEGORY_ORDER_CHAR                   '1'
#define ENUM_ORDER_CATEGORY_QUOTE                        "2"
#define ENUM_ORDER_CATEGORY_QUOTE_CHAR                   '2'
#define ENUM_ORDER_CATEGORY_MULTI_LEG_ORDER              "3"
#define ENUM_ORDER_CATEGORY_MULTI_LEG_ORDER_CHAR         '3'

// DataType OrderID

// DataType OrderQty

// DataType OrderRoutingIndicator
#define LEN_ORDER_ROUTING_INDICATOR                      1
#define ENUM_ORDER_ROUTING_INDICATOR_YES                 "Y"
#define ENUM_ORDER_ROUTING_INDICATOR_YES_CHAR            'Y'
#define ENUM_ORDER_ROUTING_INDICATOR_NO                  "N"
#define ENUM_ORDER_ROUTING_INDICATOR_NO_CHAR             'N'

// DataType OrderSide
#define ENUM_ORDER_SIDE_BUY                              1
#define ENUM_ORDER_SIDE_SELL                             2
#define ENUM_ORDER_SIDE_RECALL                           3
#define ENUM_ORDER_SIDE_EARLY_RETURN                     4

// DataType OrigClOrdID

// DataType OrigTime

// DataType OrigTradeID

// DataType Pad1
#define LEN_PAD1                                         1

// DataType Pad2
#define LEN_PAD2                                         2

// DataType Pad3
#define LEN_PAD3                                         3

// DataType Pad4
#define LEN_PAD4                                         4

// DataType Pad5
#define LEN_PAD5                                         5

// DataType Pad6
#define LEN_PAD6                                         6

// DataType Pad7
#define LEN_PAD7                                         7

// DataType PartitionID

// DataType PartyActionType
#define ENUM_PARTY_ACTION_TYPE_HALT_TRADING              1
#define ENUM_PARTY_ACTION_TYPE_REINSTATE                 2

// DataType PartyDetailDeskID
#define LEN_PARTY_DETAIL_DESKID                          3

// DataType PartyDetailExecutingTrader
#define LEN_PARTY_DETAIL_EXECUTING_TRADER                6

// DataType PartyDetailIDExecutingTrader

// DataType PartyDetailIDExecutingUnit

// DataType PartyDetailRoleQualifier
#define ENUM_PARTY_DETAIL_ROLE_QUALIFIER_TRADER          10
#define ENUM_PARTY_DETAIL_ROLE_QUALIFIER_HEAD_TRADER     11
#define ENUM_PARTY_DETAIL_ROLE_QUALIFIER_SUPERVISOR      12

// DataType PartyDetailStatus
#define ENUM_PARTY_DETAIL_STATUS_ACTIVE                  0
#define ENUM_PARTY_DETAIL_STATUS_SUSPEND                 1

// DataType PartyIDBeneficiary
#define LEN_PARTY_ID_BENEFICIARY                         9

// DataType PartyIDEnteringFirm
#define ENUM_PARTY_ID_ENTERING_FIRM_PARTICIPANT          1
#define ENUM_PARTY_ID_ENTERING_FIRM_MARKET_SUPERVISION   2

// DataType PartyIDEnteringTrader

// DataType PartyIDExecutingTrader

// DataType PartyIDExecutingUnit

// DataType PartyIDLocationID
#define LEN_PARTY_ID_LOCATIONID                          2

// DataType PartyIDOrderOriginationFirm
#define LEN_PARTY_ID_ORDER_ORIGINATION_FIRM              7

// DataType PartyIDOriginationMarket
#define ENUM_PARTY_ID_ORIGINATION_MARKET_XKFE            1
#define ENUM_PARTY_ID_ORIGINATION_MARKET_XTAF            2

// DataType PartyIDSessionID

// DataType PartyIDTakeUpTradingFirm
#define LEN_PARTY_ID_TAKE_UP_TRADING_FIRM                5

// DataType Password
#define LEN_PASSWORD                                     32

// DataType PctCount

// DataType PercentageUtilized

// DataType PositionEffect
#define LEN_POSITION_EFFECT                              1
#define ENUM_POSITION_EFFECT_CLOSE                       "C"
#define ENUM_POSITION_EFFECT_CLOSE_CHAR                  'C'
#define ENUM_POSITION_EFFECT_OPEN                        "O"
#define ENUM_POSITION_EFFECT_OPEN_CHAR                   'O'

// DataType PrevClosePx

// DataType Price

// DataType PriceMkToLimitPx

// DataType PriceValidityCheckType
#define ENUM_PRICE_VALIDITY_CHECK_TYPE_NONE              0
#define ENUM_PRICE_VALIDITY_CHECK_TYPE_OPTIONAL          1
#define ENUM_PRICE_VALIDITY_CHECK_TYPE_MANDATORY         2

// DataType PrimaryOrderID

// DataType ProductComplex
#define ENUM_PRODUCT_COMPLEX_SIMPLE_INSTRUMENT           1
#define ENUM_PRODUCT_COMPLEX_STANDARD_OPTION_STRATEGY    2
#define ENUM_PRODUCT_COMPLEX_NON_STANDARD_OPTION_STRATEGY 3
#define ENUM_PRODUCT_COMPLEX_VOLATILITY_STRATEGY         4
#define ENUM_PRODUCT_COMPLEX_FUTURES_SPREAD              5

// DataType QuoteEntryRejectReason
#define ENUM_QUOTE_ENTRY_REJECT_REASON_UNKNOWN_SECURITY  1
#define ENUM_QUOTE_ENTRY_REJECT_REASON_DUPLICATE_QUOTE   6
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_PRICE     8
#define ENUM_QUOTE_ENTRY_REJECT_REASON_NO_REFERENCE_PRICE_AVAILABLE 16
#define ENUM_QUOTE_ENTRY_REJECT_REASON_NO_SINGLE_SIDED_QUOTES 100
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_QUOTING_MODEL 103
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_SIZE      106
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_UNDERLYING_PRICE 107
#define ENUM_QUOTE_ENTRY_REJECT_REASON_BID_PRICE_NOT_REASONABLE 108
#define ENUM_QUOTE_ENTRY_REJECT_REASON_ASK_PRICE_NOT_REASONABLE 109
#define ENUM_QUOTE_ENTRY_REJECT_REASON_BID_PRICE_EXCEEDS_RANGE 110
#define ENUM_QUOTE_ENTRY_REJECT_REASON_ASK_PRICE_EXCEEDS_RANGE 111
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INSTRUMENT_STATE_FREEZE 115
#define ENUM_QUOTE_ENTRY_REJECT_REASON_DELETION_ALREADY_PENDING 116
#define ENUM_QUOTE_ENTRY_REJECT_REASON_PRE_TRADE_RISK_SESSION_LIMIT_EXCEEDED 117
#define ENUM_QUOTE_ENTRY_REJECT_REASON_PRE_TRADE_RISK_BU_LIMIT_EXCEEDED 118
#define ENUM_QUOTE_ENTRY_REJECT_REASON_CANT_PROC_IN_CURR_INSTR_STATE 131
#define ENUM_QUOTE_ENTRY_REJECT_REASON_LOCATION_ID_NOT_SET 132
#define ENUM_QUOTE_ENTRY_REJECT_REASON_CLIENT_CODE_NOT_SET 133
#define ENUM_QUOTE_ENTRY_REJECT_REASON_CLIENT_CANNOT_BE_MODIFIED 134
#define ENUM_QUOTE_ENTRY_REJECT_REASON_CLIENT_TYPE_NOT_SET 135
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_CLIENT_CODE_FOR_CLIENT_TYPE_OWN 136
#define ENUM_QUOTE_ENTRY_REJECT_REASON_OWN_CLIENT_TYPE_NOT_ALLOWED 137
#define ENUM_QUOTE_ENTRY_REJECT_REASON_MESSAGE_TAG_NOT_SET 138
#define ENUM_QUOTE_ENTRY_REJECT_REASON_PRICE_BEYOND_CIRCUIT_LIMIT 139
#define ENUM_QUOTE_ENTRY_REJECT_REASON_QUANTITY_NOT_A_MULTIPLE_OF_LOT_SIZE 140
#define ENUM_QUOTE_ENTRY_REJECT_REASON_QUOTES_NOT_ALLOWED_IN_RRM 141
#define ENUM_QUOTE_ENTRY_REJECT_REASON_AMOUNT_EXCEEDS_TVL 142
#define ENUM_QUOTE_ENTRY_REJECT_REASON_QUOTES_NOT_ALLOWED_IN_AUCTION 143
#define ENUM_QUOTE_ENTRY_REJECT_REASON_CLIENT_CODE_DEBARRED 144
#define ENUM_QUOTE_ENTRY_REJECT_REASON_PRICE_NOT_MULTIPLE_OF_TICK_SIZE 145
#define ENUM_QUOTE_ENTRY_REJECT_REASON_QUOTES_NOT_ALLOWED_IN_POST_CLOSING 146
#define ENUM_QUOTE_ENTRY_REJECT_REASON_PARTICIPANT_CODE_CANNOT_BE_MODIFIED 147
#define ENUM_QUOTE_ENTRY_REJECT_REASON_CLIENT_TYPE_CANNOT_BE_MODIFIED 148
#define ENUM_QUOTE_ENTRY_REJECT_REASON_QUOTING_NOT_ALLOWED 149
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_PARTICIPANT_CODE 150
#define ENUM_QUOTE_ENTRY_REJECT_REASON_PRICE_BAND_NOT_SET 151
#define ENUM_QUOTE_ENTRY_REJECT_REASON_TICK_SIZE_NOT_SET 152
#define ENUM_QUOTE_ENTRY_REJECT_REASON_CLIENT_TYPE_NOT_ALLOWED 153
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_VALUE     154

// DataType QuoteEntryStatus
#define ENUM_QUOTE_ENTRY_STATUS_ACCEPTED                 0
#define ENUM_QUOTE_ENTRY_STATUS_REJECTED                 5
#define ENUM_QUOTE_ENTRY_STATUS_REMOVED_AND_REJECTED     6
#define ENUM_QUOTE_ENTRY_STATUS_PENDING                  10

// DataType QuoteEventExecID

// DataType QuoteEventLiquidityInd
#define ENUM_QUOTE_EVENT_LIQUIDITY_IND_ADDED_LIQUIDITY   1
#define ENUM_QUOTE_EVENT_LIQUIDITY_IND_REMOVED_LIQUIDITY 2

// DataType QuoteEventMatchID

// DataType QuoteEventPx

// DataType QuoteEventQty

// DataType QuoteEventReason
#define ENUM_QUOTE_EVENT_REASON_PENDING_CANCELLATION_EXECUTED 14
#define ENUM_QUOTE_EVENT_REASON_INVALID_PRICE            15
#define ENUM_QUOTE_EVENT_REASON_SELF_TRADE_QUOTE_DELETED 16
#define ENUM_QUOTE_EVENT_REASON_REVERSE_TRADE_QUOTE_DELETED 17
#define ENUM_QUOTE_EVENT_REASON_CLIENT_RRM_QUOTE_DELETED 18
#define ENUM_QUOTE_EVENT_REASON_CLIENT_SUSPENDED_QUOTE_DELETED 19

// DataType QuoteEventSide
#define ENUM_QUOTE_EVENT_SIDE_BUY                        1
#define ENUM_QUOTE_EVENT_SIDE_SELL                       2

// DataType QuoteEventType
#define ENUM_QUOTE_EVENT_TYPE_REMOVED_QUOTE_SIDE         3
#define ENUM_QUOTE_EVENT_TYPE_PARTIALLY_FILLED           4
#define ENUM_QUOTE_EVENT_TYPE_FILLED                     5

// DataType QuoteID

// DataType QuoteMsgID

// DataType QuoteResponseID

// DataType QuoteSizeType
#define ENUM_QUOTE_SIZE_TYPE_TOTAL_SIZE                  1
#define ENUM_QUOTE_SIZE_TYPE_OPEN_SIZE                   2

// DataType RRMState
#define ENUM_RRM_STATE_NO_RRM                            0
#define ENUM_RRM_STATE_IN_RRM                            1
#define ENUM_RRM_STATE_OUT_OF_RRM                        2

// DataType RefApplID
#define ENUM_REF_APPLID_TRADE_ENHANCEMENT                0
#define ENUM_REF_APPLID_TRADE                            1
#define ENUM_REF_APPLID_NEWS                             2
#define ENUM_REF_APPLID_SERVICE_AVAILABILITY             3
#define ENUM_REF_APPLID_SESSION_DATA                     4
#define ENUM_REF_APPLID_LISTENER_DATA                    5
#define ENUM_REF_APPLID_RISK_CONTROL                     6
#define ENUM_REF_APPLID_ORDER_BY_ORDER                   7
#define ENUM_REF_APPLID_RISK_ADMIN                       8
#define ENUM_REF_APPLID_USER_LEVEL                       9
#define ENUM_REF_APPLID_GROUP_LEVEL                      10
#define ENUM_REF_APPLID_INSTRUMENT_LEVEL                 11

// DataType RefApplLastMsgID
#define LEN_REF_APPL_LAST_MSGID                          16

// DataType RefApplLastSeqNum

// DataType RefApplSubID

// DataType RegulatoryID

// DataType RegulatoryText
#define LEN_REGULATORY_TEXT                              20

// DataType RelatedProductComplex
#define ENUM_RELATED_PRODUCT_COMPLEX_STANDARD_OPTION_STRATEGY 2
#define ENUM_RELATED_PRODUCT_COMPLEX_NON_STANDARD_OPTION_STRATEGY 3
#define ENUM_RELATED_PRODUCT_COMPLEX_VOLATILITY_STRATEGY 4
#define ENUM_RELATED_PRODUCT_COMPLEX_FUTURES_SPREAD      5

// DataType RelatedSecurityID

// DataType RelatedSymbol

// DataType RequestOut

// DataType RequestTime

// DataType RequestingPartyClearingFirm
#define LEN_REQUESTING_PARTY_CLEARING_FIRM               9

// DataType RequestingPartyEnteringFirm
#define LEN_REQUESTING_PARTY_ENTERING_FIRM               9

// DataType RequestingPartyIDEnteringFirm
#define ENUM_REQUESTING_PARTY_ID_ENTERING_FIRM_PARTICIPANT 1
#define ENUM_REQUESTING_PARTY_ID_ENTERING_FIRM_MARKET_SUPERVISION 2

// DataType RequestingPartyIDExecutingSystem
#define ENUM_REQUESTING_PARTY_ID_EXECUTING_SYSTEM_EUREX_CLEARING 1
#define ENUM_REQUESTING_PARTY_ID_EXECUTING_SYSTEM_EUREX_TRADING 2

// DataType RequestingPartyIDExecutingTrader

// DataType ResponseIn

// DataType RiskLimitAction
#define ENUM_RISK_LIMIT_ACTION_WARNING                   4
#define ENUM_RISK_LIMIT_ACTION_QUEUE_INBOUND             0
#define ENUM_RISK_LIMIT_ACTION_REJECT                    2

// DataType RiskModeStatus
#define ENUM_RISK_MODE_STATUS_IN                         1
#define ENUM_RISK_MODE_STATUS_OUT                        2

// DataType RootPartyClearingFirm
#define LEN_ROOT_PARTY_CLEARING_FIRM                     5

// DataType RootPartyClearingOrganization
#define LEN_ROOT_PARTY_CLEARING_ORGANIZATION             4

// DataType RootPartyExecutingFirm
#define LEN_ROOT_PARTY_EXECUTING_FIRM                    5

// DataType RootPartyExecutingTrader
#define LEN_ROOT_PARTY_EXECUTING_TRADER                  6

// DataType RootPartyIDBeneficiary
#define LEN_ROOT_PARTY_ID_BENEFICIARY                    9

// DataType RootPartyIDClearingUnit

// DataType RootPartyIDExecutingTrader

// DataType RootPartyIDExecutingUnit

// DataType RootPartyIDOrderOriginationFirm
#define LEN_ROOT_PARTY_ID_ORDER_ORIGINATION_FIRM         7

// DataType RootPartyIDSessionID

// DataType RootPartyIDTakeUpTradingFirm
#define LEN_ROOT_PARTY_ID_TAKE_UP_TRADING_FIRM           5

// DataType STPCFlag
#define ENUM_STPC_FLAG_PASSIVE                           0
#define ENUM_STPC_FLAG_ACTIVE                            1

// DataType ScopeIdentifier
#define ENUM_SCOPE_IDENTIFIER_SUS_PEN_SION               1
#define ENUM_SCOPE_IDENTIFIER_PRO_DUCT_SUS_PEN_SION      2
#define ENUM_SCOPE_IDENTIFIER_COL_LAT_ERAL_SUS_PEN_SION  3
#define ENUM_SCOPE_IDENTIFIER_COO_LIN_GOFF_SUS_PEN_SION  4
#define ENUM_SCOPE_IDENTIFIER_PRO_DUCT                   5
#define ENUM_SCOPE_IDENTIFIER_MAR_KET                    6
#define ENUM_SCOPE_IDENTIFIER_COO_LIN_GOFF               7
#define ENUM_SCOPE_IDENTIFIER_COL_LAT_ERAL               8
#define ENUM_SCOPE_IDENTIFIER_PRO_DUCT_NEAR_DUR_ATION    9
#define ENUM_SCOPE_IDENTIFIER_CLI_ENT_SUS_PEN_SION       10

// DataType SecondaryGatewayID

// DataType SecondaryGatewaySubID

// DataType SecurityID

// DataType SecurityResponseID

// DataType SecuritySubType

// DataType SegmentIndicator
#define ENUM_SEGMENT_INDICATOR_CASH                      1
#define ENUM_SEGMENT_INDICATOR_EQU_ITY                   2
#define ENUM_SEGMENT_INDICATOR_REPO                      3
#define ENUM_SEGMENT_INDICATOR_SLB                       4
#define ENUM_SEGMENT_INDICATOR_SAUC                      5
#define ENUM_SEGMENT_INDICATOR_DEBT                      6
#define ENUM_SEGMENT_INDICATOR_CUR_RENCY                 7
#define ENUM_SEGMENT_INDICATOR_CDX                       8
#define ENUM_SEGMENT_INDICATOR_IRD                       9
#define ENUM_SEGMENT_INDICATOR_INX                       10
#define ENUM_SEGMENT_INDICATOR_EDX                       11
#define ENUM_SEGMENT_INDICATOR_DER_IVA_TIVES             12
#define ENUM_SEGMENT_INDICATOR_EUREX                     13
#define ENUM_SEGMENT_INDICATOR_EEX                       14
#define ENUM_SEGMENT_INDICATOR_BCX                       15

// DataType SenderLocationID

// DataType SenderSubID

// DataType SendingTime

// DataType SessionInstanceID

// DataType SessionMode
#define ENUM_SESSION_MODE_HF                             1
#define ENUM_SESSION_MODE_LF                             2
#define ENUM_SESSION_MODE_GUI                            3

// DataType SessionRejectReason
#define ENUM_SESSION_REJECT_REASON_REQUIRED_TAG_MISSING  1
#define ENUM_SESSION_REJECT_REASON_VALUE_IS_INCORRECT    5
#define ENUM_SESSION_REJECT_REASON_DECRYPTION_PROBLEM    7
#define ENUM_SESSION_REJECT_REASON_INVALID_MSGID         11
#define ENUM_SESSION_REJECT_REASON_INCORRECT_NUM_IN_GROUP_COUNT 16
#define ENUM_SESSION_REJECT_REASON_OTHER                 99
#define ENUM_SESSION_REJECT_REASON_THROTTLE_LIMIT_EXCEEDED 100
#define ENUM_SESSION_REJECT_REASON_EXPOSURE_LIMIT_EXCEEDED 101
#define ENUM_SESSION_REJECT_REASON_SERVICE_TEMPORARILY_NOT_AVAILABLE 102
#define ENUM_SESSION_REJECT_REASON_SERVICE_NOT_AVAILABLE 103
#define ENUM_SESSION_REJECT_REASON_RESULT_OF_TRANSACTION_UNKNOWN 104
#define ENUM_SESSION_REJECT_REASON_OUTBOUND_CONVERSION_ERROR 105
#define ENUM_SESSION_REJECT_REASON_HEARTBEAT_VIOLATION   152
#define ENUM_SESSION_REJECT_REASON_INTERNAL_TECHNICAL_ERROR 200
#define ENUM_SESSION_REJECT_REASON_VALIDATION_ERROR      210
#define ENUM_SESSION_REJECT_REASON_USER_ALREADY_LOGGED_IN 211
#define ENUM_SESSION_REJECT_REASON_ORDER_NOT_FOUND       10000
#define ENUM_SESSION_REJECT_REASON_PRICE_NOT_REASONABLE  10001
#define ENUM_SESSION_REJECT_REASON_CLIENT_ORDERID_NOT_UNIQUE 10002
#define ENUM_SESSION_REJECT_REASON_QUOTE_ACTIVATION_IN_PROGRESS 10003
#define ENUM_SESSION_REJECT_REASON_BU_BOOK_ORDER_LIMIT_EXCEEDED 10004
#define ENUM_SESSION_REJECT_REASON_SESSION_BOOK_ORDER_LIMIT_EXCEEDED 10005
#define ENUM_SESSION_REJECT_REASON_ACTIVITY_TIMESTAMP_NOT_MATCHED 10006

// DataType SessionStatus
#define ENUM_SESSION_STATUS_ACTIVE                       0
#define ENUM_SESSION_STATUS_LOGOUT                       4

// DataType SessionSubMode
#define ENUM_SESSION_SUB_MODE_REGULAR_TRADING_SESSION    0
#define ENUM_SESSION_SUB_MODE_FIX_TRADING_SESSION        1
#define ENUM_SESSION_SUB_MODE_REGULAR_BACK_OFFICE_SESSION 2

// DataType SettlType

// DataType Side
#define ENUM_SIDE_BUY                                    1
#define ENUM_SIDE_SELL                                   2
#define ENUM_SIDE_RECALL                                 3
#define ENUM_SIDE_EARLY_RETURN                           4

// DataType SideLastPx

// DataType SideLastQty

// DataType SideTradeID

// DataType SimpleSecurityID

// DataType SingleOrderMaxQuantity

// DataType SingleOrderMaxValue

// DataType SixLakhFlag
#define LEN_SIX_LAKH_FLAG                                1
#define ENUM_SIX_LAKH_FLAG_IS_SIX_LAKH_FLAG              "Y"
#define ENUM_SIX_LAKH_FLAG_IS_SIX_LAKH_FLAG_CHAR         'Y'
#define ENUM_SIX_LAKH_FLAG_SIX_LAKH_FLAG_NOT             "N"
#define ENUM_SIX_LAKH_FLAG_SIX_LAKH_FLAG_NOT_CHAR        'N'

// DataType StopPx

// DataType StrategyLinkID

// DataType SubscriptionScope

// DataType TargetPartyIDDeskID
#define LEN_TARGET_PARTY_ID_DESKID                       3

// DataType TargetPartyIDExecutingTrader

// DataType TargetPartyIDSessionID

// DataType TemplateID

// DataType ThrottleDisconnectLimit

// DataType ThrottleNoMsgs

// DataType ThrottleTimeInterval

// DataType TimeInForce
#define ENUM_TIME_IN_FORCE_DAY                           0
#define ENUM_TIME_IN_FORCE_GTC                           1
#define ENUM_TIME_IN_FORCE_IOC                           3
#define ENUM_TIME_IN_FORCE_GTD                           6
#define ENUM_TIME_IN_FORCE_GTCL                          7

// DataType TotNumTradeReports

// DataType TotalBuySize

// DataType TotalCollateral

// DataType TotalSellSize

// DataType TradSesEvent
#define ENUM_TRAD_SES_EVENT_START_OF_SERVICE             101
#define ENUM_TRAD_SES_EVENT_MARKET_RESET                 102
#define ENUM_TRAD_SES_EVENT_END_OF_RESTATEMENT           103
#define ENUM_TRAD_SES_EVENT_END_OF_DAY_SERVICE           104

// DataType TradSesMode
#define ENUM_TRAD_SES_MODE_TESTING                       1
#define ENUM_TRAD_SES_MODE_SIMULATED                     2
#define ENUM_TRAD_SES_MODE_PRODUCTION                    3
#define ENUM_TRAD_SES_MODE_ACCEPTANCE                    4

// DataType TradeDate

// DataType TradeID

// DataType TradeManagerStatus
#define ENUM_TRADE_MANAGER_STATUS_UNAVAILABLE            0
#define ENUM_TRADE_MANAGER_STATUS_AVAILABLE              1

// DataType TradeManagerTradeDate

// DataType TradeReportType
#define ENUM_TRADE_REPORT_TYPE_SUBMIT                    0
#define ENUM_TRADE_REPORT_TYPE_ALLEGED                   1
#define ENUM_TRADE_REPORT_TYPE_NO_WAS_REPLACED           5
#define ENUM_TRADE_REPORT_TYPE_TRADE_BREAK               7

// DataType TradeValue

// DataType TradeValueFlag
#define LEN_TRADE_VALUE_FLAG                             1

// DataType TradeVolume

// DataType TradingCapacity
#define ENUM_TRADING_CAPACITY_CUSTOMER                   1
#define ENUM_TRADING_CAPACITY_PRINCIPAL                  5
#define ENUM_TRADING_CAPACITY_MARKET_MAKER               6

// DataType TradingSessionID

// DataType TradingSessionSubID
#define ENUM_TRADING_SESSION_SUBID_CLOSING_AUCTION       4

// DataType TransactTime

// DataType TransferReason
#define ENUM_TRANSFER_REASON_OWNER                       1
#define ENUM_TRANSFER_REASON_CLEARER                     2

// DataType TrdMatchID

// DataType TrdRegTSEntryTime

// DataType TrdRegTSTimeIn

// DataType TrdRegTSTimeOut

// DataType TrdRegTSTimePriority

// DataType Trend
#define LEN_TREND                                        1

// DataType Triggered
#define ENUM_TRIGGERED_NOT_TRIGGERED                     0
#define ENUM_TRIGGERED_TRIGGERED_STOP                    1
#define ENUM_TRIGGERED_TRIGGERED_OCO                     2

// DataType UnderlyingDirtyPrice

// DataType UnderlyingPx

// DataType UnutilizedCollateral

// DataType UpperCktPx

// DataType UserStatus
#define ENUM_USER_STATUS_USER_STOPPED                    10
#define ENUM_USER_STATUS_USER_RELEASED                   11

// DataType Username

// DataType UtilizedCollateral

// DataType VarText
#define LEN_VAR_TEXT                                     2000

// DataType VarTextLen

// DataType Vega

// DataType WtAvgPx

// DataType Yield


/*
 * Structure defines for components and sequences
 */

// Structure: BestRateGrp
typedef struct
{
    int64_t BidPx;
    int64_t BidYTM;
    int64_t BidYTP;
    int64_t BidYTC;
    int32_t BidSize;
    uint32_t BidPriceLevel;
    int64_t OfferPx;
    int64_t OfferYTM;
    int64_t OfferYTP;
    int64_t OfferYTC;
    int32_t OfferSize;
    uint32_t AskPriceLevel;
} BestRateGrpSeqT;

// Structure: EnrichmentRulesGrp
typedef struct
{
    uint16_t EnrichmentRuleID;
    uint8_t PartyIDOriginationMarket;
    char Account[LEN_ACCOUNT];
    char PositionEffect[LEN_POSITION_EFFECT];
    char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
    char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
    char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
    char FreeText1[LEN_FREE_TEXT1];
    char FreeText2[LEN_FREE_TEXT2];
    char FreeText3[LEN_FREE_TEXT3];
    char Pad1[LEN_PAD1];
} EnrichmentRulesGrpSeqT;

// Structure: FillsGrp
typedef struct
{
    int64_t FillPx;
    int64_t FillYield;
    int64_t FillDirtyPx;
    int32_t FillQty;
    uint32_t FillMatchID;
    int32_t FillExecID;
    uint8_t FillLiquidityInd;
    char Pad3[LEN_PAD3];
} FillsGrpSeqT;

// Structure: GroupLevelData
typedef struct
{
    char Group[LEN_GROUP];
    char Pad6[LEN_PAD6];
    int64_t MaxBuyValue;
    int64_t MaxSellValue;
} GroupLevelDataSeqT;

// Structure: InstrmntLegExecGrp
typedef struct
{
    int64_t LegSecurityID;
    int64_t LegLastPx;
    int32_t LegLastQty;
    int32_t LegExecID;
    uint8_t LegSide;
    uint8_t NoFillsIndex;
    char Pad6[LEN_PAD6];
} InstrmntLegExecGrpSeqT;

// Structure: InstrmtLegGrp
typedef struct
{
    int64_t LegSecurityID;
    int64_t LegPrice;
    int32_t LegSymbol;
    uint32_t LegRatioQty;
    uint8_t LegSide;
    uint8_t LegSecurityType;
    char Pad6[LEN_PAD6];
} InstrmtLegGrpSeqT;

// Structure: InstrumentLevelData
typedef struct
{
    int64_t SecurityID;
    int32_t MaxBuyQuantity;
    int32_t MaxSellQuantity;
} InstrumentLevelDataSeqT;

// Structure: LegOrdGrp
typedef struct
{
    char LegAccount[LEN_LEG_ACCOUNT];
    char LegPositionEffect[LEN_LEG_POSITION_EFFECT];
    char Pad5[LEN_PAD5];
} LegOrdGrpSeqT;

// Structure: MMParameterGrp
typedef struct
{
    int64_t ExposureDuration;
    int32_t CumQty;
    int32_t PctCount;
    int32_t Delta;
    int32_t Vega;
    uint8_t ProductComplex;
    char Pad7[LEN_PAD7];
} MMParameterGrpSeqT;

// Structure: MessageHeaderIn
typedef struct
{
    uint32_t BodyLen;
    uint16_t TemplateID;
    char NetworkMsgID[LEN_NETWORK_MSGID];
    char Pad2[LEN_PAD2];
} MessageHeaderInCompT;

// Structure: MessageHeaderOut
typedef struct
{
    uint32_t BodyLen;
    uint16_t TemplateID;
    char Pad2[LEN_PAD2];
} MessageHeaderOutCompT;

// Structure: MultiLegExecGrp
typedef struct
{
    uint64_t OrderID;
    int64_t SecurityID;
    int32_t CumQty;
    int32_t CxlQty;
    uint16_t ExecRestatementReason;
    char OrdStatus[LEN_ORD_STATUS];
    char ExecType[LEN_EXEC_TYPE];
    char Pad4[LEN_PAD4];
} MultiLegExecGrpSeqT;

// Structure: MultiLegFillGrp
typedef struct
{
    int64_t FillPx;
    int64_t SecurityID;
    int32_t FillQty;
    uint32_t FillMatchID;
    int32_t FillExecID;
    char Pad4[LEN_PAD4];
} MultiLegFillGrpSeqT;

// Structure: MultiLegGrp
typedef struct
{
    int64_t SecurityID;
    uint64_t OrderID;
    int64_t Price;
    int64_t MaxPricePercentage;
    int32_t MessageTag;
    int32_t OrderQty;
    int32_t CumQty;
    int32_t CxlQty;
    uint16_t ExecRestatementReason;
    char OrdStatus[LEN_ORD_STATUS];
    char ExecType[LEN_EXEC_TYPE];
    uint8_t Side;
    uint8_t OrdType;
    char Pad2[LEN_PAD2];
} MultiLegGrpSeqT;

// Structure: MultiLegOrdGrp
typedef struct
{
    int64_t SecurityID;
    int64_t Price;
    int64_t MaxPricePercentage;
    int32_t MessageTag;
    int32_t MarketSegmentID;
    int32_t OrderQty;
    uint8_t ProductComplex;
    uint8_t Side;
    uint8_t OrdType;
    char Pad1[LEN_PAD1];
} MultiLegOrdGrpSeqT;

// Structure: NRBCHeader
typedef struct
{
    uint64_t SendingTime;
    uint32_t ApplSubID;
    uint8_t ApplID;
    uint8_t LastFragment;
    char Pad2[LEN_PAD2];
} NRBCHeaderCompT;

// Structure: NRResponseHeaderME
typedef struct
{
    uint64_t RequestTime;
    uint64_t RequestOut;
    uint64_t TrdRegTSTimeIn;
    uint64_t TrdRegTSTimeOut;
    uint64_t ResponseIn;
    uint64_t SendingTime;
    uint32_t MsgSeqNum;
    uint8_t LastFragment; //read uint8_t
    char Pad3[LEN_PAD3];
} NRResponseHeaderMECompT;

// Structure: NotAffectedOrdersGrp
typedef struct
{
    uint64_t NotAffectedOrderID;
    uint64_t NotAffOrigClOrdID;
} NotAffectedOrdersGrpSeqT;

// Structure: NotAffectedSecuritiesGrp
typedef struct
{
    uint64_t NotAffectedSecurityID;
} NotAffectedSecuritiesGrpSeqT;

// Structure: NotifHeader
typedef struct
{
    uint64_t SendingTime;
} NotifHeaderCompT;

// Structure: OBInquiryGrp
typedef struct
{
    uint64_t RequestTime;
    uint64_t OrderID;
    int64_t SecurityID;
    uint64_t SenderLocationID;
    int64_t Price;
    int64_t StopPx;
    int64_t Yield;
    int64_t UnderlyingDirtyPrice;
    uint64_t ActivityTime;
    int32_t MarketSegmentID;
    int32_t MessageTag;
    int32_t LeavesQty;
    int32_t CumQty;
    int32_t OrderQty;
    uint8_t AccountType;
    char OrdStatus[LEN_ORD_STATUS];
    uint8_t Side;
    uint8_t OrdType;
    uint8_t TimeInForce;
    char FreeText1[LEN_FREE_TEXT1];
    uint8_t Triggered;
    char CPCode[LEN_CP_CODE];
    char AlgoID[LEN_ALGOID];
    char Pad6[LEN_PAD6];
} OBInquiryGrpSeqT;

// Structure: OBInquiryGrp_R22
typedef struct
{
    uint64_t RequestTime;
    uint64_t OrderID;
    int64_t SecurityID;
    uint64_t SenderLocationID;
    int64_t Price;
    int64_t StopPx;
    int64_t Yield;
    int64_t UnderlyingDirtyPrice;
    int32_t MarketSegmentID;
    int32_t MessageTag;
    int32_t LeavesQty;
    int32_t CumQty;
    int32_t OrderQty;
    uint8_t AccountType;
    char OrdStatus[LEN_ORD_STATUS];
    uint8_t Side;
    uint8_t OrdType;
    uint8_t TimeInForce;
    char FreeText1[LEN_FREE_TEXT1];
    uint8_t Triggered;
    char CPCode[LEN_CP_CODE];
    char AlgoID[LEN_ALGOID];
    char Pad6[LEN_PAD6];
    uint64_t ActivityTime;
} OBInquiryGrp_R22SeqT;

// Structure: PartyDetailsGrp
typedef struct
{
    uint32_t PartyDetailIDExecutingTrader;
    char PartyDetailExecutingTrader[LEN_PARTY_DETAIL_EXECUTING_TRADER];
    uint8_t PartyDetailRoleQualifier;
    uint8_t PartyDetailStatus;
    char PartyDetailDeskID[LEN_PARTY_DETAIL_DESKID];
    char Pad1[LEN_PAD1];
} PartyDetailsGrpSeqT;

// Structure: QuoteEntryAckGrp
typedef struct
{
    int64_t SecurityID;
    int32_t BidCxlSize;
    int32_t OfferCxlSize;
    uint32_t QuoteEntryRejectReason;
    uint8_t QuoteEntryStatus;
    char Pad3[LEN_PAD3];
} QuoteEntryAckGrpSeqT;

// Structure: QuoteEntryGrp
typedef struct
{
    int64_t SecurityID;
    int64_t BidPx;
    int64_t OfferPx;
    int32_t BidSize;
    int32_t OfferSize;
    int32_t MessageTag;
    char Pad4[LEN_PAD4];
} QuoteEntryGrpSeqT;

// Structure: QuoteEventGrp
typedef struct
{
    uint64_t OrderID;
    uint64_t SenderLocationID;
    int64_t SecurityID;
    int64_t QuoteEventPx;
    uint64_t QuoteMsgID;
    uint32_t QuoteEventMatchID;
    int32_t MessageTag;
    int32_t QuoteEventExecID;
    int32_t QuoteEventQty;
    uint8_t QuoteEventType;
    uint8_t QuoteEventSide;
    uint8_t QuoteEventLiquidityInd;
    uint8_t QuoteEventReason;
    uint8_t AccountType;
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char Pad3[LEN_PAD3];
} QuoteEventGrpSeqT;

// Structure: QuoteLegExecGrp
typedef struct
{
    int64_t LegSecurityID;
    int64_t LegLastPx;
    int32_t LegLastQty;
    int32_t LegExecID;
    uint8_t LegSide;
    uint8_t NoQuoteEventsIndex;
    char Pad6[LEN_PAD6];
} QuoteLegExecGrpSeqT;

// Structure: RBCHeader
typedef struct
{
    uint64_t SendingTime;
    uint64_t ApplSeqNum;
    uint32_t ApplSubID;
    uint16_t PartitionID;
    uint8_t ApplResendFlag;
    uint8_t ApplID;
    uint8_t LastFragment;
    char Pad7[LEN_PAD7];
} RBCHeaderCompT;

// Structure: RBCHeaderME
typedef struct
{
    uint64_t TrdRegTSTimeOut;
    uint64_t SendingTime;
    uint32_t ApplSubID;
    uint16_t PartitionID;
    char ApplMsgID[LEN_APPL_MSGID];
    uint8_t ApplID; //read uint8_t
    uint8_t ApplResendFlag; //read uint8_t
    uint8_t LastFragment; //read uint8_t
    char Pad7[LEN_PAD7];
} RBCHeaderMECompT;

// Structure: RequestHeader
typedef struct
{
    uint32_t MsgSeqNum;
    uint32_t SenderSubID;
} RequestHeaderCompT;

// Structure: ResponseHeader
typedef struct
{
    uint64_t RequestTime;
    uint64_t SendingTime;
    uint32_t MsgSeqNum;
    char Pad4[LEN_PAD4];
} ResponseHeaderCompT;

// Structure: ResponseHeaderME
typedef struct
{
    uint64_t RequestTime;
    uint64_t RequestOut;
    uint64_t TrdRegTSTimeIn;
    uint64_t TrdRegTSTimeOut;
    uint64_t ResponseIn;
    uint64_t SendingTime;
    uint32_t MsgSeqNum;
    uint16_t PartitionID;
    uint8_t ApplID;
    char ApplMsgID[LEN_APPL_MSGID];
    uint8_t LastFragment;
} ResponseHeaderMECompT;

// Structure: SessionsGrp
typedef struct
{
    uint32_t PartyIDSessionID;
    uint8_t SessionMode;
    uint8_t SessionSubMode;
    char Pad2[LEN_PAD2];
} SessionsGrpSeqT;

/*
 * Structure defines for messages
 */

// Message:     AddComplexInstrumentRequest
// TemplateID:  10301
// Alias:       Create Strategy
// FIX MsgType: SecurityDefinitionRequest = "c"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int32_t MarketSegmentID;
    int32_t SecuritySubType;
    uint32_t RegulatoryID;
    uint8_t ProductComplex;
    uint8_t NoLegs;
    char RegulatoryText[LEN_REGULATORY_TEXT];
    char Pad6[LEN_PAD6];
    InstrmtLegGrpSeqT InstrmtLegGrp[MAX_ADD_COMPLEX_INSTRUMENT_REQUEST_INSTRMT_LEG_GRP];
} AddComplexInstrumentRequestT;

// Message:     AddComplexInstrumentResponse
// TemplateID:  10302
// Alias:       Create Strategy Response
// FIX MsgType: SecurityDefinition = "d"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    int64_t LowLimitPrice;
    int64_t HighLimitPrice;
    int64_t SecurityID;
    uint64_t LastUpdateTime;
    uint64_t SecurityResponseID;
    int32_t MarketSegmentID;
    int32_t NumberOfSecurities;
    int32_t SecuritySubType;
    uint8_t MultilegModel;
    uint8_t ImpliedMarketIndicator;
    uint8_t ProductComplex;
    uint8_t NoLegs;
    InstrmtLegGrpSeqT InstrmtLegGrp[MAX_ADD_COMPLEX_INSTRUMENT_RESPONSE_INSTRMT_LEG_GRP];
} AddComplexInstrumentResponseT;

// Message:     BroadcastErrorNotification
// TemplateID:  10032
// Alias:       Gap Fill
// FIX MsgType: ApplicationMessageReport = "BY"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NotifHeaderCompT NotifHeader;
    uint32_t ApplIDStatus;
    uint32_t RefApplSubID;
    uint16_t VarTextLen;
    uint8_t RefApplID;
    uint8_t SessionStatus;
    char Pad4[LEN_PAD4];
    char VarText[LEN_VAR_TEXT];
} BroadcastErrorNotificationT;

// Message:     CrossRequest
// TemplateID:  10118
// Alias:       Cross Request
// FIX MsgType: CrossRequest = "U16"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int64_t SecurityID;
    int32_t MarketSegmentID;
    int32_t OrderQty;
    uint32_t RegulatoryID;
    char RegulatoryText[LEN_REGULATORY_TEXT];
} CrossRequestT;

// Message:     CrossRequestResponse
// TemplateID:  10119
// Alias:       Cross Request Response
// FIX MsgType: RequestAcknowledge = "U1"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t ExecID;
} CrossRequestResponseT;

// Message:     DebtInquiryRequest
// TemplateID:  10390
// Alias:       Debt Inquiry Request
// FIX MsgType: DebtInquiryRequest = "U32"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int64_t UnderlyingPx;
    int64_t Yield;
    int64_t SecurityID;
    int32_t OrderQty;
    char Pad4[LEN_PAD4];
} DebtInquiryRequestT;

// Message:     DebtInquiryResponse
// TemplateID:  10391
// Alias:       Debt Inquiry Response
// FIX MsgType: DebtInquiryResponse = "U33"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    int64_t UnderlyingPx;
    int64_t Yield;
    int64_t AccruedInterestAmt;
    int64_t GrossTradeAmt;
    int64_t UnderlyingDirtyPrice;
    int64_t SecurityID;
    int32_t OrderQty;
    uint32_t SettlType;
} DebtInquiryResponseT;

// Message:     DeleteAllOrderBroadcast
// TemplateID:  10122
// Alias:       Order Mass Cancellation Notification
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t MassActionReportID;
    int64_t SecurityID;
    int32_t MarketSegmentID;
    uint32_t TargetPartyIDSessionID;
    uint32_t TargetPartyIDExecutingTrader;
    uint32_t PartyIDEnteringTrader;
    uint16_t NoNotAffectedOrders;
    uint8_t PartyIDEnteringFirm;
    uint8_t MassActionReason;
    uint8_t ExecInst;
    char Pad3[LEN_PAD3];
    NotAffectedOrdersGrpSeqT NotAffectedOrdersGrp[MAX_DELETE_ALL_ORDER_BROADCAST_NOT_AFFECTED_ORDERS_GRP];
} DeleteAllOrderBroadcastT;

// Message:     DeleteAllOrderNRResponse
// TemplateID:  10124
// Alias:       Order Mass Cancellation Response No Hits
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t MassActionReportID;
} DeleteAllOrderNRResponseT;

// Message:     DeleteAllOrderQuoteEventBroadcast
// TemplateID:  10308
// Alias:       Mass Cancellation Event
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t MassActionReportID;
    int64_t SecurityID;
    int32_t MarketSegmentID;
    uint8_t MassActionReason;
    uint8_t ExecInst;
    char Pad2[LEN_PAD2];

    std::string ToString() {
      std::ostringstream t_temp_oss;

      t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "TrdRegTSTimeOut: " << RBCHeaderME.TrdRegTSTimeOut << " "
               << "SendingTime: " << RBCHeaderME.SendingTime << " "
               << "ApplSubID: " << RBCHeaderME.ApplSubID << " "
               << "PartitionID: " << RBCHeaderME.PartitionID << " "
//               << "ApplMsgID: " << RBCHeaderME.ApplMsgID << " "
               << "ApplID: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.ApplID))) << " "
               << "ResendFlag: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.ApplResendFlag))) << " "
               << "LastFragment: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.LastFragment))) << " "
               << "MassActionReportID: " << MassActionReportID << " "
               << "SecurityID: " << SecurityID << " "
               << "MarketSegmentID: " << MarketSegmentID << " "
               << "MassActionReason: " << ((unsigned int)(*(uint8_t *)(&MassActionReason))) << " "
               << "ExecInst: " << ((unsigned int)(*(uint8_t *)(&ExecInst))) << " " << std::endl;

      return t_temp_oss.str();
    }

} DeleteAllOrderQuoteEventBroadcastT;

// Message:     DeleteAllOrderRequest
// TemplateID:  10120
// Alias:       Order Mass Cancellation Request
// FIX MsgType: OrderMassActionRequest = "CA"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int64_t SecurityID;
    int32_t MarketSegmentID;
    uint32_t RegulatoryID;
    uint32_t TargetPartyIDSessionID;
    uint32_t TargetPartyIDExecutingTrader;
} DeleteAllOrderRequestT;

// Message:     DeleteAllOrderResponse
// TemplateID:  10121
// Alias:       Order Mass Cancellation Response
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderMECompT ResponseHeaderME;
    uint64_t MassActionReportID;
    uint16_t NoNotAffectedOrders;
    char Pad6[LEN_PAD6];
    NotAffectedOrdersGrpSeqT NotAffectedOrdersGrp[MAX_DELETE_ALL_ORDER_RESPONSE_NOT_AFFECTED_ORDERS_GRP];
} DeleteAllOrderResponseT;

// Message:     DeleteAllQuoteBroadcast
// TemplateID:  10410
// Alias:       Quote Mass Cancellation Notification
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t MassActionReportID;
    int64_t SecurityID;
    int32_t MarketSegmentID;
    uint32_t TargetPartyIDSessionID;
    uint32_t PartyIDEnteringTrader;
    uint32_t TargetPartyIDExecutingTrader;
    uint16_t NoNotAffectedSecurities;
    uint8_t MassActionReason;
    uint8_t PartyIDEnteringFirm;
    char TargetPartyIDDeskID[LEN_TARGET_PARTY_ID_DESKID];
    char Pad1[LEN_PAD1];
    NotAffectedSecuritiesGrpSeqT NotAffectedSecuritiesGrp[MAX_DELETE_ALL_QUOTE_BROADCAST_NOT_AFFECTED_SECURITIES_GRP];
} DeleteAllQuoteBroadcastT;

// Message:     DeleteAllQuoteRequest
// TemplateID:  10408
// Alias:       Quote Mass Cancellation Request
// FIX MsgType: OrderMassActionRequest = "CA"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int32_t MarketSegmentID;
    uint32_t TargetPartyIDSessionID;
    uint32_t RegulatoryID;
    char Pad4[LEN_PAD4];
} DeleteAllQuoteRequestT;

// Message:     DeleteAllQuoteResponse
// TemplateID:  10409
// Alias:       Quote Mass Cancellation Response
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t MassActionReportID;
    uint16_t NoNotAffectedSecurities;
    char Pad6[LEN_PAD6];
    NotAffectedSecuritiesGrpSeqT NotAffectedSecuritiesGrp[MAX_DELETE_ALL_QUOTE_RESPONSE_NOT_AFFECTED_SECURITIES_GRP];
} DeleteAllQuoteResponseT;

// Message:     DeleteOrderBroadcast
// TemplateID:  10112
// Alias:       Cancel Order Notification
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    uint64_t ExecID;
    int32_t MessageTag;
    int32_t CumQty;
    int32_t CxlQty;
    int32_t MarketSegmentID;
    uint32_t PartyIDEnteringTrader;
    uint16_t ExecRestatementReason;
    uint8_t PartyIDEnteringFirm;
    char OrdStatus[LEN_ORD_STATUS];
    char ExecType[LEN_EXEC_TYPE];
    uint8_t ProductComplex;
    uint8_t Side;
    char Pad5[LEN_PAD5];
} DeleteOrderBroadcastT;

// Message:     DeleteOrderByOrderBroadcast
// TemplateID:  10116
// Alias:       Cancel Order by Order Notification
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    uint64_t SenderLocationID;
    uint64_t ExecID;
    int64_t SecurityID;
    int64_t MaxPricePercentage;
    int64_t Price;
    int64_t StopPx;
    int64_t UnderlyingDirtyPrice;
    int64_t Yield;
    int32_t MessageTag;
    int32_t MaxShow;
    int32_t CumQty;
    int32_t OrderQty;
    int32_t CxlQty;
    uint32_t PartyIDEnteringTrader;
    uint16_t ExecRestatementReason;
    uint8_t Side;
    uint8_t OrdType;
    uint8_t TimeInForce;
    uint8_t AccountType;
    uint8_t PartyIDEnteringFirm;
    char OrdStatus[LEN_ORD_STATUS];
    char ExecType[LEN_EXEC_TYPE];
    uint8_t ProductComplex;
    char FreeText1[LEN_FREE_TEXT1];
    uint8_t Triggered;
    char Pad1[LEN_PAD1];
} DeleteOrderByOrderBroadcastT;

// Message:     DeleteOrderComplexRequest
// TemplateID:  10123
// Alias:       Cancel Order Complex
// FIX MsgType: OrderCancelRequest = "F"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    uint64_t ActivityTime;
    int32_t MessageTag;
    int32_t MarketSegmentID;
    uint32_t TargetPartyIDSessionID;
    uint32_t RegulatoryID;
    char AlgoID[LEN_ALGOID];
} DeleteOrderComplexRequestT;

// Message:     DeleteOrderNRResponse
// TemplateID:  10111
// Alias:       Cancel Order Response (Lean Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    uint64_t ExecID;
    int32_t CumQty;
    int32_t CxlQty;
    char OrdStatus[LEN_ORD_STATUS + 1];
    char ExecType[LEN_EXEC_TYPE + 1];
    uint16_t ExecRestatementReason;
    uint8_t ProductComplex; //read uint8_t
    char Pad3[LEN_PAD3];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << NRResponseHeaderME.RequestTime << " "
               << "RequestOut: " << NRResponseHeaderME.RequestOut << " "
               << "TrdRegTSTimeIn: " << NRResponseHeaderME.TrdRegTSTimeIn << " "
               << "TrdRegTSTimeOut: " << NRResponseHeaderME.TrdRegTSTimeOut << " "
               << "ResponseIn: " << NRResponseHeaderME.ResponseIn << " "
               << "SendingTime: " << NRResponseHeaderME.SendingTime << " "
               << "MsgSeqNum: " << NRResponseHeaderME.MsgSeqNum << " "
               << "LastFragment: " << ((unsigned int)(*(uint8_t *)(&NRResponseHeaderME.LastFragment))) << " "
               << "OrderID: " << OrderID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "OrigClOrdID: " << OrigClOrdID << " "
               << "SecurityID: " << SecurityID << " "
               << "ExecID: " << ExecID << " "
               << "CumQty: " << CumQty << " "
               << "CxlQty: " << CxlQty << " "
               << "OrdStatus: " << OrdStatus << " "
               << "ExecType: " << ExecType << " "
               << "ExecRestatementReason: " << ExecRestatementReason << " "
               << "ProductComplex: " << ((unsigned int)(*(uint8_t *)(&ProductComplex))) << " " << std::endl;

/*    t_temp_oss << "order_number: " << OrderID << " "
               << "size: " << CxlQty << " "
               << "disclosed_size: " << CxlQty <<" "
               << "price: " << " "
               << "saos: " << ClOrdID << " "
               << "exch_id: " << SecurityID << " "
               << "msg_seq_no: " << NRResponseHeaderME.MsgSeqNum << " "
               << "entry_date_time: " << NRResponseHeaderME.RequestTime << " "
               << "last_activity_reference: " << ExecID << " " << std::endl;
*/
    return t_temp_oss.str();
  }
} DeleteOrderNRResponseT;

// Message:     DeleteOrderResponse
// TemplateID:  10110
// Alias:       Cancel Order Response (Standard Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderMECompT ResponseHeaderME;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    uint64_t ExecID;
    int32_t CumQty;
    int32_t CxlQty;
    char OrdStatus[LEN_ORD_STATUS];
    char ExecType[LEN_EXEC_TYPE];
    uint16_t ExecRestatementReason;
    uint8_t ProductComplex;
    char Pad3[LEN_PAD3];
} DeleteOrderResponseT;

// Message:     DeleteOrderSingleRequest
// TemplateID:  10109
// Alias:       Cancel Order Single
// FIX MsgType: OrderCancelRequest = "F"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    uint64_t ActivityTime;
    int32_t MessageTag;
    int32_t MarketSegmentID;
    uint32_t SimpleSecurityID;
    uint32_t TargetPartyIDSessionID;
    uint32_t RegulatoryID;
    char AlgoID[LEN_ALGOID];
    char Pad4[LEN_PAD4];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
               << "TemplateID: " << MessageHeaderIn.TemplateID << " "
               << "RequestTime: " << RequestHeader.MsgSeqNum << " "
               << "SenderSubID: " << RequestHeader.SenderSubID << " "
               << "OrderID: " << OrderID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "OrigClOrdID: " << OrigClOrdID << " "
               << "ActivityTime: " << ActivityTime << " "
               << "MessageTag: " << MessageTag << " "
               << "MarketSegmentID: " << MarketSegmentID << " "
               << "SimpleSecurityID: " << SimpleSecurityID << " "
               << "TargetPartyIDSessionID: " << TargetPartyIDSessionID << " "
               << "AlgoID: " << AlgoID << " " << std::endl;

    return t_temp_oss.str();
  }

} DeleteOrderSingleRequestT;

// Message:     ForcedLogoutNotification
// TemplateID:  10012
// Alias:       Session Logout Notification
// FIX MsgType: Logout = "5"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NotifHeaderCompT NotifHeader;
    uint16_t VarTextLen;
    char Pad6[LEN_PAD6];
    char VarText[LEN_VAR_TEXT];


    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "SendingTime: " << NotifHeader.SendingTime << " "
               << "VarTextLen: " << VarTextLen << " "
               << "VarText: " << VarText << " " << std::endl;

    return t_temp_oss.str();
  }

} ForcedLogoutNotificationT;

// Message:     GatewayRequest
// TemplateID:  10020
// Alias:       Connection Gateway Request
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t PartyIDSessionID;
    char DefaultCstmApplVerID[LEN_DEFAULT_CSTM_APPL_VERID];
    char Password[LEN_PASSWORD];
    char Pad6[LEN_PAD6];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
               << "TemplateID: " << MessageHeaderIn.TemplateID << " "
               << "MsgSeqNum: " << RequestHeader.MsgSeqNum << " "
               << "PartyIDSessionID: " << PartyIDSessionID << " "
               << "DefaultCstmApplVerID: " << DefaultCstmApplVerID << " "
               << "Password: " << Password << " " << std::endl;

    return t_temp_oss.str();
  }
} GatewayRequestT;

// Message:     GatewayResponse
// TemplateID:  10021
// Alias:       Connection Gateway Response
// FIX MsgType: ApplicationMessageRequestAck = "BX"
//#pragma pack(push, 1)
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    uint32_t GatewayID;
    uint32_t GatewaySubID;
    uint32_t SecondaryGatewayID;
    uint32_t SecondaryGatewaySubID;
    uint8_t SessionMode; //read uint8_t
    uint8_t TradSesMode; //read uint8_t
    char cryptographic_key[32];
    char cryptographic_iv[16];
    char Pad6[LEN_PAD6];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << ResponseHeader.RequestTime << " "
               << "SendingTime: " << ResponseHeader.SendingTime << " "
               << "MsgSeqNum: " << ResponseHeader.MsgSeqNum << " "
               << "GatewayID: " << GatewayID << " "
               << "GatewaySubID: " << GatewaySubID << " "
               << "SecondaryGatewayID: " << SecondaryGatewayID << " "
               << "SecondaryGatewaySubID: " << SecondaryGatewaySubID << " "
               << "SessionMode: " << ((unsigned int)(*(uint8_t *)(&SessionMode))) << " "
               << "TradSesMode: " << ((unsigned int)(*(uint8_t *)(&TradSesMode))) << " " 
               << "SecurityKey: " << cryptographic_key << " "
               << "InitializationVector: " << cryptographic_iv << std::endl;

    return t_temp_oss.str();
  }

} GatewayResponseT;
//#pragma pack(pop)

// Message:     GroupLevelLimitBroadcast
// TemplateID:  10051
// Alias:       Group Level Limit Trader
// FIX MsgType: GroupLevelLimitReport = "U26"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint8_t NoGroupLevelData;
    char Pad7[LEN_PAD7];
    GroupLevelDataSeqT GroupLevelData[MAX_GROUP_LEVEL_LIMIT_BROADCAST_GROUP_LEVEL_DATA];
} GroupLevelLimitBroadcastT;

// Message:     GwOrderAcknowledgement
// TemplateID:  10990
// Alias:       Order Confirmation
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    uint64_t PrimaryOrderID;
    uint64_t ClOrdID;
    int32_t MessageTag;
    char Pad4[LEN_PAD4];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << ResponseHeader.RequestTime << " "
               << "SendingTime: " << ResponseHeader.SendingTime << " "
               << "MsgSeqNum: " << ResponseHeader.MsgSeqNum << " "
               << "PrimaryOrderID: " << PrimaryOrderID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "MessageTag: " << MessageTag << " " << std::endl;

    return t_temp_oss.str();
  }

} GwOrderAcknowledgementT;

// Message:     Heartbeat
// TemplateID:  10011
// Alias:       Heartbeat
// FIX MsgType: Heartbeat = "0"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;

    std::string ToString() {
      std::ostringstream t_temp_oss;

      t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
                 << "TemplateID: " << MessageHeaderIn.TemplateID << " " << std::endl;

      return t_temp_oss.str();
    }

} HeartbeatT;

// Message:     HeartbeatNotification
// TemplateID:  10023
// Alias:       Heartbeat Notification
// FIX MsgType: Heartbeat = "0"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NotifHeaderCompT NotifHeader;

    std::string ToString() {
      std::ostringstream t_temp_oss;

      t_temp_oss << "MSG_LEN: " << MessageHeaderOut.BodyLen << " "
                 << "TemplateID: " << MessageHeaderOut.TemplateID << " "
                 << "SendingTime: " << NotifHeader.SendingTime << " " << std::endl;

      return t_temp_oss.str();
    }
} HeartbeatNotificationT;

// Message:     InquireEnrichmentRuleIDListRequest
// TemplateID:  10040
// Alias:       Trade Enrichment List Inquire
// FIX MsgType: TradeEnrichmentListRequest = "U7"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    char LastEntityProcessed[LEN_LAST_ENTITY_PROCESSED];
} InquireEnrichmentRuleIDListRequestT;

// Message:     InquireEnrichmentRuleIDListResponse
// TemplateID:  10041
// Alias:       Trade Enrichment List Inquire Response
// FIX MsgType: TradeEnrichmentListReport = "U9"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    char LastEntityProcessed[LEN_LAST_ENTITY_PROCESSED];
    uint16_t NoEnrichmentRules;
    char Pad6[LEN_PAD6];
    EnrichmentRulesGrpSeqT EnrichmentRulesGrp[MAX_INQUIRE_ENRICHMENT_RULE_ID_LIST_RESPONSE_ENRICHMENT_RULES_GRP];
} InquireEnrichmentRuleIDListResponseT;

// Message:     InquireMMParameterRequest
// TemplateID:  10305
// Alias:       Inquire Market Maker Parameters
// FIX MsgType: MMParameterRequest = "U17"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int32_t MarketSegmentID;
    uint32_t TargetPartyIDSessionID;
} InquireMMParameterRequestT;

// Message:     InquireMMParameterResponse
// TemplateID:  10306
// Alias:       Inquire Market Maker Parameters Response
// FIX MsgType: MMParameterResponse = "U18"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t MMParameterReportID;
    int32_t MarketSegmentID;
    uint8_t NoMMParameters;
    char Pad3[LEN_PAD3];
    MMParameterGrpSeqT MMParameterGrp[MAX_INQUIRE_MM_PARAMETER_RESPONSE_MM_PARAMETER_GRP];
} InquireMMParameterResponseT;

// Message:     InquireSessionListRequest
// TemplateID:  10035
// Alias:       Session List Inquire
// FIX MsgType: SessionDetailsListRequest = "U5"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
} InquireSessionListRequestT;

// Message:     InquireSessionListResponse
// TemplateID:  10036
// Alias:       Session List Inquire Response
// FIX MsgType: SessionDetailsListResponse = "U6"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    uint16_t NoSessions;
    char Pad6[LEN_PAD6];
    SessionsGrpSeqT SessionsGrp[MAX_INQUIRE_SESSION_LIST_RESPONSE_SESSIONS_GRP];
} InquireSessionListResponseT;

// Message:     InquireUserRequest
// TemplateID:  10038
// Alias:       User List Inquire
// FIX MsgType: PartyDetailListRequest = "CF"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    char LastEntityProcessed[LEN_LAST_ENTITY_PROCESSED];
} InquireUserRequestT;

// Message:     InquireUserResponse
// TemplateID:  10039
// Alias:       User List Inquire Response
// FIX MsgType: PartyDetailListReport = "CG"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    char LastEntityProcessed[LEN_LAST_ENTITY_PROCESSED];
    uint16_t NoPartyDetails;
    char Pad6[LEN_PAD6];
    PartyDetailsGrpSeqT PartyDetailsGrp[MAX_INQUIRE_USER_RESPONSE_PARTY_DETAILS_GRP];
} InquireUserResponseT;

// Message:     InstrumentLevelLimitBroadcast
// TemplateID:  10052
// Alias:       Instrument Level Limit Trader
// FIX MsgType: InstrumentLevelLimitReport = "U27"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint8_t NoInstrumentLevelData;
    char Pad7[LEN_PAD7];
    InstrumentLevelDataSeqT InstrumentLevelData[MAX_INSTRUMENT_LEVEL_LIMIT_BROADCAST_INSTRUMENT_LEVEL_DATA];
} InstrumentLevelLimitBroadcastT;

// Message:     LegalNotificationBroadcast
// TemplateID:  10037
// Alias:       Legal Notification
// FIX MsgType: UserNotification = "CB"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint64_t TransactTime;
    uint16_t VarTextLen;
    uint8_t UserStatus;
    char Pad5[LEN_PAD5];
    char VarText[LEN_VAR_TEXT];
} LegalNotificationBroadcastT;

// Message:     LogonRequest
// TemplateID:  10000
// Alias:       Session Logon
// FIX MsgType: Logon = "A"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t HeartBtInt;
    uint32_t PartyIDSessionID;
    char DefaultCstmApplVerID[LEN_DEFAULT_CSTM_APPL_VERID];
    char Password[LEN_PASSWORD];
    char ApplUsageOrders[LEN_APPL_USAGE_ORDERS];
    char ApplUsageQuotes[LEN_APPL_USAGE_QUOTES];
    char OrderRoutingIndicator[LEN_ORDER_ROUTING_INDICATOR];
    char FIXEngineName[LEN_FIX_ENGINE_NAME];
    char FIXEngineVersion[LEN_FIX_ENGINE_VERSION];
    char FIXEngineVendor[LEN_FIX_ENGINE_VENDOR];
    char ApplicationSystemName[LEN_APPLICATION_SYSTEM_NAME];
    char ApplicationSystemVersion[LEN_APPLICATION_SYSTEM_VERSION];
    char ApplicationSystemVendor[LEN_APPLICATION_SYSTEM_VENDOR];
    char Pad3[LEN_PAD3];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
               << "TemplateID: " << MessageHeaderIn.TemplateID << " "
               << "RequestTime: " << RequestHeader.MsgSeqNum << " "
               << "HeartBtInt: " << HeartBtInt << " "
               << "PartyIDSessionID: " << PartyIDSessionID << " "
               << "DefaultCstmApplVerID: " << DefaultCstmApplVerID << " "
               << "Password: " << Password << " "
               << "ApplUsageOrders: " << ApplUsageOrders << " "
               << "OrderRoutingIndicator: " << OrderRoutingIndicator << " "
               << "FIXEngineName: " << FIXEngineName << " "
               << "FIXEngineVersion: " << FIXEngineVersion << " "
               << "FIXEngineVendor: " << FIXEngineVendor << " "
               << "ApplicationSystemName: " << ApplicationSystemName << " "
               << "ApplicationSystemVersion: " << ApplicationSystemVersion << " "
               << "ApplicationSystemVendor: " << ApplicationSystemVendor << " " << std::endl;

    return t_temp_oss.str();
  }

} LogonRequestT;


typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t PartyIDSessionID;
    char Pad4[LEN_PAD4];
    uint64_t Filler1;

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
               << "TemplateID: " << MessageHeaderIn.TemplateID << " "
               << "RequestTime: " << RequestHeader.MsgSeqNum << " "
               << "PartyIDSessionID: " << PartyIDSessionID << " " << std::endl;

    return t_temp_oss.str();
  }

} SessionRegistrationRequestT;


typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    char status;
    char Pad1[LEN_PAD1];
    uint16_t VarTextLen;      // Counter not used
    char Pad4[LEN_PAD4];      // Fixed String not used
    char VarText[2000];       // Variable String not used

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderOut.BodyLen << " "
               << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << ResponseHeader.MsgSeqNum << " "
               << "Status: " << status << " " << std::endl;

    return t_temp_oss.str();
  }

} SessionRegistrationResponseT;


// Message:     LogonResponse
// TemplateID:  10001
// Alias:       Session Logon Response
// FIX MsgType: Logon = "A"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    int64_t ThrottleTimeInterval;
    uint64_t LastLoginTime;
    uint32_t LastLoginIP;
    uint32_t ThrottleNoMsgs;
    uint32_t ThrottleDisconnectLimit;
    uint32_t HeartBtInt;
    uint32_t SessionInstanceID;
    uint8_t TradSesMode; //read uint8_t
    uint8_t NoOfPartition; //read uint8_t
    uint8_t DaysLeftForPasswdExpiry; //read uint8_t
    uint8_t GraceLoginsLeft; //read uint8_t
    char DefaultCstmApplVerID[LEN_DEFAULT_CSTM_APPL_VERID];
    char Pad2[LEN_PAD2];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << ResponseHeader.RequestTime << " "
               << "SendingTime: " << ResponseHeader.SendingTime << " "
               << "MsgSeqNum: " << ResponseHeader.MsgSeqNum << " "
               << "ThrottleTimeInterval: " << ThrottleTimeInterval << " "
               << "LastLoginTime: " << LastLoginTime << " "
               << "LastLoginIP: " << LastLoginIP << " "
               << "ThrottleNoMsgs: " << ThrottleNoMsgs << " "
               << "ThrottleDisconnectLimit: " << ThrottleDisconnectLimit << " "
               << "HeartBtInt: " << HeartBtInt << " "
               << "SessionInstanceID: " << SessionInstanceID << " "
               << "TradSesMode: " << ((unsigned int)(*(uint8_t *)(&TradSesMode))) << " "
               << "NoOfPartition: " << ((unsigned int)(*(uint8_t *)(&NoOfPartition))) << " "
               << "DaysLeftForPasswdExpiry: " << ((unsigned int)(*(uint8_t *)(&DaysLeftForPasswdExpiry))) << " "
               << "GraceLoginsLeft: " << ((unsigned int)(*(uint8_t *)(&GraceLoginsLeft))) << " "
               << "DefaultCstmApplVerID: " << DefaultCstmApplVerID << " " << std::endl;

    return t_temp_oss.str();
  }
} LogonResponseT;

// Message:     LogoutRequest
// TemplateID:  10002
// Alias:       Session Logout
// FIX MsgType: Logout = "5"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
               << "TemplateID: " << MessageHeaderIn.TemplateID << " "
               << "RequestTime: " << RequestHeader.MsgSeqNum << " " << std::endl;

    return t_temp_oss.str();
  }

} LogoutRequestT;

// Message:     LogoutResponse
// TemplateID:  10003
// Alias:       Session Logout Response
// FIX MsgType: Logout = "5"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << ResponseHeader.RequestTime << " "
               << "SendingTime: " << ResponseHeader.SendingTime << " "
               << "MsgSeqNum: " << ResponseHeader.MsgSeqNum << " " << std::endl;

    return t_temp_oss.str();
  }
} LogoutResponseT;

// Message:     MMParameterDefinitionRequest
// TemplateID:  10303
// Alias:       Market Maker Parameter Definition
// FIX MsgType: MMParameterDefinitionRequest = "U14"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int64_t ExposureDuration;
    int32_t MarketSegmentID;
    uint32_t TargetPartyIDSessionID;
    int32_t CumQty;
    int32_t PctCount;
    int32_t Delta;
    int32_t Vega;
    uint8_t ProductComplex;
    char Pad7[LEN_PAD7];
} MMParameterDefinitionRequestT;

// Message:     MMParameterDefinitionResponse
// TemplateID:  10304
// Alias:       Market Maker Parameter Definition Response
// FIX MsgType: RequestAcknowledge = "U1"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t ExecID;
} MMParameterDefinitionResponseT;

// Message:     MarketPictureInquiryRequest
// TemplateID:  10046
// Alias:       Market Picture Query
// FIX MsgType: MarketPictureInquiryRequest = "U21"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int32_t MarketSegmentID;
    char Pad4[LEN_PAD4];
    int64_t SecurityID;
} MarketPictureInquiryRequestT;

// Message:     MarketPictureInquiryResponse
// TemplateID:  10047
// Alias:       Market Picture Query (report)
// FIX MsgType: MarketPictureInquiryResponse = "U22"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    int64_t SecurityID;
    uint64_t LastUpdatedTime;
    uint64_t TransactTime;
    uint32_t NumberOfTrades;
    int32_t TradeVolume;
    int64_t TradeValue;
    int64_t PrevClosePx;
    int64_t LastPx;
    int64_t LastYTM;
    int64_t LastYTP;
    int64_t LastYTC;
    int64_t OpenPx;
    int64_t ClosePx;
    int64_t HighPx;
    int64_t LowPx;
    int64_t EquiPx;
    int64_t LowerCktPx;
    int64_t UpperCktPx;
    int64_t BlockdealReferencePx;
    int64_t WtAvgPx;
    int32_t LastQty;
    int32_t EquiSize;
    int32_t TotalBuySize;
    int32_t TotalSellSize;
    uint16_t MarketType;
    uint16_t TradingSessionID;
    char SixLakhFlag[LEN_SIX_LAKH_FLAG];
    char TradeValueFlag[LEN_TRADE_VALUE_FLAG];
    char Trend[LEN_TREND];
    uint8_t NoMPInquiry;
    BestRateGrpSeqT BestRateGrp[MAX_MARKET_PICTURE_INQUIRY_RESPONSE_BEST_RATE_GRP];
} MarketPictureInquiryResponseT;

// Message:     MassQuoteRequest
// TemplateID:  10405
// Alias:       Mass Quote Request
// FIX MsgType: MassQuote = "i"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint64_t SenderLocationID;
    uint64_t QuoteID;
    int32_t MarketSegmentID;
    uint32_t RegulatoryID;
    uint16_t EnrichmentRuleID;
    uint8_t AccountType;
    uint8_t PriceValidityCheckType;
    uint8_t QuoteSizeType;
    uint8_t STPCFlag;
    uint8_t NoQuoteEntries;
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char Pad1[LEN_PAD1];
    QuoteEntryGrpSeqT QuoteEntryGrp[MAX_MASS_QUOTE_REQUEST_QUOTE_ENTRY_GRP];
} MassQuoteRequestT;

// Message:     MassQuoteResponse
// TemplateID:  10406
// Alias:       Mass Quote Response
// FIX MsgType: MassQuoteAcknowledgement = "b"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t QuoteID;
    uint64_t QuoteResponseID;
    int32_t MarketSegmentID;
    uint8_t NoQuoteEntries;
    char Pad3[LEN_PAD3];
    QuoteEntryAckGrpSeqT QuoteEntryAckGrp[MAX_MASS_QUOTE_RESPONSE_QUOTE_ENTRY_ACK_GRP];
} MassQuoteResponseT;

// Message:     ModifyOrderComplexRequest
// TemplateID:  10114
// Alias:       Replace Order Complex
// FIX MsgType: MultilegOrderCancelReplace = "AC"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    int64_t Price;
    int64_t MaxPricePercentage;
    uint64_t SenderLocationID;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t MarketSegmentID;
    int32_t MessageTag;
    int32_t OrderQty;
    int32_t MaxShow;
    uint32_t ExpireDate;
    uint32_t TargetPartyIDSessionID;
    uint32_t RegulatoryID;
    uint16_t Filler4;
    char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
    char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
    char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
    uint8_t AccountType;
    uint8_t ApplSeqIndicator;
    uint8_t ProductComplex;
    uint8_t Side;
    uint8_t OrdType;
    uint8_t PriceValidityCheckType;
    uint8_t ExecInst;
    uint8_t TimeInForce;
    uint8_t Filler5;
    uint8_t TradingCapacity;
    char DeltaQtyFlag[LEN_DELTA_QTY_FLAG];
    char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
    char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
    char RegulatoryText[LEN_REGULATORY_TEXT];
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char FreeText3[LEN_FREE_TEXT3];
    uint8_t NoLegs;
    char Pad2[LEN_PAD2];
    LegOrdGrpSeqT LegOrdGrp[MAX_MODIFY_ORDER_COMPLEX_REQUEST_LEG_ORD_GRP];
} ModifyOrderComplexRequestT;

// Message:     ModifyOrderNRResponse
// TemplateID:  10108
// Alias:       Replace Order Response (Lean Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    uint64_t ExecID;
    int64_t PriceMkToLimitPx;
    int64_t Yield;
    int64_t UnderlyingDirtyPrice;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t LeavesQty;
    int32_t CumQty;
    int32_t CxlQty;
    uint16_t Filler4;
    char OrdStatus[LEN_ORD_STATUS + 1];
    char ExecType[LEN_EXEC_TYPE + 1];
    uint16_t ExecRestatementReason;
    uint8_t ProductComplex; //read uint8_t
    uint8_t Filler5;

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << NRResponseHeaderME.RequestTime << " "
               << "RequestOut: " << NRResponseHeaderME.RequestOut << " "
               << "TrdRegTSTimeIn: " << NRResponseHeaderME.TrdRegTSTimeIn << " "
               << "TrdRegTSTimeOut: " << NRResponseHeaderME.TrdRegTSTimeOut << " "
               << "ResponseIn: " << NRResponseHeaderME.ResponseIn << " "
               << "SendingTime: " << NRResponseHeaderME.SendingTime << " "
               << "MsgSeqNum: " << NRResponseHeaderME.MsgSeqNum << " "
               << "LastFragment: " << ((unsigned int)(*(uint8_t *)(&NRResponseHeaderME.LastFragment))) << " "
               << "OrderID: " << OrderID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "OrigClOrdID: " << OrigClOrdID << " "
               << "SecurityID: " << SecurityID << " "
               << "ExecID: " << ExecID << " "
               << "PriceMkToLimitPx: " << PriceMkToLimitPx << " "
               << "Yield: " << Yield << " "
               << "UnderlyingDirtyPrice: " << UnderlyingDirtyPrice << " "
               << "ActivityTime: " << ActivityTime << " "
               << "LeavesQty: " << LeavesQty << " "
               << "CumQty: " << CumQty << " "
               << "CxlQty: " << CxlQty << " "
               << "OrdStatus: " << OrdStatus << " "
               << "ExecType: " << ExecType << " "
               << "ExecRestatementReason: " << ExecRestatementReason << " "
               << "ProductComplex: " << ((unsigned int)(*(uint8_t *)(&ProductComplex))) << " " << std::endl;

/*    t_temp_oss << "order_number: " << OrderID << " "
               << "size: " << (LeavesQty + CumQty + CxlQty) << " "
               << "disclosed_size: " << (LeavesQty + CumQty + CxlQty) <<" "
               << "price: " << PriceMkToLimitPx << " "
               << "saos: " << ClOrdID << " "
               << "exch_id: " << SecurityID << " "
               << "msg_seq_no: " << NRResponseHeaderME.MsgSeqNum << " "
               << "entry_date_time: " << NRResponseHeaderME.RequestTime << " "
               << "last_activity_reference: " << ExecID << " " << std::endl;
*/
    return t_temp_oss.str();
  }
} ModifyOrderNRResponseT;

// Message:     ModifyOrderResponse
// TemplateID:  10107
// Alias:       Replace Order Response (Standard Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderMECompT ResponseHeaderME;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    uint64_t ExecID;
    int64_t PriceMkToLimitPx;
    int64_t Yield;
    int64_t UnderlyingDirtyPrice;
    uint64_t TrdRegTSTimePriority;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t LeavesQty;
    int32_t CumQty;
    int32_t CxlQty;
    uint16_t Filler4;
    char OrdStatus[LEN_ORD_STATUS];
    char ExecType[LEN_EXEC_TYPE];
    uint16_t ExecRestatementReason;
    uint8_t ProductComplex;
    uint8_t Filler5;
} ModifyOrderResponseT;

// Message:     ModifyOrderSingleRequest
// TemplateID:  10106
// Alias:       Replace Order Single
// FIX MsgType: OrderCancelReplaceRequest = "G"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t Price;
    int64_t StopPx;
    int64_t MaxPricePercentage;
    uint64_t SenderLocationID;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t MessageTag;
    int32_t OrderQty;
    int32_t MaxShow;
    uint32_t ExpireDate;
    int32_t MarketSegmentID;
    uint32_t SimpleSecurityID;
    uint32_t TargetPartyIDSessionID;
    uint32_t RegulatoryID;
    uint16_t Filler4;
    char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
    char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
    char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
    uint8_t AccountType;
    uint8_t ApplSeqIndicator;
    uint8_t Side;
    uint8_t OrdType;
    uint8_t PriceValidityCheckType;
    uint8_t TimeInForce;
    uint8_t ExecInst;
    uint8_t Filler5;
    uint8_t TradingSessionSubID;
    uint8_t TradingCapacity;
    char DeltaQtyFlag[LEN_DELTA_QTY_FLAG];
    char Account[LEN_ACCOUNT];
    char PositionEffect[LEN_POSITION_EFFECT];
    char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
    char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
    char RegulatoryText[LEN_REGULATORY_TEXT];
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char FreeText3[LEN_FREE_TEXT3];
    char Pad4[LEN_PAD4];
} ModifyOrderSingleRequestT;

// Message:     ModifyOrderSingleShortRequest
// TemplateID:  10126
// Alias:       Replace Order Single (short layout)
// FIX MsgType: OrderCancelReplaceRequest = "G"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t Price;
    uint64_t SenderLocationID;
    uint64_t ActivityTime;
    int32_t OrderQty;
    int32_t MaxShow;
    uint32_t SimpleSecurityID;
    uint32_t Filler2;
    uint16_t Filler4;
    uint8_t AccountType;
    uint8_t Side;
    uint8_t PriceValidityCheckType;
    uint8_t TimeInForce;
    uint8_t ExecInst;
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char Pad1[LEN_PAD1];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
               << "TemplateID: " << MessageHeaderIn.TemplateID << " "
               << "RequestTime: " << RequestHeader.MsgSeqNum << " "
               << "SenderSubID: " << RequestHeader.SenderSubID << " "
               << "OrderID: " << OrderID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "OrigClOrdID: " << OrigClOrdID << " "
               << "Price: " << Price << " "
               << "SenderLocationID: " << SenderLocationID << " "
               << "ActivityTime: " << ActivityTime << " "
               << "OrderQty: " << OrderQty << " "
               << "MaxShow: " << MaxShow << " "
               << "SimpleSecurityID: " << SimpleSecurityID << " "
               << "AccountType: " << ((unsigned int)(*(uint8_t *)(&AccountType))) << " "
               << "Side: " << ((unsigned int)(*(uint8_t *)(&Side))) << " "
               << "TimeInForce: " << ((unsigned int)(*(uint8_t *)(&TimeInForce))) << " "
               << "AlgoID: " << AlgoID << " "
               << "ClientCode: " << FreeText1 << " "
               << "CPCode: " << CPCode << " " << std::endl;

    return t_temp_oss.str();
  }

} ModifyOrderSingleShortRequestT;

// Message:     MultiLegExecReportBroadcast
// TemplateID:  10994
// Alias:       Extended Order Information(MultiLeg)
// FIX MsgType: MultiLegExecutionReport = "U30"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t SenderLocationID;
    uint64_t ClOrdID;
    uint64_t ExecID;
    uint16_t NoLegExecs;
    uint8_t AccountType;
    uint8_t NoOfMultiLeg;
    uint8_t NoOfMultiLegExecs;
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char Pad3[LEN_PAD3];
    MultiLegGrpSeqT MultiLegGrp[MAX_MULTI_LEG_EXEC_REPORT_BROADCAST_MULTI_LEG_GRP];
    MultiLegFillGrpSeqT MultiLegFillGrp[MAX_MULTI_LEG_EXEC_REPORT_BROADCAST_MULTI_LEG_FILL_GRP];
    InstrmntLegExecGrpSeqT InstrmntLegExecGrp[MAX_MULTI_LEG_EXEC_REPORT_BROADCAST_INSTRMNT_LEG_EXEC_GRP];
} MultiLegExecReportBroadcastT;

// Message:     MultiLegExecResponse
// TemplateID:  10993
// Alias:       Immediate Execution Response(MultiLeg)
// FIX MsgType: MultiLegExecutionReport = "U30"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderMECompT ResponseHeaderME;
    uint64_t ClOrdID;
    uint64_t ExecID;
    uint16_t NoLegExecs;
    uint8_t NoOfMultiLeg;
    uint8_t NoOfMultiLegExecs;
    char AlgoID[LEN_ALGOID];
    char Pad4[LEN_PAD4];
    MultiLegExecGrpSeqT MultiLegExecGrp[MAX_MULTI_LEG_EXEC_RESPONSE_MULTI_LEG_EXEC_GRP];
    MultiLegFillGrpSeqT MultiLegFillGrp[MAX_MULTI_LEG_EXEC_RESPONSE_MULTI_LEG_FILL_GRP];
    InstrmntLegExecGrpSeqT InstrmntLegExecGrp[MAX_MULTI_LEG_EXEC_RESPONSE_INSTRMNT_LEG_EXEC_GRP];
} MultiLegExecResponseT;

// Message:     MultiLegOrderReject
// TemplateID:  10992
// Alias:       Reject(MultiLeg)
// FIX MsgType: MultiLegOrderAcknowledgement = "U29"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    int64_t SecurityID;
    uint32_t SessionRejectReason;
    uint16_t VarTextLen;
    char Pad2[LEN_PAD2];
    char VarText[LEN_VAR_TEXT];
} MultiLegOrderRejectT;

// Message:     MultiLegOrderRequest
// TemplateID:  10991
// Alias:       New Order MultiLeg
// FIX MsgType: MultiLegOrder = "U28"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint64_t SenderLocationID;
    uint64_t ClOrdID;
    uint8_t AccountType;
    char AllOrNoneFlag[LEN_ALL_OR_NONE_FLAG];
    uint8_t NoOfMultiLeg;
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char Pad5[LEN_PAD5];
    MultiLegOrdGrpSeqT MultiLegOrdGrp[MAX_MULTI_LEG_ORDER_REQUEST_MULTI_LEG_ORD_GRP];
} MultiLegOrderRequestT;

// Message:     NewOrderComplexRequest
// TemplateID:  10113
// Alias:       New Order Complex
// FIX MsgType: NewOrderMultileg = "AB"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint64_t ClOrdID;
    int64_t SecurityID;
    int64_t MaxPricePercentage;
    uint64_t SenderLocationID;
    int64_t Price;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t MessageTag;
    int32_t MarketSegmentID;
    int32_t OrderQty;
    int32_t MaxShow;
    uint32_t ExpireDate;
    uint32_t RegulatoryID;
    uint16_t Filler4;
    char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
    char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
    char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
    uint8_t AccountType;
    uint8_t ApplSeqIndicator;
    uint8_t ProductComplex;
    uint8_t Side;
    uint8_t OrdType;
    uint8_t PriceValidityCheckType;
    uint8_t ExecInst;
    uint8_t TimeInForce;
    uint8_t STPCFlag;
    uint8_t Filler5;
    uint8_t TradingCapacity;
    char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
    char RegulatoryText[LEN_REGULATORY_TEXT];
    char AlgoID[LEN_ALGOID];
    char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char FreeText3[LEN_FREE_TEXT3];
    uint8_t NoLegs;
    char Pad6[LEN_PAD6];
    LegOrdGrpSeqT LegOrdGrp[MAX_NEW_ORDER_COMPLEX_REQUEST_LEG_ORD_GRP];
} NewOrderComplexRequestT;

// Message:     NewOrderNRResponse
// TemplateID:  10102
// Alias:       New Order Response (Lean Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t OrderID;
    uint64_t ClOrdID;
    int64_t SecurityID;
    int64_t PriceMkToLimitPx;
    int64_t Yield;
    int64_t UnderlyingDirtyPrice;
    uint64_t ExecID;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    uint16_t Filler4;
    char OrdStatus[LEN_ORD_STATUS + 1];
    char ExecType[LEN_EXEC_TYPE + 1];
    uint16_t ExecRestatementReason;
    uint8_t ProductComplex; //read uint8_t
    uint8_t Filler5;
    char Pad4[LEN_PAD4];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << NRResponseHeaderME.RequestTime << " "
               << "RequestOut: " << NRResponseHeaderME.RequestOut << " "
               << "TrdRegTSTimeIn: " << NRResponseHeaderME.TrdRegTSTimeIn << " "
               << "TrdRegTSTimeOut: " << NRResponseHeaderME.TrdRegTSTimeOut << " "
               << "ResponseIn: " << NRResponseHeaderME.ResponseIn << " "
               << "SendingTime: " << NRResponseHeaderME.SendingTime << " "
               << "MsgSeqNum: " << NRResponseHeaderME.MsgSeqNum << " "
               << "LastFragment: " << ((unsigned int)(*(uint8_t *)(&NRResponseHeaderME.LastFragment))) << " "
               << "OrderID: " << OrderID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "SecurityID: " << SecurityID << " "
               << "PriceMkToLimitPx: " << PriceMkToLimitPx << " "
               << "Yield: " << Yield << " "
               << "UnderlyingDirtyPrice: " << UnderlyingDirtyPrice << " "
               << "ExecID: " << ExecID << " "
               << "ActivityTime: " << ActivityTime << " "
               << "OrdStatus: " << OrdStatus << " "
               << "ExecType: " << ExecType << " "
               << "ExecRestatementReason: " << ExecRestatementReason << " "
               << "ProductComplex: " << ((unsigned int)(*(uint8_t *)(&ProductComplex))) << " " << std::endl;

/*    t_temp_oss << "order_number: " << OrderID << " "
               << "size: " << " " << " " 
               << "disclosed_size: " << " " << " " 
               << "price: " << PriceMkToLimitPx << " "
               << "saos: " << ClOrdID << " "
               << "exch_id: " << SecurityID << " "
               << "msg_seq_no: " << NRResponseHeaderME.MsgSeqNum << " "
               << "entry_date_time: " << NRResponseHeaderME.RequestTime << " "
               << "last_activity_reference: " << ExecID << " " << std::endl;
*/
    return t_temp_oss.str();
  }
} NewOrderNRResponseT;

// Message:     NewOrderResponse
// TemplateID:  10101
// Alias:       New Order Response (Standard Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderMECompT ResponseHeaderME;
    uint64_t OrderID;
    uint64_t ClOrdID;
    int64_t SecurityID;
    int64_t PriceMkToLimitPx;
    int64_t Yield;
    int64_t UnderlyingDirtyPrice;
    uint64_t ExecID;
    uint64_t TrdRegTSEntryTime;
    uint64_t TrdRegTSTimePriority;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    uint16_t Filler4;
    char OrdStatus[LEN_ORD_STATUS];
    char ExecType[LEN_EXEC_TYPE];
    uint16_t ExecRestatementReason;
    uint8_t ProductComplex;
    uint8_t Filler5;
    char Pad4[LEN_PAD4];
} NewOrderResponseT;

// Message:     NewOrderSingleRequest
// TemplateID:  10100
// Alias:       New Order Single
// FIX MsgType: NewOrderSingle = "D"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int64_t Price;
    int64_t StopPx;
    int64_t MaxPricePercentage;
    uint64_t SenderLocationID;
    uint64_t ClOrdID;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t MessageTag;
    int32_t OrderQty;
    int32_t MaxShow;
    uint32_t ExpireDate;
    int32_t MarketSegmentID;
    uint32_t SimpleSecurityID;
    uint32_t RegulatoryID;
    uint16_t Filler4;
    char ContraBroker[LEN_CONTRA_BROKER];
    char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
    char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
    uint8_t AccountType;
    uint8_t ApplSeqIndicator;
    uint8_t Side;
    uint8_t OrdType;
    uint8_t PriceValidityCheckType;
    uint8_t TimeInForce;
    uint8_t ExecInst;
    uint8_t STPCFlag;
    uint8_t Filler5;
    uint8_t TradingSessionSubID;
    uint8_t TradingCapacity;
    char Account[LEN_ACCOUNT];
    char PositionEffect[LEN_POSITION_EFFECT];
    char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
    char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
    char RegulatoryText[LEN_REGULATORY_TEXT];
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char FreeText3[LEN_FREE_TEXT3];
} NewOrderSingleRequestT;

// Message:     NewOrderSingleShortRequest
// TemplateID:  10125
// Alias:       New Order Single (short layout)
// FIX MsgType: NewOrderSingle = "D"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int64_t Price;
    uint64_t SenderLocationID;
    uint64_t ClOrdID;
    int32_t OrderQty;
    int32_t MaxShow;
    uint32_t SimpleSecurityID;
    uint32_t Filler2;
    uint16_t Filler4;
    uint8_t AccountType;
    uint8_t Side;
    uint8_t PriceValidityCheckType;
    uint8_t TimeInForce;
    uint8_t STPCFlag;
    uint8_t ExecInst;
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
               << "TemplateID: " << MessageHeaderIn.TemplateID << " "
               << "RequestTime: " << RequestHeader.MsgSeqNum << " "
               << "SenderSubID: " << RequestHeader.SenderSubID << " "
               << "Price: " << Price << " "
               << "SenderLocationID: " << SenderLocationID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "OrderQty: " << OrderQty << " "
               << "MaxShow: " << MaxShow << " "
               << "SimpleSecurityID: " << SimpleSecurityID << " "
               << "AccountType: " << ((unsigned int)(*(uint8_t *)(&AccountType))) << " "
               << "Side: " << ((unsigned int)(*(uint8_t *)(&Side))) << " "
               << "TimeInForce: " << ((unsigned int)(*(uint8_t *)(&TimeInForce))) << " "
               << "STPCFlag: " << ((unsigned int)(*(uint8_t *)(&STPCFlag))) << " "
               << "AlgoID: " << AlgoID << " "
               << "ClientCode: " << FreeText1 << " "
               << "CPCode: " << CPCode << " " << std::endl;

    return t_temp_oss.str();
  }

} NewOrderSingleShortRequestT;

// Message:     NewsBroadcast
// TemplateID:  10031
// Alias:       News
// FIX MsgType: News = "B"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint64_t OrigTime;
    uint16_t VarTextLen;
    char Headline[LEN_HEADLINE];
    char Pad6[LEN_PAD6];
    char VarText[LEN_VAR_TEXT];
} NewsBroadcastT;

// Message:     OrderBookInquiryRequest
// TemplateID:  10999
// Alias:       Order book inquiry
// FIX MsgType: OrderBookInquiryRequest = "U19"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t PartyIDSessionID;
    uint16_t PartitionID;
    uint8_t OrdType;
    char Pad1[LEN_PAD1];
    int32_t MarketSegmentID;
    char Pad4[LEN_PAD4];
    int64_t SecurityID;
} OrderBookInquiryRequestT;

// Message:     OrderBookInquiryResponse
// TemplateID:  10998
// Alias:       Order book inquiry (report)
// FIX MsgType: OrderBookInquiryResponse = "U20"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint8_t FirstFragment;
    char Pad1[LEN_PAD1];
    uint16_t ExecRestatementReason;
    uint8_t NoOBInquiry;
    char Pad3[LEN_PAD3];
    OBInquiryGrpSeqT OBInquiryGrp[MAX_ORDER_BOOK_INQUIRY_RESPONSE_OB_INQUIRY_GRP];
} OrderBookInquiryResponseT;

// Message:     OrderExecNotification
// TemplateID:  10104
// Alias:       Book Order Execution
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t OrderID;
    uint64_t SenderLocationID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    uint64_t ExecID;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t MessageTag;
    int32_t MarketSegmentID;
    int32_t LeavesQty;
    int32_t CumQty;
    int32_t CxlQty;
    uint16_t NoLegExecs;
    uint16_t Filler4;
    uint16_t ExecRestatementReason;
    uint8_t AccountType;
    uint8_t ProductComplex;
    char OrdStatus[LEN_ORD_STATUS];
    char ExecType[LEN_EXEC_TYPE];
    uint8_t Triggered;
    uint8_t NoFills;
    uint8_t Side;
    uint8_t Filler5;
    char Account[LEN_ACCOUNT];
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char FreeText3[LEN_FREE_TEXT3];
    char Pad4[LEN_PAD4];
    FillsGrpSeqT FillsGrp[MAX_ORDER_EXEC_NOTIFICATION_FILLS_GRP];
    InstrmntLegExecGrpSeqT InstrmntLegExecGrp[MAX_ORDER_EXEC_NOTIFICATION_INSTRMNT_LEG_EXEC_GRP];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    char OrdStatusTemp[LEN_ORD_STATUS + 1];
    char ExecTypeTemp[LEN_EXEC_TYPE + 1];
    char AccountTemp[LEN_ACCOUNT + 1];
    char AlgoIDTemp[LEN_ALGOID + 1];
    char FreeText1Temp[LEN_FREE_TEXT1 + 1];
    char CPCodeTemp[LEN_CP_CODE + 1];
    char FreeText3Temp[LEN_FREE_TEXT3 + 1];

    memcpy((void*)OrdStatusTemp,
           (void*)(OrdStatus),LEN_ORD_STATUS);
    OrdStatusTemp[LEN_ORD_STATUS] = '\0';
    memcpy((void*)ExecTypeTemp,
           (void*)(ExecType),LEN_EXEC_TYPE);
    ExecTypeTemp[LEN_EXEC_TYPE] = '\0';
    memcpy((void*)AccountTemp,
           (void*)(Account),LEN_ACCOUNT);
    AccountTemp[LEN_ACCOUNT] = '\0';
    memcpy((void*)AlgoIDTemp,
           (void*)(AlgoID),LEN_ALGOID);
    AlgoIDTemp[LEN_ALGOID] = '\0';
    memcpy((void*)FreeText1Temp,
           (void*)(FreeText1),LEN_FREE_TEXT1);
    FreeText1Temp[LEN_FREE_TEXT1] = '\0';
    memcpy((void*)CPCodeTemp,
           (void*)(CPCode),LEN_CP_CODE);
    CPCodeTemp[LEN_CP_CODE] = '\0';
    memcpy((void*)FreeText3Temp,
           (void*)(FreeText3),LEN_FREE_TEXT3);
    FreeText3Temp[LEN_FREE_TEXT3] = '\0';

    for ( int i = 0 ; i < LEN_APPL_MSGID; ++i ) {
      int temp = static_cast<unsigned>(RBCHeaderME.ApplMsgID[i]);
      if ( temp > 127 || (temp > 0 && temp < 32) )
        RBCHeaderME.ApplMsgID[i] = 0;
    }

    t_temp_oss << "MSG_LEN: " << MessageHeaderOut.BodyLen << " "
               << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "TrdRegTSTimeOut: " << RBCHeaderME.TrdRegTSTimeOut << " "
               << "SendingTime: " << RBCHeaderME.SendingTime << " "
               << "ApplSubID: " << RBCHeaderME.ApplSubID << " "
               << "PartitionID: " << RBCHeaderME.PartitionID << " "
               //<< "ApplMsgID: " << RBCHeaderME.ApplMsgID << " "
               << "ApplID: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.ApplID))) << " "
               << "ApplResendFlag: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.ApplResendFlag))) << " "
               << "LastFragment: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.LastFragment))) << " "
               << "OrderID: " << OrderID << " "
               << "SenderLocationID: " << SenderLocationID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "OrigClOrdID: " << OrigClOrdID << " "
               << "SecurityID: " << SecurityID << " "
               << "ExecID: " << ExecID << " "
               << "ActivityTime: " << ActivityTime << " "
               << "MessageTag: " << MessageTag << " "
               << "MarketSegmentID: " << MarketSegmentID << " "
               << "LeavesQty: " << LeavesQty << " "
               << "CumQty: " << CumQty << " "
               << "CxlQty: " << CxlQty << " "
               << "Price: " << FillsGrp[0].FillPx << " "
               << "NoLegExecs: " << NoLegExecs << " "
               << "ExecRestatementReason: " << ExecRestatementReason << " "
               << "AccountType: " << ((unsigned int)(*(uint8_t *)(&AccountType))) << " "
               << "ProductComplex: " << ((unsigned int)(*(uint8_t *)(&ProductComplex))) << " "
               << "OrdStatus: " << OrdStatusTemp << " "
               << "ExecType: " << ExecTypeTemp << " "
               << "Triggered: " << ((unsigned int)(*(uint8_t *)(&Triggered))) << " "
               << "NoFills: " << ((unsigned int)(*(uint8_t *)(&NoFills))) << " "
               << "Side: " << ((unsigned int)(*(uint8_t *)(&Side))) << " "
               << "Account: " << AccountTemp << " "
               << "AlgoID: " << AlgoIDTemp << " "
               << "FreeText1: " << FreeText1Temp << " "
               << "CPCode: " << CPCodeTemp << " "
               << "FreeText3: " << FreeText3Temp << " " << std::endl;

/*    t_temp_oss << "order_number: " << OrderID << " "
               << "size_executed: " << CumQty << " "
               << "size_remaining: " << LeavesQty <<" "
               << "price: " << FillsGrp[0].FillPx << " "
               << "buy_sell: " << ((unsigned int)(*(uint8_t *)(&Side))) << " "
               << "saos: " << ClOrdID << " "
               << "exch_id: " << SecurityID << " "
               << "msg_seq_no: " << " "
               << "entry_date_time: " << RBCHeaderME.SendingTime << " "
               << "last_activity_reference: " << ExecID << " " << std::endl;
*/
    return t_temp_oss.str();
    }     

} OrderExecNotificationT;

// Message:     OrderExecReportBroadcast
// TemplateID:  10117
// Alias:       Extended Order Information
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    int64_t MaxPricePercentage;
    uint64_t SenderLocationID;
    uint64_t ExecID;
    uint64_t TrdRegTSEntryTime;
    uint64_t TrdRegTSTimePriority;
    int64_t Price;
    int64_t StopPx;
    int64_t UnderlyingDirtyPrice;
    int64_t Yield;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t MarketSegmentID;
    int32_t MessageTag;
    int32_t LeavesQty;
    int32_t MaxShow;
    int32_t CumQty;
    int32_t CxlQty;
    int32_t OrderQty;
    uint32_t ExpireDate;
    uint32_t PartyIDExecutingUnit;
    uint32_t PartyIDSessionID;
    uint32_t PartyIDExecutingTrader;
    uint32_t PartyIDEnteringTrader;
    uint16_t Filler4;
    uint16_t NoLegExecs;
    uint16_t ExecRestatementReason;
    uint8_t AccountType;
    uint8_t PartyIDEnteringFirm;
    uint8_t ProductComplex;
    char OrdStatus[LEN_ORD_STATUS + 1];
    char ExecType[LEN_EXEC_TYPE + 1];
    uint8_t Side;
    uint8_t OrdType;
    uint8_t TradingCapacity;
    uint8_t TimeInForce;
    uint8_t ExecInst;
    uint8_t TradingSessionSubID;
    uint8_t ApplSeqIndicator;
    uint8_t STPCFlag;
    uint8_t Filler5;
    char Account[LEN_ACCOUNT];
    char PositionEffect[LEN_POSITION_EFFECT];
    char ContraBroker[LEN_CONTRA_BROKER];
    char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
    char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
    char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
    char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
    char RegulatoryText[LEN_REGULATORY_TEXT];
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char FreeText3[LEN_FREE_TEXT3];
    uint8_t NoFills;
    uint8_t NoLegs;
    uint8_t Triggered;
    char Pad2[LEN_PAD2];
    LegOrdGrpSeqT LegOrdGrp[MAX_ORDER_EXEC_REPORT_BROADCAST_LEG_ORD_GRP];
    FillsGrpSeqT FillsGrp[MAX_ORDER_EXEC_REPORT_BROADCAST_FILLS_GRP];
    InstrmntLegExecGrpSeqT InstrmntLegExecGrp[MAX_ORDER_EXEC_REPORT_BROADCAST_INSTRMNT_LEG_EXEC_GRP];


    std::string ToString() {
      std::ostringstream t_temp_oss;

      t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "TrdRegTSTimeOut: " << RBCHeaderME.TrdRegTSTimeOut << " "
               << "SendingTime: " << RBCHeaderME.SendingTime << " "
               << "ApplSubID: " << RBCHeaderME.ApplSubID << " "
               << "PartitionID: " << RBCHeaderME.PartitionID << " "
//               << "ApplMsgID: " << RBCHeaderME.ApplMsgID << " "
               << "ApplID: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.ApplID))) << " "
               << "ResendFlag: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.ApplResendFlag))) << " "
               << "LastFragment: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.LastFragment))) << " "
               << "OrderID: " << OrderID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "OrigClOrdID: " << OrigClOrdID << " "
               << "SecurityID: " << SecurityID << " "
               << "MaxPricePercentage: " << MaxPricePercentage << " "
               << "SenderLocationID : " << SenderLocationID << " "
               << "ExecID: " << ExecID << " "
               << "TrdRegTSEntryTime: " << TrdRegTSEntryTime << " "
               << "TrdRegTSTimePriority: " << TrdRegTSTimePriority << " "
               << "Price: " << Price << " "
               << "StopPx: " << StopPx << " "
               << "UnderlyingDirtyPrice: " << UnderlyingDirtyPrice << " "
               << "Yield: " << Yield << " "
               << "ActivityTime: " << ActivityTime << " "
               << "MarketSegmentID: " << MarketSegmentID << " "
               << "MessageTag: " << MessageTag << " "
               << "LeavesQty: " << LeavesQty << " "
               << "MaxShow: " << MaxShow << " "
               << "CumQty: " << CumQty << " "
               << "CxlQty: " << CxlQty << " "
               << "OrderQty: " << OrderQty << " "
               << "PartyIDExecutingUnit: " << PartyIDExecutingUnit << " "
               << "PartyIDSessionID: " << PartyIDSessionID << " "
               << "PartyIDExecutingTrader: " << PartyIDExecutingTrader << " "
               << "PartyIDEnteringTrader: " << PartyIDEnteringTrader << " "
               << "NoLegExecs: " << NoLegExecs << " "
               << "ExecRestatementReason: " << ExecRestatementReason << " "
               << "AccountType: " << ((unsigned int)(*(uint8_t *)(&AccountType))) << " "
               << "PartyIDEnteringFirm: " << ((unsigned int)(*(uint8_t *)(&PartyIDEnteringFirm))) << " "
               << "ProductComplex: " << ((unsigned int)(*(uint8_t *)(&ProductComplex))) << " "
               << "OrdStatus: " << OrdStatus << " "
               << "ExecType: " << ExecType << " "
               << "Side: " << ((unsigned int)(*(uint8_t *)(&Side))) << " "
               << "OrdType: " << ((unsigned int)(*(uint8_t *)(&OrdType))) << " "
               << "TimeInForce: " << ((unsigned int)(*(uint8_t *)(&TimeInForce))) << " "
               << "ExecInst: " << ((unsigned int)(*(uint8_t *)(&ExecInst))) << " "
               << "ApplSeqIndicator: " << ((unsigned int)(*(uint8_t *)(&ApplSeqIndicator))) << " "
               << "STPCFlag: " << ((unsigned int)(*(uint8_t *)(&STPCFlag))) << " "
               << "PositionEffect: " << PositionEffect << " "
               << "RegulatoryText: " << RegulatoryText << " "
               << "AlgoID: " << AlgoID << " "
               << "FreeText1: " << FreeText1 << " "
               << "CPCode: " << CPCode << " "
               << "FreeText3: " << FreeText3 << " "
               << "NoFills: " << ((unsigned int)(*(uint8_t *)(&NoFills))) << " "
               << "NoLegs: " << ((unsigned int)(*(uint8_t *)(&NoLegs))) << " "
               << "Triggered: " << ((unsigned int)(*(uint8_t *)(&Triggered))) << " " << std::endl;

      return t_temp_oss.str();
    }

} OrderExecReportBroadcastT;

// Message:     OrderExecResponse
// TemplateID:  10103
// Alias:       Immediate Execution Response
// FIX MsgType: ExecutionReport = "8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderMECompT ResponseHeaderME;
    uint64_t OrderID;
    uint64_t ClOrdID;
    uint64_t OrigClOrdID;
    int64_t SecurityID;
    uint64_t ExecID;
    uint64_t TrdRegTSEntryTime;
    uint64_t TrdRegTSTimePriority;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t MarketSegmentID;
    int32_t LeavesQty;
    int32_t CumQty;
    int32_t CxlQty;
    uint16_t Filler4;
    uint16_t NoLegExecs;
    uint16_t ExecRestatementReason;
    uint8_t ProductComplex;
    char OrdStatus[LEN_ORD_STATUS];
    char ExecType[LEN_EXEC_TYPE];
    uint8_t Triggered;
    uint8_t Filler5;
    uint8_t NoFills;
    char AlgoID[LEN_ALGOID];
    FillsGrpSeqT FillsGrp[MAX_ORDER_EXEC_RESPONSE_FILLS_GRP];
    InstrmntLegExecGrpSeqT InstrmntLegExecGrp[MAX_ORDER_EXEC_RESPONSE_INSTRMNT_LEG_EXEC_GRP];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    char OrdStatusTemp[LEN_ORD_STATUS + 1];
    char ExecTypeTemp[LEN_EXEC_TYPE + 1];
    char AlgoIDTemp[LEN_ALGOID + 1];

    memcpy((void*)OrdStatusTemp,
           (void*)(OrdStatus),LEN_ORD_STATUS);
    OrdStatusTemp[LEN_ORD_STATUS] = '\0';
    memcpy((void*)ExecTypeTemp,
           (void*)(ExecType),LEN_EXEC_TYPE);
    ExecTypeTemp[LEN_EXEC_TYPE] = '\0';
    memcpy((void*)AlgoIDTemp,
           (void*)(AlgoID),LEN_ALGOID);
    AlgoIDTemp[LEN_ALGOID] = '\0';

    for ( int i = 0 ; i < LEN_APPL_MSGID; ++i ) {
      int temp = static_cast<unsigned>(ResponseHeaderME.ApplMsgID[i]);
      if ( temp > 127 || (temp > 0 && temp < 32) )
        ResponseHeaderME.ApplMsgID[i] = 0;
    }

    t_temp_oss << "MSG_LEN: " << MessageHeaderOut.BodyLen << " "
               << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << ResponseHeaderME.RequestTime << " "
               << "RequestOut: " << ResponseHeaderME.RequestOut << " "
               << "TrdRegTSTimeIn: " << ResponseHeaderME.TrdRegTSTimeIn << " "
               << "TrdRegTSTimeOut: " << ResponseHeaderME.TrdRegTSTimeOut << " "
               << "ResponseIn: " << ResponseHeaderME.ResponseIn << " "
               << "SendingTime: " << ResponseHeaderME.SendingTime << " "
               << "MsgSeqNum: " << ResponseHeaderME.MsgSeqNum << " "
               << "ApplID: " << ((unsigned int)(*(uint8_t *)(&ResponseHeaderME.ApplID))) << " "
               //<< "ApplMsgID: " << ResponseHeaderME.ApplMsgID << " "
               << "LastFragment: " << ((unsigned int)(*(uint8_t *)(&ResponseHeaderME.LastFragment))) << " "
               << "OrderID: " << OrderID << " "
               << "ClOrdID: " << ClOrdID << " "
               << "OrigClOrdID: " << OrigClOrdID << " "
               << "SecurityID: " << SecurityID << " "
               << "ExecID: " << ExecID << " "
               << "TrdRegTSEntryTime: " << TrdRegTSEntryTime << " "
               << "TrdRegTSTimePriority: " << TrdRegTSTimePriority << " "
               << "ActivityTime: " << ActivityTime << " "
               << "MarketSegmentID: " << MarketSegmentID << " "
               << "LeavesQty: " << LeavesQty << " "
               << "CumQty: " << CumQty << " "
               << "CxlQty: " << CxlQty << " "
               << "Price: " << FillsGrp[0].FillPx << " "
               << "NoLegExecs: " << NoLegExecs << " "
               << "ExecRestatementReason: " << ExecRestatementReason << " "
               << "ProductComplex: " << ((unsigned int)(*(uint8_t *)(&ProductComplex))) << " "
               << "OrdStatus: " << OrdStatusTemp << " "
               << "ExecType: " << ExecTypeTemp << " "
               << "Triggered: " << ((unsigned int)(*(uint8_t *)(&Triggered))) << " "
               << "NoFills: " << ((unsigned int)(*(uint8_t *)(&NoFills))) << " "
               << "AlgoID: " << AlgoIDTemp << " " << std::endl;

/*    t_temp_oss << "order_number: " << OrderID << " "
               << "size_executed: " << CumQty << " "
               << "size_remaining: " << LeavesQty <<" "
               << "price: " << FillsGrp[0].FillPx << " "
               << "buy_sell: " << " "
               << "saos: " << ClOrdID << " "
               << "exch_id: " << SecurityID << " "
               << "msg_seq_no: " << ResponseHeaderME.MsgSeqNum << " "
               << "entry_date_time: " << ResponseHeaderME.RequestTime << " "
               << "last_activity_reference: " << ExecID << " " << std::endl;
*/
    return t_temp_oss.str();
  }

} OrderExecResponseT;

// Message:     PartyActionReport
// TemplateID:  10042
// Alias:       Party Action Report
// FIX MsgType: PartyActionReport = "DI"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint64_t TransactTime;
    uint32_t TradeDate;
    uint32_t RequestingPartyIDExecutingTrader;
    uint32_t PartyIDExecutingUnit;
    uint32_t PartyIDExecutingTrader;
    uint32_t RequestingPartyIDExecutingSystem;
    uint16_t MarketID;
    uint8_t PartyActionType;
    uint8_t RequestingPartyIDEnteringFirm;
} PartyActionReportT;

// Message:     PartyEntitlementsUpdateReport
// TemplateID:  10034
// Alias:       Entitlement Notification
// FIX MsgType: PartyEntitlementsUpdateReport = "CZ"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint64_t TransactTime;
    uint32_t TradeDate;
    uint32_t PartyDetailIDExecutingUnit;
    uint32_t RequestingPartyIDExecutingSystem;
    uint16_t MarketID;
    char ListUpdateAction[LEN_LIST_UPDATE_ACTION];
    char RequestingPartyEnteringFirm[LEN_REQUESTING_PARTY_ENTERING_FIRM];
    char RequestingPartyClearingFirm[LEN_REQUESTING_PARTY_CLEARING_FIRM];
    uint8_t PartyDetailStatus;
    char Pad6[LEN_PAD6];
} PartyEntitlementsUpdateReportT;

// Message:     QuoteActivationNotification
// TemplateID:  10411
// Alias:       Quote Activation Notification
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t MassActionReportID;
    int32_t MarketSegmentID;
    uint32_t PartyIDEnteringTrader;
    uint16_t NoNotAffectedSecurities;
    uint8_t PartyIDEnteringFirm;
    uint8_t ProductComplex;
    uint8_t MassActionType;
    uint8_t MassActionReason;
    char Pad2[LEN_PAD2];
    NotAffectedSecuritiesGrpSeqT NotAffectedSecuritiesGrp[MAX_QUOTE_ACTIVATION_NOTIFICATION_NOT_AFFECTED_SECURITIES_GRP];
} QuoteActivationNotificationT;

// Message:     QuoteActivationRequest
// TemplateID:  10403
// Alias:       Quote Activation Request
// FIX MsgType: OrderMassActionRequest = "CA"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int32_t MarketSegmentID;
    uint32_t TargetPartyIDSessionID;
    uint32_t RegulatoryID;
    uint8_t MassActionType;
    uint8_t ProductComplex;
    char Pad2[LEN_PAD2];
} QuoteActivationRequestT;

// Message:     QuoteActivationResponse
// TemplateID:  10404
// Alias:       Quote Activation Response
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t MassActionReportID;
    uint16_t NoNotAffectedSecurities;
    char Pad6[LEN_PAD6];
    NotAffectedSecuritiesGrpSeqT NotAffectedSecuritiesGrp[MAX_QUOTE_ACTIVATION_RESPONSE_NOT_AFFECTED_SECURITIES_GRP];
} QuoteActivationResponseT;

// Message:     QuoteExecReportBroadcast
// TemplateID:  10412
// Alias:       Quote Cancellation Notification
// FIX MsgType: QuoteExecutionReport = "U8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t ExecID;
    uint8_t NoQuoteEvents;
    uint8_t STPCFlag;
    char Pad6[LEN_PAD6];
    QuoteEventGrpSeqT QuoteEventGrp[MAX_QUOTE_EXEC_REPORT_BROADCAST_QUOTE_EVENT_GRP];
} QuoteExecReportBroadcastT;

// Message:     QuoteExecutionReport
// TemplateID:  10407
// Alias:       Quote Execution Notification
// FIX MsgType: QuoteExecutionReport = "U8"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    uint64_t ExecID;
    int32_t MarketSegmentID;
    uint16_t NoLegExecs;
    uint8_t NoQuoteEvents;
    uint8_t STPCFlag;
    QuoteEventGrpSeqT QuoteEventGrp[MAX_QUOTE_EXECUTION_REPORT_QUOTE_EVENT_GRP];
    QuoteLegExecGrpSeqT QuoteLegExecGrp[MAX_QUOTE_EXECUTION_REPORT_QUOTE_LEG_EXEC_GRP];
} QuoteExecutionReportT;

// Message:     RFQRequest
// TemplateID:  10401
// Alias:       Quote Request
// FIX MsgType: QuoteRequest = "R"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    int64_t SecurityID;
    int32_t MarketSegmentID;
    int32_t OrderQty;
    uint32_t RegulatoryID;
    uint8_t Side;
    char RegulatoryText[LEN_REGULATORY_TEXT];
    char Pad7[LEN_PAD7];
} RFQRequestT;

// Message:     RFQResponse
// TemplateID:  10402
// Alias:       Quote Request Response
// FIX MsgType: RequestAcknowledge = "U1"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint64_t ExecID;
} RFQResponseT;

// Message:     Reject
// TemplateID:  10010
// Alias:       Reject
// FIX MsgType: Reject = "
#pragma pack(push, 1)
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRResponseHeaderMECompT NRResponseHeaderME;
    uint32_t SessionRejectReason;
    uint16_t VarTextLen;
    uint8_t SessionStatus; //read uint8_t
    char Pad1[LEN_PAD1];
    char VarText[LEN_VAR_TEXT];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    for ( int i = 0 ; i <= LEN_VAR_TEXT; ++i ) {
      if ( static_cast<unsigned>(VarText[i]) > 127 ) 
        VarText[i] = 0;
    }

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << NRResponseHeaderME.RequestTime << " "
               << "RequestOut: " << NRResponseHeaderME.RequestOut << " "
               << "TrdRegTSTimeIn: " << NRResponseHeaderME.TrdRegTSTimeIn << " "
               << "TrdRegTSTimeOut: " << NRResponseHeaderME.TrdRegTSTimeOut << " "
               << "ResponseIn: " << NRResponseHeaderME.ResponseIn << " "
               << "SendingTime: " << NRResponseHeaderME.SendingTime << " "
               << "MsgSeqNum: " << NRResponseHeaderME.MsgSeqNum << " "
               << "LastFragment: " << ((unsigned int)(*(uint8_t *)(&NRResponseHeaderME.LastFragment))) << " "
               << "SessionRejectReason: " << SessionRejectReason << " "
               << "VarTextLen: " << VarTextLen << " "
               << "SessionStatus: " << ((unsigned int)(*(uint8_t *)(&SessionStatus))) << " "
               << "VarText: " << VarText << " " << std::endl;

/*    t_temp_oss << "msg_seq_no: " << NRResponseHeaderME.MsgSeqNum << " "
               << "entry_date_time: " << NRResponseHeaderME.RequestTime << " "
               << "last_activity_reference: " << NRResponseHeaderME.RequestOut << " "
               << "reason_code: " << SessionRejectReason << " "
               << "reason: " << VarText << " "
               << "session_status: " << ((unsigned int)(*(uint8_t *)(&SessionStatus))) << std::endl;
*/
    return t_temp_oss.str();
  }
} RejectT;
#pragma pack(pop)

// Message:     RetransmitMEMessageRequest
// TemplateID:  10026
// Alias:       Retransmit (Order/Quote Event)
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t SubscriptionScope;
    uint16_t PartitionID;
    uint8_t RefApplID;
    char ApplBegMsgID[LEN_APPL_BEG_MSGID];
    char ApplEndMsgID[LEN_APPL_END_MSGID];
    char Pad1[LEN_PAD1];
} RetransmitMEMessageRequestT;

// Message:     RetransmitMEMessageResponse
// TemplateID:  10027
// Alias:       Retransmit Response (Order/Quote Event)
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    uint16_t ApplTotalMessageCount;
    char ApplEndMsgID[LEN_APPL_END_MSGID];
    char RefApplLastMsgID[LEN_REF_APPL_LAST_MSGID];
    char Pad6[LEN_PAD6];
} RetransmitMEMessageResponseT;

// Message:     RetransmitRequest
// TemplateID:  10008
// Alias:       Retransmit
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint64_t ApplBegSeqNum;
    uint64_t ApplEndSeqNum;
    uint32_t SubscriptionScope;
    uint16_t PartitionID;
    uint8_t RefApplID;
    char Pad1[LEN_PAD1];
} RetransmitRequestT;

// Message:     RetransmitResponse
// TemplateID:  10009
// Alias:       Retransmit Response
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    uint64_t ApplEndSeqNum;
    uint64_t RefApplLastSeqNum;
    uint16_t ApplTotalMessageCount;
    char Pad6[LEN_PAD6];
} RetransmitResponseT;

// Message:     RiskCollateralAlertAdminBroadcast
// TemplateID:  10048
// Alias:       Risk Collateral Alert Admin
// FIX MsgType: CollateralAlertAdminReport = "U23"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    int64_t TotalCollateral;
    int64_t UtilizedCollateral;
    int64_t UnutilizedCollateral;
    uint64_t OrigTime;
    int32_t PercentageUtilized;
    int32_t MarketSegmentID;
    uint16_t MarketID;
    uint16_t VarTextLen;
    uint8_t RRMState;
    uint8_t MemberType;
    int8_t IncrementDecrementStatus;
    uint8_t SegmentIndicator;
    uint8_t Duration;
    char FreeText1[LEN_FREE_TEXT1];
    char BusinessUnitSymbol[LEN_BUSINESS_UNIT_SYMBOL];
    char Pad3[LEN_PAD3];
    char VarText[LEN_VAR_TEXT];
} RiskCollateralAlertAdminBroadcastT;

// Message:     RiskCollateralAlertBroadcast
// TemplateID:  10049
// Alias:       Risk Collateral Alert Trader
// FIX MsgType: CollateralAlertReport = "U24"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint64_t OrigTime;
    int32_t PercentageUtilized;
    int32_t MarketSegmentID;
    uint16_t MarketID;
    uint16_t VarTextLen;
    uint8_t RRMState;
    uint8_t MemberType;
    int8_t IncrementDecrementStatus;
    uint8_t SegmentIndicator;
    uint8_t Duration;
    char FreeText1[LEN_FREE_TEXT1];
    char BusinessUnitSymbol[LEN_BUSINESS_UNIT_SYMBOL];
    char Pad3[LEN_PAD3];
    char VarText[LEN_VAR_TEXT];
} RiskCollateralAlertBroadcastT;

// Message:     RiskNotificationBroadcast
// TemplateID:  10033
// Alias:       Risk Notification
// FIX MsgType: PartyRiskLimitsUpdateReport = "CR"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint64_t TransactTime;
    uint32_t TradeDate;
    uint32_t PartyDetailIDExecutingUnit;
    uint32_t RequestingPartyIDExecutingSystem;
    int32_t MarketSegmentID;
    uint16_t MarketID;
    uint8_t RiskModeStatus;
    uint8_t SegmentIndicator;
    char ListUpdateAction[LEN_LIST_UPDATE_ACTION];
    uint8_t RiskLimitAction;
    uint8_t ScopeIdentifier;
    char FreeText1[LEN_FREE_TEXT1];
    char RequestingPartyEnteringFirm[LEN_REQUESTING_PARTY_ENTERING_FIRM];
    char RequestingPartyClearingFirm[LEN_REQUESTING_PARTY_CLEARING_FIRM];
    char BusinessUnitSymbol[LEN_BUSINESS_UNIT_SYMBOL];
    char Pad3[LEN_PAD3];
} RiskNotificationBroadcastT;

// Message:     ServiceAvailabilityBroadcast
// TemplateID:  10030
// Alias:       Service Availability
// FIX MsgType: UserNotification = "CB"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NRBCHeaderCompT NRBCHeader;
    uint32_t MatchingEngineTradeDate;
    uint32_t TradeManagerTradeDate;
    uint32_t ApplSeqTradeDate;
    uint16_t PartitionID;
    uint8_t MatchingEngineStatus;
    uint8_t TradeManagerStatus;
    uint8_t ApplSeqStatus;
    char Pad7[LEN_PAD7];
} ServiceAvailabilityBroadcastT;

// Message:     SessionPasswordChangeRequest
// TemplateID:  10997
// Alias:       Session Password Change
// FIX MsgType: Logon = "A"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t PartyIDSessionID;
    char Password[LEN_PASSWORD];
    char NewPassword[LEN_NEW_PASSWORD];
    char Pad4[LEN_PAD4];
} SessionPasswordChangeRequestT;

// Message:     SessionPasswordChangeResponse
// TemplateID:  10995
// Alias:       Session Password Change Response
// FIX MsgType: Logon = "A"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
} SessionPasswordChangeResponseT;

// Message:     SubscribeRequest
// TemplateID:  10025
// Alias:       Subscribe
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t SubscriptionScope;
    uint8_t RefApplID;
    char Pad3[LEN_PAD3];
} SubscribeRequestT;

// Message:     SubscribeResponse
// TemplateID:  10005
// Alias:       Subscribe Response
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    uint32_t ApplSubID;
    char Pad4[LEN_PAD4];
} SubscribeResponseT;

// Message:     TMTradingSessionStatusBroadcast
// TemplateID:  10501
// Alias:       Trade Notification Status
// FIX MsgType: TradingSessionStatus = "h"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    uint8_t TradSesEvent;
    char Pad7[LEN_PAD7];
} TMTradingSessionStatusBroadcastT;

// Message:     ThrottleUpdateNotification
// TemplateID:  10028
// Alias:       Throttle Update Notification
// FIX MsgType: UserNotification = "CB"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    NotifHeaderCompT NotifHeader;
    int64_t ThrottleTimeInterval;
    uint32_t ThrottleNoMsgs;
    uint32_t ThrottleDisconnectLimit;
} ThrottleUpdateNotificationT;

// Message:     TradeBroadcast
// TemplateID:  10500
// Alias:       Trade Notification
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    int64_t SecurityID;
    int64_t RelatedSecurityID;
    int64_t Price;
    int64_t LastPx;
    int64_t SideLastPx;
    int64_t ClearingTradePrice;
    int64_t Yield;
    int64_t UnderlyingDirtyPrice;
    uint64_t TransactTime;
    uint64_t OrderID;
    uint64_t SenderLocationID;
    uint64_t ClOrdID;
    uint64_t ActivityTime;
    uint64_t Filler1;
    uint32_t Filler2;
    int32_t MessageTag;
    uint32_t TradeID;
    uint32_t OrigTradeID;
    uint32_t RootPartyIDExecutingUnit;
    uint32_t RootPartyIDSessionID;
    uint32_t RootPartyIDExecutingTrader;
    uint32_t RootPartyIDClearingUnit;
    int32_t CumQty;
    int32_t LeavesQty;
    int32_t MarketSegmentID;
    int32_t RelatedSymbol;
    int32_t LastQty;
    int32_t SideLastQty;
    int32_t ClearingTradeQty;
    uint32_t SideTradeID;
    uint32_t MatchDate;
    uint32_t TrdMatchID;
    uint32_t StrategyLinkID;
    int32_t TotNumTradeReports;
    uint16_t Filler4;
    uint8_t MultiLegReportingType;
    uint8_t TradeReportType;
    uint8_t TransferReason;
    uint8_t Filler5;
    char RootPartyIDBeneficiary[LEN_ROOT_PARTY_ID_BENEFICIARY];
    char RootPartyIDTakeUpTradingFirm[LEN_ROOT_PARTY_ID_TAKE_UP_TRADING_FIRM];
    char RootPartyIDOrderOriginationFirm[LEN_ROOT_PARTY_ID_ORDER_ORIGINATION_FIRM];
    uint8_t AccountType;
    uint8_t MatchType;
    uint8_t MatchSubType;
    uint8_t Side;
    uint8_t AggressorIndicator;
    uint8_t TradingCapacity;
    char Account[LEN_ACCOUNT];
    char PositionEffect[LEN_POSITION_EFFECT];
    char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
    char AlgoID[LEN_ALGOID];
    char FreeText1[LEN_FREE_TEXT1];
    char CPCode[LEN_CP_CODE];
    char FreeText3[LEN_FREE_TEXT3];
    char OrderCategory[LEN_ORDER_CATEGORY];
    uint8_t OrdType;
    uint8_t RelatedProductComplex;
    uint8_t OrderSide;
    char RootPartyClearingOrganization[LEN_ROOT_PARTY_CLEARING_ORGANIZATION];
    char RootPartyExecutingFirm[LEN_ROOT_PARTY_EXECUTING_FIRM];
    char RootPartyExecutingTrader[LEN_ROOT_PARTY_EXECUTING_TRADER];
    char RootPartyClearingFirm[LEN_ROOT_PARTY_CLEARING_FIRM];
    char Pad7[LEN_PAD7];
} TradeBroadcastT;

// Message:     TradeEnhancementBroadcast
// TemplateID:  10989
// Alias:       Trade Enhancement Notification
// FIX MsgType: TradeEnhancementStatus = "U31"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    int64_t SecurityID;
    int64_t ClearingTradePrice;
    uint64_t TransactTime;
    uint64_t OrderID;
    uint32_t TradeID;
    uint32_t RootPartyIDSessionID;
    uint32_t SideTradeID;
    int32_t MarketSegmentID;
    uint32_t MatchDate;
    int32_t ClearingTradeQty;
    uint8_t AccountType;
    uint8_t Side;
    char AutoAcceptIndicator[LEN_AUTO_ACCEPT_INDICATOR];
    char CPCode[LEN_CP_CODE];
    char FreeText1[LEN_FREE_TEXT1];
    char Pad5[LEN_PAD5];
} TradeEnhancementBroadcastT;

// Message:     TradingSessionStatusBroadcast
// TemplateID:  10307
// Alias:       Trading Session Event
// FIX MsgType: TradingSessionStatus = "h"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderMECompT RBCHeaderME;
    int32_t MarketSegmentID;
    uint32_t TradeDate;
    uint8_t TradSesEvent; //read uint8_t
    char RefApplLastMsgID[LEN_REF_APPL_LAST_MSGID];
    char Pad7[LEN_PAD7];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "TrdRegTSTimeOut: " << RBCHeaderME.TrdRegTSTimeOut << " "
               << "SendingTime: " << RBCHeaderME.SendingTime << " "
               << "ApplSubID: " << RBCHeaderME.ApplSubID << " "
               << "PartitionID: " << RBCHeaderME.PartitionID << " "
               << "ApplMsgID: " << RBCHeaderME.ApplMsgID << " "
               << "ApplID: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.ApplID))) << " "
               << "ApplResendFlag: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.ApplResendFlag))) << " "
               << "LastFragment: " << ((unsigned int)(*(uint8_t *)(&RBCHeaderME.LastFragment))) << " "
               << "MarketSegmentID: " << MarketSegmentID << " "
               << "TradeDate: " << TradeDate << " "
               << "TradSesEvent: " << ((unsigned int)(*(uint8_t *)(&TradSesEvent))) << " "
               << "RefApplLastMsgID: " << RefApplLastMsgID << " " << std::endl;

    return t_temp_oss.str();
  }

} TradingSessionStatusBroadcastT;

// Message:     UnsubscribeRequest
// TemplateID:  10006
// Alias:       Unsubscribe
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t RefApplSubID;
    char Pad4[LEN_PAD4];
} UnsubscribeRequestT;

// Message:     UnsubscribeResponse
// TemplateID:  10007
// Alias:       Unsubscribe Response
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
} UnsubscribeResponseT;

// Message:     UserAndSessionPasswordChangeRequest
// TemplateID:  10044
// Alias:       User and Session Password Change Request
// FIX MsgType: Logon = "A"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t PartyIDSessionID;
    uint32_t Username;
    char Password[LEN_PASSWORD];
    char NewPassword[LEN_NEW_PASSWORD];
} UserAndSessionPasswordChangeRequestT;

// Message:     UserAndSessionPasswordChangeResponse
// TemplateID:  10045
// Alias:       User and Session Password Change Response
// FIX MsgType: Logon = "A"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
} UserAndSessionPasswordChangeResponseT;

// Message:     UserLevelLimitBroadcast
// TemplateID:  10050
// Alias:       User Level Limit Trader
// FIX MsgType: UserLevelLimitReport = "U25"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    RBCHeaderCompT RBCHeader;
    int64_t GrossBuyLimit;
    int64_t GrossSellLimit;
    int64_t NetLimit;
    int64_t SingleOrderMaxValue;
    int32_t DefaultBuyQuantity;
    int32_t DefaultSellQuantity;
    int32_t SingleOrderMaxQuantity;
    char InstrumentLevelLimitFlag[LEN_INSTRUMENT_LEVEL_LIMIT_FLAG];
    char GroupLevelLimitFlag[LEN_GROUP_LEVEL_LIMIT_FLAG];
    char Pad2[LEN_PAD2];
} UserLevelLimitBroadcastT;

// Message:     UserLoginRequest
// TemplateID:  10018
// Alias:       User Logon
// FIX MsgType: UserRequest = "BE"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t Username;
    char Password[LEN_PASSWORD];
    char Pad4[LEN_PAD4];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
               << "TemplateID: " << MessageHeaderIn.TemplateID << " "
               << "RequestTime: " << RequestHeader.MsgSeqNum << " "
               << "Username: " << Username << " "
               << "Password: " << Password << " " << std::endl;

    return t_temp_oss.str();
  }

} UserLoginRequestT;

// Message:     UserLoginResponse
// TemplateID:  10019
// Alias:       User Logon Response
// FIX MsgType: UserResponse = "BF"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
    uint64_t LastLoginTime;
    uint8_t DaysLeftForPasswdExpiry; //read uint8_t
    uint8_t GraceLoginsLeft; //read uint8_t
    char Pad6[LEN_PAD6];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << ResponseHeader.RequestTime << " "
               << "SendingTime: " << ResponseHeader.SendingTime << " "
               << "MsgSeqNum: " << ResponseHeader.MsgSeqNum << " "
               << "LastLoginTime: " << LastLoginTime << " "
               << "DaysLeftForPasswdExpiry: " << ((unsigned int)(*(uint8_t *)(&DaysLeftForPasswdExpiry))) << " "
               << "GraceLoginsLeft: " << ((unsigned int)(*(uint8_t *)(&GraceLoginsLeft))) << " " << std::endl;

    return t_temp_oss.str();
  }
} UserLoginResponseT;

// Message:     UserLogoutRequest
// TemplateID:  10029
// Alias:       User Logout
// FIX MsgType: UserRequest = "BE"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t Username;
    char Pad4[LEN_PAD4];

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "MSG_LEN: " << MessageHeaderIn.BodyLen << " "
               << "TemplateID: " << MessageHeaderIn.TemplateID << " "
               << "RequestTime: " << RequestHeader.MsgSeqNum << " "
               << "Username: " << Username << " " << std::endl;

    return t_temp_oss.str();
  }

} UserLogoutRequestT;

// Message:     UserLogoutResponse
// TemplateID:  10024
// Alias:       User Logout Response
// FIX MsgType: UserResponse = "BF"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;

    std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "TemplateID: " << MessageHeaderOut.TemplateID << " "
               << "RequestTime: " << ResponseHeader.RequestTime << " "
               << "SendingTime: " << ResponseHeader.SendingTime << " "
               << "MsgSeqNum: " << ResponseHeader.MsgSeqNum << " " << std::endl;

    return t_temp_oss.str();
  }
} UserLogoutResponseT;

// Message:     UserPasswordChangeRequest
// TemplateID:  10996
// Alias:       User Password Change
// FIX MsgType: UserRequest = "BE"
typedef struct
{
    MessageHeaderInCompT MessageHeaderIn;
    RequestHeaderCompT RequestHeader;
    uint32_t Username;
    char Password[LEN_PASSWORD];
    char NewPassword[LEN_NEW_PASSWORD];
    char Pad4[LEN_PAD4];
} UserPasswordChangeRequestT;

// Message:     UserPasswordChangeResponse
// TemplateID:  10043
// Alias:       User Password Change Response
// FIX MsgType: UserResponse = "BF"
typedef struct
{
    MessageHeaderOutCompT MessageHeaderOut;
    ResponseHeaderCompT ResponseHeader;
} UserPasswordChangeResponseT;

/*
 * Begin of DEPRECATED defines
 */

#define	TID_ADDCOMPLEXINSTRUMENTREQUEST                  10301		// < AddComplexInstrumentRequest (Create Strategy)
#define	TID_ADDCOMPLEXINSTRUMENTRESPONSE                 10302		// < AddComplexInstrumentResponse (Create Strategy Response)
#define	TID_BROADCASTERRORNOTIFICATION                   10032		// < BroadcastErrorNotification (Gap Fill)
#define	TID_CROSSREQUEST                                 10118		// < CrossRequest (Cross Request)
#define	TID_CROSSREQUESTRESPONSE                         10119		// < CrossRequestResponse (Cross Request Response)
#define	TID_DEBTINQUIRYREQUEST                           10390		// < DebtInquiryRequest (Debt Inquiry Request)
#define	TID_DEBTINQUIRYRESPONSE                          10391		// < DebtInquiryResponse (Debt Inquiry Response)
#define	TID_DELETEALLORDERBROADCAST                      10122		// < DeleteAllOrderBroadcast (Order Mass Cancellation Notification)
#define	TID_DELETEALLORDERNRRESPONSE                     10124		// < DeleteAllOrderNRResponse (Order Mass Cancellation Response No Hits)
#define	TID_DELETEALLORDERQUOTEEVENTBROADCAST            10308		// < DeleteAllOrderQuoteEventBroadcast (Mass Cancellation Event)
#define	TID_DELETEALLORDERREQUEST                        10120		// < DeleteAllOrderRequest (Order Mass Cancellation Request)
#define	TID_DELETEALLORDERRESPONSE                       10121		// < DeleteAllOrderResponse (Order Mass Cancellation Response)
#define	TID_DELETEALLQUOTEBROADCAST                      10410		// < DeleteAllQuoteBroadcast (Quote Mass Cancellation Notification)
#define	TID_DELETEALLQUOTEREQUEST                        10408		// < DeleteAllQuoteRequest (Quote Mass Cancellation Request)
#define	TID_DELETEALLQUOTERESPONSE                       10409		// < DeleteAllQuoteResponse (Quote Mass Cancellation Response)
#define	TID_DELETEORDERBROADCAST                         10112		// < DeleteOrderBroadcast (Cancel Order Notification)
#define	TID_DELETEORDERBYORDERBROADCAST                  10116		// < DeleteOrderByOrderBroadcast (Cancel Order by Order Notification)
#define	TID_DELETEORDERCOMPLEXREQUEST                    10123		// < DeleteOrderComplexRequest (Cancel Order Complex)
#define	TID_DELETEORDERNRRESPONSE                        10111		// < DeleteOrderNRResponse (Cancel Order Response (Lean Order))
#define	TID_DELETEORDERRESPONSE                          10110		// < DeleteOrderResponse (Cancel Order Response (Standard Order))
#define	TID_DELETEORDERSINGLEREQUEST                     10109		// < DeleteOrderSingleRequest (Cancel Order Single)
#define	TID_FORCEDLOGOUTNOTIFICATION                     10012		// < ForcedLogoutNotification (Session Logout Notification)
#define	TID_GATEWAYREQUEST                               10020		// < GatewayRequest (Connection Gateway Request)
#define	TID_GATEWAYRESPONSE                              10021		// < GatewayResponse (Connection Gateway Response)
#define	TID_GROUPLEVELLIMITBROADCAST                     10051		// < GroupLevelLimitBroadcast (Group Level Limit Trader)
#define	TID_GWORDERACKNOWLEDGEMENT                       10990		// < GwOrderAcknowledgement (Order Confirmation)
#define	TID_HEARTBEAT                                    10011		// < Heartbeat (Heartbeat)
#define	TID_HEARTBEATNOTIFICATION                        10023		// < HeartbeatNotification (Heartbeat Notification)
#define	TID_INQUIREENRICHMENTRULEIDLISTREQUEST           10040		// < InquireEnrichmentRuleIDListRequest (Trade Enrichment List Inquire)
#define	TID_INQUIREENRICHMENTRULEIDLISTRESPONSE          10041		// < InquireEnrichmentRuleIDListResponse (Trade Enrichment List Inquire Response)
#define	TID_INQUIREMMPARAMETERREQUEST                    10305		// < InquireMMParameterRequest (Inquire Market Maker Parameters)
#define	TID_INQUIREMMPARAMETERRESPONSE                   10306		// < InquireMMParameterResponse (Inquire Market Maker Parameters Response)
#define	TID_INQUIRESESSIONLISTREQUEST                    10035		// < InquireSessionListRequest (Session List Inquire)
#define	TID_INQUIRESESSIONLISTRESPONSE                   10036		// < InquireSessionListResponse (Session List Inquire Response)
#define	TID_INQUIREUSERREQUEST                           10038		// < InquireUserRequest (User List Inquire)
#define	TID_INQUIREUSERRESPONSE                          10039		// < InquireUserResponse (User List Inquire Response)
#define	TID_INSTRUMENTLEVELLIMITBROADCAST                10052		// < InstrumentLevelLimitBroadcast (Instrument Level Limit Trader)
#define	TID_LEGALNOTIFICATIONBROADCAST                   10037		// < LegalNotificationBroadcast (Legal Notification)
#define	TID_LOGONREQUEST                                 10000		// < LogonRequest (Session Logon)
#define	TID_LOGONRESPONSE                                10001		// < LogonResponse (Session Logon Response)
#define	TID_LOGOUTREQUEST                                10002		// < LogoutRequest (Session Logout)
#define	TID_LOGOUTRESPONSE                               10003		// < LogoutResponse (Session Logout Response)
#define	TID_MMPARAMETERDEFINITIONREQUEST                 10303		// < MMParameterDefinitionRequest (Market Maker Parameter Definition)
#define	TID_MMPARAMETERDEFINITIONRESPONSE                10304		// < MMParameterDefinitionResponse (Market Maker Parameter Definition Response)
#define	TID_MARKETPICTUREINQUIRYREQUEST                  10046		// < MarketPictureInquiryRequest (Market Picture Query )
#define	TID_MARKETPICTUREINQUIRYRESPONSE                 10047		// < MarketPictureInquiryResponse (Market Picture Query (report))
#define	TID_MASSQUOTEREQUEST                             10405		// < MassQuoteRequest (Mass Quote Request)
#define	TID_MASSQUOTERESPONSE                            10406		// < MassQuoteResponse (Mass Quote Response)
#define	TID_MODIFYORDERCOMPLEXREQUEST                    10114		// < ModifyOrderComplexRequest (Replace Order Complex)
#define	TID_MODIFYORDERNRRESPONSE                        10108		// < ModifyOrderNRResponse (Replace Order Response (Lean Order))
#define	TID_MODIFYORDERRESPONSE                          10107		// < ModifyOrderResponse (Replace Order Response (Standard Order))
#define	TID_MODIFYORDERSINGLEREQUEST                     10106		// < ModifyOrderSingleRequest (Replace Order Single)
#define	TID_MODIFYORDERSINGLESHORTREQUEST                10126		// < ModifyOrderSingleShortRequest (Replace Order Single (short layout))
#define	TID_MULTILEGEXECREPORTBROADCAST                  10994		// < MultiLegExecReportBroadcast (Extended Order Information(MultiLeg))
#define	TID_MULTILEGEXECRESPONSE                         10993		// < MultiLegExecResponse (Immediate Execution Response(MultiLeg))
#define	TID_MULTILEGORDERREJECT                          10992		// < MultiLegOrderReject (Reject(MultiLeg))
#define	TID_MULTILEGORDERREQUEST                         10991		// < MultiLegOrderRequest (New Order MultiLeg)
#define	TID_NEWORDERCOMPLEXREQUEST                       10113		// < NewOrderComplexRequest (New Order Complex)
#define	TID_NEWORDERNRRESPONSE                           10102		// < NewOrderNRResponse (New Order Response (Lean Order))
#define	TID_NEWORDERRESPONSE                             10101		// < NewOrderResponse (New Order Response (Standard Order))
#define	TID_NEWORDERSINGLEREQUEST                        10100		// < NewOrderSingleRequest (New Order Single)
#define	TID_NEWORDERSINGLESHORTREQUEST                   10125		// < NewOrderSingleShortRequest (New Order Single (short layout))
#define	TID_NEWSBROADCAST                                10031		// < NewsBroadcast (News)
#define	TID_ORDERBOOKINQUIRYREQUEST                      10999		// < OrderBookInquiryRequest (Order book inquiry)
#define	TID_ORDERBOOKINQUIRYRESPONSE                     10998		// < OrderBookInquiryResponse (Order book inquiry (report))
#define	TID_ORDEREXECNOTIFICATION                        10104		// < OrderExecNotification (Book Order Execution)
#define	TID_ORDEREXECREPORTBROADCAST                     10117		// < OrderExecReportBroadcast (Extended Order Information)
#define	TID_ORDEREXECRESPONSE                            10103		// < OrderExecResponse (Immediate Execution Response)
#define	TID_PARTYACTIONREPORT                            10042		// < PartyActionReport (Party Action Report)
#define	TID_PARTYENTITLEMENTSUPDATEREPORT                10034		// < PartyEntitlementsUpdateReport (Entitlement Notification)
#define	TID_QUOTEACTIVATIONNOTIFICATION                  10411		// < QuoteActivationNotification (Quote Activation Notification)
#define	TID_QUOTEACTIVATIONREQUEST                       10403		// < QuoteActivationRequest (Quote Activation Request)
#define	TID_QUOTEACTIVATIONRESPONSE                      10404		// < QuoteActivationResponse (Quote Activation Response)
#define	TID_QUOTEEXECREPORTBROADCAST                     10412		// < QuoteExecReportBroadcast (Quote Cancellation Notification)
#define	TID_QUOTEEXECUTIONREPORT                         10407		// < QuoteExecutionReport (Quote Execution Notification)
#define	TID_RFQREQUEST                                   10401		// < RFQRequest (Quote Request)
#define	TID_RFQRESPONSE                                  10402		// < RFQResponse (Quote Request Response)
#define	TID_REJECT                                       10010		// < Reject (Reject)
#define	TID_RETRANSMITMEMESSAGEREQUEST                   10026		// < RetransmitMEMessageRequest (Retransmit (Order/Quote Event))
#define	TID_RETRANSMITMEMESSAGERESPONSE                  10027		// < RetransmitMEMessageResponse (Retransmit Response (Order/Quote Event))
#define	TID_RETRANSMITREQUEST                            10008		// < RetransmitRequest (Retransmit)
#define	TID_RETRANSMITRESPONSE                           10009		// < RetransmitResponse (Retransmit Response)
#define	TID_RISKCOLLATERALALERTADMINBROADCAST            10048		// < RiskCollateralAlertAdminBroadcast (Risk Collateral Alert Admin)
#define	TID_RISKCOLLATERALALERTBROADCAST                 10049		// < RiskCollateralAlertBroadcast (Risk Collateral Alert Trader)
#define	TID_RISKNOTIFICATIONBROADCAST                    10033		// < RiskNotificationBroadcast (Risk Notification)
#define	TID_SERVICEAVAILABILITYBROADCAST                 10030		// < ServiceAvailabilityBroadcast (Service Availability)
#define	TID_SESSIONPASSWORDCHANGEREQUEST                 10997		// < SessionPasswordChangeRequest (Session Password Change)
#define	TID_SESSIONPASSWORDCHANGERESPONSE                10995		// < SessionPasswordChangeResponse (Session Password Change Response)
#define	TID_SUBSCRIBEREQUEST                             10025		// < SubscribeRequest (Subscribe)
#define	TID_SUBSCRIBERESPONSE                            10005		// < SubscribeResponse (Subscribe Response)
#define	TID_TMTRADINGSESSIONSTATUSBROADCAST              10501		// < TMTradingSessionStatusBroadcast (Trade Notification Status)
#define	TID_THROTTLEUPDATENOTIFICATION                   10028		// < ThrottleUpdateNotification (Throttle Update Notification)
#define	TID_TRADEBROADCAST                               10500		// < TradeBroadcast (Trade Notification)
#define	TID_TRADEENHANCEMENTBROADCAST                    10989		// < TradeEnhancementBroadcast (Trade Enhancement Notification)
#define	TID_TRADINGSESSIONSTATUSBROADCAST                10307		// < TradingSessionStatusBroadcast (Trading Session Event)
#define	TID_UNSUBSCRIBEREQUEST                           10006		// < UnsubscribeRequest (Unsubscribe)
#define	TID_UNSUBSCRIBERESPONSE                          10007		// < UnsubscribeResponse (Unsubscribe Response)
#define	TID_USERANDSESSIONPASSWORDCHANGEREQUEST          10044		// < UserAndSessionPasswordChangeRequest (User and Session Password Change Request)
#define	TID_USERANDSESSIONPASSWORDCHANGERESPONSE         10045		// < UserAndSessionPasswordChangeResponse (User and Session Password Change Response)
#define	TID_USERLEVELLIMITBROADCAST                      10050		// < UserLevelLimitBroadcast (User Level Limit Trader)
#define	TID_USERLOGINREQUEST                             10018		// < UserLoginRequest (User Logon)
#define	TID_USERLOGINRESPONSE                            10019		// < UserLoginResponse (User Logon Response)
#define	TID_USERLOGOUTREQUEST                            10029		// < UserLogoutRequest (User Logout)
#define	TID_USERLOGOUTRESPONSE                           10024		// < UserLogoutResponse (User Logout Response)
#define	TID_USERPASSWORDCHANGEREQUEST                    10996		// < UserPasswordChangeRequest (User Password Change)
#define	TID_USERPASSWORDCHANGERESPONSE                   10043		// < UserPasswordChangeResponse (User Password Change Response)
#define MAX_ADDCOMPLEXINSTRUMENTREQUEST_INSTRMTLEGGRP   	5
#define MAX_ADDCOMPLEXINSTRUMENTRESPONSE_INSTRMTLEGGRP  	5
#define MAX_DELETEALLORDERBROADCAST_NOTAFFECTEDORDERSGRP	500
#define MAX_DELETEALLORDERRESPONSE_NOTAFFECTEDORDERSGRP 	500
#define MAX_DELETEALLQUOTEBROADCAST_NOTAFFECTEDSECURITIESGRP	500
#define MAX_DELETEALLQUOTERESPONSE_NOTAFFECTEDSECURITIESGRP	500
#define MAX_GROUPLEVELLIMITBROADCAST_GROUPLEVELDATA     	100
#define MAX_INQUIREENRICHMENTRULEIDLISTRESPONSE_ENRICHMENTRULESGRP	400
#define MAX_INQUIREMMPARAMETERRESPONSE_MMPARAMETERGRP   	6
#define MAX_INQUIRESESSIONLISTRESPONSE_SESSIONSGRP      	1000
#define MAX_INQUIREUSERRESPONSE_PARTYDETAILSGRP         	1000
#define MAX_INSTRUMENTLEVELLIMITBROADCAST_INSTRUMENTLEVELDATA	100
#define MAX_MARKETPICTUREINQUIRYRESPONSE_BESTRATEGRP    	5
#define MAX_MASSQUOTEREQUEST_QUOTEENTRYGRP              	100
#define MAX_MASSQUOTERESPONSE_QUOTEENTRYACKGRP          	100
#define MAX_MODIFYORDERCOMPLEXREQUEST_LEGORDGRP         	5
#define MAX_MULTILEGEXECREPORTBROADCAST_MULTILEGGRP     	99
#define MAX_MULTILEGEXECREPORTBROADCAST_MULTILEGFILLGRP 	100
#define MAX_MULTILEGEXECREPORTBROADCAST_INSTRMNTLEGEXECGRP	600
#define MAX_MULTILEGEXECRESPONSE_MULTILEGEXECGRP        	99
#define MAX_MULTILEGEXECRESPONSE_MULTILEGFILLGRP        	100
#define MAX_MULTILEGEXECRESPONSE_INSTRMNTLEGEXECGRP     	600
#define MAX_MULTILEGORDERREQUEST_MULTILEGORDGRP         	99
#define MAX_NEWORDERCOMPLEXREQUEST_LEGORDGRP            	5
#define MAX_ORDERBOOKINQUIRYRESPONSE_OBINQUIRYGRP       	24
#define MAX_ORDEREXECNOTIFICATION_FILLSGRP              	100
#define MAX_ORDEREXECNOTIFICATION_INSTRMNTLEGEXECGRP    	600
#define MAX_ORDEREXECREPORTBROADCAST_LEGORDGRP          	5
#define MAX_ORDEREXECREPORTBROADCAST_FILLSGRP           	100
#define MAX_ORDEREXECREPORTBROADCAST_INSTRMNTLEGEXECGRP 	600
#define MAX_ORDEREXECRESPONSE_FILLSGRP                  	100
#define MAX_ORDEREXECRESPONSE_INSTRMNTLEGEXECGRP        	600
#define MAX_QUOTEACTIVATIONNOTIFICATION_NOTAFFECTEDSECURITIESGRP	500
#define MAX_QUOTEACTIVATIONRESPONSE_NOTAFFECTEDSECURITIESGRP	500
#define MAX_QUOTEEXECREPORTBROADCAST_QUOTEEVENTGRP      	100
#define MAX_QUOTEEXECUTIONREPORT_QUOTEEVENTGRP          	100
#define MAX_QUOTEEXECUTIONREPORT_QUOTELEGEXECGRP        	600

#define LEN_ACCOUNT                                      2
#define ENUM_ACCOUNTTYPE_OWN                             20
#define ENUM_ACCOUNTTYPE_CLIENT                          30
#define ENUM_ACCOUNTTYPE_SPLCLI                          40
#define ENUM_ACCOUNTTYPE_INST                            90
#define ENUM_AGGRESSORINDICATOR_PASSIVE                  0
#define ENUM_AGGRESSORINDICATOR_AGRESSOR                 1
#define ENUM_AGGRESSORSIDE_BUY                           1
#define ENUM_AGGRESSORSIDE_SELL                          2
#define LEN_ALGOID                                       16
#define LEN_ALLORNONEFLAG                                1
#define ENUM_ALLORNONEFLAG_USEALLORNONE                  "Y"
#define ENUM_ALLORNONEFLAG_USEALLORNONENOT               "N"
#define LEN_APPLBEGMSGID                                 16
#define LEN_APPLENDMSGID                                 16
#define ENUM_APPLID_TRADEENHANCEMENT                     0
#define ENUM_APPLID_TRADE                                1
#define ENUM_APPLID_NEWS                                 2
#define ENUM_APPLID_SERVICE_AVAILABILITY                 3
#define ENUM_APPLID_SESSION_DATA                         4
#define ENUM_APPLID_LISTENER_DATA                        5
#define ENUM_APPLID_RISKCONTROL                          6
#define ENUM_APPLID_ORDERBYORDER                         7
#define ENUM_APPLID_RISKADMIN                            8
#define ENUM_APPLID_USERLEVEL                            9
#define ENUM_APPLID_GROUPLEVEL                           10
#define ENUM_APPLID_INSTRUMENTLEVEL                      11
#define ENUM_APPLIDSTATUS_OUTBOUND_CONVERSION_ERROR      105
#define LEN_APPLMSGID                                    16
#define ENUM_APPLRESENDFLAG_FALSE                        0
#define ENUM_APPLRESENDFLAG_TRUE                         1
#define ENUM_APPLSEQINDICATOR_NO_RECOVERY_REQUIRED       0
#define ENUM_APPLSEQINDICATOR_RECOVERY_REQUIRED          1
#define ENUM_APPLSEQSTATUS_UNAVAILABLE                   0
#define ENUM_APPLSEQSTATUS_AVAILABLE                     1
#define LEN_APPLUSAGEORDERS                              1
#define ENUM_APPLUSAGEORDERS_AUTOMATED                   "A"
#define ENUM_APPLUSAGEORDERS_MANUAL                      "M"
#define ENUM_APPLUSAGEORDERS_AUTOSELECT                  "B"
#define ENUM_APPLUSAGEORDERS_NONE                        "N"
#define LEN_APPLUSAGEQUOTES                              1
#define ENUM_APPLUSAGEQUOTES_AUTOMATED                   "A"
#define ENUM_APPLUSAGEQUOTES_MANUAL                      "M"
#define ENUM_APPLUSAGEQUOTES_AUTOSELECT                  "B"
#define ENUM_APPLUSAGEQUOTES_NONE                        "N"
#define LEN_APPLICATIONSYSTEMNAME                        30
#define LEN_APPLICATIONSYSTEMVENDOR                      30
#define LEN_APPLICATIONSYSTEMVERSION                     30
#define LEN_AUTOACCEPTINDICATOR                          1
#define ENUM_AUTOACCEPTINDICATOR_ACCEPTED                "Y"
#define ENUM_AUTOACCEPTINDICATOR_REJECTED                "N"
#define LEN_BUSINESSUNITSYMBOL                           8
#define LEN_CPCODE                                       12
#define LEN_CONTRABROKER                                 5
#define LEN_CUSTORDERHANDLINGINST                        1
#define LEN_DEFAULTCSTMAPPLVERID                         30
#define LEN_DELTAQTYFLAG                                 1
#define ENUM_DELTAQTYFLAG_DIFFERENTIALQTY                "D"
#define ENUM_DURATION_NEAR                               1
#define ENUM_DURATION_MID                                2
#define ENUM_DURATION_FAR                                3
#define ENUM_EXECINST_H                                  1
#define ENUM_EXECINST_Q                                  2
#define ENUM_EXECINST_H_Q                                3
#define ENUM_EXECINST_H_6                                5
#define ENUM_EXECINST_Q_6                                6
#define ENUM_EXECRESTATEMENTREASON_ORDER_BOOK_RESTATEMENT 001
#define ENUM_EXECRESTATEMENTREASON_ORDER_ADDED           101
#define ENUM_EXECRESTATEMENTREASON_ORDER_MODIFIED        102
#define ENUM_EXECRESTATEMENTREASON_ORDER_CANCELLED       103
#define ENUM_EXECRESTATEMENTREASON_IOC_ORDER_CANCELLED   105
#define ENUM_EXECRESTATEMENTREASON_BOOK_ORDER_EXECUTED   108
#define ENUM_EXECRESTATEMENTREASON_MARKET_ORDER_TRIGGERED 135
#define ENUM_EXECRESTATEMENTREASON_CAO_ORDER_ACTIVATED   149
#define ENUM_EXECRESTATEMENTREASON_CAO_ORDER_INACTIVATED 150
#define ENUM_EXECRESTATEMENTREASON_OCO_ORDER_TRIGGERED   164
#define ENUM_EXECRESTATEMENTREASON_STOP_ORDER_TRIGGERED  172
#define ENUM_EXECRESTATEMENTREASON_ORDER_CANCELLATION_PENDING 197
#define ENUM_EXECRESTATEMENTREASON_PENDING_CANCELLATION_EXECUTED 199
#define ENUM_EXECRESTATEMENTREASON_BOC_ORDER_CANCELLED   212
#define ENUM_EXECRESTATEMENTREASON_PENDING_NORMAL_ORDERS_QUERIED 213
#define ENUM_EXECRESTATEMENTREASON_PENDING_STOPLOSS_ORDERS_QUERIED 214
#define ENUM_EXECRESTATEMENTREASON_RRM_ORDER_ADDED       215
#define ENUM_EXECRESTATEMENTREASON_RRM_ORDER_ACCEPTED    216
#define ENUM_EXECRESTATEMENTREASON_RRM_ORDER_UPDATION_REJECTED 217
#define ENUM_EXECRESTATEMENTREASON_RRM_ORDER_UPDATED_SUCCESSFULLY 218
#define ENUM_EXECRESTATEMENTREASON_RRM_ORDER_DELETED     219
#define ENUM_EXECRESTATEMENTREASON_RRM_MARKET_ORDER_TRIGGERED 220
#define ENUM_EXECRESTATEMENTREASON_PROV_ORDER_ADDED      221
#define ENUM_EXECRESTATEMENTREASON_PROV_ORDER_ACCEPTED   222
#define ENUM_EXECRESTATEMENTREASON_PROV_ORDER_UPDATION_REJECTED 223
#define ENUM_EXECRESTATEMENTREASON_PROV_ORDER_UPDATED_SUCCESSFULLY 224
#define ENUM_EXECRESTATEMENTREASON_PROV_ORDER_DELETED    225
#define ENUM_EXECRESTATEMENTREASON_PROV_MARKET_ORDER_TRIGGERED 226
#define ENUM_EXECRESTATEMENTREASON_CALLAUC_ORDER_ADDED   227
#define ENUM_EXECRESTATEMENTREASON_CALLAUC_ORDER_ACCEPTED 228
#define ENUM_EXECRESTATEMENTREASON_CALLAUC_ORDER_UPDATION_REJECTED 229
#define ENUM_EXECRESTATEMENTREASON_CALLAUC_ORDER_UPDATED_SUCCESSFULLY 230
#define ENUM_EXECRESTATEMENTREASON_CALLAUC_ORDER_DELETED 231
#define ENUM_EXECRESTATEMENTREASON_GTCL_ORDER_DELETED    232
#define ENUM_EXECRESTATEMENTREASON_EOD_ORDER_DELETED     233
#define ENUM_EXECRESTATEMENTREASON_HALT_ORDER_DELETED    234
#define ENUM_EXECRESTATEMENTREASON_BLOCK_DEAL_ORDER_TIMED_OUT 235
#define ENUM_EXECRESTATEMENTREASON_OUT_OF_PRICE_BAND_ORDER 236
#define ENUM_EXECRESTATEMENTREASON_ORDER_WORSE_THAN_CLOSE_PRICE 237
#define ENUM_EXECRESTATEMENTREASON_AUCTION_MARKET_ORDER_TRIGGERED 238
#define ENUM_EXECRESTATEMENTREASON_PENDING_BLOCKDEAL_ORDERS_QUERIED 239
#define ENUM_EXECRESTATEMENTREASON_BU_SUSPENDED          240
#define ENUM_EXECRESTATEMENTREASON_COOLING_OFF_RRM_ORDER_DELETED 241
#define ENUM_EXECRESTATEMENTREASON_MWPL_RRM_ORDER_DELETED 242
#define ENUM_EXECRESTATEMENTREASON_CALL_AUCTION_UNCROSS_ORDER_DELETED 243
#define ENUM_EXECRESTATEMENTREASON_COLLATERAL_RRM        244
#define ENUM_EXECRESTATEMENTREASON_PENDING_MARKET_ORDERS_QUERIED 245
#define ENUM_EXECRESTATEMENTREASON_SELF_TRADE_ORDER_DELETED 246
#define ENUM_EXECRESTATEMENTREASON_REVERSE_TRADE_ORDER_DELETED 247
#define ENUM_EXECRESTATEMENTREASON_PENDING_OCO_ORDERS_QUERIED 248
#define ENUM_EXECRESTATEMENTREASON_NLT_ORDER_TIMED_OUT   249
#define ENUM_EXECRESTATEMENTREASON_CLIENT_RRM_FOR_PRODUCT 250
#define ENUM_EXECRESTATEMENTREASON_CLIENT_SUSPENDED      251
#define ENUM_EXECRESTATEMENTREASON_CLIENT_RRM_FOR_CONTRACT 252
#define ENUM_EXECRESTATEMENTREASON_MEMBER_RRM_FOR_CONTRACT 253
#define ENUM_EXECRESTATEMENTREASON_CLIENT_RRM            254
#define LEN_EXECTYPE                                     1
#define ENUM_EXECTYPE_NEW                                "0"
#define ENUM_EXECTYPE_CANCELED                           "4"
#define ENUM_EXECTYPE_REPLACED                           "5"
#define ENUM_EXECTYPE_PENDING_CANCEL_E                   "6"
#define ENUM_EXECTYPE_SUSPENDED                          "9"
#define ENUM_EXECTYPE_RESTATED                           "D"
#define ENUM_EXECTYPE_TRIGGERED                          "L"
#define ENUM_EXECTYPE_TRADE                              "F"
#define ENUM_EXECTYPE_RRMACCEPT                          "M"
#define ENUM_EXECTYPE_RRMREJECT                          "N"
#define ENUM_EXECTYPE_PROVACCEPT                         "X"
#define ENUM_EXECTYPE_PROVREJECT                         "Y"
#define LEN_FIXENGINENAME                                30
#define LEN_FIXENGINEVENDOR                              30
#define LEN_FIXENGINEVERSION                             30
#define ENUM_FILLLIQUIDITYIND_ADDED_LIQUIDITY            1
#define ENUM_FILLLIQUIDITYIND_REMOVED_LIQUIDITY          2
#define ENUM_FILLLIQUIDITYIND_TRIGGERED_STOP_ORDER       5
#define ENUM_FILLLIQUIDITYIND_TRIGGERED_OCO_ORDER        6
#define ENUM_FILLLIQUIDITYIND_TRIGGERED_MARKET_ORDER     7
#define ENUM_FIRSTFRAGMENT_NOT_FIRST_MESSAGE             0
#define ENUM_FIRSTFRAGMENT_FIRST_MESSAGE                 1
#define LEN_FREETEXT1                                    12
#define LEN_FREETEXT2                                    12
#define LEN_FREETEXT3                                    12
#define LEN_FUNCCATEGORY                                 100
#define ENUM_FUNCCATEGORY_ORDER_HANDLING                 "Order Handling                                                                                      "
#define ENUM_FUNCCATEGORY_SESSION_LAYER                  "Session Layer                                                                                       "
#define ENUM_FUNCCATEGORY_QUOTE_HANDLING                 "Quote Handling                                                                                      "
#define ENUM_FUNCCATEGORY_QUOTE_AND_CROSS_REQUEST        "Quote and Cross Request                                                                             "
#define ENUM_FUNCCATEGORY_STRATEGY_CREATION              "Strategy Creation                                                                                   "
#define ENUM_FUNCCATEGORY_OTHER                          "Other                                                                                               "
#define ENUM_FUNCCATEGORY_BROADCAST                      "Broadcast                                                                                           "
#define LEN_GROUP                                        2
#define LEN_GROUPLEVELLIMITFLAG                          1
#define ENUM_GROUPLEVELLIMITFLAG_USEGROUPLEVELLIMIT      "Y"
#define ENUM_GROUPLEVELLIMITFLAG_USEGROUPLEVELLIMITNOT   "N"
#define LEN_HEADLINE                                     256
#define ENUM_IMPLIEDMARKETINDICATOR_NOT_IMPLIED          0
#define ENUM_IMPLIEDMARKETINDICATOR_IMPLIED_IN_OUT       3
#define ENUM_INCREMENTDECREMENTSTATUS_INCREMENT          1
#define ENUM_INCREMENTDECREMENTSTATUS_NOCHANGE           0
#define ENUM_INCREMENTDECREMENTSTATUS_DECREMENT          -1
#define LEN_INSTRUMENTLEVELLIMITFLAG                     1
#define ENUM_INSTRUMENTLEVELLIMITFLAG_USEINSTRUMENTLEVELLIMIT "Y"
#define ENUM_INSTRUMENTLEVELLIMITFLAG_USEINSTRUMENTLEVELLIMITNOT "N"
#define LEN_LASTENTITYPROCESSED                          16
#define ENUM_LASTFRAGMENT_NOT_LAST_MESSAGE               0
#define ENUM_LASTFRAGMENT_LAST_MESSAGE                   1
#define LEN_LEGACCOUNT                                   2
#define LEN_LEGPOSITIONEFFECT                            1
#define ENUM_LEGPOSITIONEFFECT_CLOSE                     "C"
#define ENUM_LEGPOSITIONEFFECT_OPEN                      "O"
#define ENUM_LEGSECURITYTYPE_MULTILEG_INSTRUMENT         1
#define ENUM_LEGSECURITYTYPE_UNDERLYING_LEG              2
#define ENUM_LEGSIDE_BUY                                 1
#define ENUM_LEGSIDE_SELL                                2
#define LEN_LISTUPDATEACTION                             1
#define ENUM_LISTUPDATEACTION_ADD                        "A"
#define ENUM_LISTUPDATEACTION_DELETE                     "D"
#define ENUM_MARKETID_XEUR                               1
#define ENUM_MARKETID_XEEE                               2
#define ENUM_MASSACTIONREASON_NO_SPECIAL_REASON          0
#define ENUM_MASSACTIONREASON_STOP_TRADING               1
#define ENUM_MASSACTIONREASON_EMERGENCY                  2
#define ENUM_MASSACTIONREASON_MARKET_MAKER_PROTECTION    3
#define ENUM_MASSACTIONREASON_STOP_BUTTON_ACTIVATED      4
#define ENUM_MASSACTIONREASON_BUSSINESSUNIT_SUSPENDED    5
#define ENUM_MASSACTIONREASON_SESSION_LOSS               6
#define ENUM_MASSACTIONREASON_COLLATERAL_RRM             7
#define ENUM_MASSACTIONREASON_PRICE_BAND_SHRINK          8
#define ENUM_MASSACTIONREASON_ORDERS_WORSE_THAN_CLOSE_PRICE 9
#define ENUM_MASSACTIONREASON_PRODUCT_STATE_HALT         105
#define ENUM_MASSACTIONREASON_PRODUCT_STATE_HOLIDAY      106
#define ENUM_MASSACTIONREASON_INSTRUMENT_SUSPENDED       107
#define ENUM_MASSACTIONREASON_COMPLEX_INSTRUMENT_DELETION 109
#define ENUM_MASSACTIONREASON_VOLATILITY_INTERRUPTION    110
#define ENUM_MASSACTIONREASON_PRODUCT_TEMPORARILY_NOT_TRADABLE 111
#define ENUM_MASSACTIONREASON_PRODUCT_STATE_CLOSING      114
#define ENUM_MASSACTIONREASON_PRODUCT_STATE_EOD          115
#define ENUM_MASSACTIONREASON_COOLING_OFF                116
#define ENUM_MASSACTIONREASON_MWPL_RRM                   117
#define ENUM_MASSACTIONTYPE_SUSPEND_QUOTES               1
#define ENUM_MASSACTIONTYPE_RELEASE_QUOTES               2
#define ENUM_MATCHSUBTYPE_OPENING_AUCTION                1
#define ENUM_MATCHSUBTYPE_CLOSING_AUCTION                2
#define ENUM_MATCHSUBTYPE_INTRADAY_AUCTION               3
#define ENUM_MATCHSUBTYPE_CIRCUIT_BREAKER_AUCTION        4
#define ENUM_MATCHTYPE_CONFIRMED_TRADE_REPORT            3
#define ENUM_MATCHTYPE_AUTO_MATCH_INCOMING               4
#define ENUM_MATCHTYPE_CROSS_AUCTION                     5
#define ENUM_MATCHTYPE_CALL_AUCTION                      7
#define ENUM_MATCHTYPE_AUTO_MATCH_RESTING                11
#define ENUM_MATCHINGENGINESTATUS_UNAVAILABLE            0
#define ENUM_MATCHINGENGINESTATUS_AVAILABLE              1
#define ENUM_MEMBERTYPE_TRADINGMEMBER                    1
#define ENUM_MEMBERTYPE_CLEARINGMEMBER                   2
#define ENUM_MEMBERTYPE_PROPRIETARY                      3
#define LEN_MSGTYPE                                      3
#define ENUM_MSGTYPE_HEARTBEAT                           "0  "
#define ENUM_MSGTYPE_REJECT                              "3  "
#define ENUM_MSGTYPE_LOGOUT                              "5  "
#define ENUM_MSGTYPE_EXECUTIONREPORT                     "8  "
#define ENUM_MSGTYPE_LOGON                               "A  "
#define ENUM_MSGTYPE_NEWORDERMULTILEG                    "AB "
#define ENUM_MSGTYPE_MULTILEGORDERCANCELREPLACE          "AC "
#define ENUM_MSGTYPE_TRADECAPTUREREPORT                  "AE "
#define ENUM_MSGTYPE_TRADECAPTUREREPORTACK               "AR "
#define ENUM_MSGTYPE_NEWS                                "B  "
#define ENUM_MSGTYPE_USERREQUEST                         "BE "
#define ENUM_MSGTYPE_USERRESPONSE                        "BF "
#define ENUM_MSGTYPE_APPLICATIONMESSAGEREQUEST           "BW "
#define ENUM_MSGTYPE_APPLICATIONMESSAGEREQUESTACK        "BX "
#define ENUM_MSGTYPE_APPLICATIONMESSAGEREPORT            "BY "
#define ENUM_MSGTYPE_ORDERMASSACTIONREPORT               "BZ "
#define ENUM_MSGTYPE_ORDERMASSACTIONREQUEST              "CA "
#define ENUM_MSGTYPE_USERNOTIFICATION                    "CB "
#define ENUM_MSGTYPE_PARTYRISKLIMITSUPDATEREPORT         "CR "
#define ENUM_MSGTYPE_PARTYENTITLEMENTSUPDATEREPORT       "CZ "
#define ENUM_MSGTYPE_NEWORDERSINGLE                      "D  "
#define ENUM_MSGTYPE_ORDERCANCELREQUEST                  "F  "
#define ENUM_MSGTYPE_ORDERCANCELREPLACEREQUEST           "G  "
#define ENUM_MSGTYPE_QUOTEREQUEST                        "R  "
#define ENUM_MSGTYPE_MARKETDATASNAPSHOTFULLREFRESH       "W  "
#define ENUM_MSGTYPE_MASSQUOTEACKNOWLEDGEMENT            "b  "
#define ENUM_MSGTYPE_SECURITYDEFINITIONREQUEST           "c  "
#define ENUM_MSGTYPE_SECURITYDEFINITION                  "d  "
#define ENUM_MSGTYPE_TRADINGSESSIONSTATUS                "h  "
#define ENUM_MSGTYPE_MASSQUOTE                           "i  "
#define ENUM_MSGTYPE_REQUESTACKNOWLEDGE                  "U1 "
#define ENUM_MSGTYPE_SESSIONDETAILSLISTREQUEST           "U5 "
#define ENUM_MSGTYPE_SESSIONDETAILSLISTRESPONSE          "U6 "
#define ENUM_MSGTYPE_QUOTEEXECUTIONREPORT                "U8 "
#define ENUM_MSGTYPE_MMPARAMETERDEFINITIONREQUEST        "U14"
#define ENUM_MSGTYPE_CROSSREQUEST                        "U16"
#define ENUM_MSGTYPE_MMPARAMETERREQUEST                  "U17"
#define ENUM_MSGTYPE_MMPARAMETERRESPONSE                 "U18"
#define ENUM_MSGTYPE_PARTYDETAILLISTREQUEST              "CF "
#define ENUM_MSGTYPE_PARTYDETAILLISTREPORT               "CG "
#define ENUM_MSGTYPE_TRADEENRICHMENTLISTREQUEST          "U7 "
#define ENUM_MSGTYPE_TRADEENRICHMENTLISTREPORT           "U9 "
#define ENUM_MSGTYPE_PARTYACTIONREPORT                   "DI "
#define ENUM_MSGTYPE_ORDERBOOKINQUIRYREQUEST             "U19"
#define ENUM_MSGTYPE_ORDERBOOKINQUIRYRESPONSE            "U20"
#define ENUM_MSGTYPE_MARKETPICTUREINQUIRYREQUEST         "U21"
#define ENUM_MSGTYPE_MARKETPICTUREINQUIRYRESPONSE        "U22"
#define ENUM_MSGTYPE_COLLATERALALERTADMINREPORT          "U23"
#define ENUM_MSGTYPE_COLLATERALALERTREPORT               "U24"
#define ENUM_MSGTYPE_USERLEVELLIMITREPORT                "U25"
#define ENUM_MSGTYPE_GROUPLEVELLIMITREPORT               "U26"
#define ENUM_MSGTYPE_INSTRUMENTLEVELLIMITREPORT          "U27"
#define ENUM_MSGTYPE_MULTILEGORDER                       "U28"
#define ENUM_MSGTYPE_MULTILEGORDERACKNOWLEDGEMENT        "U29"
#define ENUM_MSGTYPE_MULTILEGEXECUTIONREPORT             "U30"
#define ENUM_MSGTYPE_TRADEENHANCEMENTSTATUS              "U31"
#define ENUM_MSGTYPE_DEBTINQUIRYREQUEST                  "U32"
#define ENUM_MSGTYPE_DEBTINQUIRYRESPONSE                 "U33"
#define ENUM_MULTILEGREPORTINGTYPE_SINGLE_SECURITY       1
#define ENUM_MULTILEGREPORTINGTYPE_INDIVIDUAL_LEG_OF_A_MULTILEG_SECURITY 2
#define ENUM_MULTILEGMODEL_PREDEFINED_MULTILEG_SECURITY  0
#define ENUM_MULTILEGMODEL_USER_DEFINED_MULTLEG          1
#define LEN_NETWORKMSGID                                 8
#define LEN_NEWPASSWORD                                  32
#define LEN_ORDSTATUS                                    1
#define ENUM_ORDSTATUS_NEW                               "0"
#define ENUM_ORDSTATUS_PARTIALLY_FILLED                  "1"
#define ENUM_ORDSTATUS_FILLED                            "2"
#define ENUM_ORDSTATUS_CANCELED                          "4"
#define ENUM_ORDSTATUS_PENDING_CANCEL                    "6"
#define ENUM_ORDSTATUS_SUSPENDED                         "9"
#define ENUM_ORDTYPE_MARKET                              1
#define ENUM_ORDTYPE_LIMIT                               2
#define ENUM_ORDTYPE_STOP                                3
#define ENUM_ORDTYPE_STOP_LIMIT                          4
#define ENUM_ORDTYPE_MARKET_TO_LIMIT                     5
#define ENUM_ORDTYPE_BLOCK_DEAL                          6
#define ENUM_ORDTYPE_QUOTE                               7
#define ENUM_ORDTYPE_NLT                                 8
#define LEN_ORDERCATEGORY                                1
#define ENUM_ORDERCATEGORY_ORDER                         "1"
#define ENUM_ORDERCATEGORY_QUOTE                         "2"
#define ENUM_ORDERCATEGORY_MULTILEG_ORDER                "3"
#define LEN_ORDERROUTINGINDICATOR                        1
#define ENUM_ORDERROUTINGINDICATOR_YES                   "Y"
#define ENUM_ORDERROUTINGINDICATOR_NO                    "N"
#define ENUM_ORDERSIDE_BUY                               1
#define ENUM_ORDERSIDE_SELL                              2
#define ENUM_ORDERSIDE_RECALL                            3
#define ENUM_ORDERSIDE_EARLY_RETURN                      4
#define LEN_PAD1                                         1
#define LEN_PAD2                                         2
#define LEN_PAD3                                         3
#define LEN_PAD4                                         4
#define LEN_PAD5                                         5
#define LEN_PAD6                                         6
#define LEN_PAD7                                         7
#define ENUM_PARTYACTIONTYPE_HALT_TRADING                1
#define ENUM_PARTYACTIONTYPE_REINSTATE                   2
#define LEN_PARTYDETAILDESKID                            3
#define LEN_PARTYDETAILEXECUTINGTRADER                   6
#define ENUM_PARTYDETAILROLEQUALIFIER_TRADER             10
#define ENUM_PARTYDETAILROLEQUALIFIER_HEAD_TRADER        11
#define ENUM_PARTYDETAILROLEQUALIFIER_SUPERVISOR         12
#define ENUM_PARTYDETAILSTATUS_ACTIVE                    0
#define ENUM_PARTYDETAILSTATUS_SUSPEND                   1
#define LEN_PARTYIDBENEFICIARY                           9
#define ENUM_PARTYIDENTERINGFIRM_PARTICIPANT             1
#define ENUM_PARTYIDENTERINGFIRM_MARKETSUPERVISION       2
#define LEN_PARTYIDLOCATIONID                            2
#define LEN_PARTYIDORDERORIGINATIONFIRM                  7
#define ENUM_PARTYIDORIGINATIONMARKET_XKFE               1
#define ENUM_PARTYIDORIGINATIONMARKET_XTAF               2
#define LEN_PARTYIDTAKEUPTRADINGFIRM                     5
#define LEN_PASSWORD                                     32
#define LEN_POSITIONEFFECT                               1
#define ENUM_POSITIONEFFECT_CLOSE                        "C"
#define ENUM_POSITIONEFFECT_OPEN                         "O"
#define ENUM_PRICEVALIDITYCHECKTYPE_NONE                 0
#define ENUM_PRICEVALIDITYCHECKTYPE_OPTIONAL             1
#define ENUM_PRICEVALIDITYCHECKTYPE_MANDATORY            2
#define ENUM_PRODUCTCOMPLEX_SIMPLE_INSTRUMENT            1
#define ENUM_PRODUCTCOMPLEX_STANDARD_OPTION_STRATEGY     2
#define ENUM_PRODUCTCOMPLEX_NON_STANDARD_OPTION_STRATEGY 3
#define ENUM_PRODUCTCOMPLEX_VOLATILITY_STRATEGY          4
#define ENUM_PRODUCTCOMPLEX_FUTURES_SPREAD               5
#define ENUM_QUOTEENTRYREJECTREASON_UNKNOWN_SECURITY     1
#define ENUM_QUOTEENTRYREJECTREASON_DUPLICATE_QUOTE      6
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_PRICE        8
#define ENUM_QUOTEENTRYREJECTREASON_NO_REFERENCE_PRICE_AVAILABLE 16
#define ENUM_QUOTEENTRYREJECTREASON_NO_SINGLE_SIDED_QUOTES 100
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_QUOTING_MODEL 103
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_SIZE         106
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_UNDERLYING_PRICE 107
#define ENUM_QUOTEENTRYREJECTREASON_BID_PRICE_NOT_REASONABLE 108
#define ENUM_QUOTEENTRYREJECTREASON_ASK_PRICE_NOT_REASONABLE 109
#define ENUM_QUOTEENTRYREJECTREASON_BID_PRICE_EXCEEDS_RANGE 110
#define ENUM_QUOTEENTRYREJECTREASON_ASK_PRICE_EXCEEDS_RANGE 111
#define ENUM_QUOTEENTRYREJECTREASON_INSTRUMENT_STATE_FREEZE 115
#define ENUM_QUOTEENTRYREJECTREASON_DELETION_ALREADY_PENDING 116
#define ENUM_QUOTEENTRYREJECTREASON_PRE_TRADE_RISK_SESSION_LIMIT_EXCEEDED 117
#define ENUM_QUOTEENTRYREJECTREASON_PRE_TRADE_RISK_BU_LIMIT_EXCEEDED 118
#define ENUM_QUOTEENTRYREJECTREASON_CANT_PROC_IN_CURR_INSTR_STATE 131
#define ENUM_QUOTEENTRYREJECTREASON_LOCATION_ID_NOT_SET  132
#define ENUM_QUOTEENTRYREJECTREASON_CLIENT_CODE_NOT_SET  133
#define ENUM_QUOTEENTRYREJECTREASON_CLIENT_CANNOT_BE_MODIFIED 134
#define ENUM_QUOTEENTRYREJECTREASON_CLIENT_TYPE_NOT_SET  135
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_CLIENT_CODE_FOR_CLIENT_TYPE_OWN 136
#define ENUM_QUOTEENTRYREJECTREASON_OWN_CLIENT_TYPE_NOT_ALLOWED 137
#define ENUM_QUOTEENTRYREJECTREASON_MESSAGE_TAG_NOT_SET  138
#define ENUM_QUOTEENTRYREJECTREASON_PRICE_BEYOND_CIRCUIT_LIMIT 139
#define ENUM_QUOTEENTRYREJECTREASON_QUANTITY_NOT_A_MULTIPLE_OF_LOT_SIZE 140
#define ENUM_QUOTEENTRYREJECTREASON_QUOTES_NOT_ALLOWED_IN_RRM 141
#define ENUM_QUOTEENTRYREJECTREASON_AMOUNT_EXCEEDS_TVL   142
#define ENUM_QUOTEENTRYREJECTREASON_QUOTES_NOT_ALLOWED_IN_AUCTION 143
#define ENUM_QUOTEENTRYREJECTREASON_CLIENT_CODE_DEBARRED 144
#define ENUM_QUOTEENTRYREJECTREASON_PRICE_NOT_MULTIPLE_OF_TICK_SIZE 145
#define ENUM_QUOTEENTRYREJECTREASON_QUOTES_NOT_ALLOWED_IN_POST_CLOSING 146
#define ENUM_QUOTEENTRYREJECTREASON_PARTICIPANT_CODE_CANNOT_BE_MODIFIED 147
#define ENUM_QUOTEENTRYREJECTREASON_CLIENT_TYPE_CANNOT_BE_MODIFIED 148
#define ENUM_QUOTEENTRYREJECTREASON_QUOTING_NOT_ALLOWED  149
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_PARTICIPANT_CODE 150
#define ENUM_QUOTEENTRYREJECTREASON_PRICE_BAND_NOT_SET   151
#define ENUM_QUOTEENTRYREJECTREASON_TICK_SIZE_NOT_SET    152
#define ENUM_QUOTEENTRYREJECTREASON_CLIENT_TYPE_NOT_ALLOWED 153
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_VALUE        154
#define ENUM_QUOTEENTRYSTATUS_ACCEPTED                   0
#define ENUM_QUOTEENTRYSTATUS_REJECTED                   5
#define ENUM_QUOTEENTRYSTATUS_REMOVED_AND_REJECTED       6
#define ENUM_QUOTEENTRYSTATUS_PENDING                    10
#define ENUM_QUOTEEVENTLIQUIDITYIND_ADDED_LIQUIDITY      1
#define ENUM_QUOTEEVENTLIQUIDITYIND_REMOVED_LIQUIDITY    2
#define ENUM_QUOTEEVENTREASON_PENDING_CANCELLATION_EXECUTED 14
#define ENUM_QUOTEEVENTREASON_INVALID_PRICE              15
#define ENUM_QUOTEEVENTREASON_SELF_TRADE_QUOTE_DELETED   16
#define ENUM_QUOTEEVENTREASON_REVERSE_TRADE_QUOTE_DELETED 17
#define ENUM_QUOTEEVENTREASON_CLIENT_RRM_QUOTE_DELETED   18
#define ENUM_QUOTEEVENTREASON_CLIENT_SUSPENDED_QUOTE_DELETED 19
#define ENUM_QUOTEEVENTSIDE_BUY                          1
#define ENUM_QUOTEEVENTSIDE_SELL                         2
#define ENUM_QUOTEEVENTTYPE_REMOVED_QUOTE_SIDE           3
#define ENUM_QUOTEEVENTTYPE_PARTIALLY_FILLED             4
#define ENUM_QUOTEEVENTTYPE_FILLED                       5
#define ENUM_QUOTESIZETYPE_TOTALSIZE                     1
#define ENUM_QUOTESIZETYPE_OPENSIZE                      2
#define ENUM_RRMSTATE_NORRM                              0
#define ENUM_RRMSTATE_INRRM                              1
#define ENUM_RRMSTATE_OUTOFRRM                           2
#define ENUM_REFAPPLID_TRADEENHANCEMENT                  0
#define ENUM_REFAPPLID_TRADE                             1
#define ENUM_REFAPPLID_NEWS                              2
#define ENUM_REFAPPLID_SERVICE_AVAILABILITY              3
#define ENUM_REFAPPLID_SESSION_DATA                      4
#define ENUM_REFAPPLID_LISTENER_DATA                     5
#define ENUM_REFAPPLID_RISKCONTROL                       6
#define ENUM_REFAPPLID_ORDERBYORDER                      7
#define ENUM_REFAPPLID_RISKADMIN                         8
#define ENUM_REFAPPLID_USERLEVEL                         9
#define ENUM_REFAPPLID_GROUPLEVEL                        10
#define ENUM_REFAPPLID_INSTRUMENTLEVEL                   11
#define LEN_REFAPPLLASTMSGID                             16
#define LEN_REGULATORYTEXT                               20
#define ENUM_RELATEDPRODUCTCOMPLEX_STANDARD_OPTION_STRATEGY 2
#define ENUM_RELATEDPRODUCTCOMPLEX_NON_STANDARD_OPTION_STRATEGY 3
#define ENUM_RELATEDPRODUCTCOMPLEX_VOLATILITY_STRATEGY   4
#define ENUM_RELATEDPRODUCTCOMPLEX_FUTURES_SPREAD        5
#define LEN_REQUESTINGPARTYCLEARINGFIRM                  9
#define LEN_REQUESTINGPARTYENTERINGFIRM                  9
#define ENUM_REQUESTINGPARTYIDENTERINGFIRM_PARTICIPANT   1
#define ENUM_REQUESTINGPARTYIDENTERINGFIRM_MARKETSUPERVISION 2
#define ENUM_REQUESTINGPARTYIDEXECUTINGSYSTEM_EUREXCLEARING 1
#define ENUM_REQUESTINGPARTYIDEXECUTINGSYSTEM_EUREXTRADING 2
#define ENUM_RISKLIMITACTION_WARNING                     4
#define ENUM_RISKLIMITACTION_QUEUEINBOUND                0
#define ENUM_RISKLIMITACTION_REJECT                      2
#define ENUM_RISKMODESTATUS_IN                           1
#define ENUM_RISKMODESTATUS_OUT                          2
#define LEN_ROOTPARTYCLEARINGFIRM                        5
#define LEN_ROOTPARTYCLEARINGORGANIZATION                4
#define LEN_ROOTPARTYEXECUTINGFIRM                       5
#define LEN_ROOTPARTYEXECUTINGTRADER                     6
#define LEN_ROOTPARTYIDBENEFICIARY                       9
#define LEN_ROOTPARTYIDORDERORIGINATIONFIRM              7
#define LEN_ROOTPARTYIDTAKEUPTRADINGFIRM                 5
#define ENUM_STPCFLAG_PASSIVE                            0
#define ENUM_STPCFLAG_ACTIVE                             1
#define ENUM_SCOPEIDENTIFIER_SUSPENSION                  1
#define ENUM_SCOPEIDENTIFIER_PRODUCT_SUSPENSION          2
#define ENUM_SCOPEIDENTIFIER_COLLATERAL_SUSPENSION       3
#define ENUM_SCOPEIDENTIFIER_COOLINGOFF_SUSPENSION       4
#define ENUM_SCOPEIDENTIFIER_PRODUCT                     5
#define ENUM_SCOPEIDENTIFIER_MARKET                      6
#define ENUM_SCOPEIDENTIFIER_COOLINGOFF                  7
#define ENUM_SCOPEIDENTIFIER_COLLATERAL                  8
#define ENUM_SCOPEIDENTIFIER_PRODUCT_NEAR_DURATION       9
#define ENUM_SCOPEIDENTIFIER_CLIENT_SUSPENSION           10
#define ENUM_SEGMENTINDICATOR_CASH                       1
#define ENUM_SEGMENTINDICATOR_EQUITY                     2
#define ENUM_SEGMENTINDICATOR_REPO                       3
#define ENUM_SEGMENTINDICATOR_SLB                        4
#define ENUM_SEGMENTINDICATOR_SAUC                       5
#define ENUM_SEGMENTINDICATOR_DEBT                       6
#define ENUM_SEGMENTINDICATOR_CURRENCY                   7
#define ENUM_SEGMENTINDICATOR_CDX                        8
#define ENUM_SEGMENTINDICATOR_IRD                        9
#define ENUM_SEGMENTINDICATOR_INX                        10
#define ENUM_SEGMENTINDICATOR_EDX                        11
#define ENUM_SEGMENTINDICATOR_DERIVATIVES                12
#define ENUM_SEGMENTINDICATOR_EUREX                      13
#define ENUM_SEGMENTINDICATOR_EEX                        14
#define ENUM_SEGMENTINDICATOR_BCX                        15
#define ENUM_SESSIONMODE_HF                              1
#define ENUM_SESSIONMODE_LF                              2
#define ENUM_SESSIONMODE_GUI                             3
#define ENUM_SESSIONREJECTREASON_REQUIRED_TAG_MISSING    1
#define ENUM_SESSIONREJECTREASON_VALUE_IS_INCORRECT      5
#define ENUM_SESSIONREJECTREASON_DECRYPTION_PROBLEM      7
#define ENUM_SESSIONREJECTREASON_INVALID_MSGID           11
#define ENUM_SESSIONREJECTREASON_INCORRECT_NUMINGROUP_COUNT 16
#define ENUM_SESSIONREJECTREASON_OTHER                   99
#define ENUM_SESSIONREJECTREASON_THROTTLE_LIMIT_EXCEEDED 100
#define ENUM_SESSIONREJECTREASON_EXPOSURE_LIMIT_EXCEEDED 101
#define ENUM_SESSIONREJECTREASON_SERVICE_TEMPORARILY_NOT_AVAILABLE 102
#define ENUM_SESSIONREJECTREASON_SERVICE_NOT_AVAILABLE   103
#define ENUM_SESSIONREJECTREASON_RESULT_OF_TRANSACTION_UNKNOWN 104
#define ENUM_SESSIONREJECTREASON_OUTBOUND_CONVERSION_ERROR 105
#define ENUM_SESSIONREJECTREASON_HEARTBEAT_VIOLATION     152
#define ENUM_SESSIONREJECTREASON_INTERNAL_TECHNICAL_ERROR 200
#define ENUM_SESSIONREJECTREASON_VALIDATION_ERROR        210
#define ENUM_SESSIONREJECTREASON_USER_ALREADY_LOGGED_IN  211
#define ENUM_SESSIONREJECTREASON_ORDER_NOT_FOUND         10000
#define ENUM_SESSIONREJECTREASON_PRICE_NOT_REASONABLE    10001
#define ENUM_SESSIONREJECTREASON_CLIENTORDERID_NOT_UNIQUE 10002
#define ENUM_SESSIONREJECTREASON_QUOTE_ACTIVATION_IN_PROGRESS 10003
#define ENUM_SESSIONREJECTREASON_BU_BOOK_ORDER_LIMIT_EXCEEDED 10004
#define ENUM_SESSIONREJECTREASON_SESSION_BOOK_ORDER_LIMIT_EXCEEDED 10005
#define ENUM_SESSIONREJECTREASON_ACTIVITY_TIMESTAMP_NOT_MATCHED 10006
#define ENUM_SESSIONSTATUS_ACTIVE                        0
#define ENUM_SESSIONSTATUS_LOGOUT                        4
#define ENUM_SESSIONSUBMODE_REGULAR_TRADING_SESSION      0
#define ENUM_SESSIONSUBMODE_FIX_TRADING_SESSION          1
#define ENUM_SESSIONSUBMODE_REGULAR_BACK_OFFICE_SESSION  2
#define ENUM_SIDE_BUY                                    1
#define ENUM_SIDE_SELL                                   2
#define ENUM_SIDE_RECALL                                 3
#define ENUM_SIDE_EARLY_RETURN                           4
#define LEN_SIXLAKHFLAG                                  1
#define ENUM_SIXLAKHFLAG_ISSIXLAKHFLAG                   "Y"
#define ENUM_SIXLAKHFLAG_SIXLAKHFLAGNOT                  "N"
#define LEN_TARGETPARTYIDDESKID                          3
#define ENUM_TIMEINFORCE_DAY                             0
#define ENUM_TIMEINFORCE_GTC                             1
#define ENUM_TIMEINFORCE_IOC                             3
#define ENUM_TIMEINFORCE_GTD                             6
#define ENUM_TIMEINFORCE_GTCL                            7
#define ENUM_TRADSESEVENT_START_OF_SERVICE               101
#define ENUM_TRADSESEVENT_MARKET_RESET                   102
#define ENUM_TRADSESEVENT_END_OF_RESTATEMENT             103
#define ENUM_TRADSESEVENT_END_OF_DAY_SERVICE             104
#define ENUM_TRADSESMODE_TESTING                         1
#define ENUM_TRADSESMODE_SIMULATED                       2
#define ENUM_TRADSESMODE_PRODUCTION                      3
#define ENUM_TRADSESMODE_ACCEPTANCE                      4
#define ENUM_TRADEMANAGERSTATUS_UNAVAILABLE              0
#define ENUM_TRADEMANAGERSTATUS_AVAILABLE                1
#define ENUM_TRADEREPORTTYPE_SUBMIT                      0
#define ENUM_TRADEREPORTTYPE_ALLEGED                     1
#define ENUM_TRADEREPORTTYPE_NO_WAS_REPLACED             5
#define ENUM_TRADEREPORTTYPE_TRADE_BREAK                 7
#define LEN_TRADEVALUEFLAG                               1
#define ENUM_TRADINGCAPACITY_CUSTOMER                    1
#define ENUM_TRADINGCAPACITY_PRINCIPAL                   5
#define ENUM_TRADINGCAPACITY_MARKET_MAKER                6
#define ENUM_TRADINGSESSIONSUBID_CLOSING_AUCTION         4
#define ENUM_TRANSFERREASON_OWNER                        1
#define ENUM_TRANSFERREASON_CLEARER                      2
#define LEN_TREND                                        1
#define ENUM_TRIGGERED_NOT_TRIGGERED                     0
#define ENUM_TRIGGERED_TRIGGERED_STOP                    1
#define ENUM_TRIGGERED_TRIGGERED_OCO                     2
#define ENUM_USERSTATUS_USER_STOPPED                     10
#define ENUM_USERSTATUS_USER_RELEASED                    11
#define LEN_VARTEXT                                      2000

/*
 * End of DEPRECATED defines
 */

#if defined(__cplusplus) || defined(c_plusplus)
} /* close scope of 'extern "C"' declaration. */
#endif

#endif

