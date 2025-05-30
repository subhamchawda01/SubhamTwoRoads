/*******************************************************************************
 * Permission to use this/these file(s) is subject to the Terms of Use set
 * forth in the file Terms_of_Use.txt accompanying this file.
 *******************************************************************************
 *
 *  FILE NAME: ETILayoutsNS_Derivatives.h
 *
 *  INTERFACE VERSION:   6.0
 *
 *  SUBVERSION:          D0002
 *
 *  BUILD NUMBER:        6.0.426.ga-6000426-20
 *
 *  DESCRIPTION:
 *
 *    This header file documents the binary message format of ETI.
 *    - All integers are in little endian byte order.
 *    - Padding bytes in following structures (char PadX[...]) are not required to be initialized.
 *
 *    DISCLAIMER:
 *
 *      Supported on Linux/x64 platforms with GNU C/C++ version 4.1 and 4.4.
 *
 *      This header file is meant to be compatible (but not supported) with any C/C++
 *      compiler/architecture that defines C99 compliant integer types in stdint.h and
 *      corresponds with the following alignment and padding requirements:
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

#ifndef __ETI_DERIVATIVES_LAYOUTS_WITH_NAMESPACE__
#define __ETI_DERIVATIVES_LAYOUTS_WITH_NAMESPACE__

#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define ETI_INTERFACE_VERSION "6.0"
#define ETI_BUILD_NUMBER "6.0.426.ga-6000426-20"

/*
 * No Value defines
 */
#define NO_VALUE_SLONG ((int64_t)0x8000000000000000L)
#define NO_VALUE_ULONG ((uint64_t)0xffffffffffffffffUL)
#define NO_VALUE_SINT ((int32_t)0x80000000)
#define NO_VALUE_UINT ((uint32_t)0xffffffff)
#define NO_VALUE_SSHORT ((int16_t)0x8000)
#define NO_VALUE_USHORT ((uint16_t)0xffff)
#define NO_VALUE_SCHAR ((int8_t)0x80)
#define NO_VALUE_UCHAR ((uint8_t)0xff)
#define NO_VALUE_STR 0
#define NO_VALUE_DATA_16 \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

/*
 * Template IDs defines
 */
#define TID_ADD_COMPLEX_INSTRUMENT_REQUEST 10301   // < AddComplexInstrumentRequest (Create Strategy)
#define TID_ADD_COMPLEX_INSTRUMENT_RESPONSE 10302  // < AddComplexInstrumentResponse (Create Strategy Response)
#define TID_ADD_FLEXIBLE_INSTRUMENT_REQUEST \
  10309  // < AddFlexibleInstrumentRequest (Create Flexible Instrument Request)
#define TID_ADD_FLEXIBLE_INSTRUMENT_RESPONSE \
  10310                                         // < AddFlexibleInstrumentResponse (Create Flexible Instrument Response)
#define TID_APPROVE_TES_TRADE_REQUEST 10603     // < ApproveTESTradeRequest (Approve TES Trade Request)
#define TID_BROADCAST_ERROR_NOTIFICATION 10032  // < BroadcastErrorNotification (Gap Fill)
#define TID_CROSS_REQUEST 10118                 // < CrossRequest (Cross Request)
#define TID_CROSS_REQUEST_RESPONSE 10119        // < CrossRequestResponse (Cross Request Response)
#define TID_DELETE_ALL_ORDER_BROADCAST 10122    // < DeleteAllOrderBroadcast (Order Mass Cancellation Notification)
#define TID_DELETE_ALL_ORDER_NR_RESPONSE 10124  // < DeleteAllOrderNRResponse (Order Mass Cancellation Response No Hits)
#define TID_DELETE_ALL_ORDER_QUOTE_EVENT_BROADCAST \
  10308                                            // < DeleteAllOrderQuoteEventBroadcast (Mass Cancellation Event)
#define TID_DELETE_ALL_ORDER_REQUEST 10120         // < DeleteAllOrderRequest (Order Mass Cancellation Request)
#define TID_DELETE_ALL_ORDER_RESPONSE 10121        // < DeleteAllOrderResponse (Order Mass Cancellation Response)
#define TID_DELETE_ALL_QUOTE_BROADCAST 10410       // < DeleteAllQuoteBroadcast (Quote Mass Cancellation Notification)
#define TID_DELETE_ALL_QUOTE_REQUEST 10408         // < DeleteAllQuoteRequest (Quote Mass Cancellation Request)
#define TID_DELETE_ALL_QUOTE_RESPONSE 10409        // < DeleteAllQuoteResponse (Quote Mass Cancellation Response)
#define TID_DELETE_ORDER_BROADCAST 10112           // < DeleteOrderBroadcast (Cancel Order Notification)
#define TID_DELETE_ORDER_COMPLEX_REQUEST 10123     // < DeleteOrderComplexRequest (Cancel Order Multi Leg)
#define TID_DELETE_ORDER_NR_RESPONSE 10111         // < DeleteOrderNRResponse (Cancel Order Response (Lean Order))
#define TID_DELETE_ORDER_RESPONSE 10110            // < DeleteOrderResponse (Cancel Order Response (Standard Order))
#define TID_DELETE_ORDER_SINGLE_REQUEST 10109      // < DeleteOrderSingleRequest (Cancel Order Single)
#define TID_DELETE_TES_TRADE_REQUEST 10602         // < DeleteTESTradeRequest (Delete TES Trade Request)
#define TID_ENTER_TES_TRADE_REQUEST 10600          // < EnterTESTradeRequest (Enter TES Trade Request)
#define TID_FORCED_LOGOUT_NOTIFICATION 10012       // < ForcedLogoutNotification (Session Logout Notification)
#define TID_FORCED_USER_LOGOUT_NOTIFICATION 10043  // < ForcedUserLogoutNotification (User Logout Notification)
#define TID_GATEWAY_REQUEST 10020                  // < GatewayRequest (Connection Gateway Request)
#define TID_GATEWAY_RESPONSE 10021                 // < GatewayResponse (Connection Gateway Response)
#define TID_HEARTBEAT 10011                        // < Heartbeat (Heartbeat)
#define TID_HEARTBEAT_NOTIFICATION 10023           // < HeartbeatNotification (Heartbeat Notification)
#define TID_INQUIRE_ENRICHMENT_RULE_ID_LIST_REQUEST \
  10040  // < InquireEnrichmentRuleIDListRequest (Trade Enrichment List Inquire)
#define TID_INQUIRE_ENRICHMENT_RULE_ID_LIST_RESPONSE \
  10041  // < InquireEnrichmentRuleIDListResponse (Trade Enrichment List Inquire Response)
#define TID_INQUIRE_MM_PARAMETER_REQUEST 10305  // < InquireMMParameterRequest (Inquire Market Maker Parameters)
#define TID_INQUIRE_MM_PARAMETER_RESPONSE \
  10306  // < InquireMMParameterResponse (Inquire Market Maker Parameters Response)
#define TID_INQUIRE_SESSION_LIST_REQUEST 10035     // < InquireSessionListRequest (Session List Inquire)
#define TID_INQUIRE_SESSION_LIST_RESPONSE 10036    // < InquireSessionListResponse (Session List Inquire Response)
#define TID_INQUIRE_USER_REQUEST 10038             // < InquireUserRequest (User List Inquire)
#define TID_INQUIRE_USER_RESPONSE 10039            // < InquireUserResponse (User List Inquire Response)
#define TID_LEGAL_NOTIFICATION_BROADCAST 10037     // < LegalNotificationBroadcast (Legal Notification)
#define TID_LOGON_REQUEST 10000                    // < LogonRequest (Session Logon)
#define TID_LOGON_RESPONSE 10001                   // < LogonResponse (Session Logon Response)
#define TID_LOGOUT_REQUEST 10002                   // < LogoutRequest (Session Logout)
#define TID_LOGOUT_RESPONSE 10003                  // < LogoutResponse (Session Logout Response)
#define TID_MM_PARAMETER_DEFINITION_REQUEST 10303  // < MMParameterDefinitionRequest (Market Maker Parameter Definition)
#define TID_MM_PARAMETER_DEFINITION_RESPONSE \
  10304                                // < MMParameterDefinitionResponse (Market Maker Parameter Definition Response)
#define TID_MASS_QUOTE_REQUEST 10405   // < MassQuoteRequest (Mass Quote)
#define TID_MASS_QUOTE_RESPONSE 10406  // < MassQuoteResponse (Mass Quote Response)
#define TID_MODIFY_ORDER_COMPLEX_REQUEST 10114  // < ModifyOrderComplexRequest (Replace Order Multi Leg)
#define TID_MODIFY_ORDER_NR_RESPONSE 10108      // < ModifyOrderNRResponse (Replace Order Response (Lean Order))
#define TID_MODIFY_ORDER_RESPONSE 10107         // < ModifyOrderResponse (Replace Order Response (Standard Order))
#define TID_MODIFY_ORDER_SINGLE_REQUEST 10106   // < ModifyOrderSingleRequest (Replace Order Single)
#define TID_MODIFY_ORDER_SINGLE_SHORT_REQUEST \
  10126                                      // < ModifyOrderSingleShortRequest (Replace Order Single (short layout))
#define TID_MODIFY_TES_TRADE_REQUEST 10601   // < ModifyTESTradeRequest (Modify TES Trade Request)
#define TID_NEW_ORDER_COMPLEX_REQUEST 10113  // < NewOrderComplexRequest (New Order Multi Leg)
#define TID_NEW_ORDER_NR_RESPONSE 10102      // < NewOrderNRResponse (New Order Response (Lean Order))
#define TID_NEW_ORDER_RESPONSE 10101         // < NewOrderResponse (New Order Response (Standard Order))
#define TID_NEW_ORDER_SINGLE_REQUEST 10100   // < NewOrderSingleRequest (New Order Single)
#define TID_NEW_ORDER_SINGLE_SHORT_REQUEST 10125    // < NewOrderSingleShortRequest (New Order Single (short layout))
#define TID_NEWS_BROADCAST 10031                    // < NewsBroadcast (News)
#define TID_ORDER_EXEC_NOTIFICATION 10104           // < OrderExecNotification (Book Order Execution)
#define TID_ORDER_EXEC_REPORT_BROADCAST 10117       // < OrderExecReportBroadcast (Extended Order Information)
#define TID_ORDER_EXEC_RESPONSE 10103               // < OrderExecResponse (Immediate Execution Response)
#define TID_PARTY_ACTION_REPORT 10042               // < PartyActionReport (Party Action Report)
#define TID_PARTY_ENTITLEMENTS_UPDATE_REPORT 10034  // < PartyEntitlementsUpdateReport (Entitlement Notification)
#define TID_QUOTE_ACTIVATION_NOTIFICATION 10411     // < QuoteActivationNotification (Quote Activation Notification)
#define TID_QUOTE_ACTIVATION_REQUEST 10403          // < QuoteActivationRequest (Quote Activation Request)
#define TID_QUOTE_ACTIVATION_RESPONSE 10404         // < QuoteActivationResponse (Quote Activation Response)
#define TID_QUOTE_EXECUTION_REPORT 10407            // < QuoteExecutionReport (Quote Execution Notification)
#define TID_RFQ_REQUEST 10401                       // < RFQRequest (Quote Request)
#define TID_RFQ_RESPONSE 10402                      // < RFQResponse (Quote Request Response)
#define TID_REJECT 10010                            // < Reject (Reject)
#define TID_RETRANSMIT_ME_MESSAGE_REQUEST 10026     // < RetransmitMEMessageRequest (Retransmit (Order/Quote Event))
#define TID_RETRANSMIT_ME_MESSAGE_RESPONSE \
  10027                                // < RetransmitMEMessageResponse (Retransmit Response (Order/Quote Event))
#define TID_RETRANSMIT_REQUEST 10008   // < RetransmitRequest (Retransmit)
#define TID_RETRANSMIT_RESPONSE 10009  // < RetransmitResponse (Retransmit Response)
#define TID_RISK_NOTIFICATION_BROADCAST 10033    // < RiskNotificationBroadcast (Risk Notification)
#define TID_SRQS_CREATE_DEAL_NOTIFICATION 10708  // < SRQSCreateDealNotification (SRQS Create Deal Notification)
#define TID_SRQS_DEAL_NOTIFICATION 10709         // < SRQSDealNotification (SRQS Deal Notification)
#define TID_SRQS_DEAL_RESPONSE 10705             // < SRQSDealResponse (SRQS Deal Response)
#define TID_SRQS_ENTER_QUOTE_REQUEST 10702       // < SRQSEnterQuoteRequest (SRQS Enter Quote Request)
#define TID_SRQS_HIT_QUOTE_REQUEST 10704         // < SRQSHitQuoteRequest (SRQS Hit Quote Request)
#define TID_SRQS_NEGOTIATION_NOTIFICATION \
  10713  // < SRQSNegotiationNotification (SRQS Negotiation Notification for Respondent)
#define TID_SRQS_NEGOTIATION_REQUESTER_NOTIFICATION \
  10712  // < SRQSNegotiationRequesterNotification (SRQS Negotiation Notification for Requester)
#define TID_SRQS_NEGOTIATION_STATUS_NOTIFICATION \
  10715  // < SRQSNegotiationStatusNotification (SRQS Negotiation Status Notification)
#define TID_SRQS_OPEN_NEGOTIATION_NOTIFICATION \
  10711  // < SRQSOpenNegotiationNotification (SRQS Open Negotiation Notification for Respondent)
#define TID_SRQS_OPEN_NEGOTIATION_REQUEST 10700  // < SRQSOpenNegotiationRequest (SRQS Open Negotiation Request)
#define TID_SRQS_OPEN_NEGOTIATION_REQUESTER_NOTIFICATION \
  10710  // < SRQSOpenNegotiationRequesterNotification (SRQS Open Negotiation Notification for Requester)
#define TID_SRQS_QUOTE_NOTIFICATION 10707          // < SRQSQuoteNotification (SRQS Quote Notification)
#define TID_SRQS_QUOTE_RESPONSE 10703              // < SRQSQuoteResponse (SRQS Quote Response)
#define TID_SRQS_STATUS_BROADCAST 10714            // < SRQSStatusBroadcast (SRQS Status Notification)
#define TID_SRQS_UPDATE_DEAL_STATUS_REQUEST 10706  // < SRQSUpdateDealStatusRequest (SRQS Update Deal Request)
#define TID_SRQS_UPDATE_NEGOTIATION_REQUEST 10701  // < SRQSUpdateNegotiationRequest (SRQS Update Negotiation Request)
#define TID_SERVICE_AVAILABILITY_BROADCAST 10030   // < ServiceAvailabilityBroadcast (Service Availability)
#define TID_SERVICE_AVAILABILITY_MARKET_BROADCAST \
  10044                                    // < ServiceAvailabilityMarketBroadcast (Service Availability Market)
#define TID_SUBSCRIBE_REQUEST 10025        // < SubscribeRequest (Subscribe)
#define TID_SUBSCRIBE_RESPONSE 10005       // < SubscribeResponse (Subscribe Response)
#define TID_TES_APPROVE_BROADCAST 10607    // < TESApproveBroadcast (Approve TES Trade Broadcast)
#define TID_TES_BROADCAST 10604            // < TESBroadcast (TES Broadcast)
#define TID_TES_DELETE_BROADCAST 10606     // < TESDeleteBroadcast (Delete TES Trade Broadcast)
#define TID_TES_EXECUTION_BROADCAST 10610  // < TESExecutionBroadcast (TES Execution Broadcast)
#define TID_TES_RESPONSE 10611             // < TESResponse (TES Response)
#define TID_TES_TRADE_BROADCAST 10614      // < TESTradeBroadcast (TES Trade Broadcast)
#define TID_TES_TRADING_SESSION_STATUS_BROADCAST 10615  // < TESTradingSessionStatusBroadcast (TES Status Broadcast )
#define TID_TES_UPLOAD_BROADCAST 10613                  // < TESUploadBroadcast (TES Trade Upload Broadcast )
#define TID_TM_TRADING_SESSION_STATUS_BROADCAST 10501   // < TMTradingSessionStatusBroadcast (Trade Notification Status)
#define TID_THROTTLE_UPDATE_NOTIFICATION 10028          // < ThrottleUpdateNotification (Throttle Update Notification)
#define TID_TRADE_BROADCAST 10500                       // < TradeBroadcast (Trade Notification)
#define TID_TRADING_SESSION_STATUS_BROADCAST 10307      // < TradingSessionStatusBroadcast (Trading Session Event)
#define TID_UNSUBSCRIBE_REQUEST 10006                   // < UnsubscribeRequest (Unsubscribe)
#define TID_UNSUBSCRIBE_RESPONSE 10007                  // < UnsubscribeResponse (Unsubscribe Response)
#define TID_UPLOAD_TES_TRADE_REQUEST 10612              // < UploadTESTradeRequest (Upload TES Trade Request)
#define TID_USER_LOGIN_REQUEST 10018                    // < UserLoginRequest (User Logon)
#define TID_USER_LOGIN_RESPONSE 10019                   // < UserLoginResponse (User Logon Response)
#define TID_USER_LOGOUT_REQUEST 10029                   // < UserLogoutRequest (User Logout)
#define TID_USER_LOGOUT_RESPONSE 10024                  // < UserLogoutResponse (User Logout Response)
#define TID_SESSION_REGISTRATION_RESPONSE 10054

//================================== Our defines ========================//

#define ETI_SESSION_LOGON_START_SEQUENCE 1
#define ETI_CONNECTION_GATEWAY_REQUEST_START_SEQUENCE 1
#define ETI_MAX_MSG_SIZE 72000

#define ETI_SESSION_STATUS_ACTIVE 0
#define ETI_SESSION_STATUS_LOGOUT 4

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
// binary values are used in byte order, pre computed network orders
#define N_TID_ADD_COMPLEX_INSTRUMENT_REQUEST 0x3D28   // < AddComplexInstrumentRequest (Create Strategy)
#define N_TID_ADD_COMPLEX_INSTRUMENT_RESPONSE 0x3E28  // < AddComplexInstrumentResponse (Create Strategy Response)
#define N_TID_BROADCAST_ERROR_NOTIFICATION 0x3027     // < BroadcastErrorNotification (Gap Fill)
#define N_TID_CROSS_REQUEST 0x8627                    // < CrossRequest (Cross Request)
#define N_TID_CROSS_REQUEST_RESPONSE 0x8727           // < CrossRequestResponse (Cross Request Response)
#define N_TID_DELETE_ALL_ORDER_BROADCAST 0x8A27  // < DeleteAllOrderBroadcast (Order Mass Cancellation Notification)
#define N_TID_DELETE_ALL_ORDER_NR_RESPONSE \
  0x8C27  // < DeleteAllOrderNRResponse (Order Mass Cancellation Response No Hits)
#define N_TID_DELETE_ALL_ORDER_QUOTE_EVENT_BROADCAST \
  0x4428                                           // < DeleteAllOrderQuoteEventBroadcast (Mass Cancellation Event)
#define N_TID_DELETE_ALL_ORDER_REQUEST 0x8827      // < DeleteAllOrderRequest (Order Mass Cancellation Request)
#define N_TID_DELETE_ALL_ORDER_RESPONSE 0x8927     // < DeleteAllOrderResponse (Order Mass Cancellation Response)
#define N_TID_DELETE_ALL_QUOTE_BROADCAST 0xAA28    // < DeleteAllQuoteBroadcast (Quote Mass Cancellation Notification)
#define N_TID_DELETE_ALL_QUOTE_REQUEST 0xA828      // < DeleteAllQuoteRequest (Quote Mass Cancellation Request)
#define N_TID_DELETE_ALL_QUOTE_RESPONSE 0xA928     // < DeleteAllQuoteResponse (Quote Mass Cancellation Response)
#define N_TID_DELETE_ORDER_BROADCAST 0x8027        // < DeleteOrderBroadcast (Cancel Order Notification)
#define N_TID_DELETE_ORDER_COMPLEX_REQUEST 0x8B27  // < DeleteOrderComplexRequest (Cancel Order Multi Leg)
#define N_TID_DELETE_ORDER_NR_RESPONSE 0x7F27      // < DeleteOrderNRResponse (Cancel Order Response (Lean Order))
#define N_TID_DELETE_ORDER_RESPONSE 0x7E27         // < DeleteOrderResponse (Cancel Order Response (Standard Order))
#define N_TID_DELETE_ORDER_SINGLE_REQUEST 0x7D27   // < DeleteOrderSingleRequest (Cancel Order Single)
#define N_TID_FORCED_LOGOUT_NOTIFICATION 0x1C27    // < ForcedLogoutNotification (Session Logout Notification)
#define N_TID_GATEWAY_REQUEST 0x2427               // < GatewayRequest (Connection Gateway Request)
#define N_TID_GATEWAY_RESPONSE 0x2527              // < GatewayResponse (Connection Gateway Response)
#define N_TID_HEARTBEAT 0x1B27                     // < Heartbeat (Heartbeat)
#define N_TID_HEARTBEAT_NOTIFICATION 0x2727        // < HeartbeatNotification (Heartbeat Notification)
#define N_TID_INQUIRE_MM_PARAMETER_REQUEST 0x4128  // < InquireMMParameterRequest (Inquire Market Maker Parameters)
#define N_TID_INQUIRE_MM_PARAMETER_RESPONSE \
  0x4228                              // < InquireMMParameterResponse (Inquire Market Maker Parameters Response)
#define N_TID_LOGON_REQUEST 0x1027    // < LogonRequest (Session Logon)
#define N_TID_LOGON_RESPONSE 0x1127   // < LogonResponse (Session Logon Response)
#define N_TID_LOGOUT_REQUEST 0x1227   // < LogoutRequest (Session Logout)
#define N_TID_LOGOUT_RESPONSE 0x1327  // < LogoutResponse (Session Logout Response)
#define N_TID_MM_PARAMETER_DEFINITION_REQUEST \
  0x3F28  // < MMParameterDefinitionRequest (Market Maker Parameter Definition)
#define N_TID_MM_PARAMETER_DEFINITION_RESPONSE \
  0x4028                                 // < MMParameterDefinitionResponse (Market Maker Parameter Definition Response)
#define N_TID_MASS_QUOTE_REQUEST 0xA528  // < MassQuoteRequest (Mass Quote)
#define N_TID_MASS_QUOTE_RESPONSE 0xA628           // < MassQuoteResponse (Mass Quote Response)
#define N_TID_MODIFY_ORDER_COMPLEX_REQUEST 0x8227  // < ModifyOrderComplexRequest (Replace Order Multi Leg)
#define N_TID_MODIFY_ORDER_NR_RESPONSE 0x7C27      // < ModifyOrderNRResponse (Replace Order Response (Lean Order))
#define N_TID_MODIFY_ORDER_RESPONSE 0x7B27         // < ModifyOrderResponse (Replace Order Response (Standard Order))
#define N_TID_MODIFY_ORDER_SINGLE_REQUEST 0x7A27   // < ModifyOrderSingleRequest (Replace Order Single)
#define N_TID_MODIFY_ORDER_SINGLE_SHORT_REQUEST \
  0x8E27                                        // < ModifyOrderSingleShortRequest (Replace Order Single (short layout))
#define N_TID_NEW_ORDER_COMPLEX_REQUEST 0x8127  // < NewOrderComplexRequest (New Order Multi Leg)
#define N_TID_NEW_ORDER_NR_RESPONSE 0x7627      // < NewOrderNRResponse (New Order Response (Lean Order))
#define N_TID_NEW_ORDER_RESPONSE 0x7527         // < NewOrderResponse (New Order Response (Standard Order))
#define N_TID_NEW_ORDER_SINGLE_REQUEST 0x7427   // < NewOrderSingleRequest (New Order Single)
#define N_TID_NEW_ORDER_SINGLE_SHORT_REQUEST 0x8D27    // < NewOrderSingleShortRequest (New Order Single (short layout))
#define N_TID_NEWS_BROADCAST 0x2F27                    // < NewsBroadcast (News)
#define N_TID_ORDER_EXEC_NOTIFICATION 0x7827           // < OrderExecNotification (Book Order Execution)
#define N_TID_ORDER_EXEC_REPORT_BROADCAST 0x8527       // < OrderExecReportBroadcast (Extended Order Information)
#define N_TID_ORDER_EXEC_RESPONSE 0x7727               // < OrderExecResponse (Immediate Execution Response)
#define N_TID_PARTY_ENTITLEMENTS_UPDATE_REPORT 0x3227  // < PartyEntitlementsUpdateReport (Entitlement Notification)
#define N_TID_QUOTE_ACTIVATION_NOTIFICATION 0xAB28     // < QuoteActivationNotification (Quote Activation Notification)
#define N_TID_QUOTE_ACTIVATION_REQUEST 0xA328          // < QuoteActivationRequest (Quote Activation Request)
#define N_TID_QUOTE_ACTIVATION_RESPONSE 0xA428         // < QuoteActivationResponse (Quote Activation Response)
#define N_TID_QUOTE_EXECUTION_REPORT 0xA728            // < QuoteExecutionReport (Quote Execution Notification)
#define N_TID_RFQ_REQUEST 0xA128                       // < RFQRequest (Quote Request)
#define N_TID_RFQ_RESPONSE 0xA228                      // < RFQResponse (Quote Request Response)
#define N_TID_REJECT 0x1A27                            // < Reject (Reject)
#define N_TID_RETRANSMIT_ME_MESSAGE_REQUEST 0x2A27     // < RetransmitMEMessageRequest (Retransmit (Order/Quote Event))
#define N_TID_RETRANSMIT_ME_MESSAGE_RESPONSE \
  0x2B27  // < RetransmitMEMessageResponse (Retransmit Response (Order/Quote Event))
#define N_TID_THROTTLE_UPDATE_NOTIFICATION 0x2C27      // < ThrottleUpdateNotification (Throttle Update Notification)
#define N_TID_TRADE_BROADCAST 0x0429                   // < TradeBroadcast (Trade Notification)
#define N_TID_TRADING_SESSION_STATUS_BROADCAST 0x4328  // < TradingSessionStatusBroadcast (Trading Session Event)
#define N_TID_UNSUBSCRIBE_REQUEST 0x1627               // < UnsubscribeRequest (Unsubscribe)
#define N_TID_UNSUBSCRIBE_RESPONSE 0x1727              // < UnsubscribeResponse (Unsubscribe Response)
#define N_TID_USER_LOGIN_REQUEST 0x2227                // < UserLoginRequest (User Logon)
#define N_TID_USER_LOGIN_RESPONSE 0x2327               // < UserLoginResponse (User Logon Response)
#define N_TID_USER_LOGOUT_REQUEST 0x2D27               // < UserLogoutRequest (User Logout)
#define N_TID_USER_LOGOUT_RESPONSE 0x2827              // < UserLogoutResponse (User Logout Response)

//=========================================================================================//

const int ETI_DERIVATIVES_TID_MIN = 10000;  // lowest assigned template ID
const int ETI_DERIVATIVES_TID_MAX = 10715;  // highest assigned template ID

/*
 * Max defines for sequences defines
 */
#define MAX_ADD_COMPLEX_INSTRUMENT_REQUEST_INSTRMT_LEG_GRP 20
#define MAX_ADD_COMPLEX_INSTRUMENT_RESPONSE_INSTRMT_LEG_GRP 20
#define MAX_DELETE_ALL_ORDER_BROADCAST_NOT_AFFECTED_ORDERS_GRP 500
#define MAX_DELETE_ALL_ORDER_BROADCAST_AFFECTED_ORD_GRP 500
#define MAX_DELETE_ALL_ORDER_RESPONSE_NOT_AFFECTED_ORDERS_GRP 500
#define MAX_DELETE_ALL_ORDER_RESPONSE_AFFECTED_ORD_GRP 500
#define MAX_DELETE_ALL_QUOTE_BROADCAST_NOT_AFFECTED_SECURITIES_GRP 500
#define MAX_DELETE_ALL_QUOTE_RESPONSE_NOT_AFFECTED_SECURITIES_GRP 500
#define MAX_ENTER_TES_TRADE_REQUEST_SIDE_ALLOC_GRP 30
#define MAX_ENTER_TES_TRADE_REQUEST_TRD_INSTRMNT_LEG_GRP 20
#define MAX_ENTER_TES_TRADE_REQUEST_INSTRUMENT_EVENT_GRP 2
#define MAX_ENTER_TES_TRADE_REQUEST_INSTRUMENT_ATTRIBUTE_GRP 6
#define MAX_ENTER_TES_TRADE_REQUEST_UNDERLYING_STIP_GRP 1
#define MAX_INQUIRE_ENRICHMENT_RULE_ID_LIST_RESPONSE_ENRICHMENT_RULES_GRP 400
#define MAX_INQUIRE_MM_PARAMETER_RESPONSE_MM_PARAMETER_GRP 9
#define MAX_INQUIRE_SESSION_LIST_RESPONSE_SESSIONS_GRP 1000
#define MAX_INQUIRE_USER_RESPONSE_PARTY_DETAILS_GRP 1000
#define MAX_MASS_QUOTE_REQUEST_QUOTE_ENTRY_GRP 100
#define MAX_MASS_QUOTE_RESPONSE_QUOTE_ENTRY_ACK_GRP 100
#define MAX_MODIFY_ORDER_COMPLEX_REQUEST_LEG_ORD_GRP 20
#define MAX_MODIFY_TES_TRADE_REQUEST_SIDE_ALLOC_GRP 30
#define MAX_MODIFY_TES_TRADE_REQUEST_TRD_INSTRMNT_LEG_GRP 20
#define MAX_NEW_ORDER_COMPLEX_REQUEST_LEG_ORD_GRP 20
#define MAX_ORDER_EXEC_NOTIFICATION_FILLS_GRP 100
#define MAX_ORDER_EXEC_NOTIFICATION_INSTRMNT_LEG_EXEC_GRP 600
#define MAX_ORDER_EXEC_REPORT_BROADCAST_LEG_ORD_GRP 20
#define MAX_ORDER_EXEC_REPORT_BROADCAST_FILLS_GRP 100
#define MAX_ORDER_EXEC_REPORT_BROADCAST_INSTRMNT_LEG_EXEC_GRP 600
#define MAX_ORDER_EXEC_RESPONSE_FILLS_GRP 100
#define MAX_ORDER_EXEC_RESPONSE_INSTRMNT_LEG_EXEC_GRP 600
#define MAX_QUOTE_ACTIVATION_NOTIFICATION_NOT_AFFECTED_SECURITIES_GRP 500
#define MAX_QUOTE_ACTIVATION_RESPONSE_NOT_AFFECTED_SECURITIES_GRP 500
#define MAX_QUOTE_EXECUTION_REPORT_QUOTE_EVENT_GRP 100
#define MAX_QUOTE_EXECUTION_REPORT_QUOTE_LEG_EXEC_GRP 600
#define MAX_SRQS_CREATE_DEAL_NOTIFICATION_SRQS_TRD_INSTRMNT_LEG_GRP 20
#define MAX_SRQS_NEGOTIATION_REQUESTER_NOTIFICATION_TARGET_PARTIES 50
#define MAX_SRQS_OPEN_NEGOTIATION_NOTIFICATION_QUOT_REQ_LEGS_GRP 20
#define MAX_SRQS_OPEN_NEGOTIATION_REQUEST_QUOT_REQ_LEGS_GRP 20
#define MAX_SRQS_OPEN_NEGOTIATION_REQUEST_TARGET_PARTIES 50
#define MAX_SRQS_OPEN_NEGOTIATION_REQUESTER_NOTIFICATION_QUOT_REQ_LEGS_GRP 20
#define MAX_SRQS_OPEN_NEGOTIATION_REQUESTER_NOTIFICATION_TARGET_PARTIES 50
#define MAX_SRQS_UPDATE_NEGOTIATION_REQUEST_TARGET_PARTIES 50
#define MAX_TES_APPROVE_BROADCAST_TRD_INSTRMNT_LEG_GRP 20
#define MAX_TES_APPROVE_BROADCAST_INSTRUMENT_EVENT_GRP 2
#define MAX_TES_APPROVE_BROADCAST_INSTRUMENT_ATTRIBUTE_GRP 6
#define MAX_TES_APPROVE_BROADCAST_UNDERLYING_STIP_GRP 1
#define MAX_TES_BROADCAST_SIDE_ALLOC_GRPBC 30
#define MAX_TES_BROADCAST_TRD_INSTRMNT_LEG_GRP 20
#define MAX_TES_BROADCAST_INSTRUMENT_EVENT_GRP 2
#define MAX_TES_BROADCAST_INSTRUMENT_ATTRIBUTE_GRP 6
#define MAX_TES_BROADCAST_UNDERLYING_STIP_GRP 1
#define MAX_TES_UPLOAD_BROADCAST_SIDE_ALLOC_EXT_GRP 30
#define MAX_TES_UPLOAD_BROADCAST_TRD_INSTRMNT_LEG_GRP 20
#define MAX_TES_UPLOAD_BROADCAST_INSTRUMENT_EVENT_GRP 2
#define MAX_TES_UPLOAD_BROADCAST_INSTRUMENT_ATTRIBUTE_GRP 6
#define MAX_TES_UPLOAD_BROADCAST_UNDERLYING_STIP_GRP 1
#define MAX_UPLOAD_TES_TRADE_REQUEST_SIDE_ALLOC_EXT_GRP 30
#define MAX_UPLOAD_TES_TRADE_REQUEST_TRD_INSTRMNT_LEG_GRP 20
#define MAX_UPLOAD_TES_TRADE_REQUEST_INSTRUMENT_EVENT_GRP 2
#define MAX_UPLOAD_TES_TRADE_REQUEST_INSTRUMENT_ATTRIBUTE_GRP 6
#define MAX_UPLOAD_TES_TRADE_REQUEST_UNDERLYING_STIP_GRP 1

/*
 * Data Type defines
 */

// DataType Account
#define LEN_ACCOUNT 2

// DataType ApplBegMsgID
#define LEN_APPL_BEG_MSGID 16

// DataType ApplEndMsgID
#define LEN_APPL_END_MSGID 16

// DataType ApplID
#define ENUM_APPLID_TRADE 1
#define ENUM_APPLID_NEWS 2
#define ENUM_APPLID_SERVICE_AVAILABILITY 3
#define ENUM_APPLID_SESSION_DATA 4
#define ENUM_APPLID_LISTENER_DATA 5
#define ENUM_APPLID_RISK_CONTROL 6
#define ENUM_APPLID_TES_MAINTENANCE 7
#define ENUM_APPLID_TES_TRADE 8
#define ENUM_APPLID_SRQS_MAINTENANCE 9
#define ENUM_APPLID_SERVICE_AVAILABILITY_MARKET 10

// DataType ApplIDStatus
#define ENUM_APPL_ID_STATUS_OUTBOUND_CONVERSION_ERROR 105

// DataType ApplMsgID
#define LEN_APPL_MSGID 16

// DataType ApplResendFlag
#define ENUM_APPL_RESEND_FLAG_FALSE 0
#define ENUM_APPL_RESEND_FLAG_TRUE 1

// DataType ApplSeqIndicator
#define ENUM_APPL_SEQ_INDICATOR_NO_RECOVERY_REQUIRED 0
#define ENUM_APPL_SEQ_INDICATOR_RECOVERY_REQUIRED 1

// DataType ApplSeqStatus
#define ENUM_APPL_SEQ_STATUS_UNAVAILABLE 0
#define ENUM_APPL_SEQ_STATUS_AVAILABLE 1

// DataType ApplUsageOrders
#define LEN_APPL_USAGE_ORDERS 1
#define ENUM_APPL_USAGE_ORDERS_AUTOMATED "A"
#define ENUM_APPL_USAGE_ORDERS_AUTOMATED_CHAR 'A'
#define ENUM_APPL_USAGE_ORDERS_MANUAL "M"
#define ENUM_APPL_USAGE_ORDERS_MANUAL_CHAR 'M'
#define ENUM_APPL_USAGE_ORDERS_AUTO_SELECT "B"
#define ENUM_APPL_USAGE_ORDERS_AUTO_SELECT_CHAR 'B'
#define ENUM_APPL_USAGE_ORDERS_NONE "N"
#define ENUM_APPL_USAGE_ORDERS_NONE_CHAR 'N'

// DataType ApplUsageQuotes
#define LEN_APPL_USAGE_QUOTES 1
#define ENUM_APPL_USAGE_QUOTES_AUTOMATED "A"
#define ENUM_APPL_USAGE_QUOTES_AUTOMATED_CHAR 'A'
#define ENUM_APPL_USAGE_QUOTES_MANUAL "M"
#define ENUM_APPL_USAGE_QUOTES_MANUAL_CHAR 'M'
#define ENUM_APPL_USAGE_QUOTES_AUTO_SELECT "B"
#define ENUM_APPL_USAGE_QUOTES_AUTO_SELECT_CHAR 'B'
#define ENUM_APPL_USAGE_QUOTES_NONE "N"
#define ENUM_APPL_USAGE_QUOTES_NONE_CHAR 'N'

// DataType ApplicationSystemName
#define LEN_APPLICATION_SYSTEM_NAME 30

// DataType ApplicationSystemVendor
#define LEN_APPLICATION_SYSTEM_VENDOR 30

// DataType ApplicationSystemVersion
#define LEN_APPLICATION_SYSTEM_VERSION 30

// DataType ComplianceText
#define LEN_COMPLIANCE_TEXT 20

// DataType CoreSrvc
#define LEN_CORE_SRVC 100
#define ENUM_CORE_SRVC_TRADING \
  "Order and Quote Management                                                                          "
#define ENUM_CORE_SRVC_ORDER_QUOTE_RETRANSMISSION \
  "Retransmission of Order and Quote Events                                                            "
#define ENUM_CORE_SRVC_TRADE_RETRANSMISSION \
  "Trades                                                                                              "
#define ENUM_CORE_SRVC_TES \
  "T7 Entry                                                                                            "
#define ENUM_CORE_SRVC_SRQS \
  "Selective Request for Quote Service                                                                 "
#define ENUM_CORE_SRVC_NONE \
  "None                                                                                                "

// DataType CrossedIndicator
#define ENUM_CROSSED_INDICATOR_NO_CROSSING 0
#define ENUM_CROSSED_INDICATOR_CROSS_REJECTED 1

// DataType CustOrderHandlingInst
#define LEN_CUST_ORDER_HANDLING_INST 1

// DataType DefaultCstmApplVerID
#define LEN_DEFAULT_CSTM_APPL_VERID 30

// DataType DefaultCstmApplVerSubID
#define LEN_DEFAULT_CSTM_APPL_VER_SUBID 5
#define ENUM_DEFAULT_CSTM_APPL_VER_SUBID_DERIVATIVES "D0002"

// DataType DeleteReason
#define ENUM_DELETE_REASON_NO_SPECIAL_REASON 100
#define ENUM_DELETE_REASON_TAS_CHANGE 101
#define ENUM_DELETE_REASON_INTRADAY_EXPIRATION 102
#define ENUM_DELETE_REASON_RISK_EVENT 103
#define ENUM_DELETE_REASON_STOP_TRADING 104
#define ENUM_DELETE_REASON_INSTRUMENT_DELETION 105
#define ENUM_DELETE_REASON_INSTRUMENT_SUSPENSION 106

// DataType EventType
#define ENUM_EVENT_TYPE_SWAP_START_DATE 8
#define ENUM_EVENT_TYPE_SWAP_END_DATE 9

// DataType ExecInst
#define ENUM_EXEC_INST_H 1
#define ENUM_EXEC_INST_Q 2
#define ENUM_EXEC_INST_H_Q 3
#define ENUM_EXEC_INST_H_6 5
#define ENUM_EXEC_INST_Q_6 6

// DataType ExecRestatementReason
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_BOOK_RESTATEMENT 1
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_ADDED 101
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_MODIFIED 102
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_CANCELLED 103
#define ENUM_EXEC_RESTATEMENT_REASON_IOC_ORDER_CANCELLED 105
#define ENUM_EXEC_RESTATEMENT_REASON_BOOK_ORDER_EXECUTED 108
#define ENUM_EXEC_RESTATEMENT_REASON_MARKET_ORDER_TRIGGERED 135
#define ENUM_EXEC_RESTATEMENT_REASON_CAO_ORDER_ACTIVATED 149
#define ENUM_EXEC_RESTATEMENT_REASON_CAO_ORDER_INACTIVATED 150
#define ENUM_EXEC_RESTATEMENT_REASON_OCO_ORDER_TRIGGERED 164
#define ENUM_EXEC_RESTATEMENT_REASON_STOP_ORDER_TRIGGERED 172
#define ENUM_EXEC_RESTATEMENT_REASON_OWNERSHIP_CHANGED 181
#define ENUM_EXEC_RESTATEMENT_REASON_ORDER_CANCELLATION_PENDING 197
#define ENUM_EXEC_RESTATEMENT_REASON_PENDING_CANCELLATION_EXECUTED 199
#define ENUM_EXEC_RESTATEMENT_REASON_BOC_ORDER_CANCELLED 212

// DataType ExecType
#define LEN_EXEC_TYPE 1
#define ENUM_EXEC_TYPE_NEW "0"
#define ENUM_EXEC_TYPE_NEW_CHAR '0'
#define ENUM_EXEC_TYPE_CANCELED "4"
#define ENUM_EXEC_TYPE_CANCELED_CHAR '4'
#define ENUM_EXEC_TYPE_REPLACED "5"
#define ENUM_EXEC_TYPE_REPLACED_CHAR '5'
#define ENUM_EXEC_TYPE_PENDING_CANCEL_E "6"
#define ENUM_EXEC_TYPE_PENDING_CANCEL_E_CHAR '6'
#define ENUM_EXEC_TYPE_SUSPENDED "9"
#define ENUM_EXEC_TYPE_SUSPENDED_CHAR '9'
#define ENUM_EXEC_TYPE_RESTATED "D"
#define ENUM_EXEC_TYPE_RESTATED_CHAR 'D'
#define ENUM_EXEC_TYPE_TRIGGERED "L"
#define ENUM_EXEC_TYPE_TRIGGERED_CHAR 'L'
#define ENUM_EXEC_TYPE_TRADE "F"
#define ENUM_EXEC_TYPE_TRADE_CHAR 'F'

// DataType ExecutingTraderQualifier
#define ENUM_EXECUTING_TRADER_QUALIFIER_ALGO 22
#define ENUM_EXECUTING_TRADER_QUALIFIER_HUMAN 24

// DataType ExerciseStyle
#define ENUM_EXERCISE_STYLE_EUROPEAN 0
#define ENUM_EXERCISE_STYLE_AMERICAN 1

// DataType FIXClOrdID
#define LEN_FIX_CL_ORDID 20

// DataType FIXEngineName
#define LEN_FIX_ENGINE_NAME 30

// DataType FIXEngineVendor
#define LEN_FIX_ENGINE_VENDOR 30

// DataType FIXEngineVersion
#define LEN_FIX_ENGINE_VERSION 30

// DataType FIXQuoteResponseID
#define LEN_FIX_QUOTE_RESPONSEID 20

// DataType FillLiquidityInd
#define ENUM_FILL_LIQUIDITY_IND_ADDED_LIQUIDITY 1
#define ENUM_FILL_LIQUIDITY_IND_REMOVED_LIQUIDITY 2
#define ENUM_FILL_LIQUIDITY_IND_TRIGGERED_STOP_ORDER 5
#define ENUM_FILL_LIQUIDITY_IND_TRIGGERED_OCO_ORDER 6
#define ENUM_FILL_LIQUIDITY_IND_TRIGGERED_MARKET_ORDER 7

// DataType FirmNegotiationID
#define LEN_FIRM_NEGOTIATIONID 20

// DataType FirmTradeID
#define LEN_FIRM_TRADEID 20

// DataType FreeText1
#define LEN_FREE_TEXT1 12

// DataType FreeText2
#define LEN_FREE_TEXT2 12

// DataType FreeText3
#define LEN_FREE_TEXT3 12

// DataType FuncCategory
#define LEN_FUNC_CATEGORY 100
#define ENUM_FUNC_CATEGORY_ORDER_HANDLING \
  "Order Handling                                                                                      "
#define ENUM_FUNC_CATEGORY_SESSION_LAYER \
  "Session Layer                                                                                       "
#define ENUM_FUNC_CATEGORY_QUOTE_HANDLING \
  "Quote Handling                                                                                      "
#define ENUM_FUNC_CATEGORY_BEST_QUOTE_HANDLING \
  "Best Quote Handling                                                                                 "
#define ENUM_FUNC_CATEGORY_QUOTE_AND_CROSS_REQUEST \
  "Quote and Cross Request                                                                             "
#define ENUM_FUNC_CATEGORY_STRATEGY_CREATION \
  "Strategy Creation                                                                                   "
#define ENUM_FUNC_CATEGORY_FLEXIBLE_INSTRUMENT_CREATION \
  "Flexible Instrument Creation                                                                        "
#define ENUM_FUNC_CATEGORY_TES_TRADING \
  "TES Trading                                                                                         "
#define ENUM_FUNC_CATEGORY_SRQS \
  "Selective Request for Quote Service                                                                 "
#define ENUM_FUNC_CATEGORY_OTHER \
  "Other                                                                                               "
#define ENUM_FUNC_CATEGORY_BROADCAST \
  "Broadcast                                                                                           "

// DataType GatewayStatus
#define ENUM_GATEWAY_STATUS_STANDBY 0
#define ENUM_GATEWAY_STATUS_ACTIVE 1

// DataType Headline
#define LEN_HEADLINE 256

// DataType HedgeType
#define ENUM_HEDGE_TYPE_DURATION_HEDGE 0
#define ENUM_HEDGE_TYPE_NOMINAL_HEDGE 1
#define ENUM_HEDGE_TYPE_PRICE_FACTOR_HEDGE 2

// DataType ImpliedMarketIndicator
#define ENUM_IMPLIED_MARKET_INDICATOR_NOT_IMPLIED 0
#define ENUM_IMPLIED_MARKET_INDICATOR_IMPLIED_IN_OUT 3

// DataType InstrAttribType
#define ENUM_INSTR_ATTRIB_TYPE_VARIABLE_RATE 5
#define ENUM_INSTR_ATTRIB_TYPE_COUPON_RATE 100
#define ENUM_INSTR_ATTRIB_TYPE_OFFSET_TO_THE_VARIABLE_COUPON_RATE 101
#define ENUM_INSTR_ATTRIB_TYPE_SWAP_CUSTOMER_1 102
#define ENUM_INSTR_ATTRIB_TYPE_SWAP_CUSTOMER_2 103
#define ENUM_INSTR_ATTRIB_TYPE_CASH_BASKET_REFERENCE 104

// DataType InstrAttribValue
#define LEN_INSTR_ATTRIB_VALUE 32

// DataType LastEntityProcessed
#define LEN_LAST_ENTITY_PROCESSED 16

// DataType LastFragment
#define ENUM_LAST_FRAGMENT_NOT_LAST_MESSAGE 0
#define ENUM_LAST_FRAGMENT_LAST_MESSAGE 1

// DataType LastMkt
#define ENUM_LAST_MKT_XEUR 1
#define ENUM_LAST_MKT_XEEE 2

// DataType LeavesQtyDisclosureInstruction
#define ENUM_LEAVES_QTY_DISCLOSURE_INSTRUCTION_NO 0
#define ENUM_LEAVES_QTY_DISCLOSURE_INSTRUCTION_YES 1

// DataType LegAccount
#define LEN_LEG_ACCOUNT 2

// DataType LegPositionEffect
#define LEN_LEG_POSITION_EFFECT 1
#define ENUM_LEG_POSITION_EFFECT_CLOSE "C"
#define ENUM_LEG_POSITION_EFFECT_CLOSE_CHAR 'C'
#define ENUM_LEG_POSITION_EFFECT_OPEN "O"
#define ENUM_LEG_POSITION_EFFECT_OPEN_CHAR 'O'

// DataType LegSecurityType
#define ENUM_LEG_SECURITY_TYPE_MULTILEG_INSTRUMENT 1
#define ENUM_LEG_SECURITY_TYPE_UNDERLYING_LEG 2

// DataType LegSide
#define ENUM_LEG_SIDE_BUY 1
#define ENUM_LEG_SIDE_SELL 2

// DataType ListUpdateAction
#define LEN_LIST_UPDATE_ACTION 1
#define ENUM_LIST_UPDATE_ACTION_ADD "A"
#define ENUM_LIST_UPDATE_ACTION_ADD_CHAR 'A'
#define ENUM_LIST_UPDATE_ACTION_DELETE "D"
#define ENUM_LIST_UPDATE_ACTION_DELETE_CHAR 'D'

// DataType MarketID
#define ENUM_MARKETID_XEUR 1
#define ENUM_MARKETID_XEEE 2

// DataType MassActionReason
#define ENUM_MASS_ACTION_REASON_NO_SPECIAL_REASON 0
#define ENUM_MASS_ACTION_REASON_STOP_TRADING 1
#define ENUM_MASS_ACTION_REASON_EMERGENCY 2
#define ENUM_MASS_ACTION_REASON_MARKET_MAKER_PROTECTION 3
#define ENUM_MASS_ACTION_REASON_SESSION_LOSS 6
#define ENUM_MASS_ACTION_REASON_DUPLICATE_SESSION_LOGIN 7
#define ENUM_MASS_ACTION_REASON_CLEARING_RISK_CONTROL 8
#define ENUM_MASS_ACTION_REASON_PRODUCT_STATE_HALT 105
#define ENUM_MASS_ACTION_REASON_PRODUCT_STATE_HOLIDAY 106
#define ENUM_MASS_ACTION_REASON_INSTRUMENT_SUSPENDED 107
#define ENUM_MASS_ACTION_REASON_COMPLEX_INSTRUMENT_DELETION 109
#define ENUM_MASS_ACTION_REASON_VOLATILITY_INTERRUPTION 110
#define ENUM_MASS_ACTION_REASON_PRODUCT_TEMPORARILY_NOT_TRADEABLE 111

// DataType MassActionType
#define ENUM_MASS_ACTION_TYPE_SUSPEND_QUOTES 1
#define ENUM_MASS_ACTION_TYPE_RELEASE_QUOTES 2

// DataType MatchSubType
#define ENUM_MATCH_SUB_TYPE_OPENING_AUCTION 1
#define ENUM_MATCH_SUB_TYPE_CLOSING_AUCTION 2
#define ENUM_MATCH_SUB_TYPE_INTRADAY_AUCTION 3
#define ENUM_MATCH_SUB_TYPE_CIRCUIT_BREAKER_AUCTION 4

// DataType MatchType
#define ENUM_MATCH_TYPE_CONFIRMED_TRADE_REPORT 3
#define ENUM_MATCH_TYPE_AUTO_MATCH_INCOMING 4
#define ENUM_MATCH_TYPE_CROSS_AUCTION 5
#define ENUM_MATCH_TYPE_CALL_AUCTION 7
#define ENUM_MATCH_TYPE_AUTO_MATCH_RESTING 11

// DataType MatchingEngineStatus
#define ENUM_MATCHING_ENGINE_STATUS_UNAVAILABLE 0
#define ENUM_MATCHING_ENGINE_STATUS_AVAILABLE 1

// DataType MessageEventSource
#define LEN_MESSAGE_EVENT_SOURCE 1
#define ENUM_MESSAGE_EVENT_SOURCE_BROADCAST_TO_INITIATOR "I"
#define ENUM_MESSAGE_EVENT_SOURCE_BROADCAST_TO_INITIATOR_CHAR 'I'
#define ENUM_MESSAGE_EVENT_SOURCE_BROADCAST_TO_APPROVER "A"
#define ENUM_MESSAGE_EVENT_SOURCE_BROADCAST_TO_APPROVER_CHAR 'A'
#define ENUM_MESSAGE_EVENT_SOURCE_BROADCAST_TO_REQUESTER "R"
#define ENUM_MESSAGE_EVENT_SOURCE_BROADCAST_TO_REQUESTER_CHAR 'R'
#define ENUM_MESSAGE_EVENT_SOURCE_BROADCAST_TO_QUOTE_SUBMITTER "Q"
#define ENUM_MESSAGE_EVENT_SOURCE_BROADCAST_TO_QUOTE_SUBMITTER_CHAR 'Q'

// DataType MsgType
#define LEN_MSG_TYPE 3
#define ENUM_MSG_TYPE_HEARTBEAT "0  "
#define ENUM_MSG_TYPE_REJECT "3  "
#define ENUM_MSG_TYPE_LOGOUT "5  "
#define ENUM_MSG_TYPE_EXECUTION_REPORT "8  "
#define ENUM_MSG_TYPE_LOGON "A  "
#define ENUM_MSG_TYPE_NEW_ORDER_MULTILEG "AB "
#define ENUM_MSG_TYPE_MULTILEG_ORDER_CANCEL_REPLACE "AC "
#define ENUM_MSG_TYPE_TRADE_CAPTURE_REPORT "AE "
#define ENUM_MSG_TYPE_TRADE_CAPTURE_REPORT_ACK "AR "
#define ENUM_MSG_TYPE_NEWS "B  "
#define ENUM_MSG_TYPE_USER_REQUEST "BE "
#define ENUM_MSG_TYPE_USER_RESPONSE "BF "
#define ENUM_MSG_TYPE_APPLICATION_MESSAGE_REQUEST "BW "
#define ENUM_MSG_TYPE_APPLICATION_MESSAGE_REQUEST_ACK "BX "
#define ENUM_MSG_TYPE_APPLICATION_MESSAGE_REPORT "BY "
#define ENUM_MSG_TYPE_ORDER_MASS_ACTION_REPORT "BZ "
#define ENUM_MSG_TYPE_ORDER_MASS_ACTION_REQUEST "CA "
#define ENUM_MSG_TYPE_USER_NOTIFICATION "CB "
#define ENUM_MSG_TYPE_PARTY_RISK_LIMITS_UPDATE_REPORT "CR "
#define ENUM_MSG_TYPE_PARTY_ENTITLEMENTS_UPDATE_REPORT "CZ "
#define ENUM_MSG_TYPE_NEW_ORDER_SINGLE "D  "
#define ENUM_MSG_TYPE_ORDER_CANCEL_REQUEST "F  "
#define ENUM_MSG_TYPE_ORDER_CANCEL_REPLACE_REQUEST "G  "
#define ENUM_MSG_TYPE_QUOTE_REQUEST "R  "
#define ENUM_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH "W  "
#define ENUM_MSG_TYPE_MASS_QUOTE_ACKNOWLEDGEMENT "b  "
#define ENUM_MSG_TYPE_SECURITY_DEFINITION_REQUEST "c  "
#define ENUM_MSG_TYPE_SECURITY_DEFINITION "d  "
#define ENUM_MSG_TYPE_TRADING_SESSION_STATUS "h  "
#define ENUM_MSG_TYPE_MASS_QUOTE "i  "
#define ENUM_MSG_TYPE_QUOTE "S  "
#define ENUM_MSG_TYPE_QUOTE_ACK "CW "
#define ENUM_MSG_TYPE_QUOTE_STATUS_REPORT "AI "
#define ENUM_MSG_TYPE_QUOTE_RESPONSE "AJ "
#define ENUM_MSG_TYPE_REQUEST_ACKNOWLEDGE "U1 "
#define ENUM_MSG_TYPE_SESSION_DETAILS_LIST_REQUEST "U5 "
#define ENUM_MSG_TYPE_SESSION_DETAILS_LIST_RESPONSE "U6 "
#define ENUM_MSG_TYPE_QUOTE_EXECUTION_REPORT "U8 "
#define ENUM_MSG_TYPE_MM_PARAMETER_DEFINITION_REQUEST "U14"
#define ENUM_MSG_TYPE_CROSS_REQUEST "U16"
#define ENUM_MSG_TYPE_MM_PARAMETER_REQUEST "U17"
#define ENUM_MSG_TYPE_MM_PARAMETER_RESPONSE "U18"
#define ENUM_MSG_TYPE_PARTY_DETAIL_LIST_REQUEST "CF "
#define ENUM_MSG_TYPE_PARTY_DETAIL_LIST_REPORT "CG "
#define ENUM_MSG_TYPE_TRADE_ENRICHMENT_LIST_REQUEST "U7 "
#define ENUM_MSG_TYPE_TRADE_ENRICHMENT_LIST_REPORT "U9 "
#define ENUM_MSG_TYPE_PARTY_ACTION_REPORT "DI "

// DataType MultiLegReportingType
#define ENUM_MULTI_LEG_REPORTING_TYPE_SINGLE_SECURITY 1
#define ENUM_MULTI_LEG_REPORTING_TYPE_INDIVIDUAL_LEG_OF_A_MULTILEG_SECURITY 2

// DataType MultilegModel
#define ENUM_MULTILEG_MODEL_PREDEFINED_MULTILEG_SECURITY 0
#define ENUM_MULTILEG_MODEL_USER_DEFINED_MULTLEG 1

// DataType MultilegPriceModel
#define ENUM_MULTILEG_PRICE_MODEL_STANDARD 0
#define ENUM_MULTILEG_PRICE_MODEL_USER_DEFINED 1

// DataType NetworkMsgID
#define LEN_NETWORK_MSGID 8

// DataType OrdStatus
#define LEN_ORD_STATUS 1
#define ENUM_ORD_STATUS_NEW "0"
#define ENUM_ORD_STATUS_NEW_CHAR '0'
#define ENUM_ORD_STATUS_PARTIALLY_FILLED "1"
#define ENUM_ORD_STATUS_PARTIALLY_FILLED_CHAR '1'
#define ENUM_ORD_STATUS_FILLED "2"
#define ENUM_ORD_STATUS_FILLED_CHAR '2'
#define ENUM_ORD_STATUS_CANCELED "4"
#define ENUM_ORD_STATUS_CANCELED_CHAR '4'
#define ENUM_ORD_STATUS_PENDING_CANCEL "6"
#define ENUM_ORD_STATUS_PENDING_CANCEL_CHAR '6'
#define ENUM_ORD_STATUS_SUSPENDED "9"
#define ENUM_ORD_STATUS_SUSPENDED_CHAR '9'

// DataType OrdType
#define ENUM_ORD_TYPE_MARKET 1
#define ENUM_ORD_TYPE_LIMIT 2
#define ENUM_ORD_TYPE_STOP 3
#define ENUM_ORD_TYPE_STOP_LIMIT 4

// DataType OrderAttributeLiquidityProvision
#define ENUM_ORDER_ATTRIBUTE_LIQUIDITY_PROVISION_Y 1
#define ENUM_ORDER_ATTRIBUTE_LIQUIDITY_PROVISION_N 0

// DataType OrderAttributeRiskReduction
#define ENUM_ORDER_ATTRIBUTE_RISK_REDUCTION_Y 1
#define ENUM_ORDER_ATTRIBUTE_RISK_REDUCTION_N 0

// DataType OrderCategory
#define LEN_ORDER_CATEGORY 1
#define ENUM_ORDER_CATEGORY_ORDER "1"
#define ENUM_ORDER_CATEGORY_ORDER_CHAR '1'
#define ENUM_ORDER_CATEGORY_QUOTE "2"
#define ENUM_ORDER_CATEGORY_QUOTE_CHAR '2'

// DataType OrderRoutingIndicator
#define LEN_ORDER_ROUTING_INDICATOR 1
#define ENUM_ORDER_ROUTING_INDICATOR_YES "Y"
#define ENUM_ORDER_ROUTING_INDICATOR_YES_CHAR 'Y'
#define ENUM_ORDER_ROUTING_INDICATOR_NO "N"
#define ENUM_ORDER_ROUTING_INDICATOR_NO_CHAR 'N'

// DataType OrderSide
#define ENUM_ORDER_SIDE_BUY 1
#define ENUM_ORDER_SIDE_SELL 2

// DataType OwnershipIndicator
#define ENUM_OWNERSHIP_INDICATOR_NO_CHANGE_OF_OWNERSHIP 0
#define ENUM_OWNERSHIP_INDICATOR_CHANGE_TO_EXECUTING_TRADER 1

// DataType Pad1
#define LEN_PAD1 1

// DataType Pad1_1
#define LEN_PAD1_1 1

// DataType Pad2
#define LEN_PAD2 2

// DataType Pad2_1
#define LEN_PAD2_1 2

// DataType Pad2_2
#define LEN_PAD2_2 2

// DataType Pad3
#define LEN_PAD3 3

// DataType Pad4
#define LEN_PAD4 4

// DataType Pad4_1
#define LEN_PAD4_1 4

// DataType Pad5
#define LEN_PAD5 5

// DataType Pad5_1
#define LEN_PAD5_1 5

// DataType Pad6
#define LEN_PAD6 6

// DataType Pad6_1
#define LEN_PAD6_1 6

// DataType Pad6_2
#define LEN_PAD6_2 6

// DataType Pad7
#define LEN_PAD7 7

// DataType Pad7_1
#define LEN_PAD7_1 7

// DataType PartyActionType
#define ENUM_PARTY_ACTION_TYPE_HALT_TRADING 1
#define ENUM_PARTY_ACTION_TYPE_REINSTATE 2

// DataType PartyDetailDeskID
#define LEN_PARTY_DETAIL_DESKID 3

// DataType PartyDetailExecutingTrader
#define LEN_PARTY_DETAIL_EXECUTING_TRADER 6

// DataType PartyDetailRoleQualifier
#define ENUM_PARTY_DETAIL_ROLE_QUALIFIER_TRADER 10
#define ENUM_PARTY_DETAIL_ROLE_QUALIFIER_HEAD_TRADER 11
#define ENUM_PARTY_DETAIL_ROLE_QUALIFIER_SUPERVISOR 12

// DataType PartyDetailStatus
#define ENUM_PARTY_DETAIL_STATUS_ACTIVE 0
#define ENUM_PARTY_DETAIL_STATUS_SUSPEND 1

// DataType PartyEnteringTrader
#define LEN_PARTY_ENTERING_TRADER 6

// DataType PartyExecutingFirm
#define LEN_PARTY_EXECUTING_FIRM 5

// DataType PartyExecutingTrader
#define LEN_PARTY_EXECUTING_TRADER 6

// DataType PartyIDBeneficiary
#define LEN_PARTY_ID_BENEFICIARY 9

// DataType PartyIDEnteringFirm
#define ENUM_PARTY_ID_ENTERING_FIRM_PARTICIPANT 1
#define ENUM_PARTY_ID_ENTERING_FIRM_MARKET_SUPERVISION 2

// DataType PartyIDLocationID
#define LEN_PARTY_ID_LOCATIONID 2

// DataType PartyIDOrderOriginationFirm
#define LEN_PARTY_ID_ORDER_ORIGINATION_FIRM 7

// DataType PartyIDOriginationMarket
#define ENUM_PARTY_ID_ORIGINATION_MARKET_XKFE 1
#define ENUM_PARTY_ID_ORIGINATION_MARKET_XTAF 2

// DataType PartyIDPositionAccount
#define LEN_PARTY_ID_POSITION_ACCOUNT 32

// DataType PartyIDSettlementLocation
#define ENUM_PARTY_ID_SETTLEMENT_LOCATION_CLEARSTREM_BANKING_FRANKFURT 1
#define ENUM_PARTY_ID_SETTLEMENT_LOCATION_CLEARSTREM_BANKING_LUXEMBURG 2
#define ENUM_PARTY_ID_SETTLEMENT_LOCATION_CLS_GROUP 3
#define ENUM_PARTY_ID_SETTLEMENT_LOCATION_EUROCLEAR 4

// DataType PartyIDTakeUpTradingFirm
#define LEN_PARTY_ID_TAKE_UP_TRADING_FIRM 5

// DataType PartyIdInvestmentDecisionMakerQualifier
#define ENUM_PARTY_ID_INVESTMENT_DECISION_MAKER_QUALIFIER_ALGO 22
#define ENUM_PARTY_ID_INVESTMENT_DECISION_MAKER_QUALIFIER_HUMAN 24

// DataType Password
#define LEN_PASSWORD 32

// DataType PositionEffect
#define LEN_POSITION_EFFECT 1
#define ENUM_POSITION_EFFECT_CLOSE "C"
#define ENUM_POSITION_EFFECT_CLOSE_CHAR 'C'
#define ENUM_POSITION_EFFECT_OPEN "O"
#define ENUM_POSITION_EFFECT_OPEN_CHAR 'O'

// DataType PriceDisclosureInstruction
#define ENUM_PRICE_DISCLOSURE_INSTRUCTION_NO 0
#define ENUM_PRICE_DISCLOSURE_INSTRUCTION_YES 1

// DataType PriceValidityCheckType
#define ENUM_PRICE_VALIDITY_CHECK_TYPE_NONE 0
#define ENUM_PRICE_VALIDITY_CHECK_TYPE_OPTIONAL 1
#define ENUM_PRICE_VALIDITY_CHECK_TYPE_MANDATORY 2

// DataType ProductComplex
#define ENUM_PRODUCT_COMPLEX_SIMPLE_INSTRUMENT 1
#define ENUM_PRODUCT_COMPLEX_STANDARD_OPTION_STRATEGY 2
#define ENUM_PRODUCT_COMPLEX_NON_STANDARD_OPTION_STRATEGY 3
#define ENUM_PRODUCT_COMPLEX_VOLATILITY_STRATEGY 4
#define ENUM_PRODUCT_COMPLEX_FUTURES_SPREAD 5
#define ENUM_PRODUCT_COMPLEX_INTER_PRODUCT_SPREAD 6
#define ENUM_PRODUCT_COMPLEX_STANDARD_FUTURE_STRATEGY 7
#define ENUM_PRODUCT_COMPLEX_PACK_AND_BUNDLE 8
#define ENUM_PRODUCT_COMPLEX_STRIP 9
#define ENUM_PRODUCT_COMPLEX_FLEXIBLE_SIMPLE_INSTRUMENT 10

// DataType PutOrCall
#define ENUM_PUT_OR_CALL_PUT 0
#define ENUM_PUT_OR_CALL_CALL 1

// DataType QuoteCancelType
#define ENUM_QUOTE_CANCEL_TYPE_CANCEL_ALL_QUOTES 4

// DataType QuoteEntryRejectReason
#define ENUM_QUOTE_ENTRY_REJECT_REASON_UNKNOWN_SECURITY 1
#define ENUM_QUOTE_ENTRY_REJECT_REASON_DUPLICATE_QUOTE 6
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_PRICE 8
#define ENUM_QUOTE_ENTRY_REJECT_REASON_NO_REFERENCE_PRICE_AVAILABLE 16
#define ENUM_QUOTE_ENTRY_REJECT_REASON_NO_SINGLE_SIDED_QUOTES 100
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_QUOTING_MODEL 103
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_SIZE 106
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_UNDERLYING_PRICE 107
#define ENUM_QUOTE_ENTRY_REJECT_REASON_BID_PRICE_NOT_REASONABLE 108
#define ENUM_QUOTE_ENTRY_REJECT_REASON_ASK_PRICE_NOT_REASONABLE 109
#define ENUM_QUOTE_ENTRY_REJECT_REASON_BID_PRICE_EXCEEDS_RANGE 110
#define ENUM_QUOTE_ENTRY_REJECT_REASON_ASK_PRICE_EXCEEDS_RANGE 111
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INSTRUMENT_STATE_FREEZE 115
#define ENUM_QUOTE_ENTRY_REJECT_REASON_DELETION_ALREADY_PENDING 116
#define ENUM_QUOTE_ENTRY_REJECT_REASON_PRE_TRADE_RISK_SESSION_LIMIT_EXCEEDED 117
#define ENUM_QUOTE_ENTRY_REJECT_REASON_PRE_TRADE_RISK_BU_LIMIT_EXCEEDED 118
#define ENUM_QUOTE_ENTRY_REJECT_REASON_ENTITLEMENT_NOT_ASSIGNED_FOR_UNDERLYING 119
#define ENUM_QUOTE_ENTRY_REJECT_REASON_CURRENTLY_NOT_TRADEABLE_ON_BOOK 124
#define ENUM_QUOTE_ENTRY_REJECT_REASON_QUANTITY_LIMIT_EXCEEDED 125
#define ENUM_QUOTE_ENTRY_REJECT_REASON_VALUE_LIMIT_EXCEEDED 126
#define ENUM_QUOTE_ENTRY_REJECT_REASON_INVALID_QUOTE_SPREAD 127
#define ENUM_QUOTE_ENTRY_REJECT_REASON_CANT_PROC_IN_CURR_INSTR_STATE 131

// DataType QuoteEntryStatus
#define ENUM_QUOTE_ENTRY_STATUS_ACCEPTED 0
#define ENUM_QUOTE_ENTRY_STATUS_REJECTED 5
#define ENUM_QUOTE_ENTRY_STATUS_REMOVED_AND_REJECTED 6
#define ENUM_QUOTE_ENTRY_STATUS_PENDING 10

// DataType QuoteEventLiquidityInd
#define ENUM_QUOTE_EVENT_LIQUIDITY_IND_ADDED_LIQUIDITY 1
#define ENUM_QUOTE_EVENT_LIQUIDITY_IND_REMOVED_LIQUIDITY 2

// DataType QuoteEventReason
#define ENUM_QUOTE_EVENT_REASON_PENDING_CANCELLATION_EXECUTED 14
#define ENUM_QUOTE_EVENT_REASON_INVALID_PRICE 15
#define ENUM_QUOTE_EVENT_REASON_CROSS_REJECTED 16

// DataType QuoteEventSide
#define ENUM_QUOTE_EVENT_SIDE_BUY 1
#define ENUM_QUOTE_EVENT_SIDE_SELL 2

// DataType QuoteEventType
#define ENUM_QUOTE_EVENT_TYPE_MODIFIED_QUOTE_SIDE 2
#define ENUM_QUOTE_EVENT_TYPE_REMOVED_QUOTE_SIDE 3
#define ENUM_QUOTE_EVENT_TYPE_PARTIALLY_FILLED 4
#define ENUM_QUOTE_EVENT_TYPE_FILLED 5

// DataType QuoteInstruction
#define ENUM_QUOTE_INSTRUCTION_DO_NOT_QUOTE 0
#define ENUM_QUOTE_INSTRUCTION_QUOTE 1

// DataType QuoteRefPriceSource
#define ENUM_QUOTE_REF_PRICE_SOURCE_UNDERLYING 1
#define ENUM_QUOTE_REF_PRICE_SOURCE_CUSTOM_INDEX 2

// DataType QuoteReqID
#define LEN_QUOTE_REQID 20

// DataType QuoteSizeType
#define ENUM_QUOTE_SIZE_TYPE_TOTAL_SIZE 1
#define ENUM_QUOTE_SIZE_TYPE_OPEN_SIZE 2

// DataType QuoteStatus
#define ENUM_QUOTE_STATUS_REMOVED 6
#define ENUM_QUOTE_STATUS_EXPIRED 7
#define ENUM_QUOTE_STATUS_ACTIVE 16

// DataType QuoteType
#define ENUM_QUOTE_TYPE_INDICATIVE 0
#define ENUM_QUOTE_TYPE_TRADEABLE 1

// DataType RefApplID
#define ENUM_REF_APPLID_TRADE 1
#define ENUM_REF_APPLID_NEWS 2
#define ENUM_REF_APPLID_SERVICE_AVAILABILITY 3
#define ENUM_REF_APPLID_SESSION_DATA 4
#define ENUM_REF_APPLID_LISTENER_DATA 5
#define ENUM_REF_APPLID_RISK_CONTROL 6
#define ENUM_REF_APPLID_TES_MAINTENANCE 7
#define ENUM_REF_APPLID_TES_TRADE 8
#define ENUM_REF_APPLID_SRQS_MAINTENANCE 9
#define ENUM_REF_APPLID_SERVICE_AVAILABILITY_MARKET 10

// DataType RefApplLastMsgID
#define LEN_REF_APPL_LAST_MSGID 16

// DataType RelatedProductComplex
#define ENUM_RELATED_PRODUCT_COMPLEX_STANDARD_OPTION_STRATEGY 2
#define ENUM_RELATED_PRODUCT_COMPLEX_NON_STANDARD_OPTION_STRATEGY 3
#define ENUM_RELATED_PRODUCT_COMPLEX_VOLATILITY_STRATEGY 4
#define ENUM_RELATED_PRODUCT_COMPLEX_FUTURES_SPREAD 5
#define ENUM_RELATED_PRODUCT_COMPLEX_INTER_PRODUCT_SPREAD 6
#define ENUM_RELATED_PRODUCT_COMPLEX_STANDARD_FUTURE_STRATEGY 7
#define ENUM_RELATED_PRODUCT_COMPLEX_PACK_AND_BUNDLE 8
#define ENUM_RELATED_PRODUCT_COMPLEX_STRIP 9

// DataType RequestingPartyClearingFirm
#define LEN_REQUESTING_PARTY_CLEARING_FIRM 9

// DataType RequestingPartyEnteringFirm
#define LEN_REQUESTING_PARTY_ENTERING_FIRM 9

// DataType RequestingPartyIDEnteringFirm
#define ENUM_REQUESTING_PARTY_ID_ENTERING_FIRM_PARTICIPANT 1
#define ENUM_REQUESTING_PARTY_ID_ENTERING_FIRM_MARKET_SUPERVISION 2

// DataType RequestingPartyIDExecutingSystem
#define ENUM_REQUESTING_PARTY_ID_EXECUTING_SYSTEM_EUREX_CLEARING 1
#define ENUM_REQUESTING_PARTY_ID_EXECUTING_SYSTEM_T7 2

// DataType RiskLimitAction
#define ENUM_RISK_LIMIT_ACTION_QUEUE_INBOUND 0
#define ENUM_RISK_LIMIT_ACTION_REJECT 2
#define ENUM_RISK_LIMIT_ACTION_WARNING 4

// DataType RootPartyClearingFirm
#define LEN_ROOT_PARTY_CLEARING_FIRM 5

// DataType RootPartyClearingOrganization
#define LEN_ROOT_PARTY_CLEARING_ORGANIZATION 4

// DataType RootPartyEnteringTrader
#define LEN_ROOT_PARTY_ENTERING_TRADER 6

// DataType RootPartyExecutingFirm
#define LEN_ROOT_PARTY_EXECUTING_FIRM 5

// DataType RootPartyExecutingTrader
#define LEN_ROOT_PARTY_EXECUTING_TRADER 6

// DataType RootPartyIDBeneficiary
#define LEN_ROOT_PARTY_ID_BENEFICIARY 9

// DataType RootPartyIDOrderOriginationFirm
#define LEN_ROOT_PARTY_ID_ORDER_ORIGINATION_FIRM 7

// DataType RootPartyIDPositionAccount
#define LEN_ROOT_PARTY_ID_POSITION_ACCOUNT 32

// DataType RootPartyIDTakeUpTradingFirm
#define LEN_ROOT_PARTY_ID_TAKE_UP_TRADING_FIRM 5

// DataType SecondaryGatewayStatus
#define ENUM_SECONDARY_GATEWAY_STATUS_STANDBY 0
#define ENUM_SECONDARY_GATEWAY_STATUS_ACTIVE 1

// DataType SelectiveRequestForQuoteServiceStatus
#define ENUM_SELECTIVE_REQUEST_FOR_QUOTE_SERVICE_STATUS_UNAVAILABLE 0
#define ENUM_SELECTIVE_REQUEST_FOR_QUOTE_SERVICE_STATUS_AVAILABLE 1

// DataType SessionMode
#define ENUM_SESSION_MODE_HF 1
#define ENUM_SESSION_MODE_LF 2
#define ENUM_SESSION_MODE_GUI 3

// DataType SessionRejectReason
#define ENUM_SESSION_REJECT_REASON_REQUIRED_TAG_MISSING 1
#define ENUM_SESSION_REJECT_REASON_VALUE_IS_INCORRECT 5
#define ENUM_SESSION_REJECT_REASON_DECRYPTION_PROBLEM 7
#define ENUM_SESSION_REJECT_REASON_INVALID_MSGID 11
#define ENUM_SESSION_REJECT_REASON_INCORRECT_NUM_IN_GROUP_COUNT 16
#define ENUM_SESSION_REJECT_REASON_OTHER 99
#define ENUM_SESSION_REJECT_REASON_THROTTLE_LIMIT_EXCEEDED 100
#define ENUM_SESSION_REJECT_REASON_EXPOSURE_LIMIT_EXCEEDED 101
#define ENUM_SESSION_REJECT_REASON_SERVICE_TEMPORARILY_NOT_AVAILABLE 102
#define ENUM_SESSION_REJECT_REASON_SERVICE_NOT_AVAILABLE 103
#define ENUM_SESSION_REJECT_REASON_RESULT_OF_TRANSACTION_UNKNOWN 104
#define ENUM_SESSION_REJECT_REASON_OUTBOUND_CONVERSION_ERROR 105
#define ENUM_SESSION_REJECT_REASON_HEARTBEAT_VIOLATION 152
#define ENUM_SESSION_REJECT_REASON_INTERNAL_TECHNICAL_ERROR 200
#define ENUM_SESSION_REJECT_REASON_VALIDATION_ERROR 210
#define ENUM_SESSION_REJECT_REASON_USER_ALREADY_LOGGED_IN 211
#define ENUM_SESSION_REJECT_REASON_SESSION_GATEWAY_ASSIGNMENT_EXPIRED 214
#define ENUM_SESSION_REJECT_REASON_GATEWAY_NOT_RESERVED_TO_SESSION 215
#define ENUM_SESSION_REJECT_REASON_GATEWAY_IS_STANDBY 216
#define ENUM_SESSION_REJECT_REASON_SESSION_LOGIN_LIMIT_REACHED 217
#define ENUM_SESSION_REJECT_REASON_PARTITION_NOT_REACHABLE_BY_HF_GATEWAY 218
#define ENUM_SESSION_REJECT_REASON_PARTITION_NOT_REACHABLE_BY_PS_GATEWAY 219
#define ENUM_SESSION_REJECT_REASON_NO_GATEWAY_AVAILABLE 222
#define ENUM_SESSION_REJECT_REASON_USER_ENTITLEMENT_DATA_TIMEOUT 223
#define ENUM_SESSION_REJECT_REASON_ORDER_NOT_FOUND 10000
#define ENUM_SESSION_REJECT_REASON_PRICE_NOT_REASONABLE 10001
#define ENUM_SESSION_REJECT_REASON_CLIENT_ORDERID_NOT_UNIQUE 10002
#define ENUM_SESSION_REJECT_REASON_QUOTE_ACTIVATION_IN_PROGRESS 10003
#define ENUM_SESSION_REJECT_REASON_BU_BOOK_ORDER_LIMIT_EXCEEDED 10004
#define ENUM_SESSION_REJECT_REASON_SESSION_BOOK_ORDER_LIMIT_EXCEEDED 10005
#define ENUM_SESSION_REJECT_REASON_STOP_BID_PRICE_NOT_REASONABLE 10006
#define ENUM_SESSION_REJECT_REASON_STOP_ASK_PRICE_NOT_REASONABLE 10007
#define ENUM_SESSION_REJECT_REASON_ORDER_NOT_EXECUTABLE_WITHIN_VALIDITY 10008
#define ENUM_SESSION_REJECT_REASON_CREATE_CI_THROTTLE_EXCEEDED 10010
#define ENUM_SESSION_REJECT_REASON_TRANSACTION_NOT_ALLOWED_IN_CURRENT_STATE 10011

// DataType SessionStatus
#define ENUM_SESSION_STATUS_ACTIVE 0
#define ENUM_SESSION_STATUS_LOGOUT 4

// DataType SessionSubMode
#define ENUM_SESSION_SUB_MODE_REGULAR_TRADING_SESSION 0
#define ENUM_SESSION_SUB_MODE_FIX_TRADING_SESSION 1
#define ENUM_SESSION_SUB_MODE_REGULAR_BACK_OFFICE_SESSION 2

// DataType SettlMethod
#define LEN_SETTL_METHOD 1
#define ENUM_SETTL_METHOD_CASH_SETTLEMENT "C"
#define ENUM_SETTL_METHOD_CASH_SETTLEMENT_CHAR 'C'
#define ENUM_SETTL_METHOD_PHYSICAL_SETTLEMENT "P"
#define ENUM_SETTL_METHOD_PHYSICAL_SETTLEMENT_CHAR 'P'

// DataType Side
#define ENUM_SIDE_BUY 1
#define ENUM_SIDE_SELL 2

// DataType SideDisclosureInstruction
#define ENUM_SIDE_DISCLOSURE_INSTRUCTION_NO 0
#define ENUM_SIDE_DISCLOSURE_INSTRUCTION_YES 1

// DataType SideLiquidityInd
#define ENUM_SIDE_LIQUIDITY_IND_ADDED_LIQUIDITY 1
#define ENUM_SIDE_LIQUIDITY_IND_REMOVED_LIQUIDITY 2
#define ENUM_SIDE_LIQUIDITY_IND_AUCTION 4

// DataType SkipValidations
#define ENUM_SKIP_VALIDATIONS_FALSE 0
#define ENUM_SKIP_VALIDATIONS_TRUE 1

// DataType Symbol
#define LEN_SYMBOL 4

// DataType T7EntryServiceRtmStatus
#define ENUM_T7_ENTRY_SERVICE_RTM_STATUS_UNAVAILABLE 0
#define ENUM_T7_ENTRY_SERVICE_RTM_STATUS_AVAILABLE 1

// DataType T7EntryServiceStatus
#define ENUM_T7_ENTRY_SERVICE_STATUS_UNAVAILABLE 0
#define ENUM_T7_ENTRY_SERVICE_STATUS_AVAILABLE 1

// DataType TargetPartyEnteringTrader
#define LEN_TARGET_PARTY_ENTERING_TRADER 6

// DataType TargetPartyExecutingFirm
#define LEN_TARGET_PARTY_EXECUTING_FIRM 5

// DataType TargetPartyExecutingTrader
#define LEN_TARGET_PARTY_EXECUTING_TRADER 6

// DataType TargetPartyIDDeskID
#define LEN_TARGET_PARTY_ID_DESKID 3

// DataType Text
#define LEN_TEXT 12

// DataType TimeInForce
#define ENUM_TIME_IN_FORCE_DAY 0
#define ENUM_TIME_IN_FORCE_GTC 1
#define ENUM_TIME_IN_FORCE_IOC 3
#define ENUM_TIME_IN_FORCE_GTD 6

// DataType TradSesEvent
#define ENUM_TRAD_SES_EVENT_START_OF_SERVICE 101
#define ENUM_TRAD_SES_EVENT_MARKET_RESET 102
#define ENUM_TRAD_SES_EVENT_END_OF_RESTATEMENT 103
#define ENUM_TRAD_SES_EVENT_END_OF_DAY_SERVICE 104
#define ENUM_TRAD_SES_EVENT_SERVICE_RESUMED 105

// DataType TradSesMode
#define ENUM_TRAD_SES_MODE_TESTING 1
#define ENUM_TRAD_SES_MODE_SIMULATED 2
#define ENUM_TRAD_SES_MODE_PRODUCTION 3
#define ENUM_TRAD_SES_MODE_ACCEPTANCE 4

// DataType TradeAllocStatus
#define ENUM_TRADE_ALLOC_STATUS_PENDING 1
#define ENUM_TRADE_ALLOC_STATUS_APPROVED 2
#define ENUM_TRADE_ALLOC_STATUS_AUTO_APPROVED 3
#define ENUM_TRADE_ALLOC_STATUS_UPLOADED 4
#define ENUM_TRADE_ALLOC_STATUS_CANCELED 5

// DataType TradeManagerStatus
#define ENUM_TRADE_MANAGER_STATUS_UNAVAILABLE 0
#define ENUM_TRADE_MANAGER_STATUS_AVAILABLE 1

// DataType TradePublishIndicator
#define ENUM_TRADE_PUBLISH_INDICATOR_DO_NOT_PUBLISH_TRADE 0
#define ENUM_TRADE_PUBLISH_INDICATOR_PUBLISH_TRADE 1

// DataType TradeReportID
#define LEN_TRADE_REPORTID 20

// DataType TradeReportText
#define LEN_TRADE_REPORT_TEXT 20

// DataType TradeReportType
#define ENUM_TRADE_REPORT_TYPE_SUBMIT 0
#define ENUM_TRADE_REPORT_TYPE_ALLEGED 1
#define ENUM_TRADE_REPORT_TYPE_ACCEPT 2
#define ENUM_TRADE_REPORT_TYPE_DECLINE 3
#define ENUM_TRADE_REPORT_TYPE_NO_WAS_REPLACED 5
#define ENUM_TRADE_REPORT_TYPE_TRADE_REPORT_CANCEL 6
#define ENUM_TRADE_REPORT_TYPE_TRADE_BREAK 7
#define ENUM_TRADE_REPORT_TYPE_ALLEGED_NEW 11
#define ENUM_TRADE_REPORT_TYPE_ALLEGED_NO_WAS 13

// DataType TradeUnderlying
#define ENUM_TRADE_UNDERLYING_NO 1
#define ENUM_TRADE_UNDERLYING_YES 2

// DataType TradingCapacity
#define ENUM_TRADING_CAPACITY_CUSTOMER 1
#define ENUM_TRADING_CAPACITY_PRINCIPAL 5
#define ENUM_TRADING_CAPACITY_MARKET_MAKER 6

// DataType TradingSessionSubID
#define ENUM_TRADING_SESSION_SUBID_CLOSING_AUCTION 4

// DataType TransferReason
#define ENUM_TRANSFER_REASON_OWNER 1
#define ENUM_TRANSFER_REASON_CLEARER 2

// DataType TrdRptStatus
#define ENUM_TRD_RPT_STATUS_ACCEPTED 0
#define ENUM_TRD_RPT_STATUS_REJECTED 1
#define ENUM_TRD_RPT_STATUS_CANCELLED 2
#define ENUM_TRD_RPT_STATUS_PENDING_NEW 4
#define ENUM_TRD_RPT_STATUS_TERMINATED 7

// DataType TrdType
#define ENUM_TRD_TYPE_BLOCK_TRADE 1
#define ENUM_TRD_TYPE_EXCHANGE_FOR_SWAP 12
#define ENUM_TRD_TYPE_EXCHANGE_BASIS_FACILITY 55
#define ENUM_TRD_TYPE_VOLA_TRADE 1000
#define ENUM_TRD_TYPE_EFP_FIN_TRADE 1001
#define ENUM_TRD_TYPE_EFP_INDEX_FUTURES_TRADE 1002
#define ENUM_TRD_TYPE_TRADE_AT_MARKET 1004

// DataType Triggered
#define ENUM_TRIGGERED_NOT_TRIGGERED 0
#define ENUM_TRIGGERED_TRIGGERED_STOP 1
#define ENUM_TRIGGERED_TRIGGERED_OCO 2

// DataType UnderlyingCurrency
#define LEN_UNDERLYING_CURRENCY 3

// DataType UnderlyingIssuer
#define LEN_UNDERLYING_ISSUER 30

// DataType UnderlyingSecurityDesc
#define LEN_UNDERLYING_SECURITY_DESC 30

// DataType UnderlyingSecurityID
#define LEN_UNDERLYING_SECURITYID 12

// DataType UnderlyingStipType
#define LEN_UNDERLYING_STIP_TYPE 7
#define ENUM_UNDERLYING_STIP_TYPE_PAY_FREQUENCY "PAYFREQ"

// DataType UnderlyingStipValue
#define LEN_UNDERLYING_STIP_VALUE 32

// DataType UserStatus
#define ENUM_USER_STATUS_USER_FORCED_LOGOUT 7
#define ENUM_USER_STATUS_USER_STOPPED 10
#define ENUM_USER_STATUS_USER_RELEASED 11

// DataType ValueCheckTypeValue
#define ENUM_VALUE_CHECK_TYPE_VALUE_DO_NOT_CHECK 0
#define ENUM_VALUE_CHECK_TYPE_VALUE_CHECK 1

// DataType VarText
#define LEN_VAR_TEXT 2000

/*
 * Structure defines for components and sequences
 */

// Structure: EnrichmentRulesGrp
typedef struct {
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
typedef struct {
  int64_t FillPx;
  int32_t FillQty;
  uint32_t FillMatchID;
  int32_t FillExecID;
  uint8_t FillLiquidityInd;
  char Pad3[LEN_PAD3];
} FillsGrpSeqT;

// Structure: InstrmntLegExecGrp
typedef struct {
  int64_t LegSecurityID;
  int64_t LegLastPx;
  int32_t LegLastQty;
  int32_t LegExecID;
  uint8_t LegSide;
  uint8_t FillRefID;
  char Pad6[LEN_PAD6];
} InstrmntLegExecGrpSeqT;

// Structure: InstrmtLegGrp
typedef struct {
  int64_t LegSecurityID;
  int64_t LegPrice;
  int32_t LegSymbol;
  uint32_t LegRatioQty;
  uint8_t LegSide;
  uint8_t LegSecurityType;
  char Pad6[LEN_PAD6];
} InstrmtLegGrpSeqT;

// Structure: InstrumentAttributeGrp
typedef struct {
  uint8_t InstrAttribType;
  char InstrAttribValue[LEN_INSTR_ATTRIB_VALUE];
  char Pad7[LEN_PAD7];
} InstrumentAttributeGrpSeqT;

// Structure: InstrumentEventGrp
typedef struct {
  uint32_t EventDate;
  uint8_t EventType;
  char Pad3[LEN_PAD3];
} InstrumentEventGrpSeqT;

// Structure: LegOrdGrp
typedef struct {
  char LegAccount[LEN_LEG_ACCOUNT];
  char LegPositionEffect[LEN_LEG_POSITION_EFFECT];
  char Pad5[LEN_PAD5];
} LegOrdGrpSeqT;

// Structure: MMParameterGrp
typedef struct {
  int64_t ExposureDuration;
  int32_t CumQty;
  int32_t PctCount;
  int32_t Delta;
  int32_t Vega;
  uint8_t ProductComplex;
  char Pad7[LEN_PAD7];
} MMParameterGrpSeqT;

// Structure: MessageHeaderIn
typedef struct {
  uint32_t BodyLen;
  uint16_t TemplateID;
  char NetworkMsgID[LEN_NETWORK_MSGID];
  char Pad2[LEN_PAD2];
} MessageHeaderInCompT;

// Structure: MessageHeaderOut
typedef struct {
  uint32_t BodyLen;
  uint16_t TemplateID;
  char Pad2[LEN_PAD2];
} MessageHeaderOutCompT;

// Structure: NRBCHeader
typedef struct {
  uint64_t SendingTime;
  uint32_t ApplSubID;
  uint8_t ApplID;
  uint8_t LastFragment;
  char Pad2[LEN_PAD2];
} NRBCHeaderCompT;

// Structure: NRResponseHeaderME
typedef struct {
  uint64_t RequestTime;
  uint64_t RequestOut;
  uint64_t TrdRegTSTimeIn;
  uint64_t TrdRegTSTimeOut;
  uint64_t ResponseIn;
  uint64_t SendingTime;
  uint32_t MsgSeqNum;
  uint8_t LastFragment;
  char Pad3[LEN_PAD3];
} NRResponseHeaderMECompT;

// Structure: NotAffectedOrdersGrp
typedef struct {
  uint64_t NotAffectedOrderID;
  uint64_t NotAffOrigClOrdID;
} NotAffectedOrdersGrpSeqT;

// Structure: NotAffectedSecuritiesGrp
typedef struct { uint64_t NotAffectedSecurityID; } NotAffectedSecuritiesGrpSeqT;

// Structure: NotifHeader
typedef struct { uint64_t SendingTime; } NotifHeaderCompT;

// Structure: PartyDetailsGrp
typedef struct {
  uint32_t PartyDetailIDExecutingTrader;
  char PartyDetailExecutingTrader[LEN_PARTY_DETAIL_EXECUTING_TRADER];
  uint8_t PartyDetailRoleQualifier;
  uint8_t PartyDetailStatus;
  char PartyDetailDeskID[LEN_PARTY_DETAIL_DESKID];
  char Pad1[LEN_PAD1];
} PartyDetailsGrpSeqT;

// Structure: QuotReqLegsGrp
typedef struct {
  int64_t LegSecurityID;
  uint32_t LegRatioQty;
  int32_t LegSymbol;
  uint8_t LegSecurityType;
  uint8_t LegSide;
  char Pad6[LEN_PAD6];
} QuotReqLegsGrpSeqT;

// Structure: QuoteEntryAckGrp
typedef struct {
  int64_t SecurityID;
  int32_t BidCxlSize;
  int32_t OfferCxlSize;
  uint32_t QuoteEntryRejectReason;
  uint8_t QuoteEntryStatus;
  char Pad3[LEN_PAD3];
} QuoteEntryAckGrpSeqT;

// Structure: QuoteEntryGrp
typedef struct {
  int64_t SecurityID;
  int64_t BidPx;
  int64_t OfferPx;
  int32_t BidSize;
  int32_t OfferSize;
} QuoteEntryGrpSeqT;

// Structure: QuoteEventGrp
typedef struct {
  int64_t SecurityID;
  int64_t QuoteEventPx;
  uint64_t QuoteMsgID;
  uint32_t QuoteEventMatchID;
  int32_t QuoteEventExecID;
  int32_t QuoteEventQty;
  uint8_t QuoteEventType;
  uint8_t QuoteEventSide;
  uint8_t QuoteEventLiquidityInd;
  uint8_t QuoteEventReason;
} QuoteEventGrpSeqT;

// Structure: QuoteLegExecGrp
typedef struct {
  int64_t LegSecurityID;
  int64_t LegLastPx;
  int32_t LegLastQty;
  int32_t LegExecID;
  uint8_t LegSide;
  uint8_t NoQuoteEventsIndex;
  char Pad6[LEN_PAD6];
} QuoteLegExecGrpSeqT;

// Structure: RBCHeader
typedef struct {
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
typedef struct {
  uint64_t TrdRegTSTimeOut;
  uint64_t NotificationIn;
  uint64_t SendingTime;
  uint32_t ApplSubID;
  uint16_t PartitionID;
  char ApplMsgID[LEN_APPL_MSGID];
  uint8_t ApplID;
  uint8_t ApplResendFlag;
  uint8_t LastFragment;
  char Pad7[LEN_PAD7];
} RBCHeaderMECompT;

// Structure: RequestHeader
typedef struct {
  uint32_t MsgSeqNum;
  uint32_t SenderSubID;
} RequestHeaderCompT;

// Structure: ResponseHeader
typedef struct {
  uint64_t RequestTime;
  uint64_t SendingTime;
  uint32_t MsgSeqNum;
  char Pad4[LEN_PAD4];
} ResponseHeaderCompT;

// Structure: ResponseHeaderME
typedef struct {
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

// Structure: SRQSTrdInstrmntLegGrp
typedef struct {
  int64_t LegSecurityID;
  int64_t LegBestBidPx;
  int64_t LegBestOfferPx;
  int32_t LegBestBidSize;
  int32_t LegBestOfferSize;
} SRQSTrdInstrmntLegGrpSeqT;

// Structure: SessionsGrp
typedef struct {
  uint32_t PartyIDSessionID;
  uint8_t SessionMode;
  uint8_t SessionSubMode;
  char Pad2[LEN_PAD2];
} SessionsGrpSeqT;

// Structure: SideAllocExtGrp
typedef struct {
  uint32_t IndividualAllocID;
  int32_t AllocQty;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  uint8_t Side;
  uint8_t TradeAllocStatus;
  uint8_t TradingCapacity;
  char PositionEffect[LEN_POSITION_EFFECT];
  uint8_t OrderAttributeLiquidityProvision;
  char Account[LEN_ACCOUNT];
  char PartyIDPositionAccount[LEN_PARTY_ID_POSITION_ACCOUNT];
  char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
  char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
  char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char Pad6[LEN_PAD6];
} SideAllocExtGrpSeqT;

// Structure: SideAllocGrp
typedef struct {
  uint32_t IndividualAllocID;
  int32_t AllocQty;
  uint8_t Side;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char Pad4[LEN_PAD4];
} SideAllocGrpSeqT;

// Structure: SideAllocGrpBC
typedef struct {
  uint32_t IndividualAllocID;
  int32_t AllocQty;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  uint8_t Side;
  uint8_t TradeAllocStatus;
  char Pad3[LEN_PAD3];
} SideAllocGrpBCSeqT;

// Structure: TargetParties
typedef struct {
  uint8_t SideDisclosureInstruction;
  uint8_t PriceDisclosureInstruction;
  uint8_t LeavesQtyDisclosureInstruction;
  uint8_t QuoteInstruction;
  char TargetPartyExecutingFirm[LEN_TARGET_PARTY_EXECUTING_FIRM];
  char TargetPartyExecutingTrader[LEN_TARGET_PARTY_EXECUTING_TRADER];
  char Pad1[LEN_PAD1];
} TargetPartiesSeqT;

// Structure: TrdInstrmntLegGrp
typedef struct {
  int64_t LegSecurityID;
  int64_t LegPrice;
} TrdInstrmntLegGrpSeqT;

// Structure: UnderlyingStipGrp
typedef struct {
  char UnderlyingStipValue[LEN_UNDERLYING_STIP_VALUE];
  char UnderlyingStipType[LEN_UNDERLYING_STIP_TYPE];
  char Pad1[LEN_PAD1];
} UnderlyingStipGrpSeqT;

/*
 * Structure defines for messages
 */

// Message:	    AddComplexInstrumentRequest
// TemplateID:  10301
// Alias:       Create Strategy
// FIX MsgType: SecurityDefinitionRequest = "c"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int32_t MarketSegmentID;
  int32_t SecuritySubType;
  uint8_t ProductComplex;
  uint8_t NoLegs;
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char Pad2[LEN_PAD2];
  InstrmtLegGrpSeqT InstrmtLegGrp[MAX_ADD_COMPLEX_INSTRUMENT_REQUEST_INSTRMT_LEG_GRP];
} AddComplexInstrumentRequestT;

// Message:	    AddComplexInstrumentResponse
// TemplateID:  10302
// Alias:       Create Strategy Response
// FIX MsgType: SecurityDefinition = "d"
typedef struct {
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

// Message:	    AddFlexibleInstrumentRequest
// TemplateID:  10309
// Alias:       Create Flexible Instrument Request
// FIX MsgType: SecurityDefinitionRequest = "c"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t StrikePrice;
  int32_t MarketSegmentID;
  uint32_t MaturityDate;
  char SettlMethod[LEN_SETTL_METHOD];
  uint8_t OptAttribute;
  uint8_t PutOrCall;
  uint8_t ExerciseStyle;
  char ComplianceText[LEN_COMPLIANCE_TEXT];
} AddFlexibleInstrumentRequestT;

// Message:	    AddFlexibleInstrumentResponse
// TemplateID:  10310
// Alias:       Create Flexible Instrument Response
// FIX MsgType: SecurityDefinition = "d"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t SecurityResponseID;
  int64_t SecurityID;
  int64_t StrikePrice;
  int32_t MarketSegmentID;
  uint32_t MaturityDate;
  uint8_t ProductComplex;
  char SettlMethod[LEN_SETTL_METHOD];
  uint8_t OptAttribute;
  uint8_t PutOrCall;
  uint8_t ExerciseStyle;
  char Symbol[LEN_SYMBOL];
  char Pad7[LEN_PAD7];
} AddFlexibleInstrumentResponseT;

// Message:	    ApproveTESTradeRequest
// TemplateID:  10603
// Alias:       Approve TES Trade Request
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t PartyIDClientID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  uint32_t PackageID;
  uint32_t AllocID;
  int32_t AllocQty;
  uint32_t TESExecID;
  int32_t MarketSegmentID;
  int32_t RelatedMarketSegmentID;
  uint16_t TrdType;
  uint8_t TradingCapacity;
  uint8_t TradeReportType;
  uint8_t Side;
  uint8_t OrderAttributeLiquidityProvision;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char TradeReportID[LEN_TRADE_REPORTID];
  char PositionEffect[LEN_POSITION_EFFECT];
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char Account[LEN_ACCOUNT];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char PartyIDPositionAccount[LEN_PARTY_ID_POSITION_ACCOUNT];
  char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
  char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
  char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char Pad6[LEN_PAD6];
} ApproveTESTradeRequestT;

// Message:	    BroadcastErrorNotification
// TemplateID:  10032
// Alias:       Gap Fill
// FIX MsgType: ApplicationMessageReport = "BY"
typedef struct {
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

// Message:	    CrossRequest
// TemplateID:  10118
// Alias:       Cross Request
// FIX MsgType: CrossRequest = "U16"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t SecurityID;
  int32_t MarketSegmentID;
  int32_t OrderQty;
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char Pad4[LEN_PAD4];
} CrossRequestT;

// Message:	    CrossRequestResponse
// TemplateID:  10119
// Alias:       Cross Request Response
// FIX MsgType: RequestAcknowledge = "U1"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t ExecID;
} CrossRequestResponseT;

// Message:	    DeleteAllOrderBroadcast
// TemplateID:  10122
// Alias:       Order Mass Cancellation Notification
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderMECompT RBCHeaderME;
  uint64_t MassActionReportID;
  int64_t SecurityID;
  int64_t Price;
  int32_t MarketSegmentID;
  uint32_t TargetPartyIDSessionID;
  uint32_t TargetPartyIDExecutingTrader;
  uint32_t PartyIDEnteringTrader;
  uint16_t NoNotAffectedOrders;
  uint8_t PartyIDEnteringFirm;
  uint8_t MassActionReason;
  uint8_t ExecInst;
  uint8_t Side;
  char Pad2[LEN_PAD2];
  NotAffectedOrdersGrpSeqT NotAffectedOrdersGrp[MAX_DELETE_ALL_ORDER_BROADCAST_NOT_AFFECTED_ORDERS_GRP];
} DeleteAllOrderBroadcastT;

// Message:	    DeleteAllOrderNRResponse
// TemplateID:  10124
// Alias:       Order Mass Cancellation Response No Hits
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t MassActionReportID;
} DeleteAllOrderNRResponseT;

// Message:	    DeleteAllOrderQuoteEventBroadcast
// TemplateID:  10308
// Alias:       Mass Cancellation Event
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderMECompT RBCHeaderME;
  uint64_t MassActionReportID;
  int64_t SecurityID;
  int32_t MarketSegmentID;
  uint8_t MassActionReason;
  uint8_t ExecInst;
  char Pad2[LEN_PAD2];
} DeleteAllOrderQuoteEventBroadcastT;

// Message:	    DeleteAllOrderRequest
// TemplateID:  10120
// Alias:       Order Mass Cancellation Request
// FIX MsgType: OrderMassActionRequest = "CA"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t SecurityID;
  int64_t Price;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t MarketSegmentID;
  uint32_t TargetPartyIDSessionID;
  uint32_t TargetPartyIDExecutingTrader;
  uint8_t Side;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char Pad1[LEN_PAD1];
} DeleteAllOrderRequestT;

// Message:	    DeleteAllOrderResponse
// TemplateID:  10121
// Alias:       Order Mass Cancellation Response
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderMECompT ResponseHeaderME;
  uint64_t MassActionReportID;
  uint16_t NoNotAffectedOrders;
  char Pad6[LEN_PAD6];
  NotAffectedOrdersGrpSeqT NotAffectedOrdersGrp[MAX_DELETE_ALL_ORDER_RESPONSE_NOT_AFFECTED_ORDERS_GRP];
} DeleteAllOrderResponseT;

// Message:	    DeleteAllQuoteBroadcast
// TemplateID:  10410
// Alias:       Quote Mass Cancellation Notification
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct {
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

// Message:	    DeleteAllQuoteRequest
// TemplateID:  10408
// Alias:       Quote Mass Cancellation Request
// FIX MsgType: OrderMassActionRequest = "CA"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t MarketSegmentID;
  uint32_t TargetPartyIDSessionID;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char Pad6[LEN_PAD6];
} DeleteAllQuoteRequestT;

// Message:	    DeleteAllQuoteResponse
// TemplateID:  10409
// Alias:       Quote Mass Cancellation Response
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t MassActionReportID;
  uint16_t NoNotAffectedSecurities;
  char Pad6[LEN_PAD6];
  NotAffectedSecuritiesGrpSeqT NotAffectedSecuritiesGrp[MAX_DELETE_ALL_QUOTE_RESPONSE_NOT_AFFECTED_SECURITIES_GRP];
} DeleteAllQuoteResponseT;

// Message:	    DeleteOrderBroadcast
// TemplateID:  10112
// Alias:       Cancel Order Notification
// FIX MsgType: ExecutionReport = "8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderMECompT RBCHeaderME;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t SecurityID;
  uint64_t ExecID;
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
  char FIXClOrdID[LEN_FIX_CL_ORDID];
  char Pad5[LEN_PAD5];
} DeleteOrderBroadcastT;

// Message:	    DeleteOrderComplexRequest
// TemplateID:  10123
// Alias:       Cancel Order Multi Leg
// FIX MsgType: OrderCancelRequest = "F"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t SecurityID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t MarketSegmentID;
  uint32_t TargetPartyIDSessionID;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char FIXClOrdID[LEN_FIX_CL_ORDID];
  char Pad2[LEN_PAD2];
} DeleteOrderComplexRequestT;

// Message:	    DeleteOrderNRResponse
// TemplateID:  10111
// Alias:       Cancel Order Response (Lean Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
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
} DeleteOrderNRResponseT;

// Message:	    DeleteOrderResponse
// TemplateID:  10110
// Alias:       Cancel Order Response (Standard Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct {
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

// Message:	    DeleteOrderSingleRequest
// TemplateID:  10109
// Alias:       Cancel Order Single
// FIX MsgType: OrderCancelRequest = "F"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t MarketSegmentID;
  uint32_t SimpleSecurityID;
  uint32_t TargetPartyIDSessionID;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char FIXClOrdID[LEN_FIX_CL_ORDID];
  char Pad6[LEN_PAD6];
} DeleteOrderSingleRequestT;

// Message:	    DeleteTESTradeRequest
// TemplateID:  10602
// Alias:       Delete TES Trade Request
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint32_t PackageID;
  int32_t MarketSegmentID;
  uint32_t TESExecID;
  int32_t RelatedMarketSegmentID;
  uint16_t TrdType;
  uint8_t TradeReportType;
  char TradeReportID[LEN_TRADE_REPORTID];
  char Pad1[LEN_PAD1];
} DeleteTESTradeRequestT;

// Message:	    EnterTESTradeRequest
// TemplateID:  10600
// Alias:       Enter TES Trade Request
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t SecurityID;
  int64_t LastPx;
  uint64_t TransBkdTime;
  int64_t UnderlyingPx;
  int64_t RelatedClosePrice;
  int32_t MarketSegmentID;
  uint32_t UnderlyingSettlementDate;
  uint32_t UnderlyingMaturityDate;
  uint32_t RelatedTradeID;
  int32_t RelatedMarketSegmentID;
  int32_t RelatedTradeQuantity;
  int64_t UnderlyingQty;
  uint16_t TrdType;
  uint8_t ProductComplex;
  uint8_t TradeReportType;
  uint8_t TradePublishIndicator;
  uint8_t NoSideAllocs;
  uint8_t NoEvents;
  uint8_t NoLegs;
  uint8_t NoInstrAttrib;
  uint8_t NoUnderlyingStips;
  uint8_t PartyIDSettlementLocation;
  uint8_t HedgeType;
  char TradeReportText[LEN_TRADE_REPORT_TEXT];
  char TradeReportID[LEN_TRADE_REPORTID];
  char UnderlyingSecurityID[LEN_UNDERLYING_SECURITYID];
  char UnderlyingSecurityDesc[LEN_UNDERLYING_SECURITY_DESC];
  char UnderlyingCurrency[LEN_UNDERLYING_CURRENCY];
  char UnderlyingIssuer[LEN_UNDERLYING_ISSUER];
  char Pad1[LEN_PAD1];
  SideAllocGrpSeqT SideAllocGrp[MAX_ENTER_TES_TRADE_REQUEST_SIDE_ALLOC_GRP];
  TrdInstrmntLegGrpSeqT TrdInstrmntLegGrp[MAX_ENTER_TES_TRADE_REQUEST_TRD_INSTRMNT_LEG_GRP];
  InstrumentEventGrpSeqT InstrumentEventGrp[MAX_ENTER_TES_TRADE_REQUEST_INSTRUMENT_EVENT_GRP];
  InstrumentAttributeGrpSeqT InstrumentAttributeGrp[MAX_ENTER_TES_TRADE_REQUEST_INSTRUMENT_ATTRIBUTE_GRP];
  UnderlyingStipGrpSeqT UnderlyingStipGrp[MAX_ENTER_TES_TRADE_REQUEST_UNDERLYING_STIP_GRP];
} EnterTESTradeRequestT;

// Message:	    ForcedLogoutNotification
// TemplateID:  10012
// Alias:       Session Logout Notification
// FIX MsgType: Logout = "5"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NotifHeaderCompT NotifHeader;
  uint16_t VarTextLen;
  char Pad6[LEN_PAD6];
  char VarText[LEN_VAR_TEXT];
} ForcedLogoutNotificationT;

// Message:	    ForcedUserLogoutNotification
// TemplateID:  10043
// Alias:       User Logout Notification
// FIX MsgType: Logout = "5"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NotifHeaderCompT NotifHeader;
  uint8_t UserStatus;
  char Pad3[LEN_PAD3];
  uint32_t Username;
  uint16_t VarTextLen;
  char Pad6[LEN_PAD6];
  char VarText[LEN_VAR_TEXT];
} ForcedUserLogoutNotificationT;

// Message:	    GatewayRequest
// TemplateID:  10020
// Alias:       Connection Gateway Request
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint32_t PartyIDSessionID;
  uint16_t PartitionID;
  char DefaultCstmApplVerID[LEN_DEFAULT_CSTM_APPL_VERID];
  char Password[LEN_PASSWORD];
  char Pad4[LEN_PAD4];
} GatewayRequestT;

// Message:	    GatewayResponse
// TemplateID:  10021
// Alias:       Connection Gateway Response
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  uint32_t GatewayID;
  uint32_t GatewaySubID;
  uint32_t SecondaryGatewayID;
  uint32_t SecondaryGatewaySubID;
  uint8_t GatewayStatus;
  uint8_t SecondaryGatewayStatus;
  uint8_t SessionMode;
  uint8_t TradSesMode;
  char Pad4[LEN_PAD4];
} GatewayResponseT;

// Message:	    Heartbeat
// TemplateID:  10011
// Alias:       Heartbeat
// FIX MsgType: Heartbeat = "0"
typedef struct { MessageHeaderInCompT MessageHeaderIn; } HeartbeatT;

// Message:	    HeartbeatNotification
// TemplateID:  10023
// Alias:       Heartbeat Notification
// FIX MsgType: Heartbeat = "0"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NotifHeaderCompT NotifHeader;
} HeartbeatNotificationT;

// Message:	    InquireEnrichmentRuleIDListRequest
// TemplateID:  10040
// Alias:       Trade Enrichment List Inquire
// FIX MsgType: TradeEnrichmentListRequest = "U7"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  char LastEntityProcessed[LEN_LAST_ENTITY_PROCESSED];
} InquireEnrichmentRuleIDListRequestT;

// Message:	    InquireEnrichmentRuleIDListResponse
// TemplateID:  10041
// Alias:       Trade Enrichment List Inquire Response
// FIX MsgType: TradeEnrichmentListReport = "U9"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  char LastEntityProcessed[LEN_LAST_ENTITY_PROCESSED];
  uint16_t NoEnrichmentRules;
  char Pad6[LEN_PAD6];
  EnrichmentRulesGrpSeqT EnrichmentRulesGrp[MAX_INQUIRE_ENRICHMENT_RULE_ID_LIST_RESPONSE_ENRICHMENT_RULES_GRP];
} InquireEnrichmentRuleIDListResponseT;

// Message:	    InquireMMParameterRequest
// TemplateID:  10305
// Alias:       Inquire Market Maker Parameters
// FIX MsgType: MMParameterRequest = "U17"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int32_t MarketSegmentID;
  uint32_t TargetPartyIDSessionID;
} InquireMMParameterRequestT;

// Message:	    InquireMMParameterResponse
// TemplateID:  10306
// Alias:       Inquire Market Maker Parameters Response
// FIX MsgType: MMParameterResponse = "U18"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t MMParameterReportID;
  int32_t MarketSegmentID;
  uint8_t NoMMParameters;
  char Pad3[LEN_PAD3];
  MMParameterGrpSeqT MMParameterGrp[MAX_INQUIRE_MM_PARAMETER_RESPONSE_MM_PARAMETER_GRP];
} InquireMMParameterResponseT;

// Message:	    InquireSessionListRequest
// TemplateID:  10035
// Alias:       Session List Inquire
// FIX MsgType: SessionDetailsListRequest = "U5"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
} InquireSessionListRequestT;

// Message:	    InquireSessionListResponse
// TemplateID:  10036
// Alias:       Session List Inquire Response
// FIX MsgType: SessionDetailsListResponse = "U6"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  uint16_t NoSessions;
  char Pad6[LEN_PAD6];
  SessionsGrpSeqT SessionsGrp[MAX_INQUIRE_SESSION_LIST_RESPONSE_SESSIONS_GRP];
} InquireSessionListResponseT;

// Message:	    InquireUserRequest
// TemplateID:  10038
// Alias:       User List Inquire
// FIX MsgType: PartyDetailListRequest = "CF"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  char LastEntityProcessed[LEN_LAST_ENTITY_PROCESSED];
} InquireUserRequestT;

// Message:	    InquireUserResponse
// TemplateID:  10039
// Alias:       User List Inquire Response
// FIX MsgType: PartyDetailListReport = "CG"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  char LastEntityProcessed[LEN_LAST_ENTITY_PROCESSED];
  uint16_t NoPartyDetails;
  char Pad6[LEN_PAD6];
  PartyDetailsGrpSeqT PartyDetailsGrp[MAX_INQUIRE_USER_RESPONSE_PARTY_DETAILS_GRP];
} InquireUserResponseT;

// Message:	    LegalNotificationBroadcast
// TemplateID:  10037
// Alias:       Legal Notification
// FIX MsgType: UserNotification = "CB"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  uint16_t VarTextLen;
  uint8_t UserStatus;
  char Pad5[LEN_PAD5];
  char VarText[LEN_VAR_TEXT];
} LegalNotificationBroadcastT;

// Message:	    LogonRequest
// TemplateID:  10000
// Alias:       Session Logon
// FIX MsgType: Logon = "A"
typedef struct {
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
} LogonRequestT;

// Message:	    LogonResponse
// TemplateID:  10001
// Alias:       Session Logon Response
// FIX MsgType: Logon = "A"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  int64_t ThrottleTimeInterval;
  uint32_t ThrottleNoMsgs;
  uint32_t ThrottleDisconnectLimit;
  uint32_t HeartBtInt;
  uint32_t SessionInstanceID;
  uint16_t MarketID;
  uint8_t TradSesMode;
  char DefaultCstmApplVerID[LEN_DEFAULT_CSTM_APPL_VERID];
  char DefaultCstmApplVerSubID[LEN_DEFAULT_CSTM_APPL_VER_SUBID];
  char Pad2[LEN_PAD2];
} LogonResponseT;

// Message:	    LogoutRequest
// TemplateID:  10002
// Alias:       Session Logout
// FIX MsgType: Logout = "5"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
} LogoutRequestT;

// Message:	    LogoutResponse
// TemplateID:  10003
// Alias:       Session Logout Response
// FIX MsgType: Logout = "5"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
} LogoutResponseT;

// Message:	    MMParameterDefinitionRequest
// TemplateID:  10303
// Alias:       Market Maker Parameter Definition
// FIX MsgType: MMParameterDefinitionRequest = "U14"
typedef struct {
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

// Message:	    MMParameterDefinitionResponse
// TemplateID:  10304
// Alias:       Market Maker Parameter Definition Response
// FIX MsgType: RequestAcknowledge = "U1"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t ExecID;
} MMParameterDefinitionResponseT;

// Message:	    MassQuoteRequest
// TemplateID:  10405
// Alias:       Mass Quote
// FIX MsgType: MassQuote = "i"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t QuoteID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t MarketSegmentID;
  uint32_t MatchInstCrossID;
  uint16_t EnrichmentRuleID;
  uint8_t PriceValidityCheckType;
  uint8_t ValueCheckTypeValue;
  uint8_t QuoteSizeType;
  uint8_t OrderAttributeLiquidityProvision;
  uint8_t NoQuoteEntries;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char Pad7[LEN_PAD7];
  QuoteEntryGrpSeqT QuoteEntryGrp[MAX_MASS_QUOTE_REQUEST_QUOTE_ENTRY_GRP];
} MassQuoteRequestT;

// Message:	    MassQuoteResponse
// TemplateID:  10406
// Alias:       Mass Quote Response
// FIX MsgType: MassQuoteAcknowledgement = "b"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t QuoteID;
  uint64_t QuoteResponseID;
  int32_t MarketSegmentID;
  uint8_t NoQuoteEntries;
  char Pad3[LEN_PAD3];
  QuoteEntryAckGrpSeqT QuoteEntryAckGrp[MAX_MASS_QUOTE_RESPONSE_QUOTE_ENTRY_ACK_GRP];
} MassQuoteResponseT;

// Message:	    ModifyOrderComplexRequest
// TemplateID:  10114
// Alias:       Replace Order Multi Leg
// FIX MsgType: MultilegOrderCancelReplace = "AC"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t SecurityID;
  int64_t Price;
  uint64_t PartyIDClientID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t MarketSegmentID;
  int32_t OrderQty;
  uint32_t ExpireDate;
  uint32_t MatchInstCrossID;
  uint32_t TargetPartyIDSessionID;
  char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
  char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
  uint8_t ApplSeqIndicator;
  uint8_t ProductComplex;
  uint8_t Side;
  uint8_t OrdType;
  uint8_t PriceValidityCheckType;
  uint8_t ValueCheckTypeValue;
  uint8_t OrderAttributeLiquidityProvision;
  uint8_t ExecInst;
  uint8_t TimeInForce;
  uint8_t TradingCapacity;
  uint8_t OwnershipIndicator;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char PartyIDPositionAccount[LEN_PARTY_ID_POSITION_ACCOUNT];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char FIXClOrdID[LEN_FIX_CL_ORDID];
  uint8_t NoLegs;
  char Pad2[LEN_PAD2];
  LegOrdGrpSeqT LegOrdGrp[MAX_MODIFY_ORDER_COMPLEX_REQUEST_LEG_ORD_GRP];
} ModifyOrderComplexRequestT;

// Message:	    ModifyOrderNRResponse
// TemplateID:  10108
// Alias:       Replace Order Response (Lean Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t SecurityID;
  uint64_t ExecID;
  int32_t LeavesQty;
  int32_t CumQty;
  int32_t CxlQty;
  char OrdStatus[LEN_ORD_STATUS];
  char ExecType[LEN_EXEC_TYPE];
  uint16_t ExecRestatementReason;
  uint8_t CrossedIndicator;
  uint8_t ProductComplex;
  uint8_t Triggered;
  char Pad5[LEN_PAD5];
} ModifyOrderNRResponseT;

// Message:	    ModifyOrderResponse
// TemplateID:  10107
// Alias:       Replace Order Response (Standard Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderMECompT ResponseHeaderME;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t SecurityID;
  uint64_t ExecID;
  uint64_t TrdRegTSTimePriority;
  int32_t LeavesQty;
  int32_t CumQty;
  int32_t CxlQty;
  char OrdStatus[LEN_ORD_STATUS];
  char ExecType[LEN_EXEC_TYPE];
  uint16_t ExecRestatementReason;
  uint8_t CrossedIndicator;
  uint8_t ProductComplex;
  uint8_t Triggered;
  char Pad5[LEN_PAD5];
} ModifyOrderResponseT;

// Message:	    ModifyOrderSingleRequest
// TemplateID:  10106
// Alias:       Replace Order Single
// FIX MsgType: OrderCancelReplaceRequest = "G"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t Price;
  int64_t StopPx;
  uint64_t PartyIDClientID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t OrderQty;
  uint32_t ExpireDate;
  int32_t MarketSegmentID;
  uint32_t SimpleSecurityID;
  uint32_t MatchInstCrossID;
  uint32_t TargetPartyIDSessionID;
  char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
  char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
  uint8_t ApplSeqIndicator;
  uint8_t Side;
  uint8_t OrdType;
  uint8_t PriceValidityCheckType;
  uint8_t ValueCheckTypeValue;
  uint8_t OrderAttributeLiquidityProvision;
  uint8_t TimeInForce;
  uint8_t ExecInst;
  uint8_t TradingSessionSubID;
  uint8_t TradingCapacity;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char Account[LEN_ACCOUNT];
  char PartyIDPositionAccount[LEN_PARTY_ID_POSITION_ACCOUNT];
  char PositionEffect[LEN_POSITION_EFFECT];
  uint8_t OwnershipIndicator;
  char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char FIXClOrdID[LEN_FIX_CL_ORDID];
  char Pad4[LEN_PAD4];
} ModifyOrderSingleRequestT;

// Message:	    ModifyOrderSingleShortRequest
// TemplateID:  10126
// Alias:       Replace Order Single (short layout)
// FIX MsgType: OrderCancelReplaceRequest = "G"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t Price;
  uint64_t PartyIDClientID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t OrderQty;
  uint32_t SimpleSecurityID;
  uint32_t MatchInstCrossID;
  uint16_t EnrichmentRuleID;
  uint8_t Side;
  uint8_t PriceValidityCheckType;
  uint8_t ValueCheckTypeValue;
  uint8_t OrderAttributeLiquidityProvision;
  uint8_t TimeInForce;
  uint8_t ExecInst;
  uint8_t TradingCapacity;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char Pad1[LEN_PAD1];
} ModifyOrderSingleShortRequestT;

// Message:	    ModifyTESTradeRequest
// TemplateID:  10601
// Alias:       Modify TES Trade Request
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t LastPx;
  uint64_t TransBkdTime;
  int32_t MarketSegmentID;
  uint32_t PackageID;
  uint32_t TESExecID;
  int32_t RelatedMarketSegmentID;
  uint16_t TrdType;
  uint8_t TradeReportType;
  uint8_t TradePublishIndicator;
  uint8_t NoSideAllocs;
  uint8_t NoLegs;
  char TradeReportText[LEN_TRADE_REPORT_TEXT];
  char TradeReportID[LEN_TRADE_REPORTID];
  char Pad2[LEN_PAD2];
  SideAllocGrpSeqT SideAllocGrp[MAX_MODIFY_TES_TRADE_REQUEST_SIDE_ALLOC_GRP];
  TrdInstrmntLegGrpSeqT TrdInstrmntLegGrp[MAX_MODIFY_TES_TRADE_REQUEST_TRD_INSTRMNT_LEG_GRP];
} ModifyTESTradeRequestT;

// Message:	    NewOrderComplexRequest
// TemplateID:  10113
// Alias:       New Order Multi Leg
// FIX MsgType: NewOrderMultileg = "AB"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t ClOrdID;
  int64_t SecurityID;
  int64_t Price;
  uint64_t PartyIDClientID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t MarketSegmentID;
  int32_t OrderQty;
  uint32_t ExpireDate;
  uint32_t MatchInstCrossID;
  char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
  char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
  uint8_t ApplSeqIndicator;
  uint8_t ProductComplex;
  uint8_t Side;
  uint8_t OrdType;
  uint8_t PriceValidityCheckType;
  uint8_t ValueCheckTypeValue;
  uint8_t OrderAttributeLiquidityProvision;
  uint8_t OrderAttributeRiskReduction;
  uint8_t ExecInst;
  uint8_t TimeInForce;
  uint8_t TradingCapacity;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char PartyIDPositionAccount[LEN_PARTY_ID_POSITION_ACCOUNT];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char FIXClOrdID[LEN_FIX_CL_ORDID];
  uint8_t NoLegs;
  char Pad6[LEN_PAD6];
  LegOrdGrpSeqT LegOrdGrp[MAX_NEW_ORDER_COMPLEX_REQUEST_LEG_ORD_GRP];
} NewOrderComplexRequestT;

// Message:	    NewOrderNRResponse
// TemplateID:  10102
// Alias:       New Order Response (Lean Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t OrderID;
  uint64_t ClOrdID;
  int64_t SecurityID;
  uint64_t ExecID;
  char OrdStatus[LEN_ORD_STATUS];
  char ExecType[LEN_EXEC_TYPE];
  uint16_t ExecRestatementReason;
  uint8_t CrossedIndicator;
  uint8_t ProductComplex;
  uint8_t Triggered;
  char Pad1[LEN_PAD1];
} NewOrderNRResponseT;

// Message:	    NewOrderResponse
// TemplateID:  10101
// Alias:       New Order Response (Standard Order)
// FIX MsgType: ExecutionReport = "8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderMECompT ResponseHeaderME;
  uint64_t OrderID;
  uint64_t ClOrdID;
  int64_t SecurityID;
  uint64_t ExecID;
  uint64_t TrdRegTSEntryTime;
  uint64_t TrdRegTSTimePriority;
  char OrdStatus[LEN_ORD_STATUS];
  char ExecType[LEN_EXEC_TYPE];
  uint16_t ExecRestatementReason;
  uint8_t CrossedIndicator;
  uint8_t ProductComplex;
  uint8_t Triggered;
  char Pad1[LEN_PAD1];
} NewOrderResponseT;

// Message:	    NewOrderSingleRequest
// TemplateID:  10100
// Alias:       New Order Single
// FIX MsgType: NewOrderSingle = "D"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t Price;
  int64_t StopPx;
  uint64_t ClOrdID;
  uint64_t PartyIDClientID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t OrderQty;
  uint32_t ExpireDate;
  int32_t MarketSegmentID;
  uint32_t SimpleSecurityID;
  uint32_t MatchInstCrossID;
  char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
  char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
  uint8_t ApplSeqIndicator;
  uint8_t Side;
  uint8_t OrdType;
  uint8_t PriceValidityCheckType;
  uint8_t ValueCheckTypeValue;
  uint8_t OrderAttributeLiquidityProvision;
  uint8_t OrderAttributeRiskReduction;
  uint8_t TimeInForce;
  uint8_t ExecInst;
  uint8_t TradingSessionSubID;
  uint8_t TradingCapacity;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char Account[LEN_ACCOUNT];
  char PartyIDPositionAccount[LEN_PARTY_ID_POSITION_ACCOUNT];
  char PositionEffect[LEN_POSITION_EFFECT];
  char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char FIXClOrdID[LEN_FIX_CL_ORDID];
} NewOrderSingleRequestT;

// Message:	    NewOrderSingleShortRequest
// TemplateID:  10125
// Alias:       New Order Single (short layout)
// FIX MsgType: NewOrderSingle = "D"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t Price;
  uint64_t ClOrdID;
  uint64_t PartyIDClientID;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t OrderQty;
  uint32_t SimpleSecurityID;
  uint32_t MatchInstCrossID;
  uint16_t EnrichmentRuleID;
  uint8_t Side;
  uint8_t PriceValidityCheckType;
  uint8_t ValueCheckTypeValue;
  uint8_t OrderAttributeLiquidityProvision;
  uint8_t TimeInForce;
  uint8_t ExecInst;
  uint8_t TradingCapacity;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char Pad1[LEN_PAD1];
} NewOrderSingleShortRequestT;

// Message:	    NewsBroadcast
// TemplateID:  10031
// Alias:       News
// FIX MsgType: News = "B"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t OrigTime;
  uint16_t VarTextLen;
  char Headline[LEN_HEADLINE];
  char Pad6[LEN_PAD6];
  char VarText[LEN_VAR_TEXT];
} NewsBroadcastT;

// Message:	    OrderExecNotification
// TemplateID:  10104
// Alias:       Book Order Execution
// FIX MsgType: ExecutionReport = "8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderMECompT RBCHeaderME;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t SecurityID;
  uint64_t ExecID;
  int32_t MarketSegmentID;
  int32_t LeavesQty;
  int32_t CumQty;
  int32_t CxlQty;
  uint16_t NoLegExecs;
  uint16_t ExecRestatementReason;
  uint8_t Side;
  uint8_t ProductComplex;
  char OrdStatus[LEN_ORD_STATUS];
  char ExecType[LEN_EXEC_TYPE];
  uint8_t Triggered;
  uint8_t CrossedIndicator;
  char FIXClOrdID[LEN_FIX_CL_ORDID];
  uint8_t NoFills;
  char Pad1[LEN_PAD1];
  FillsGrpSeqT FillsGrp[MAX_ORDER_EXEC_NOTIFICATION_FILLS_GRP];
  InstrmntLegExecGrpSeqT InstrmntLegExecGrp[MAX_ORDER_EXEC_NOTIFICATION_INSTRMNT_LEG_EXEC_GRP];
} OrderExecNotificationT;

// Message:	    OrderExecReportBroadcast
// TemplateID:  10117
// Alias:       Extended Order Information
// FIX MsgType: ExecutionReport = "8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderMECompT RBCHeaderME;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t SecurityID;
  uint64_t ExecID;
  uint64_t TrdRegTSEntryTime;
  uint64_t TrdRegTSTimePriority;
  int64_t Price;
  int64_t StopPx;
  int32_t MarketSegmentID;
  int32_t LeavesQty;
  int32_t CumQty;
  int32_t CxlQty;
  int32_t OrderQty;
  uint32_t ExpireDate;
  uint32_t MatchInstCrossID;
  uint32_t PartyIDExecutingUnit;
  uint32_t PartyIDSessionID;
  uint32_t PartyIDExecutingTrader;
  uint32_t PartyIDEnteringTrader;
  uint16_t NoLegExecs;
  uint16_t ExecRestatementReason;
  uint8_t PartyIDEnteringFirm;
  uint8_t ProductComplex;
  char OrdStatus[LEN_ORD_STATUS];
  char ExecType[LEN_EXEC_TYPE];
  uint8_t Side;
  uint8_t OrdType;
  uint8_t TradingCapacity;
  uint8_t TimeInForce;
  uint8_t ExecInst;
  uint8_t TradingSessionSubID;
  uint8_t ApplSeqIndicator;
  char Account[LEN_ACCOUNT];
  char PartyIDPositionAccount[LEN_PARTY_ID_POSITION_ACCOUNT];
  char PositionEffect[LEN_POSITION_EFFECT];
  char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
  char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
  char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char FIXClOrdID[LEN_FIX_CL_ORDID];
  uint8_t NoFills;
  uint8_t NoLegs;
  uint8_t Triggered;
  uint8_t CrossedIndicator;
  char Pad2[LEN_PAD2];
  LegOrdGrpSeqT LegOrdGrp[MAX_ORDER_EXEC_REPORT_BROADCAST_LEG_ORD_GRP];
  FillsGrpSeqT FillsGrp[MAX_ORDER_EXEC_REPORT_BROADCAST_FILLS_GRP];
  InstrmntLegExecGrpSeqT InstrmntLegExecGrp[MAX_ORDER_EXEC_REPORT_BROADCAST_INSTRMNT_LEG_EXEC_GRP];
} OrderExecReportBroadcastT;

// Message:	    OrderExecResponse
// TemplateID:  10103
// Alias:       Immediate Execution Response
// FIX MsgType: ExecutionReport = "8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderMECompT ResponseHeaderME;
  uint64_t OrderID;
  uint64_t ClOrdID;
  uint64_t OrigClOrdID;
  int64_t SecurityID;
  uint64_t ExecID;
  uint64_t TrdRegTSEntryTime;
  uint64_t TrdRegTSTimePriority;
  int32_t MarketSegmentID;
  int32_t LeavesQty;
  int32_t CumQty;
  int32_t CxlQty;
  uint16_t NoLegExecs;
  uint16_t ExecRestatementReason;
  uint8_t Side;
  uint8_t ProductComplex;
  char OrdStatus[LEN_ORD_STATUS];
  char ExecType[LEN_EXEC_TYPE];
  uint8_t Triggered;
  uint8_t CrossedIndicator;
  uint8_t NoFills;
  char Pad5[LEN_PAD5];
  FillsGrpSeqT FillsGrp[MAX_ORDER_EXEC_RESPONSE_FILLS_GRP];
  InstrmntLegExecGrpSeqT InstrmntLegExecGrp[MAX_ORDER_EXEC_RESPONSE_INSTRMNT_LEG_EXEC_GRP];
} OrderExecResponseT;

// Message:	    PartyActionReport
// TemplateID:  10042
// Alias:       Party Action Report
// FIX MsgType: PartyActionReport = "DI"
typedef struct {
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

// Message:	    PartyEntitlementsUpdateReport
// TemplateID:  10034
// Alias:       Entitlement Notification
// FIX MsgType: PartyEntitlementsUpdateReport = "CZ"
typedef struct {
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

// Message:	    QuoteActivationNotification
// TemplateID:  10411
// Alias:       Quote Activation Notification
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct {
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

// Message:	    QuoteActivationRequest
// TemplateID:  10403
// Alias:       Quote Activation Request
// FIX MsgType: OrderMassActionRequest = "CA"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t PartyIdInvestmentDecisionMaker;
  uint64_t ExecutingTrader;
  int32_t MarketSegmentID;
  uint32_t TargetPartyIDSessionID;
  uint8_t MassActionType;
  uint8_t ProductComplex;
  uint8_t PartyIdInvestmentDecisionMakerQualifier;
  uint8_t ExecutingTraderQualifier;
  char Pad4[LEN_PAD4];
} QuoteActivationRequestT;

// Message:	    QuoteActivationResponse
// TemplateID:  10404
// Alias:       Quote Activation Response
// FIX MsgType: OrderMassActionReport = "BZ"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t MassActionReportID;
  uint16_t NoNotAffectedSecurities;
  char Pad6[LEN_PAD6];
  NotAffectedSecuritiesGrpSeqT NotAffectedSecuritiesGrp[MAX_QUOTE_ACTIVATION_RESPONSE_NOT_AFFECTED_SECURITIES_GRP];
} QuoteActivationResponseT;

// Message:	    QuoteExecutionReport
// TemplateID:  10407
// Alias:       Quote Execution Notification
// FIX MsgType: QuoteExecutionReport = "U8"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderMECompT RBCHeaderME;
  uint64_t ExecID;
  int32_t MarketSegmentID;
  uint16_t NoLegExecs;
  uint8_t NoQuoteEvents;
  char Pad1[LEN_PAD1];
  QuoteEventGrpSeqT QuoteEventGrp[MAX_QUOTE_EXECUTION_REPORT_QUOTE_EVENT_GRP];
  QuoteLegExecGrpSeqT QuoteLegExecGrp[MAX_QUOTE_EXECUTION_REPORT_QUOTE_LEG_EXEC_GRP];
} QuoteExecutionReportT;

// Message:	    RFQRequest
// TemplateID:  10401
// Alias:       Quote Request
// FIX MsgType: QuoteRequest = "R"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t SecurityID;
  int32_t MarketSegmentID;
  int32_t OrderQty;
  uint8_t Side;
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char Pad3[LEN_PAD3];
} RFQRequestT;

// Message:	    RFQResponse
// TemplateID:  10402
// Alias:       Quote Request Response
// FIX MsgType: RequestAcknowledge = "U1"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint64_t ExecID;
} RFQResponseT;

// Message:	    Reject
// TemplateID:  10010
// Alias:       Reject
// FIX MsgType: Reject = "3"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRResponseHeaderMECompT NRResponseHeaderME;
  uint32_t SessionRejectReason;
  uint16_t VarTextLen;
  uint8_t SessionStatus;
  char Pad1[LEN_PAD1];
  char VarText[LEN_VAR_TEXT];
} RejectT;

// Message:	    RetransmitMEMessageRequest
// TemplateID:  10026
// Alias:       Retransmit (Order/Quote Event)
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint32_t SubscriptionScope;
  uint16_t PartitionID;
  uint8_t RefApplID;
  char ApplBegMsgID[LEN_APPL_BEG_MSGID];
  char ApplEndMsgID[LEN_APPL_END_MSGID];
  char Pad1[LEN_PAD1];
} RetransmitMEMessageRequestT;

// Message:	    RetransmitMEMessageResponse
// TemplateID:  10027
// Alias:       Retransmit Response (Order/Quote Event)
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  uint16_t ApplTotalMessageCount;
  char ApplEndMsgID[LEN_APPL_END_MSGID];
  char RefApplLastMsgID[LEN_REF_APPL_LAST_MSGID];
  char Pad6[LEN_PAD6];
} RetransmitMEMessageResponseT;

// Message:	    RetransmitRequest
// TemplateID:  10008
// Alias:       Retransmit
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t ApplBegSeqNum;
  uint64_t ApplEndSeqNum;
  uint16_t PartitionID;
  uint8_t RefApplID;
  char Pad5[LEN_PAD5];
} RetransmitRequestT;

// Message:	    RetransmitResponse
// TemplateID:  10009
// Alias:       Retransmit Response
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  uint64_t ApplEndSeqNum;
  uint64_t RefApplLastSeqNum;
  uint16_t ApplTotalMessageCount;
  char Pad6[LEN_PAD6];
} RetransmitResponseT;

// Message:	    RiskNotificationBroadcast
// TemplateID:  10033
// Alias:       Risk Notification
// FIX MsgType: PartyRiskLimitsUpdateReport = "CR"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  uint32_t TradeDate;
  uint32_t PartyDetailIDExecutingUnit;
  uint32_t RequestingPartyIDExecutingSystem;
  uint16_t MarketID;
  char ListUpdateAction[LEN_LIST_UPDATE_ACTION];
  uint8_t RiskLimitAction;
  char RequestingPartyEnteringFirm[LEN_REQUESTING_PARTY_ENTERING_FIRM];
  char RequestingPartyClearingFirm[LEN_REQUESTING_PARTY_CLEARING_FIRM];
  char Pad6[LEN_PAD6];
} RiskNotificationBroadcastT;

// Message:	    SRQSCreateDealNotification
// TemplateID:  10708
// Alias:       SRQS Create Deal Notification
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  int64_t LastPx;
  uint64_t QuoteID;
  int64_t SecurityID;
  uint64_t ExpireTime;
  uint32_t NegotiationID;
  uint32_t TradeID;
  int32_t LastQty;
  uint8_t TrdRptStatus;
  char MessageEventSource[LEN_MESSAGE_EVENT_SOURCE];
  uint8_t Side;
  uint8_t NoLegs;
  char RootPartyExecutingFirm[LEN_ROOT_PARTY_EXECUTING_FIRM];
  char RootPartyExecutingTrader[LEN_ROOT_PARTY_EXECUTING_TRADER];
  char RootPartyEnteringTrader[LEN_ROOT_PARTY_ENTERING_TRADER];
  char TargetPartyExecutingFirm[LEN_TARGET_PARTY_EXECUTING_FIRM];
  char TargetPartyExecutingTrader[LEN_TARGET_PARTY_EXECUTING_TRADER];
  char FirmTradeID[LEN_FIRM_TRADEID];
  char FirmNegotiationID[LEN_FIRM_NEGOTIATIONID];
  char FreeText1[LEN_FREE_TEXT1];
  SRQSTrdInstrmntLegGrpSeqT SRQSTrdInstrmntLegGrp[MAX_SRQS_CREATE_DEAL_NOTIFICATION_SRQS_TRD_INSTRMNT_LEG_GRP];
} SRQSCreateDealNotificationT;

// Message:	    SRQSDealNotification
// TemplateID:  10709
// Alias:       SRQS Deal Notification
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  uint32_t NegotiationID;
  uint32_t TradeID;
  uint8_t TrdRptStatus;
  char MessageEventSource[LEN_MESSAGE_EVENT_SOURCE];
  char RootPartyExecutingFirm[LEN_ROOT_PARTY_EXECUTING_FIRM];
  char RootPartyExecutingTrader[LEN_ROOT_PARTY_EXECUTING_TRADER];
  char RootPartyEnteringTrader[LEN_ROOT_PARTY_ENTERING_TRADER];
  char TargetPartyExecutingFirm[LEN_TARGET_PARTY_EXECUTING_FIRM];
  char TargetPartyExecutingTrader[LEN_TARGET_PARTY_EXECUTING_TRADER];
  char TargetPartyEnteringTrader[LEN_TARGET_PARTY_ENTERING_TRADER];
  char FirmTradeID[LEN_FIRM_TRADEID];
  char FirmNegotiationID[LEN_FIRM_NEGOTIATIONID];
  char Pad4[LEN_PAD4];
} SRQSDealNotificationT;

// Message:	    SRQSDealResponse
// TemplateID:  10705
// Alias:       SRQS Deal Response
// FIX MsgType: QuoteAck = "CW"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  int64_t SecurityID;
  uint64_t QuoteID;
  uint32_t NegotiationID;
  uint32_t TradeID;
  uint32_t SecondaryTradeID;
  char FirmTradeID[LEN_FIRM_TRADEID];
  char FirmNegotiationID[LEN_FIRM_NEGOTIATIONID];
  char Pad4[LEN_PAD4];
} SRQSDealResponseT;

// Message:	    SRQSEnterQuoteRequest
// TemplateID:  10702
// Alias:       SRQS Enter Quote Request
// FIX MsgType: QuoteRequest = "R"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t BidPx;
  int64_t OfferPx;
  int32_t MarketSegmentID;
  uint32_t NegotiationID;
  int32_t BidSize;
  int32_t OfferSize;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char FreeText1[LEN_FREE_TEXT1];
  char Pad1[LEN_PAD1];
} SRQSEnterQuoteRequestT;

// Message:	    SRQSHitQuoteRequest
// TemplateID:  10704
// Alias:       SRQS Hit Quote Request
// FIX MsgType: QuoteResponse = "AJ"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint64_t QuoteID;
  uint64_t ValidUntilTime;
  int32_t MarketSegmentID;
  uint32_t NegotiationID;
  int32_t OrderQty;
  uint8_t Side;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char FirmTradeID[LEN_FIRM_TRADEID];
  char FreeText1[LEN_FREE_TEXT1];
} SRQSHitQuoteRequestT;

// Message:	    SRQSNegotiationNotification
// TemplateID:  10713
// Alias:       SRQS Negotiation Notification for Respondent
// FIX MsgType: QuoteStatusReport = "AI"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  int64_t QuoteRefPrice;
  int64_t BidPx;
  int64_t OfferPx;
  uint32_t NegotiationID;
  int32_t LeavesQty;
  uint8_t QuoteType;
  uint8_t QuoteStatus;
  uint8_t QuoteInstruction;
  uint8_t Side;
  uint8_t QuoteRefPriceSource;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char PartyEnteringTrader[LEN_PARTY_ENTERING_TRADER];
  char TargetPartyExecutingFirm[LEN_TARGET_PARTY_EXECUTING_FIRM];
  char TargetPartyExecutingTrader[LEN_TARGET_PARTY_EXECUTING_TRADER];
  char FirmNegotiationID[LEN_FIRM_NEGOTIATIONID];
  char Pad3[LEN_PAD3];
} SRQSNegotiationNotificationT;

// Message:	    SRQSNegotiationRequesterNotification
// TemplateID:  10712
// Alias:       SRQS Negotiation Notification for Requester
// FIX MsgType: QuoteStatusReport = "AI"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  int64_t QuoteRefPrice;
  int64_t BidPx;
  int64_t OfferPx;
  uint32_t NegotiationID;
  int32_t OrderQty;
  int32_t LeavesQty;
  uint8_t QuoteType;
  uint8_t QuoteStatus;
  uint8_t NoTargetPartyIDs;
  uint8_t Side;
  uint8_t QuoteRefPriceSource;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char PartyEnteringTrader[LEN_PARTY_ENTERING_TRADER];
  char FirmNegotiationID[LEN_FIRM_NEGOTIATIONID];
  char Pad2[LEN_PAD2];
  TargetPartiesSeqT TargetParties[MAX_SRQS_NEGOTIATION_REQUESTER_NOTIFICATION_TARGET_PARTIES];
} SRQSNegotiationRequesterNotificationT;

// Message:	    SRQSNegotiationStatusNotification
// TemplateID:  10715
// Alias:       SRQS Negotiation Status Notification
// FIX MsgType: QuoteStatusReport = "AI"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  uint32_t NegotiationID;
  uint8_t QuoteStatus;
  char FirmNegotiationID[LEN_FIRM_NEGOTIATIONID];
  char Pad7[LEN_PAD7];
} SRQSNegotiationStatusNotificationT;

// Message:	    SRQSOpenNegotiationNotification
// TemplateID:  10711
// Alias:       SRQS Open Negotiation Notification for Respondent
// FIX MsgType: QuoteStatusReport = "AI"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  int64_t SecurityID;
  int64_t BidPx;
  int64_t OfferPx;
  int64_t QuoteRefPrice;
  uint32_t NegotiationID;
  int32_t MarketSegmentID;
  int32_t LeavesQty;
  uint8_t QuoteType;
  uint8_t QuoteStatus;
  uint8_t NoLegs;
  uint8_t Side;
  uint8_t QuoteRefPriceSource;
  uint8_t TradeUnderlying;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char PartyEnteringTrader[LEN_PARTY_ENTERING_TRADER];
  char TargetPartyExecutingFirm[LEN_TARGET_PARTY_EXECUTING_FIRM];
  char TargetPartyExecutingTrader[LEN_TARGET_PARTY_EXECUTING_TRADER];
  char FirmNegotiationID[LEN_FIRM_NEGOTIATIONID];
  char Pad6[LEN_PAD6];
  QuotReqLegsGrpSeqT QuotReqLegsGrp[MAX_SRQS_OPEN_NEGOTIATION_NOTIFICATION_QUOT_REQ_LEGS_GRP];
} SRQSOpenNegotiationNotificationT;

// Message:	    SRQSOpenNegotiationRequest
// TemplateID:  10700
// Alias:       SRQS Open Negotiation Request
// FIX MsgType: QuoteRequest = "R"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t SecurityID;
  int64_t BidPx;
  int64_t OfferPx;
  int64_t QuoteRefPrice;
  int32_t MarketSegmentID;
  int32_t OrderQty;
  uint8_t QuoteType;
  uint8_t NoLegs;
  uint8_t NoTargetPartyIDs;
  uint8_t Side;
  uint8_t QuoteRefPriceSource;
  uint8_t TradeUnderlying;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char QuoteReqID[LEN_QUOTE_REQID];
  char Pad3[LEN_PAD3];
  QuotReqLegsGrpSeqT QuotReqLegsGrp[MAX_SRQS_OPEN_NEGOTIATION_REQUEST_QUOT_REQ_LEGS_GRP];
  TargetPartiesSeqT TargetParties[MAX_SRQS_OPEN_NEGOTIATION_REQUEST_TARGET_PARTIES];
} SRQSOpenNegotiationRequestT;

// Message:	    SRQSOpenNegotiationRequesterNotification
// TemplateID:  10710
// Alias:       SRQS Open Negotiation Notification for Requester
// FIX MsgType: QuoteStatusReport = "AI"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  int64_t SecurityID;
  int64_t BidPx;
  int64_t OfferPx;
  int64_t QuoteRefPrice;
  uint32_t NegotiationID;
  int32_t MarketSegmentID;
  int32_t OrderQty;
  uint8_t QuoteType;
  uint8_t QuoteStatus;
  uint8_t NoLegs;
  uint8_t NoTargetPartyIDs;
  uint8_t Side;
  uint8_t QuoteRefPriceSource;
  uint8_t TradeUnderlying;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char PartyEnteringTrader[LEN_PARTY_ENTERING_TRADER];
  char FirmNegotiationID[LEN_FIRM_NEGOTIATIONID];
  QuotReqLegsGrpSeqT QuotReqLegsGrp[MAX_SRQS_OPEN_NEGOTIATION_REQUESTER_NOTIFICATION_QUOT_REQ_LEGS_GRP];
  TargetPartiesSeqT TargetParties[MAX_SRQS_OPEN_NEGOTIATION_REQUESTER_NOTIFICATION_TARGET_PARTIES];
} SRQSOpenNegotiationRequesterNotificationT;

// Message:	    SRQSQuoteNotification
// TemplateID:  10707
// Alias:       SRQS Quote Notification
// FIX MsgType: Quote = "S"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  uint64_t QuoteID;
  uint64_t SecondaryQuoteID;
  int64_t BidPx;
  int64_t OfferPx;
  uint32_t NegotiationID;
  int32_t BidSize;
  int32_t OfferSize;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char PartyEnteringTrader[LEN_PARTY_ENTERING_TRADER];
  char QuoteReqID[LEN_QUOTE_REQID];
  char FreeText1[LEN_FREE_TEXT1];
  char Pad3[LEN_PAD3];
} SRQSQuoteNotificationT;

// Message:	    SRQSQuoteResponse
// TemplateID:  10703
// Alias:       SRQS Quote Response
// FIX MsgType: QuoteAck = "CW"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  uint64_t QuoteID;
  uint32_t NegotiationID;
  char QuoteReqID[LEN_QUOTE_REQID];
} SRQSQuoteResponseT;

// Message:	    SRQSStatusBroadcast
// TemplateID:  10714
// Alias:       SRQS Status Notification
// FIX MsgType: QuoteStatusReport = "AI"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint32_t TradeDate;
  uint8_t TradSesEvent;
  char Pad3[LEN_PAD3];
} SRQSStatusBroadcastT;

// Message:	    SRQSUpdateDealStatusRequest
// TemplateID:  10706
// Alias:       SRQS Update Deal Request
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int32_t MarketSegmentID;
  uint32_t NegotiationID;
  uint32_t TradeID;
  uint8_t TradeReportType;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
} SRQSUpdateDealStatusRequestT;

// Message:	    SRQSUpdateNegotiationRequest
// TemplateID:  10701
// Alias:       SRQS Update Negotiation Request
// FIX MsgType: QuoteRequest = "R"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t QuoteRefPrice;
  int64_t BidPx;
  int64_t OfferPx;
  int32_t MarketSegmentID;
  uint32_t NegotiationID;
  int32_t OrderQty;
  uint8_t NoTargetPartyIDs;
  uint8_t Side;
  uint8_t QuoteCancelType;
  uint8_t QuoteRefPriceSource;
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  char Pad5[LEN_PAD5];
  TargetPartiesSeqT TargetParties[MAX_SRQS_UPDATE_NEGOTIATION_REQUEST_TARGET_PARTIES];
} SRQSUpdateNegotiationRequestT;

// Message:	    ServiceAvailabilityBroadcast
// TemplateID:  10030
// Alias:       Service Availability
// FIX MsgType: UserNotification = "CB"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRBCHeaderCompT NRBCHeader;
  uint32_t MatchingEngineTradeDate;
  uint32_t TradeManagerTradeDate;
  uint32_t ApplSeqTradeDate;
  uint32_t T7EntryServiceTradeDate;
  uint32_t T7EntryServiceRtmTradeDate;
  uint16_t PartitionID;
  uint8_t MatchingEngineStatus;
  uint8_t TradeManagerStatus;
  uint8_t ApplSeqStatus;
  uint8_t T7EntryServiceStatus;
  uint8_t T7EntryServiceRtmStatus;
  char Pad5[LEN_PAD5];
} ServiceAvailabilityBroadcastT;

// Message:	    ServiceAvailabilityMarketBroadcast
// TemplateID:  10044
// Alias:       Service Availability Market
// FIX MsgType: UserNotification = "CB"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NRBCHeaderCompT NRBCHeader;
  uint32_t SelectiveRequestForQuoteServiceTradeDate;
  uint8_t SelectiveRequestForQuoteServiceStatus;
  char Pad3[LEN_PAD3];
} ServiceAvailabilityMarketBroadcastT;

// Message:	    SubscribeRequest
// TemplateID:  10025
// Alias:       Subscribe
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint32_t SubscriptionScope;
  uint8_t RefApplID;
  char Pad3[LEN_PAD3];
} SubscribeRequestT;

// Message:	    SubscribeResponse
// TemplateID:  10005
// Alias:       Subscribe Response
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  uint32_t ApplSubID;
  char Pad4[LEN_PAD4];
} SubscribeResponseT;

// Message:	    TESApproveBroadcast
// TemplateID:  10607
// Alias:       Approve TES Trade Broadcast
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  int64_t SecurityID;
  int64_t LastPx;
  uint64_t TransactTime;
  int64_t UnderlyingPx;
  uint64_t TransBkdTime;
  int64_t RelatedClosePrice;
  int32_t MarketSegmentID;
  uint32_t PackageID;
  uint32_t TESExecID;
  int32_t AllocQty;
  uint32_t AllocID;
  uint32_t UnderlyingSettlementDate;
  uint32_t UnderlyingMaturityDate;
  uint32_t RelatedTradeID;
  int32_t RelatedMarketSegmentID;
  int32_t RelatedTradeQuantity;
  int64_t UnderlyingQty;
  uint16_t TrdType;
  uint8_t Side;
  uint8_t TradePublishIndicator;
  uint8_t ProductComplex;
  uint8_t TradeReportType;
  uint8_t TradingCapacity;
  uint8_t PartyIDSettlementLocation;
  uint8_t TradeAllocStatus;
  uint8_t HedgeType;
  uint8_t NoLegs;
  uint8_t NoEvents;
  uint8_t NoInstrAttrib;
  uint8_t NoUnderlyingStips;
  char MessageEventSource[LEN_MESSAGE_EVENT_SOURCE];
  char TradeReportID[LEN_TRADE_REPORTID];
  char PartyExecutingFirm[LEN_PARTY_EXECUTING_FIRM];
  char PartyExecutingTrader[LEN_PARTY_EXECUTING_TRADER];
  uint8_t PartyIDEnteringFirm;
  char PartyEnteringTrader[LEN_PARTY_ENTERING_TRADER];
  char PositionEffect[LEN_POSITION_EFFECT];
  char RootPartyExecutingFirm[LEN_ROOT_PARTY_EXECUTING_FIRM];
  char RootPartyExecutingTrader[LEN_ROOT_PARTY_EXECUTING_TRADER];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char PartyIDTakeUpTradingFirm[LEN_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char Account[LEN_ACCOUNT];
  char PartyIDPositionAccount[LEN_PARTY_ID_POSITION_ACCOUNT];
  char PartyIDOrderOriginationFirm[LEN_PARTY_ID_ORDER_ORIGINATION_FIRM];
  char PartyIDBeneficiary[LEN_PARTY_ID_BENEFICIARY];
  char PartyIDLocationID[LEN_PARTY_ID_LOCATIONID];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char ComplianceText[LEN_COMPLIANCE_TEXT];
  char UnderlyingSecurityID[LEN_UNDERLYING_SECURITYID];
  char UnderlyingSecurityDesc[LEN_UNDERLYING_SECURITY_DESC];
  char UnderlyingCurrency[LEN_UNDERLYING_CURRENCY];
  char UnderlyingIssuer[LEN_UNDERLYING_ISSUER];
  char Pad2[LEN_PAD2];
  TrdInstrmntLegGrpSeqT TrdInstrmntLegGrp[MAX_TES_APPROVE_BROADCAST_TRD_INSTRMNT_LEG_GRP];
  InstrumentEventGrpSeqT InstrumentEventGrp[MAX_TES_APPROVE_BROADCAST_INSTRUMENT_EVENT_GRP];
  InstrumentAttributeGrpSeqT InstrumentAttributeGrp[MAX_TES_APPROVE_BROADCAST_INSTRUMENT_ATTRIBUTE_GRP];
  UnderlyingStipGrpSeqT UnderlyingStipGrp[MAX_TES_APPROVE_BROADCAST_UNDERLYING_STIP_GRP];
} TESApproveBroadcastT;

// Message:	    TESBroadcast
// TemplateID:  10604
// Alias:       TES Broadcast
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  int64_t SecurityID;
  int64_t LastPx;
  uint64_t TransactTime;
  int64_t UnderlyingPx;
  uint64_t TransBkdTime;
  int64_t RelatedClosePrice;
  int32_t MarketSegmentID;
  uint32_t PackageID;
  uint32_t TESExecID;
  uint32_t UnderlyingSettlementDate;
  uint32_t UnderlyingMaturityDate;
  uint32_t RelatedTradeID;
  int32_t RelatedMarketSegmentID;
  int32_t RelatedTradeQuantity;
  int64_t UnderlyingQty;
  uint16_t TrdType;
  uint8_t TradeReportType;
  uint8_t ProductComplex;
  uint8_t TradePublishIndicator;
  uint8_t NoEvents;
  uint8_t NoInstrAttrib;
  uint8_t NoUnderlyingStips;
  uint8_t NoSideAllocs;
  uint8_t NoLegs;
  uint8_t PartyIDSettlementLocation;
  uint8_t HedgeType;
  char MessageEventSource[LEN_MESSAGE_EVENT_SOURCE];
  char TradeReportText[LEN_TRADE_REPORT_TEXT];
  char TradeReportID[LEN_TRADE_REPORTID];
  char RootPartyExecutingFirm[LEN_ROOT_PARTY_EXECUTING_FIRM];
  char RootPartyExecutingTrader[LEN_ROOT_PARTY_EXECUTING_TRADER];
  char UnderlyingSecurityID[LEN_UNDERLYING_SECURITYID];
  char UnderlyingSecurityDesc[LEN_UNDERLYING_SECURITY_DESC];
  char UnderlyingCurrency[LEN_UNDERLYING_CURRENCY];
  char UnderlyingIssuer[LEN_UNDERLYING_ISSUER];
  char Pad5[LEN_PAD5];
  SideAllocGrpBCSeqT SideAllocGrpBC[MAX_TES_BROADCAST_SIDE_ALLOC_GRPBC];
  TrdInstrmntLegGrpSeqT TrdInstrmntLegGrp[MAX_TES_BROADCAST_TRD_INSTRMNT_LEG_GRP];
  InstrumentEventGrpSeqT InstrumentEventGrp[MAX_TES_BROADCAST_INSTRUMENT_EVENT_GRP];
  InstrumentAttributeGrpSeqT InstrumentAttributeGrp[MAX_TES_BROADCAST_INSTRUMENT_ATTRIBUTE_GRP];
  UnderlyingStipGrpSeqT UnderlyingStipGrp[MAX_TES_BROADCAST_UNDERLYING_STIP_GRP];
} TESBroadcastT;

// Message:	    TESDeleteBroadcast
// TemplateID:  10606
// Alias:       Delete TES Trade Broadcast
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  int32_t MarketSegmentID;
  uint32_t PackageID;
  uint32_t TESExecID;
  uint16_t TrdType;
  uint8_t DeleteReason;
  uint8_t TradeReportType;
  char MessageEventSource[LEN_MESSAGE_EVENT_SOURCE];
  char TradeReportID[LEN_TRADE_REPORTID];
  char Pad3[LEN_PAD3];
} TESDeleteBroadcastT;

// Message:	    TESExecutionBroadcast
// TemplateID:  10610
// Alias:       TES Execution Broadcast
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint64_t TransactTime;
  int32_t MarketSegmentID;
  uint32_t PackageID;
  uint32_t TESExecID;
  uint32_t AllocID;
  uint16_t TrdType;
  uint8_t TradeReportType;
  char MessageEventSource[LEN_MESSAGE_EVENT_SOURCE];
  char Pad4[LEN_PAD4];
} TESExecutionBroadcastT;

// Message:	    TESResponse
// TemplateID:  10611
// Alias:       TES Response
// FIX MsgType: TradeCaptureReportAck = "AR"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
  uint32_t TESExecID;
  char TradeReportID[LEN_TRADE_REPORTID];
} TESResponseT;

// Message:	    TESTradeBroadcast
// TemplateID:  10614
// Alias:       TES Trade Broadcast
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  int64_t SecurityID;
  int64_t LastPx;
  int64_t ClearingTradePrice;
  uint64_t TransactTime;
  int64_t RelatedSecurityID;
  uint32_t PackageID;
  int32_t LastQty;
  int32_t MarketSegmentID;
  uint32_t TradeID;
  uint32_t TradeDate;
  uint32_t SideTradeID;
  uint32_t RootPartyIDSessionID;
  uint32_t OrigTradeID;
  int32_t ClearingTradeQty;
  uint32_t RootPartyIDExecutingUnit;
  uint32_t RootPartyIDExecutingTrader;
  uint32_t RootPartyIDClearingUnit;
  uint32_t StrategyLinkID;
  int32_t RelatedSymbol;
  int32_t TotNumTradeReports;
  uint16_t TrdType;
  uint8_t ProductComplex;
  uint8_t RelatedProductComplex;
  uint8_t Side;
  uint8_t TradingCapacity;
  uint8_t TradeReportType;
  uint8_t TransferReason;
  uint8_t MultiLegReportingType;
  char PositionEffect[LEN_POSITION_EFFECT];
  uint8_t MultilegPriceModel;
  char Account[LEN_ACCOUNT];
  char RootPartyIDPositionAccount[LEN_ROOT_PARTY_ID_POSITION_ACCOUNT];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char RootPartyExecutingFirm[LEN_ROOT_PARTY_EXECUTING_FIRM];
  char RootPartyExecutingTrader[LEN_ROOT_PARTY_EXECUTING_TRADER];
  char RootPartyClearingFirm[LEN_ROOT_PARTY_CLEARING_FIRM];
  char RootPartyClearingOrganization[LEN_ROOT_PARTY_CLEARING_ORGANIZATION];
  char RootPartyIDBeneficiary[LEN_ROOT_PARTY_ID_BENEFICIARY];
  char RootPartyIDTakeUpTradingFirm[LEN_ROOT_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char RootPartyIDOrderOriginationFirm[LEN_ROOT_PARTY_ID_ORDER_ORIGINATION_FIRM];
  char Pad1[LEN_PAD1];
} TESTradeBroadcastT;

// Message:	    TESTradingSessionStatusBroadcast
// TemplateID:  10615
// Alias:       TES Status Broadcast
// FIX MsgType: TradingSessionStatus = "h"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint32_t TradeDate;
  uint8_t TradSesEvent;
  char Pad3[LEN_PAD3];
} TESTradingSessionStatusBroadcastT;

// Message:	    TESUploadBroadcast
// TemplateID:  10613
// Alias:       TES Trade Upload Broadcast
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  int64_t SecurityID;
  int64_t LastPx;
  uint64_t TransBkdTime;
  uint64_t TransactTime;
  int64_t UnderlyingPx;
  int64_t RelatedClosePrice;
  int32_t MarketSegmentID;
  uint32_t PackageID;
  uint32_t TESExecID;
  uint32_t UnderlyingSettlementDate;
  uint32_t UnderlyingMaturityDate;
  uint32_t RelatedTradeID;
  int32_t RelatedMarketSegmentID;
  int32_t RelatedTradeQuantity;
  int64_t UnderlyingQty;
  uint16_t TrdType;
  uint8_t ProductComplex;
  uint8_t TradeReportType;
  uint8_t TradePublishIndicator;
  uint8_t NoSideAllocs;
  uint8_t NoLegs;
  uint8_t NoEvents;
  uint8_t NoInstrAttrib;
  uint8_t NoUnderlyingStips;
  uint8_t HedgeType;
  uint8_t PartyIDSettlementLocation;
  char MessageEventSource[LEN_MESSAGE_EVENT_SOURCE];
  char TradeReportID[LEN_TRADE_REPORTID];
  char RootPartyExecutingFirm[LEN_ROOT_PARTY_EXECUTING_FIRM];
  char RootPartyExecutingTrader[LEN_ROOT_PARTY_EXECUTING_TRADER];
  char UnderlyingSecurityID[LEN_UNDERLYING_SECURITYID];
  char UnderlyingSecurityDesc[LEN_UNDERLYING_SECURITY_DESC];
  char UnderlyingCurrency[LEN_UNDERLYING_CURRENCY];
  char UnderlyingIssuer[LEN_UNDERLYING_ISSUER];
  char Pad1[LEN_PAD1];
  SideAllocExtGrpSeqT SideAllocExtGrp[MAX_TES_UPLOAD_BROADCAST_SIDE_ALLOC_EXT_GRP];
  TrdInstrmntLegGrpSeqT TrdInstrmntLegGrp[MAX_TES_UPLOAD_BROADCAST_TRD_INSTRMNT_LEG_GRP];
  InstrumentEventGrpSeqT InstrumentEventGrp[MAX_TES_UPLOAD_BROADCAST_INSTRUMENT_EVENT_GRP];
  InstrumentAttributeGrpSeqT InstrumentAttributeGrp[MAX_TES_UPLOAD_BROADCAST_INSTRUMENT_ATTRIBUTE_GRP];
  UnderlyingStipGrpSeqT UnderlyingStipGrp[MAX_TES_UPLOAD_BROADCAST_UNDERLYING_STIP_GRP];
} TESUploadBroadcastT;

// Message:	    TMTradingSessionStatusBroadcast
// TemplateID:  10501
// Alias:       Trade Notification Status
// FIX MsgType: TradingSessionStatus = "h"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  uint8_t TradSesEvent;
  char Pad7[LEN_PAD7];
} TMTradingSessionStatusBroadcastT;

// Message:	    ThrottleUpdateNotification
// TemplateID:  10028
// Alias:       Throttle Update Notification
// FIX MsgType: UserNotification = "CB"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  NotifHeaderCompT NotifHeader;
  int64_t ThrottleTimeInterval;
  uint32_t ThrottleNoMsgs;
  uint32_t ThrottleDisconnectLimit;
} ThrottleUpdateNotificationT;

// Message:	    TradeBroadcast
// TemplateID:  10500
// Alias:       Trade Notification
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderCompT RBCHeader;
  int64_t SecurityID;
  int64_t RelatedSecurityID;
  int64_t Price;
  int64_t LastPx;
  int64_t SideLastPx;
  int64_t ClearingTradePrice;
  uint64_t TransactTime;
  uint64_t OrderID;
  uint64_t ClOrdID;
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
  uint8_t MultiLegReportingType;
  uint8_t TradeReportType;
  uint8_t TransferReason;
  char RootPartyIDBeneficiary[LEN_ROOT_PARTY_ID_BENEFICIARY];
  char RootPartyIDTakeUpTradingFirm[LEN_ROOT_PARTY_ID_TAKE_UP_TRADING_FIRM];
  char RootPartyIDOrderOriginationFirm[LEN_ROOT_PARTY_ID_ORDER_ORIGINATION_FIRM];
  uint8_t MatchType;
  uint8_t MatchSubType;
  uint8_t Side;
  uint8_t SideLiquidityInd;
  uint8_t TradingCapacity;
  char Account[LEN_ACCOUNT];
  char RootPartyIDPositionAccount[LEN_ROOT_PARTY_ID_POSITION_ACCOUNT];
  char PositionEffect[LEN_POSITION_EFFECT];
  char CustOrderHandlingInst[LEN_CUST_ORDER_HANDLING_INST];
  char FreeText1[LEN_FREE_TEXT1];
  char FreeText2[LEN_FREE_TEXT2];
  char FreeText3[LEN_FREE_TEXT3];
  char OrderCategory[LEN_ORDER_CATEGORY];
  uint8_t OrdType;
  uint8_t RelatedProductComplex;
  uint8_t OrderSide;
  char RootPartyClearingOrganization[LEN_ROOT_PARTY_CLEARING_ORGANIZATION];
  char RootPartyExecutingFirm[LEN_ROOT_PARTY_EXECUTING_FIRM];
  char RootPartyExecutingTrader[LEN_ROOT_PARTY_EXECUTING_TRADER];
  char RootPartyClearingFirm[LEN_ROOT_PARTY_CLEARING_FIRM];
  char Pad3[LEN_PAD3];
} TradeBroadcastT;

// Message:	    TradingSessionStatusBroadcast
// TemplateID:  10307
// Alias:       Trading Session Event
// FIX MsgType: TradingSessionStatus = "h"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  RBCHeaderMECompT RBCHeaderME;
  int32_t MarketSegmentID;
  uint32_t TradeDate;
  uint8_t TradSesEvent;
  char RefApplLastMsgID[LEN_REF_APPL_LAST_MSGID];
  char Pad7[LEN_PAD7];
} TradingSessionStatusBroadcastT;

// Message:	    UnsubscribeRequest
// TemplateID:  10006
// Alias:       Unsubscribe
// FIX MsgType: ApplicationMessageRequest = "BW"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint32_t RefApplSubID;
  char Pad4[LEN_PAD4];
} UnsubscribeRequestT;

// Message:	    UnsubscribeResponse
// TemplateID:  10007
// Alias:       Unsubscribe Response
// FIX MsgType: ApplicationMessageRequestAck = "BX"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
} UnsubscribeResponseT;

// Message:	    UploadTESTradeRequest
// TemplateID:  10612
// Alias:       Upload TES Trade Request
// FIX MsgType: TradeCaptureReport = "AE"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  int64_t SecurityID;
  int64_t LastPx;
  uint64_t TransBkdTime;
  int64_t UnderlyingPx;
  int64_t RelatedClosePrice;
  int32_t MarketSegmentID;
  uint32_t UnderlyingSettlementDate;
  uint32_t UnderlyingMaturityDate;
  uint32_t RelatedTradeID;
  int32_t RelatedMarketSegmentID;
  int32_t RelatedTradeQuantity;
  int64_t UnderlyingQty;
  uint16_t TrdType;
  uint8_t ProductComplex;
  uint8_t TradeReportType;
  uint8_t TradePublishIndicator;
  uint8_t NoSideAllocs;
  uint8_t NoLegs;
  uint8_t NoEvents;
  uint8_t NoInstrAttrib;
  uint8_t NoUnderlyingStips;
  uint8_t SkipValidations;
  uint8_t HedgeType;
  uint8_t PartyIDSettlementLocation;
  char TradeReportID[LEN_TRADE_REPORTID];
  char TradeReportText[LEN_TRADE_REPORT_TEXT];
  char UnderlyingSecurityID[LEN_UNDERLYING_SECURITYID];
  char UnderlyingSecurityDesc[LEN_UNDERLYING_SECURITY_DESC];
  char UnderlyingCurrency[LEN_UNDERLYING_CURRENCY];
  char UnderlyingIssuer[LEN_UNDERLYING_ISSUER];
  SideAllocExtGrpSeqT SideAllocExtGrp[MAX_UPLOAD_TES_TRADE_REQUEST_SIDE_ALLOC_EXT_GRP];
  TrdInstrmntLegGrpSeqT TrdInstrmntLegGrp[MAX_UPLOAD_TES_TRADE_REQUEST_TRD_INSTRMNT_LEG_GRP];
  InstrumentEventGrpSeqT InstrumentEventGrp[MAX_UPLOAD_TES_TRADE_REQUEST_INSTRUMENT_EVENT_GRP];
  InstrumentAttributeGrpSeqT InstrumentAttributeGrp[MAX_UPLOAD_TES_TRADE_REQUEST_INSTRUMENT_ATTRIBUTE_GRP];
  UnderlyingStipGrpSeqT UnderlyingStipGrp[MAX_UPLOAD_TES_TRADE_REQUEST_UNDERLYING_STIP_GRP];
} UploadTESTradeRequestT;

// Message:	    UserLoginRequest
// TemplateID:  10018
// Alias:       User Logon
// FIX MsgType: UserRequest = "BE"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint32_t Username;
  char Password[LEN_PASSWORD];
  char Pad4[LEN_PAD4];
} UserLoginRequestT;

// Message:	    UserLoginResponse
// TemplateID:  10019
// Alias:       User Logon Response
// FIX MsgType: UserResponse = "BF"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
} UserLoginResponseT;

// Message:	    UserLogoutRequest
// TemplateID:  10029
// Alias:       User Logout
// FIX MsgType: UserRequest = "BE"
typedef struct {
  MessageHeaderInCompT MessageHeaderIn;
  RequestHeaderCompT RequestHeader;
  uint32_t Username;
  char Pad4[LEN_PAD4];
} UserLogoutRequestT;

// Message:	    UserLogoutResponse
// TemplateID:  10024
// Alias:       User Logout Response
// FIX MsgType: UserResponse = "BF"
typedef struct {
  MessageHeaderOutCompT MessageHeaderOut;
  ResponseHeaderCompT ResponseHeader;
} UserLogoutResponseT;

/*
 * Begin of DEPRECATED defines
 */

#define BYTE_ARRAY_OF_0_16 \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

#define TID_ADDCOMPLEXINSTRUMENTREQUEST 10301   // < AddComplexInstrumentRequest (Create Strategy)
#define TID_ADDCOMPLEXINSTRUMENTRESPONSE 10302  // < AddComplexInstrumentResponse (Create Strategy Response)
#define TID_ADDFLEXIBLEINSTRUMENTREQUEST 10309  // < AddFlexibleInstrumentRequest (Create Flexible Instrument Request)
#define TID_ADDFLEXIBLEINSTRUMENTRESPONSE \
  10310                                       // < AddFlexibleInstrumentResponse (Create Flexible Instrument Response)
#define TID_APPROVETESTRADEREQUEST 10603      // < ApproveTESTradeRequest (Approve TES Trade Request)
#define TID_BROADCASTERRORNOTIFICATION 10032  // < BroadcastErrorNotification (Gap Fill)
#define TID_CROSSREQUEST 10118                // < CrossRequest (Cross Request)
#define TID_CROSSREQUESTRESPONSE 10119        // < CrossRequestResponse (Cross Request Response)
#define TID_DELETEALLORDERBROADCAST 10122     // < DeleteAllOrderBroadcast (Order Mass Cancellation Notification)
#define TID_DELETEALLORDERNRRESPONSE 10124    // < DeleteAllOrderNRResponse (Order Mass Cancellation Response No Hits)
#define TID_DELETEALLORDERQUOTEEVENTBROADCAST 10308  // < DeleteAllOrderQuoteEventBroadcast (Mass Cancellation Event)
#define TID_DELETEALLORDERREQUEST 10120              // < DeleteAllOrderRequest (Order Mass Cancellation Request)
#define TID_DELETEALLORDERRESPONSE 10121             // < DeleteAllOrderResponse (Order Mass Cancellation Response)
#define TID_DELETEALLQUOTEBROADCAST 10410            // < DeleteAllQuoteBroadcast (Quote Mass Cancellation Notification)
#define TID_DELETEALLQUOTEREQUEST 10408              // < DeleteAllQuoteRequest (Quote Mass Cancellation Request)
#define TID_DELETEALLQUOTERESPONSE 10409             // < DeleteAllQuoteResponse (Quote Mass Cancellation Response)
#define TID_DELETEORDERBROADCAST 10112               // < DeleteOrderBroadcast (Cancel Order Notification)
#define TID_DELETEORDERCOMPLEXREQUEST 10123          // < DeleteOrderComplexRequest (Cancel Order Multi Leg)
#define TID_DELETEORDERNRRESPONSE 10111              // < DeleteOrderNRResponse (Cancel Order Response (Lean Order))
#define TID_DELETEORDERRESPONSE 10110                // < DeleteOrderResponse (Cancel Order Response (Standard Order))
#define TID_DELETEORDERSINGLEREQUEST 10109           // < DeleteOrderSingleRequest (Cancel Order Single)
#define TID_DELETETESTRADEREQUEST 10602              // < DeleteTESTradeRequest (Delete TES Trade Request)
#define TID_ENTERTESTRADEREQUEST 10600               // < EnterTESTradeRequest (Enter TES Trade Request)
#define TID_FORCEDLOGOUTNOTIFICATION 10012           // < ForcedLogoutNotification (Session Logout Notification)
#define TID_FORCEDUSERLOGOUTNOTIFICATION 10043       // < ForcedUserLogoutNotification (User Logout Notification)
#define TID_GATEWAYREQUEST 10020                     // < GatewayRequest (Connection Gateway Request)
#define TID_GATEWAYRESPONSE 10021                    // < GatewayResponse (Connection Gateway Response)
#define TID_HEARTBEAT 10011                          // < Heartbeat (Heartbeat)
#define TID_HEARTBEATNOTIFICATION 10023              // < HeartbeatNotification (Heartbeat Notification)
#define TID_INQUIREENRICHMENTRULEIDLISTREQUEST \
  10040  // < InquireEnrichmentRuleIDListRequest (Trade Enrichment List Inquire)
#define TID_INQUIREENRICHMENTRULEIDLISTRESPONSE \
  10041  // < InquireEnrichmentRuleIDListResponse (Trade Enrichment List Inquire Response)
#define TID_INQUIREMMPARAMETERREQUEST 10305   // < InquireMMParameterRequest (Inquire Market Maker Parameters)
#define TID_INQUIREMMPARAMETERRESPONSE 10306  // < InquireMMParameterResponse (Inquire Market Maker Parameters Response)
#define TID_INQUIRESESSIONLISTREQUEST 10035   // < InquireSessionListRequest (Session List Inquire)
#define TID_INQUIRESESSIONLISTRESPONSE 10036  // < InquireSessionListResponse (Session List Inquire Response)
#define TID_INQUIREUSERREQUEST 10038          // < InquireUserRequest (User List Inquire)
#define TID_INQUIREUSERRESPONSE 10039         // < InquireUserResponse (User List Inquire Response)
#define TID_LEGALNOTIFICATIONBROADCAST 10037  // < LegalNotificationBroadcast (Legal Notification)
#define TID_LOGONREQUEST 10000                // < LogonRequest (Session Logon)
#define TID_LOGONRESPONSE 10001               // < LogonResponse (Session Logon Response)
#define TID_LOGOUTREQUEST 10002               // < LogoutRequest (Session Logout)
#define TID_LOGOUTRESPONSE 10003              // < LogoutResponse (Session Logout Response)
#define TID_MMPARAMETERDEFINITIONREQUEST 10303  // < MMParameterDefinitionRequest (Market Maker Parameter Definition)
#define TID_MMPARAMETERDEFINITIONRESPONSE \
  10304                              // < MMParameterDefinitionResponse (Market Maker Parameter Definition Response)
#define TID_MASSQUOTEREQUEST 10405   // < MassQuoteRequest (Mass Quote)
#define TID_MASSQUOTERESPONSE 10406  // < MassQuoteResponse (Mass Quote Response)
#define TID_MODIFYORDERCOMPLEXREQUEST 10114  // < ModifyOrderComplexRequest (Replace Order Multi Leg)
#define TID_MODIFYORDERNRRESPONSE 10108      // < ModifyOrderNRResponse (Replace Order Response (Lean Order))
#define TID_MODIFYORDERRESPONSE 10107        // < ModifyOrderResponse (Replace Order Response (Standard Order))
#define TID_MODIFYORDERSINGLEREQUEST 10106   // < ModifyOrderSingleRequest (Replace Order Single)
#define TID_MODIFYORDERSINGLESHORTREQUEST \
  10126                                       // < ModifyOrderSingleShortRequest (Replace Order Single (short layout))
#define TID_MODIFYTESTRADEREQUEST 10601       // < ModifyTESTradeRequest (Modify TES Trade Request)
#define TID_NEWORDERCOMPLEXREQUEST 10113      // < NewOrderComplexRequest (New Order Multi Leg)
#define TID_NEWORDERNRRESPONSE 10102          // < NewOrderNRResponse (New Order Response (Lean Order))
#define TID_NEWORDERRESPONSE 10101            // < NewOrderResponse (New Order Response (Standard Order))
#define TID_NEWORDERSINGLEREQUEST 10100       // < NewOrderSingleRequest (New Order Single)
#define TID_NEWORDERSINGLESHORTREQUEST 10125  // < NewOrderSingleShortRequest (New Order Single (short layout))
#define TID_NEWSBROADCAST 10031               // < NewsBroadcast (News)
#define TID_ORDEREXECNOTIFICATION 10104       // < OrderExecNotification (Book Order Execution)
#define TID_ORDEREXECREPORTBROADCAST 10117    // < OrderExecReportBroadcast (Extended Order Information)
#define TID_ORDEREXECRESPONSE 10103           // < OrderExecResponse (Immediate Execution Response)
#define TID_PARTYACTIONREPORT 10042           // < PartyActionReport (Party Action Report)
#define TID_PARTYENTITLEMENTSUPDATEREPORT 10034  // < PartyEntitlementsUpdateReport (Entitlement Notification)
#define TID_QUOTEACTIVATIONNOTIFICATION 10411    // < QuoteActivationNotification (Quote Activation Notification)
#define TID_QUOTEACTIVATIONREQUEST 10403         // < QuoteActivationRequest (Quote Activation Request)
#define TID_QUOTEACTIVATIONRESPONSE 10404        // < QuoteActivationResponse (Quote Activation Response)
#define TID_QUOTEEXECUTIONREPORT 10407           // < QuoteExecutionReport (Quote Execution Notification)
#define TID_RFQREQUEST 10401                     // < RFQRequest (Quote Request)
#define TID_RFQRESPONSE 10402                    // < RFQResponse (Quote Request Response)
#define TID_REJECT 10010                         // < Reject (Reject)
#define TID_RETRANSMITMEMESSAGEREQUEST 10026     // < RetransmitMEMessageRequest (Retransmit (Order/Quote Event))
#define TID_RETRANSMITMEMESSAGERESPONSE \
  10027                                       // < RetransmitMEMessageResponse (Retransmit Response (Order/Quote Event))
#define TID_RETRANSMITREQUEST 10008           // < RetransmitRequest (Retransmit)
#define TID_RETRANSMITRESPONSE 10009          // < RetransmitResponse (Retransmit Response)
#define TID_RISKNOTIFICATIONBROADCAST 10033   // < RiskNotificationBroadcast (Risk Notification)
#define TID_SRQSCREATEDEALNOTIFICATION 10708  // < SRQSCreateDealNotification (SRQS Create Deal Notification)
#define TID_SRQSDEALNOTIFICATION 10709        // < SRQSDealNotification (SRQS Deal Notification)
#define TID_SRQSDEALRESPONSE 10705            // < SRQSDealResponse (SRQS Deal Response)
#define TID_SRQSENTERQUOTEREQUEST 10702       // < SRQSEnterQuoteRequest (SRQS Enter Quote Request)
#define TID_SRQSHITQUOTEREQUEST 10704         // < SRQSHitQuoteRequest (SRQS Hit Quote Request)
#define TID_SRQSNEGOTIATIONNOTIFICATION \
  10713  // < SRQSNegotiationNotification (SRQS Negotiation Notification for Respondent)
#define TID_SRQSNEGOTIATIONREQUESTERNOTIFICATION \
  10712  // < SRQSNegotiationRequesterNotification (SRQS Negotiation Notification for Requester)
#define TID_SRQSNEGOTIATIONSTATUSNOTIFICATION \
  10715  // < SRQSNegotiationStatusNotification (SRQS Negotiation Status Notification)
#define TID_SRQSOPENNEGOTIATIONNOTIFICATION \
  10711  // < SRQSOpenNegotiationNotification (SRQS Open Negotiation Notification for Respondent)
#define TID_SRQSOPENNEGOTIATIONREQUEST 10700  // < SRQSOpenNegotiationRequest (SRQS Open Negotiation Request)
#define TID_SRQSOPENNEGOTIATIONREQUESTERNOTIFICATION \
  10710  // < SRQSOpenNegotiationRequesterNotification (SRQS Open Negotiation Notification for Requester)
#define TID_SRQSQUOTENOTIFICATION 10707         // < SRQSQuoteNotification (SRQS Quote Notification)
#define TID_SRQSQUOTERESPONSE 10703             // < SRQSQuoteResponse (SRQS Quote Response)
#define TID_SRQSSTATUSBROADCAST 10714           // < SRQSStatusBroadcast (SRQS Status Notification)
#define TID_SRQSUPDATEDEALSTATUSREQUEST 10706   // < SRQSUpdateDealStatusRequest (SRQS Update Deal Request)
#define TID_SRQSUPDATENEGOTIATIONREQUEST 10701  // < SRQSUpdateNegotiationRequest (SRQS Update Negotiation Request)
#define TID_SERVICEAVAILABILITYBROADCAST 10030  // < ServiceAvailabilityBroadcast (Service Availability)
#define TID_SERVICEAVAILABILITYMARKETBROADCAST \
  10044                                  // < ServiceAvailabilityMarketBroadcast (Service Availability Market)
#define TID_SUBSCRIBEREQUEST 10025       // < SubscribeRequest (Subscribe)
#define TID_SUBSCRIBERESPONSE 10005      // < SubscribeResponse (Subscribe Response)
#define TID_TESAPPROVEBROADCAST 10607    // < TESApproveBroadcast (Approve TES Trade Broadcast)
#define TID_TESBROADCAST 10604           // < TESBroadcast (TES Broadcast)
#define TID_TESDELETEBROADCAST 10606     // < TESDeleteBroadcast (Delete TES Trade Broadcast)
#define TID_TESEXECUTIONBROADCAST 10610  // < TESExecutionBroadcast (TES Execution Broadcast)
#define TID_TESRESPONSE 10611            // < TESResponse (TES Response)
#define TID_TESTRADEBROADCAST 10614      // < TESTradeBroadcast (TES Trade Broadcast)
#define TID_TESTRADINGSESSIONSTATUSBROADCAST 10615  // < TESTradingSessionStatusBroadcast (TES Status Broadcast )
#define TID_TESUPLOADBROADCAST 10613                // < TESUploadBroadcast (TES Trade Upload Broadcast )
#define TID_TMTRADINGSESSIONSTATUSBROADCAST 10501   // < TMTradingSessionStatusBroadcast (Trade Notification Status)
#define TID_THROTTLEUPDATENOTIFICATION 10028        // < ThrottleUpdateNotification (Throttle Update Notification)
#define TID_TRADEBROADCAST 10500                    // < TradeBroadcast (Trade Notification)
#define TID_TRADINGSESSIONSTATUSBROADCAST 10307     // < TradingSessionStatusBroadcast (Trading Session Event)
#define TID_UNSUBSCRIBEREQUEST 10006                // < UnsubscribeRequest (Unsubscribe)
#define TID_UNSUBSCRIBERESPONSE 10007               // < UnsubscribeResponse (Unsubscribe Response)
#define TID_UPLOADTESTRADEREQUEST 10612             // < UploadTESTradeRequest (Upload TES Trade Request)
#define TID_USERLOGINREQUEST 10018                  // < UserLoginRequest (User Logon)
#define TID_USERLOGINRESPONSE 10019                 // < UserLoginResponse (User Logon Response)
#define TID_USERLOGOUTREQUEST 10029                 // < UserLogoutRequest (User Logout)
#define TID_USERLOGOUTRESPONSE 10024                // < UserLogoutResponse (User Logout Response)

#define MAX_ADDCOMPLEXINSTRUMENTREQUEST_INSTRMTLEGGRP 20
#define MAX_ADDCOMPLEXINSTRUMENTRESPONSE_INSTRMTLEGGRP 20
#define MAX_DELETEALLORDERBROADCAST_NOTAFFECTEDORDERSGRP 500
#define MAX_DELETEALLORDERBROADCAST_AFFECTEDORDGRP 500
#define MAX_DELETEALLORDERRESPONSE_NOTAFFECTEDORDERSGRP 500
#define MAX_DELETEALLORDERRESPONSE_AFFECTEDORDGRP 500
#define MAX_DELETEALLQUOTEBROADCAST_NOTAFFECTEDSECURITIESGRP 500
#define MAX_DELETEALLQUOTERESPONSE_NOTAFFECTEDSECURITIESGRP 500
#define MAX_ENTERTESTRADEREQUEST_SIDEALLOCGRP 30
#define MAX_ENTERTESTRADEREQUEST_TRDINSTRMNTLEGGRP 20
#define MAX_ENTERTESTRADEREQUEST_INSTRUMENTEVENTGRP 2
#define MAX_ENTERTESTRADEREQUEST_INSTRUMENTATTRIBUTEGRP 6
#define MAX_ENTERTESTRADEREQUEST_UNDERLYINGSTIPGRP 1
#define MAX_INQUIREENRICHMENTRULEIDLISTRESPONSE_ENRICHMENTRULESGRP 400
#define MAX_INQUIREMMPARAMETERRESPONSE_MMPARAMETERGRP 9
#define MAX_INQUIRESESSIONLISTRESPONSE_SESSIONSGRP 1000
#define MAX_INQUIREUSERRESPONSE_PARTYDETAILSGRP 1000
#define MAX_MASSQUOTEREQUEST_QUOTEENTRYGRP 100
#define MAX_MASSQUOTERESPONSE_QUOTEENTRYACKGRP 100
#define MAX_MODIFYORDERCOMPLEXREQUEST_LEGORDGRP 20
#define MAX_MODIFYTESTRADEREQUEST_SIDEALLOCGRP 30
#define MAX_MODIFYTESTRADEREQUEST_TRDINSTRMNTLEGGRP 20
#define MAX_NEWORDERCOMPLEXREQUEST_LEGORDGRP 20
#define MAX_ORDEREXECNOTIFICATION_FILLSGRP 100
#define MAX_ORDEREXECNOTIFICATION_INSTRMNTLEGEXECGRP 600
#define MAX_ORDEREXECREPORTBROADCAST_LEGORDGRP 20
#define MAX_ORDEREXECREPORTBROADCAST_FILLSGRP 100
#define MAX_ORDEREXECREPORTBROADCAST_INSTRMNTLEGEXECGRP 600
#define MAX_ORDEREXECRESPONSE_FILLSGRP 100
#define MAX_ORDEREXECRESPONSE_INSTRMNTLEGEXECGRP 600
#define MAX_QUOTEACTIVATIONNOTIFICATION_NOTAFFECTEDSECURITIESGRP 500
#define MAX_QUOTEACTIVATIONRESPONSE_NOTAFFECTEDSECURITIESGRP 500
#define MAX_QUOTEEXECUTIONREPORT_QUOTEEVENTGRP 100
#define MAX_QUOTEEXECUTIONREPORT_QUOTELEGEXECGRP 600
#define MAX_SRQSCREATEDEALNOTIFICATION_SRQSTRDINSTRMNTLEGGRP 20
#define MAX_SRQSNEGOTIATIONREQUESTERNOTIFICATION_TARGETPARTIES 50
#define MAX_SRQSOPENNEGOTIATIONNOTIFICATION_QUOTREQLEGSGRP 20
#define MAX_SRQSOPENNEGOTIATIONREQUEST_QUOTREQLEGSGRP 20
#define MAX_SRQSOPENNEGOTIATIONREQUEST_TARGETPARTIES 50
#define MAX_SRQSOPENNEGOTIATIONREQUESTERNOTIFICATION_QUOTREQLEGSGRP 20
#define MAX_SRQSOPENNEGOTIATIONREQUESTERNOTIFICATION_TARGETPARTIES 50
#define MAX_SRQSUPDATENEGOTIATIONREQUEST_TARGETPARTIES 50
#define MAX_TESAPPROVEBROADCAST_TRDINSTRMNTLEGGRP 20
#define MAX_TESAPPROVEBROADCAST_INSTRUMENTEVENTGRP 2
#define MAX_TESAPPROVEBROADCAST_INSTRUMENTATTRIBUTEGRP 6
#define MAX_TESAPPROVEBROADCAST_UNDERLYINGSTIPGRP 1
#define MAX_TESBROADCAST_SIDEALLOCGRPBC 30
#define MAX_TESBROADCAST_TRDINSTRMNTLEGGRP 20
#define MAX_TESBROADCAST_INSTRUMENTEVENTGRP 2
#define MAX_TESBROADCAST_INSTRUMENTATTRIBUTEGRP 6
#define MAX_TESBROADCAST_UNDERLYINGSTIPGRP 1
#define MAX_TESUPLOADBROADCAST_SIDEALLOCEXTGRP 30
#define MAX_TESUPLOADBROADCAST_TRDINSTRMNTLEGGRP 20
#define MAX_TESUPLOADBROADCAST_INSTRUMENTEVENTGRP 2
#define MAX_TESUPLOADBROADCAST_INSTRUMENTATTRIBUTEGRP 6
#define MAX_TESUPLOADBROADCAST_UNDERLYINGSTIPGRP 1
#define MAX_UPLOADTESTRADEREQUEST_SIDEALLOCEXTGRP 30
#define MAX_UPLOADTESTRADEREQUEST_TRDINSTRMNTLEGGRP 20
#define MAX_UPLOADTESTRADEREQUEST_INSTRUMENTEVENTGRP 2
#define MAX_UPLOADTESTRADEREQUEST_INSTRUMENTATTRIBUTEGRP 6
#define MAX_UPLOADTESTRADEREQUEST_UNDERLYINGSTIPGRP 1

#define LEN_ACCOUNT 2
#define LEN_APPLBEGMSGID 16
#define LEN_APPLENDMSGID 16
#define ENUM_APPLID_TRADE 1
#define ENUM_APPLID_NEWS 2
#define ENUM_APPLID_SERVICE_AVAILABILITY 3
#define ENUM_APPLID_SESSION_DATA 4
#define ENUM_APPLID_LISTENER_DATA 5
#define ENUM_APPLID_RISKCONTROL 6
#define ENUM_APPLID_TES_MAINTENANCE 7
#define ENUM_APPLID_TES_TRADE 8
#define ENUM_APPLID_SRQS_MAINTENANCE 9
#define ENUM_APPLID_SERVICE_AVAILABILITY_MARKET 10
#define ENUM_APPLIDSTATUS_OUTBOUND_CONVERSION_ERROR 105
#define LEN_APPLMSGID 16
#define ENUM_APPLRESENDFLAG_FALSE 0
#define ENUM_APPLRESENDFLAG_TRUE 1
#define ENUM_APPLSEQINDICATOR_NO_RECOVERY_REQUIRED 0
#define ENUM_APPLSEQINDICATOR_RECOVERY_REQUIRED 1
#define ENUM_APPLSEQSTATUS_UNAVAILABLE 0
#define ENUM_APPLSEQSTATUS_AVAILABLE 1
#define LEN_APPLUSAGEORDERS 1
#define ENUM_APPLUSAGEORDERS_AUTOMATED "A"
#define ENUM_APPLUSAGEORDERS_MANUAL "M"
#define ENUM_APPLUSAGEORDERS_AUTOSELECT "B"
#define ENUM_APPLUSAGEORDERS_NONE "N"
#define LEN_APPLUSAGEQUOTES 1
#define ENUM_APPLUSAGEQUOTES_AUTOMATED "A"
#define ENUM_APPLUSAGEQUOTES_MANUAL "M"
#define ENUM_APPLUSAGEQUOTES_AUTOSELECT "B"
#define ENUM_APPLUSAGEQUOTES_NONE "N"
#define LEN_APPLICATIONSYSTEMNAME 30
#define LEN_APPLICATIONSYSTEMVENDOR 30
#define LEN_APPLICATIONSYSTEMVERSION 30
#define LEN_COMPLIANCETEXT 20
#define LEN_CORESRVC 100
#define ENUM_CORESRVC_TRADING \
  "Order and Quote Management                                                                          "
#define ENUM_CORESRVC_ORDER_QUOTE_RETRANSMISSION \
  "Retransmission of Order and Quote Events                                                            "
#define ENUM_CORESRVC_TRADE_RETRANSMISSION \
  "Trades                                                                                              "
#define ENUM_CORESRVC_TES \
  "T7 Entry                                                                                            "
#define ENUM_CORESRVC_SRQS \
  "Selective Request for Quote Service                                                                 "
#define ENUM_CORESRVC_NONE \
  "None                                                                                                "
#define ENUM_CROSSEDINDICATOR_NO_CROSSING 0
#define ENUM_CROSSEDINDICATOR_CROSS_REJECTED 1
#define LEN_CUSTORDERHANDLINGINST 1
#define LEN_DEFAULTCSTMAPPLVERID 30
#define LEN_DEFAULTCSTMAPPLVERSUBID 5
#define ENUM_DEFAULTCSTMAPPLVERSUBID_CASH "C0002"
#define ENUM_DEFAULTCSTMAPPLVERSUBID_DERIVATIVES "D0002"
#define ENUM_DELETEREASON_NO_SPECIAL_REASON 100
#define ENUM_DELETEREASON_TAS_CHANGE 101
#define ENUM_DELETEREASON_INTRADAY_EXPIRATION 102
#define ENUM_DELETEREASON_RISK_EVENT 103
#define ENUM_DELETEREASON_STOP_TRADING 104
#define ENUM_DELETEREASON_INSTRUMENT_DELETION 105
#define ENUM_DELETEREASON_INSTRUMENT_SUSPENSION 106
#define ENUM_EVENTTYPE_SWAP_START_DATE 8
#define ENUM_EVENTTYPE_SWAP_END_DATE 9
#define ENUM_EXECINST_H 1
#define ENUM_EXECINST_Q 2
#define ENUM_EXECINST_H_Q 3
#define ENUM_EXECINST_H_6 5
#define ENUM_EXECINST_Q_6 6
#define ENUM_EXECRESTATEMENTREASON_CORPORATE_ACTION 0
#define ENUM_EXECRESTATEMENTREASON_ORDER_BOOK_RESTATEMENT 1
#define ENUM_EXECRESTATEMENTREASON_EXCHANGE_OPTION 8
#define ENUM_EXECRESTATEMENTREASON_ORDER_ADDED 101
#define ENUM_EXECRESTATEMENTREASON_ORDER_MODIFIED 102
#define ENUM_EXECRESTATEMENTREASON_ORDER_CANCELLED 103
#define ENUM_EXECRESTATEMENTREASON_IOC_ORDER_CANCELLED 105
#define ENUM_EXECRESTATEMENTREASON_FOK_ORDER_CANCELLED 107
#define ENUM_EXECRESTATEMENTREASON_BOOK_ORDER_EXECUTED 108
#define ENUM_EXECRESTATEMENTREASON_INSTRUMENT_STATE_CHANGE 122
#define ENUM_EXECRESTATEMENTREASON_MARKET_ORDER_TRIGGERED 135
#define ENUM_EXECRESTATEMENTREASON_END_OF_DAY_PROCESSING 146
#define ENUM_EXECRESTATEMENTREASON_ORDER_EXPIRATION 148
#define ENUM_EXECRESTATEMENTREASON_CAO_ORDER_ACTIVATED 149
#define ENUM_EXECRESTATEMENTREASON_CAO_ORDER_INACTIVATED 150
#define ENUM_EXECRESTATEMENTREASON_OAO_ORDER_ACTIVATED 151
#define ENUM_EXECRESTATEMENTREASON_OAO_ORDER_INACTIVATED 152
#define ENUM_EXECRESTATEMENTREASON_AAO_ORDER_ACTIVATED 153
#define ENUM_EXECRESTATEMENTREASON_AAO_ORDER_INACTIVATED 154
#define ENUM_EXECRESTATEMENTREASON_ORDER_REFRESHED 155
#define ENUM_EXECRESTATEMENTREASON_OCO_ORDER_TRIGGERED 164
#define ENUM_EXECRESTATEMENTREASON_STOP_ORDER_TRIGGERED 172
#define ENUM_EXECRESTATEMENTREASON_OWNERSHIP_CHANGED 181
#define ENUM_EXECRESTATEMENTREASON_ORDER_CANCELLATION_PENDING 197
#define ENUM_EXECRESTATEMENTREASON_PENDING_CANCELLATION_EXECUTED 199
#define ENUM_EXECRESTATEMENTREASON_BOC_ORDER_CANCELLED 212
#define ENUM_EXECRESTATEMENTREASON_TRAILING_STOP_UPDATE 213
#define ENUM_EXECRESTATEMENTREASON_EXCEEDS_MAXIMUM_QUANTITY 237
#define ENUM_EXECRESTATEMENTREASON_INVALID_LIMIT_PRICE 238
#define ENUM_EXECRESTATEMENTREASON_USER_DOES_NOT_EXIST 241
#define ENUM_EXECRESTATEMENTREASON_SESSION_DOES_NOT_EXIST 242
#define ENUM_EXECRESTATEMENTREASON_INVALID_STOP_PRICE 243
#define ENUM_EXECRESTATEMENTREASON_INSTRUMENT_DOES_NOT_EXIST 245
#define ENUM_EXECRESTATEMENTREASON_BUSINESS_UNIT_RISK_EVENT 246
#define ENUM_EXECRESTATEMENTREASON_DIVIDEND_PAYMENT 292
#define ENUM_EXECRESTATEMENTREASON_LAST_TRADING_DAY 294
#define ENUM_EXECRESTATEMENTREASON_TRADING_PARAMETER_CHANGE 295
#define ENUM_EXECRESTATEMENTREASON_CURRENCY_CHANGE 296
#define ENUM_EXECRESTATEMENTREASON_PRODUCT_ASSIGNMENT_CHANGE 297
#define ENUM_EXECRESTATEMENTREASON_REFERENCE_PRICE_CHANGE 298
#define ENUM_EXECRESTATEMENTREASON_TICK_RULE_CHANGE 300
#define LEN_EXECTYPE 1
#define ENUM_EXECTYPE_NEW "0"
#define ENUM_EXECTYPE_CANCELED "4"
#define ENUM_EXECTYPE_REPLACED "5"
#define ENUM_EXECTYPE_PENDING_CANCEL_E "6"
#define ENUM_EXECTYPE_SUSPENDED "9"
#define ENUM_EXECTYPE_RESTATED "D"
#define ENUM_EXECTYPE_TRIGGERED "L"
#define ENUM_EXECTYPE_TRADE "F"
#define ENUM_EXECUTINGTRADERQUALIFIER_ALGO 22
#define ENUM_EXECUTINGTRADERQUALIFIER_HUMAN 24
#define ENUM_EXERCISESTYLE_EUROPEAN 0
#define ENUM_EXERCISESTYLE_AMERICAN 1
#define LEN_FIXCLORDID 20
#define LEN_FIXENGINENAME 30
#define LEN_FIXENGINEVENDOR 30
#define LEN_FIXENGINEVERSION 30
#define LEN_FIXQUOTERESPONSEID 20
#define ENUM_FILLLIQUIDITYIND_ADDED_LIQUIDITY 1
#define ENUM_FILLLIQUIDITYIND_REMOVED_LIQUIDITY 2
#define ENUM_FILLLIQUIDITYIND_TRIGGERED_STOP_ORDER 5
#define ENUM_FILLLIQUIDITYIND_TRIGGERED_OCO_ORDER 6
#define ENUM_FILLLIQUIDITYIND_TRIGGERED_MARKET_ORDER 7
#define LEN_FIRMNEGOTIATIONID 20
#define LEN_FIRMTRADEID 20
#define LEN_FREETEXT1 12
#define LEN_FREETEXT2 12
#define LEN_FREETEXT3 12
#define LEN_FUNCCATEGORY 100
#define ENUM_FUNCCATEGORY_ORDER_HANDLING \
  "Order Handling                                                                                      "
#define ENUM_FUNCCATEGORY_SESSION_LAYER \
  "Session Layer                                                                                       "
#define ENUM_FUNCCATEGORY_QUOTE_HANDLING \
  "Quote Handling                                                                                      "
#define ENUM_FUNCCATEGORY_BEST_QUOTE_HANDLING \
  "Best Quote Handling                                                                                 "
#define ENUM_FUNCCATEGORY_QUOTE_AND_CROSS_REQUEST \
  "Quote and Cross Request                                                                             "
#define ENUM_FUNCCATEGORY_STRATEGY_CREATION \
  "Strategy Creation                                                                                   "
#define ENUM_FUNCCATEGORY_FLEXIBLE_INSTRUMENT_CREATION \
  "Flexible Instrument Creation                                                                        "
#define ENUM_FUNCCATEGORY_TES_TRADING \
  "TES Trading                                                                                         "
#define ENUM_FUNCCATEGORY_SRQS \
  "Selective Request for Quote Service                                                                 "
#define ENUM_FUNCCATEGORY_OTHER \
  "Other                                                                                               "
#define ENUM_FUNCCATEGORY_BROADCAST \
  "Broadcast                                                                                           "
#define ENUM_GATEWAYSTATUS_STANDBY 0
#define ENUM_GATEWAYSTATUS_ACTIVE 1
#define LEN_HEADLINE 256
#define ENUM_HEDGETYPE_DURATION_HEDGE 0
#define ENUM_HEDGETYPE_NOMINAL_HEDGE 1
#define ENUM_HEDGETYPE_PRICE_FACTOR_HEDGE 2
#define ENUM_IMPLIEDMARKETINDICATOR_NOT_IMPLIED 0
#define ENUM_IMPLIEDMARKETINDICATOR_IMPLIED_IN_OUT 3
#define ENUM_INSTRATTRIBTYPE_VARIABLE_RATE 5
#define ENUM_INSTRATTRIBTYPE_COUPON_RATE 100
#define ENUM_INSTRATTRIBTYPE_OFFSET_TO_THE_VARIABLE_COUPON_RATE 101
#define ENUM_INSTRATTRIBTYPE_SWAP_CUSTOMER_1 102
#define ENUM_INSTRATTRIBTYPE_SWAP_CUSTOMER_2 103
#define ENUM_INSTRATTRIBTYPE_CASH_BASKET_REFERENCE 104
#define LEN_INSTRATTRIBVALUE 32
#define LEN_LASTENTITYPROCESSED 16
#define ENUM_LASTFRAGMENT_NOT_LAST_MESSAGE 0
#define ENUM_LASTFRAGMENT_LAST_MESSAGE 1
#define ENUM_LASTMKT_XEUR 1
#define ENUM_LASTMKT_XEEE 2
#define ENUM_LASTMKT_XETR 3
#define ENUM_LASTMKT_XVIE 4
#define ENUM_LASTMKT_XDUB 5
#define ENUM_LEAVESQTYDISCLOSUREINSTRUCTION_NO 0
#define ENUM_LEAVESQTYDISCLOSUREINSTRUCTION_YES 1
#define LEN_LEGACCOUNT 2
#define LEN_LEGPOSITIONEFFECT 1
#define ENUM_LEGPOSITIONEFFECT_CLOSE "C"
#define ENUM_LEGPOSITIONEFFECT_OPEN "O"
#define ENUM_LEGSECURITYTYPE_MULTILEG_INSTRUMENT 1
#define ENUM_LEGSECURITYTYPE_UNDERLYING_LEG 2
#define ENUM_LEGSIDE_BUY 1
#define ENUM_LEGSIDE_SELL 2
#define LEN_LISTUPDATEACTION 1
#define ENUM_LISTUPDATEACTION_ADD "A"
#define ENUM_LISTUPDATEACTION_DELETE "D"
#define ENUM_MARKETID_XEUR 1
#define ENUM_MARKETID_XEEE 2
#define ENUM_MARKETID_XETR 3
#define ENUM_MARKETID_XVIE 4
#define ENUM_MARKETID_XDUB 5
#define ENUM_MASSACTIONREASON_NO_SPECIAL_REASON 0
#define ENUM_MASSACTIONREASON_STOP_TRADING 1
#define ENUM_MASSACTIONREASON_EMERGENCY 2
#define ENUM_MASSACTIONREASON_MARKET_MAKER_PROTECTION 3
#define ENUM_MASSACTIONREASON_SESSION_LOSS 6
#define ENUM_MASSACTIONREASON_DUPLICATE_SESSION_LOGIN 7
#define ENUM_MASSACTIONREASON_CLEARING_RISK_CONTROL 8
#define ENUM_MASSACTIONREASON_PRODUCT_STATE_HALT 105
#define ENUM_MASSACTIONREASON_PRODUCT_STATE_HOLIDAY 106
#define ENUM_MASSACTIONREASON_INSTRUMENT_SUSPENDED 107
#define ENUM_MASSACTIONREASON_COMPLEX_INSTRUMENT_DELETION 109
#define ENUM_MASSACTIONREASON_VOLATILITY_INTERRUPTION 110
#define ENUM_MASSACTIONREASON_PRODUCT_TEMPORARILY_NOT_TRADEABLE 111
#define ENUM_MASSACTIONREASON_INSTRUMENT_STOPPED 113
#define ENUM_MASSACTIONTYPE_SUSPEND_QUOTES 1
#define ENUM_MASSACTIONTYPE_RELEASE_QUOTES 2
#define ENUM_MATCHSUBTYPE_OPENING_AUCTION 1
#define ENUM_MATCHSUBTYPE_CLOSING_AUCTION 2
#define ENUM_MATCHSUBTYPE_INTRADAY_AUCTION 3
#define ENUM_MATCHSUBTYPE_CIRCUIT_BREAKER_AUCTION 4
#define ENUM_MATCHTYPE_CONFIRMED_TRADE_REPORT 3
#define ENUM_MATCHTYPE_AUTO_MATCH_INCOMING 4
#define ENUM_MATCHTYPE_CROSS_AUCTION 5
#define ENUM_MATCHTYPE_CALL_AUCTION 7
#define ENUM_MATCHTYPE_SYSTEMATIC_INTERNALISER 9
#define ENUM_MATCHTYPE_AUTO_MATCH_RESTING 11
#define ENUM_MATCHTYPE_AUTO_MATCH_AT_MID_POINT 12
#define ENUM_MATCHINGENGINESTATUS_UNAVAILABLE 0
#define ENUM_MATCHINGENGINESTATUS_AVAILABLE 1
#define LEN_MESSAGEEVENTSOURCE 1
#define ENUM_MESSAGEEVENTSOURCE_BROADCAST_TO_INITIATOR "I"
#define ENUM_MESSAGEEVENTSOURCE_BROADCAST_TO_APPROVER "A"
#define ENUM_MESSAGEEVENTSOURCE_BROADCAST_TO_REQUESTER "R"
#define ENUM_MESSAGEEVENTSOURCE_BROADCAST_TO_QUOTE_SUBMITTER "Q"
#define LEN_MSGTYPE 3
#define ENUM_MSGTYPE_HEARTBEAT "0  "
#define ENUM_MSGTYPE_REJECT "3  "
#define ENUM_MSGTYPE_LOGOUT "5  "
#define ENUM_MSGTYPE_EXECUTIONREPORT "8  "
#define ENUM_MSGTYPE_LOGON "A  "
#define ENUM_MSGTYPE_NEWORDERMULTILEG "AB "
#define ENUM_MSGTYPE_MULTILEGORDERCANCELREPLACE "AC "
#define ENUM_MSGTYPE_TRADECAPTUREREPORT "AE "
#define ENUM_MSGTYPE_TRADECAPTUREREPORTACK "AR "
#define ENUM_MSGTYPE_NEWS "B  "
#define ENUM_MSGTYPE_USERREQUEST "BE "
#define ENUM_MSGTYPE_USERRESPONSE "BF "
#define ENUM_MSGTYPE_APPLICATIONMESSAGEREQUEST "BW "
#define ENUM_MSGTYPE_APPLICATIONMESSAGEREQUESTACK "BX "
#define ENUM_MSGTYPE_APPLICATIONMESSAGEREPORT "BY "
#define ENUM_MSGTYPE_ORDERMASSACTIONREPORT "BZ "
#define ENUM_MSGTYPE_ORDERMASSACTIONREQUEST "CA "
#define ENUM_MSGTYPE_USERNOTIFICATION "CB "
#define ENUM_MSGTYPE_PARTYRISKLIMITSUPDATEREPORT "CR "
#define ENUM_MSGTYPE_PARTYENTITLEMENTSUPDATEREPORT "CZ "
#define ENUM_MSGTYPE_NEWORDERSINGLE "D  "
#define ENUM_MSGTYPE_ORDERCANCELREQUEST "F  "
#define ENUM_MSGTYPE_ORDERCANCELREPLACEREQUEST "G  "
#define ENUM_MSGTYPE_QUOTEREQUEST "R  "
#define ENUM_MSGTYPE_MARKETDATASNAPSHOTFULLREFRESH "W  "
#define ENUM_MSGTYPE_MASSQUOTEACKNOWLEDGEMENT "b  "
#define ENUM_MSGTYPE_SECURITYDEFINITIONREQUEST "c  "
#define ENUM_MSGTYPE_SECURITYDEFINITION "d  "
#define ENUM_MSGTYPE_TRADINGSESSIONSTATUS "h  "
#define ENUM_MSGTYPE_MASSQUOTE "i  "
#define ENUM_MSGTYPE_QUOTE "S  "
#define ENUM_MSGTYPE_QUOTEACK "CW "
#define ENUM_MSGTYPE_QUOTESTATUSREPORT "AI "
#define ENUM_MSGTYPE_QUOTERESPONSE "AJ "
#define ENUM_MSGTYPE_REQUESTACKNOWLEDGE "U1 "
#define ENUM_MSGTYPE_SESSIONDETAILSLISTREQUEST "U5 "
#define ENUM_MSGTYPE_SESSIONDETAILSLISTRESPONSE "U6 "
#define ENUM_MSGTYPE_QUOTEEXECUTIONREPORT "U8 "
#define ENUM_MSGTYPE_MMPARAMETERDEFINITIONREQUEST "U14"
#define ENUM_MSGTYPE_CROSSREQUEST "U16"
#define ENUM_MSGTYPE_MMPARAMETERREQUEST "U17"
#define ENUM_MSGTYPE_MMPARAMETERRESPONSE "U18"
#define ENUM_MSGTYPE_PARTYDETAILLISTREQUEST "CF "
#define ENUM_MSGTYPE_PARTYDETAILLISTREPORT "CG "
#define ENUM_MSGTYPE_TRADEENRICHMENTLISTREQUEST "U7 "
#define ENUM_MSGTYPE_TRADEENRICHMENTLISTREPORT "U9 "
#define ENUM_MSGTYPE_PARTYACTIONREPORT "DI "
#define ENUM_MULTILEGREPORTINGTYPE_SINGLE_SECURITY 1
#define ENUM_MULTILEGREPORTINGTYPE_INDIVIDUAL_LEG_OF_A_MULTILEG_SECURITY 2
#define ENUM_MULTILEGMODEL_PREDEFINED_MULTILEG_SECURITY 0
#define ENUM_MULTILEGMODEL_USER_DEFINED_MULTLEG 1
#define ENUM_MULTILEGPRICEMODEL_STANDARD 0
#define ENUM_MULTILEGPRICEMODEL_USERDEFINED 1
#define LEN_NETWORKMSGID 8
#define LEN_ORDSTATUS 1
#define ENUM_ORDSTATUS_NEW "0"
#define ENUM_ORDSTATUS_PARTIALLY_FILLED "1"
#define ENUM_ORDSTATUS_FILLED "2"
#define ENUM_ORDSTATUS_CANCELED "4"
#define ENUM_ORDSTATUS_PENDING_CANCEL "6"
#define ENUM_ORDSTATUS_SUSPENDED "9"
#define ENUM_ORDTYPE_MARKET 1
#define ENUM_ORDTYPE_LIMIT 2
#define ENUM_ORDTYPE_STOP 3
#define ENUM_ORDTYPE_STOP_LIMIT 4
#define ENUM_ORDERATTRIBUTELIQUIDITYPROVISION_Y 1
#define ENUM_ORDERATTRIBUTELIQUIDITYPROVISION_N 0
#define ENUM_ORDERATTRIBUTERISKREDUCTION_Y 1
#define ENUM_ORDERATTRIBUTERISKREDUCTION_N 0
#define LEN_ORDERCATEGORY 1
#define ENUM_ORDERCATEGORY_ORDER "1"
#define ENUM_ORDERCATEGORY_QUOTE "2"
#define LEN_ORDERROUTINGINDICATOR 1
#define ENUM_ORDERROUTINGINDICATOR_YES "Y"
#define ENUM_ORDERROUTINGINDICATOR_NO "N"
#define ENUM_ORDERSIDE_BUY 1
#define ENUM_ORDERSIDE_SELL 2
#define ENUM_OWNERSHIPINDICATOR_NO_CHANGE_OF_OWNERSHIP 0
#define ENUM_OWNERSHIPINDICATOR_CHANGE_TO_EXECUTING_TRADER 1
#define LEN_PAD1 1
#define LEN_PAD1_1 1
#define LEN_PAD2 2
#define LEN_PAD2_1 2
#define LEN_PAD2_2 2
#define LEN_PAD3 3
#define LEN_PAD4 4
#define LEN_PAD4_1 4
#define LEN_PAD5 5
#define LEN_PAD5_1 5
#define LEN_PAD6 6
#define LEN_PAD6_1 6
#define LEN_PAD6_2 6
#define LEN_PAD7 7
#define LEN_PAD7_1 7
#define ENUM_PARTYACTIONTYPE_HALT_TRADING 1
#define ENUM_PARTYACTIONTYPE_REINSTATE 2
#define LEN_PARTYDETAILDESKID 3
#define LEN_PARTYDETAILEXECUTINGTRADER 6
#define ENUM_PARTYDETAILROLEQUALIFIER_TRADER 10
#define ENUM_PARTYDETAILROLEQUALIFIER_HEAD_TRADER 11
#define ENUM_PARTYDETAILROLEQUALIFIER_SUPERVISOR 12
#define ENUM_PARTYDETAILSTATUS_ACTIVE 0
#define ENUM_PARTYDETAILSTATUS_SUSPEND 1
#define LEN_PARTYENTERINGTRADER 6
#define LEN_PARTYEXECUTINGFIRM 5
#define LEN_PARTYEXECUTINGTRADER 6
#define LEN_PARTYIDBENEFICIARY 9
#define ENUM_PARTYIDENTERINGFIRM_PARTICIPANT 1
#define ENUM_PARTYIDENTERINGFIRM_MARKETSUPERVISION 2
#define LEN_PARTYIDLOCATIONID 2
#define LEN_PARTYIDORDERORIGINATIONFIRM 7
#define ENUM_PARTYIDORIGINATIONMARKET_XKFE 1
#define ENUM_PARTYIDORIGINATIONMARKET_XTAF 2
#define LEN_PARTYIDPOSITIONACCOUNT 32
#define ENUM_PARTYIDSETTLEMENTLOCATION_CLEARSTREM_BANKING_FRANKFURT 1
#define ENUM_PARTYIDSETTLEMENTLOCATION_CLEARSTREM_BANKING_LUXEMBURG 2
#define ENUM_PARTYIDSETTLEMENTLOCATION_CLS_GROUP 3
#define ENUM_PARTYIDSETTLEMENTLOCATION_EUROCLEAR 4
#define LEN_PARTYIDTAKEUPTRADINGFIRM 5
#define ENUM_PARTYIDINVESTMENTDECISIONMAKERQUALIFIER_ALGO 22
#define ENUM_PARTYIDINVESTMENTDECISIONMAKERQUALIFIER_HUMAN 24
#define LEN_PASSWORD 32
#define LEN_POSITIONEFFECT 1
#define ENUM_POSITIONEFFECT_CLOSE "C"
#define ENUM_POSITIONEFFECT_OPEN "O"
#define ENUM_PRICEDISCLOSUREINSTRUCTION_NO 0
#define ENUM_PRICEDISCLOSUREINSTRUCTION_YES 1
#define ENUM_PRICEVALIDITYCHECKTYPE_NONE 0
#define ENUM_PRICEVALIDITYCHECKTYPE_OPTIONAL 1
#define ENUM_PRICEVALIDITYCHECKTYPE_MANDATORY 2
#define ENUM_PRODUCTCOMPLEX_SIMPLE_INSTRUMENT 1
#define ENUM_PRODUCTCOMPLEX_STANDARD_OPTION_STRATEGY 2
#define ENUM_PRODUCTCOMPLEX_NON_STANDARD_OPTION_STRATEGY 3
#define ENUM_PRODUCTCOMPLEX_VOLATILITY_STRATEGY 4
#define ENUM_PRODUCTCOMPLEX_FUTURES_SPREAD 5
#define ENUM_PRODUCTCOMPLEX_INTER_PRODUCT_SPREAD 6
#define ENUM_PRODUCTCOMPLEX_STANDARD_FUTURE_STRATEGY 7
#define ENUM_PRODUCTCOMPLEX_PACK_AND_BUNDLE 8
#define ENUM_PRODUCTCOMPLEX_STRIP 9
#define ENUM_PRODUCTCOMPLEX_FLEXIBLE_SIMPLE_INSTRUMENT 10
#define ENUM_PUTORCALL_PUT 0
#define ENUM_PUTORCALL_CALL 1
#define ENUM_QUOTECANCELTYPE_CANCEL_ALL_QUOTES 4
#define ENUM_QUOTEENTRYREJECTREASON_UNKNOWN_SECURITY 1
#define ENUM_QUOTEENTRYREJECTREASON_DUPLICATE_QUOTE 6
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_PRICE 8
#define ENUM_QUOTEENTRYREJECTREASON_NO_REFERENCE_PRICE_AVAILABLE 16
#define ENUM_QUOTEENTRYREJECTREASON_NO_SINGLE_SIDED_QUOTES 100
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_QUOTING_MODEL 103
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_SIZE 106
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_UNDERLYING_PRICE 107
#define ENUM_QUOTEENTRYREJECTREASON_BID_PRICE_NOT_REASONABLE 108
#define ENUM_QUOTEENTRYREJECTREASON_ASK_PRICE_NOT_REASONABLE 109
#define ENUM_QUOTEENTRYREJECTREASON_BID_PRICE_EXCEEDS_RANGE 110
#define ENUM_QUOTEENTRYREJECTREASON_ASK_PRICE_EXCEEDS_RANGE 111
#define ENUM_QUOTEENTRYREJECTREASON_INSTRUMENT_STATE_FREEZE 115
#define ENUM_QUOTEENTRYREJECTREASON_DELETION_ALREADY_PENDING 116
#define ENUM_QUOTEENTRYREJECTREASON_PRE_TRADE_RISK_SESSION_LIMIT_EXCEEDED 117
#define ENUM_QUOTEENTRYREJECTREASON_PRE_TRADE_RISK_BU_LIMIT_EXCEEDED 118
#define ENUM_QUOTEENTRYREJECTREASON_ENTITLEMENT_NOT_ASSIGNED_FOR_UNDERLYING 119
#define ENUM_QUOTEENTRYREJECTREASON_BID_VALUE_EXCEEDS_LIMIT 120
#define ENUM_QUOTEENTRYREJECTREASON_ASK_VALUE_EXCEEDS_LIMIT 121
#define ENUM_QUOTEENTRYREJECTREASON_NOT_TRADEABLE_FOR_BUSINESSUNIT 122
#define ENUM_QUOTEENTRYREJECTREASON_CURRENTLY_NOT_TRADEABLE_ON_BOOK 124
#define ENUM_QUOTEENTRYREJECTREASON_QUANTITY_LIMIT_EXCEEDED 125
#define ENUM_QUOTEENTRYREJECTREASON_VALUE_LIMIT_EXCEEDED 126
#define ENUM_QUOTEENTRYREJECTREASON_INVALID_QUOTE_SPREAD 127
#define ENUM_QUOTEENTRYREJECTREASON_CANT_PROC_IN_CURR_INSTR_STATE 131
#define ENUM_QUOTEENTRYSTATUS_ACCEPTED 0
#define ENUM_QUOTEENTRYSTATUS_REJECTED 5
#define ENUM_QUOTEENTRYSTATUS_REMOVED_AND_REJECTED 6
#define ENUM_QUOTEENTRYSTATUS_PENDING 10
#define ENUM_QUOTEEVENTLIQUIDITYIND_ADDED_LIQUIDITY 1
#define ENUM_QUOTEEVENTLIQUIDITYIND_REMOVED_LIQUIDITY 2
#define ENUM_QUOTEEVENTREASON_PENDING_CANCELLATION_EXECUTED 14
#define ENUM_QUOTEEVENTREASON_INVALID_PRICE 15
#define ENUM_QUOTEEVENTREASON_CROSS_REJECTED 16
#define ENUM_QUOTEEVENTSIDE_BUY 1
#define ENUM_QUOTEEVENTSIDE_SELL 2
#define ENUM_QUOTEEVENTTYPE_MODIFIED_QUOTE_SIDE 2
#define ENUM_QUOTEEVENTTYPE_REMOVED_QUOTE_SIDE 3
#define ENUM_QUOTEEVENTTYPE_PARTIALLY_FILLED 4
#define ENUM_QUOTEEVENTTYPE_FILLED 5
#define ENUM_QUOTEINSTRUCTION_DO_NOT_QUOTE 0
#define ENUM_QUOTEINSTRUCTION_QUOTE 1
#define ENUM_QUOTEREFPRICESOURCE_UNDERLYING 1
#define ENUM_QUOTEREFPRICESOURCE_CUSTOM_INDEX 2
#define LEN_QUOTEREQID 20
#define ENUM_QUOTESIZETYPE_TOTALSIZE 1
#define ENUM_QUOTESIZETYPE_OPENSIZE 2
#define ENUM_QUOTESTATUS_REMOVED 6
#define ENUM_QUOTESTATUS_EXPIRED 7
#define ENUM_QUOTESTATUS_ACTIVE 16
#define ENUM_QUOTETYPE_INDICATIVE 0
#define ENUM_QUOTETYPE_TRADEABLE 1
#define ENUM_REFAPPLID_TRADE 1
#define ENUM_REFAPPLID_NEWS 2
#define ENUM_REFAPPLID_SERVICE_AVAILABILITY 3
#define ENUM_REFAPPLID_SESSION_DATA 4
#define ENUM_REFAPPLID_LISTENER_DATA 5
#define ENUM_REFAPPLID_RISKCONTROL 6
#define ENUM_REFAPPLID_TES_MAINTENANCE 7
#define ENUM_REFAPPLID_TES_TRADE 8
#define ENUM_REFAPPLID_SRQS_MAINTENANCE 9
#define ENUM_REFAPPLID_SERVICE_AVAILABILITY_MARKET 10
#define LEN_REFAPPLLASTMSGID 16
#define ENUM_RELATEDPRODUCTCOMPLEX_STANDARD_OPTION_STRATEGY 2
#define ENUM_RELATEDPRODUCTCOMPLEX_NON_STANDARD_OPTION_STRATEGY 3
#define ENUM_RELATEDPRODUCTCOMPLEX_VOLATILITY_STRATEGY 4
#define ENUM_RELATEDPRODUCTCOMPLEX_FUTURES_SPREAD 5
#define ENUM_RELATEDPRODUCTCOMPLEX_INTER_PRODUCT_SPREAD 6
#define ENUM_RELATEDPRODUCTCOMPLEX_STANDARD_FUTURE_STRATEGY 7
#define ENUM_RELATEDPRODUCTCOMPLEX_PACK_AND_BUNDLE 8
#define ENUM_RELATEDPRODUCTCOMPLEX_STRIP 9
#define LEN_REQUESTINGPARTYCLEARINGFIRM 9
#define LEN_REQUESTINGPARTYENTERINGFIRM 9
#define ENUM_REQUESTINGPARTYIDENTERINGFIRM_PARTICIPANT 1
#define ENUM_REQUESTINGPARTYIDENTERINGFIRM_MARKETSUPERVISION 2
#define ENUM_REQUESTINGPARTYIDEXECUTINGSYSTEM_EUREXCLEARING 1
#define ENUM_REQUESTINGPARTYIDEXECUTINGSYSTEM_T7 2
#define ENUM_RISKLIMITACTION_QUEUEINBOUND 0
#define ENUM_RISKLIMITACTION_REJECT 2
#define ENUM_RISKLIMITACTION_WARNING 4
#define LEN_ROOTPARTYCLEARINGFIRM 5
#define LEN_ROOTPARTYCLEARINGORGANIZATION 4
#define LEN_ROOTPARTYENTERINGTRADER 6
#define LEN_ROOTPARTYEXECUTINGFIRM 5
#define LEN_ROOTPARTYEXECUTINGTRADER 6
#define LEN_ROOTPARTYIDBENEFICIARY 9
#define LEN_ROOTPARTYIDORDERORIGINATIONFIRM 7
#define LEN_ROOTPARTYIDPOSITIONACCOUNT 32
#define LEN_ROOTPARTYIDTAKEUPTRADINGFIRM 5
#define ENUM_SECONDARYGATEWAYSTATUS_STANDBY 0
#define ENUM_SECONDARYGATEWAYSTATUS_ACTIVE 1
#define ENUM_SELECTIVEREQUESTFORQUOTESERVICESTATUS_UNAVAILABLE 0
#define ENUM_SELECTIVEREQUESTFORQUOTESERVICESTATUS_AVAILABLE 1
#define ENUM_SESSIONMODE_HF 1
#define ENUM_SESSIONMODE_LF 2
#define ENUM_SESSIONMODE_GUI 3
#define ENUM_SESSIONREJECTREASON_REQUIRED_TAG_MISSING 1
#define ENUM_SESSIONREJECTREASON_VALUE_IS_INCORRECT 5
#define ENUM_SESSIONREJECTREASON_DECRYPTION_PROBLEM 7
#define ENUM_SESSIONREJECTREASON_INVALID_MSGID 11
#define ENUM_SESSIONREJECTREASON_INCORRECT_NUMINGROUP_COUNT 16
#define ENUM_SESSIONREJECTREASON_OTHER 99
#define ENUM_SESSIONREJECTREASON_THROTTLE_LIMIT_EXCEEDED 100
#define ENUM_SESSIONREJECTREASON_EXPOSURE_LIMIT_EXCEEDED 101
#define ENUM_SESSIONREJECTREASON_SERVICE_TEMPORARILY_NOT_AVAILABLE 102
#define ENUM_SESSIONREJECTREASON_SERVICE_NOT_AVAILABLE 103
#define ENUM_SESSIONREJECTREASON_RESULT_OF_TRANSACTION_UNKNOWN 104
#define ENUM_SESSIONREJECTREASON_OUTBOUND_CONVERSION_ERROR 105
#define ENUM_SESSIONREJECTREASON_HEARTBEAT_VIOLATION 152
#define ENUM_SESSIONREJECTREASON_INTERNAL_TECHNICAL_ERROR 200
#define ENUM_SESSIONREJECTREASON_VALIDATION_ERROR 210
#define ENUM_SESSIONREJECTREASON_USER_ALREADY_LOGGED_IN 211
#define ENUM_SESSIONREJECTREASON_SESSION_GATEWAY_ASSIGNMENT_EXPIRED 214
#define ENUM_SESSIONREJECTREASON_GATEWAY_NOT_RESERVED_TO_SESSION 215
#define ENUM_SESSIONREJECTREASON_GATEWAY_IS_STANDBY 216
#define ENUM_SESSIONREJECTREASON_SESSION_LOGIN_LIMIT_REACHED 217
#define ENUM_SESSIONREJECTREASON_PARTITION_NOT_REACHABLE_BY_HFGATEWAY 218
#define ENUM_SESSIONREJECTREASON_PARTITION_NOT_REACHABLE_BY_PSGATEWAY 219
#define ENUM_SESSIONREJECTREASON_NO_GATEWAY_AVAILABLE 222
#define ENUM_SESSIONREJECTREASON_USER_ENTITLEMENT_DATA_TIMEOUT 223
#define ENUM_SESSIONREJECTREASON_ORDER_NOT_FOUND 10000
#define ENUM_SESSIONREJECTREASON_PRICE_NOT_REASONABLE 10001
#define ENUM_SESSIONREJECTREASON_CLIENTORDERID_NOT_UNIQUE 10002
#define ENUM_SESSIONREJECTREASON_QUOTE_ACTIVATION_IN_PROGRESS 10003
#define ENUM_SESSIONREJECTREASON_BU_BOOK_ORDER_LIMIT_EXCEEDED 10004
#define ENUM_SESSIONREJECTREASON_SESSION_BOOK_ORDER_LIMIT_EXCEEDED 10005
#define ENUM_SESSIONREJECTREASON_STOP_BID_PRICE_NOT_REASONABLE 10006
#define ENUM_SESSIONREJECTREASON_STOP_ASK_PRICE_NOT_REASONABLE 10007
#define ENUM_SESSIONREJECTREASON_ORDER_NOT_EXECUTABLE_WITHIN_VALIDITY 10008
#define ENUM_SESSIONREJECTREASON_INVALID_TRADING_RESTRICTION_FOR_INSTRUMENT_STATE 10009
#define ENUM_SESSIONREJECTREASON_CREATE_CI_THROTTLE_EXCEEDED 10010
#define ENUM_SESSIONREJECTREASON_TRANSACTION_NOT_ALLOWED_IN_CURRENT_STATE 10011
#define ENUM_SESSIONSTATUS_ACTIVE 0
#define ENUM_SESSIONSTATUS_LOGOUT 4
#define ENUM_SESSIONSUBMODE_REGULAR_TRADING_SESSION 0
#define ENUM_SESSIONSUBMODE_FIX_TRADING_SESSION 1
#define ENUM_SESSIONSUBMODE_REGULAR_BACK_OFFICE_SESSION 2
#define LEN_SETTLMETHOD 1
#define ENUM_SETTLMETHOD_CASH_SETTLEMENT "C"
#define ENUM_SETTLMETHOD_PHYSICAL_SETTLEMENT "P"
#define ENUM_SIDE_BUY 1
#define ENUM_SIDE_SELL 2
#define ENUM_SIDEDISCLOSUREINSTRUCTION_NO 0
#define ENUM_SIDEDISCLOSUREINSTRUCTION_YES 1
#define ENUM_SIDELIQUIDITYIND_ADDED_LIQUIDITY 1
#define ENUM_SIDELIQUIDITYIND_REMOVED_LIQUIDITY 2
#define ENUM_SIDELIQUIDITYIND_AUCTION 4
#define ENUM_SKIPVALIDATIONS_FALSE 0
#define ENUM_SKIPVALIDATIONS_TRUE 1
#define LEN_SYMBOL 4
#define ENUM_T7ENTRYSERVICERTMSTATUS_UNAVAILABLE 0
#define ENUM_T7ENTRYSERVICERTMSTATUS_AVAILABLE 1
#define ENUM_T7ENTRYSERVICESTATUS_UNAVAILABLE 0
#define ENUM_T7ENTRYSERVICESTATUS_AVAILABLE 1
#define LEN_TARGETPARTYENTERINGTRADER 6
#define LEN_TARGETPARTYEXECUTINGFIRM 5
#define LEN_TARGETPARTYEXECUTINGTRADER 6
#define LEN_TARGETPARTYIDDESKID 3
#define LEN_TEXT 12
#define ENUM_TIMEINFORCE_DAY 0
#define ENUM_TIMEINFORCE_GTC 1
#define ENUM_TIMEINFORCE_IOC 3
#define ENUM_TIMEINFORCE_FOK 4
#define ENUM_TIMEINFORCE_GTX 5
#define ENUM_TIMEINFORCE_GTD 6
#define ENUM_TRADSESEVENT_START_OF_SERVICE 101
#define ENUM_TRADSESEVENT_MARKET_RESET 102
#define ENUM_TRADSESEVENT_END_OF_RESTATEMENT 103
#define ENUM_TRADSESEVENT_END_OF_DAY_SERVICE 104
#define ENUM_TRADSESEVENT_SERVICE_RESUMED 105
#define ENUM_TRADSESMODE_TESTING 1
#define ENUM_TRADSESMODE_SIMULATED 2
#define ENUM_TRADSESMODE_PRODUCTION 3
#define ENUM_TRADSESMODE_ACCEPTANCE 4
#define ENUM_TRADEALLOCSTATUS_PENDING 1
#define ENUM_TRADEALLOCSTATUS_APPROVED 2
#define ENUM_TRADEALLOCSTATUS_AUTO_APPROVED 3
#define ENUM_TRADEALLOCSTATUS_UPLOADED 4
#define ENUM_TRADEALLOCSTATUS_CANCELED 5
#define ENUM_TRADEMANAGERSTATUS_UNAVAILABLE 0
#define ENUM_TRADEMANAGERSTATUS_AVAILABLE 1
#define ENUM_TRADEPUBLISHINDICATOR_DO_NOT_PUBLISH_TRADE 0
#define ENUM_TRADEPUBLISHINDICATOR_PUBLISH_TRADE 1
#define LEN_TRADEREPORTID 20
#define LEN_TRADEREPORTTEXT 20
#define ENUM_TRADEREPORTTYPE_SUBMIT 0
#define ENUM_TRADEREPORTTYPE_ALLEGED 1
#define ENUM_TRADEREPORTTYPE_ACCEPT 2
#define ENUM_TRADEREPORTTYPE_DECLINE 3
#define ENUM_TRADEREPORTTYPE_NO_WAS_REPLACED 5
#define ENUM_TRADEREPORTTYPE_TRADE_REPORT_CANCEL 6
#define ENUM_TRADEREPORTTYPE_TRADE_BREAK 7
#define ENUM_TRADEREPORTTYPE_ALLEGED_NEW 11
#define ENUM_TRADEREPORTTYPE_ALLEGED_NO_WAS 13
#define ENUM_TRADEUNDERLYING_NO 1
#define ENUM_TRADEUNDERLYING_YES 2
#define ENUM_TRADINGCAPACITY_CUSTOMER 1
#define ENUM_TRADINGCAPACITY_PRINCIPAL 5
#define ENUM_TRADINGCAPACITY_MARKET_MAKER 6
#define ENUM_TRADINGCAPACITY_SYSTEMATIC_INTERNALISER 8
#define ENUM_TRADINGCAPACITY_RISKLESS_PRINCIPAL 9
#define ENUM_TRADINGSESSIONSUBID_OPENING_AUCTION 2
#define ENUM_TRADINGSESSIONSUBID_CLOSING_AUCTION 4
#define ENUM_TRADINGSESSIONSUBID_ANY_AUCTION 8
#define ENUM_TRANSFERREASON_OWNER 1
#define ENUM_TRANSFERREASON_CLEARER 2
#define ENUM_TRDRPTSTATUS_ACCEPTED 0
#define ENUM_TRDRPTSTATUS_REJECTED 1
#define ENUM_TRDRPTSTATUS_CANCELLED 2
#define ENUM_TRDRPTSTATUS_PENDING_NEW 4
#define ENUM_TRDRPTSTATUS_TERMINATED 7
#define ENUM_TRDTYPE_BLOCK_TRADE 1
#define ENUM_TRDTYPE_EXCHANGE_FOR_SWAP 12
#define ENUM_TRDTYPE_EXCHANGE_BASIS_FACILITY 55
#define ENUM_TRDTYPE_VOLA_TRADE 1000
#define ENUM_TRDTYPE_EFP_FIN_TRADE 1001
#define ENUM_TRDTYPE_EFP_INDEX_FUTURES_TRADE 1002
#define ENUM_TRDTYPE_TRADE_AT_MARKET 1004
#define ENUM_TRIGGERED_NOT_TRIGGERED 0
#define ENUM_TRIGGERED_TRIGGERED_STOP 1
#define ENUM_TRIGGERED_TRIGGERED_OCO 2
#define LEN_UNDERLYINGCURRENCY 3
#define LEN_UNDERLYINGISSUER 30
#define LEN_UNDERLYINGSECURITYDESC 30
#define LEN_UNDERLYINGSECURITYID 12
#define LEN_UNDERLYINGSTIPTYPE 7
#define ENUM_UNDERLYINGSTIPTYPE_PAY_FREQUENCY "PAYFREQ"
#define LEN_UNDERLYINGSTIPVALUE 32
#define ENUM_USERSTATUS_USER_FORCED_LOGOUT 7
#define ENUM_USERSTATUS_USER_STOPPED 10
#define ENUM_USERSTATUS_USER_RELEASED 11
#define ENUM_VALUECHECKTYPEVALUE_DO_NOT_CHECK 0
#define ENUM_VALUECHECKTYPEVALUE_CHECK 1
#define LEN_VARTEXT 2000

/*
 * End of DEPRECATED defines
 */

#if defined(__cplusplus) || defined(c_plusplus)
} /* close scope of 'extern "C"' declaration. */
#endif

#endif
