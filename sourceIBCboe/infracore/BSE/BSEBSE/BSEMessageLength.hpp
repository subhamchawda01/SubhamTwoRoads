// =====================================================================================
//
//       Filename:  BSEMessageLengths.hpp
//
//    Description:  Class to hold pre-computed bse message lengths, to avoid frequent calls to sizeof operator
//
//        Version:  1.0
//        Created:  11/08/2012 03:57:43 AM
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

#include <iostream>

namespace HFSAT {
namespace BSE {

class BSEMessageLength {
 public:
  unsigned int FillsGrpSeqT_MSG_LENGTH_;
  unsigned int InstrmntLegExecGrpSeqT_MSG_LENGTH_;
  unsigned int InstrmtLegGrpSeqT_MSG_LENGTH_;
  unsigned int LegOrdGrpSeqT_MSG_LENGTH_;
  unsigned int MMParameterGrpSeqT_MSG_LENGTH_;
  unsigned int MessageHeaderInCompT_MSG_LENGTH_;
  unsigned int MessageHeaderOutCompT_MSG_LENGTH_;
  unsigned int NRBCHeaderCompT_MSG_LENGTH_;
  unsigned int NRResponseHeaderMECompT_MSG_LENGTH_;
  unsigned int NotAffectedOrdersGrpSeqT_MSG_LENGTH_;
  unsigned int NotAffectedSecuritiesGrpSeqT_MSG_LENGTH_;
  unsigned int NotifHeaderCompT_MSG_LENGTH_;
  unsigned int QuoteEntryAckGrpSeqT_MSG_LENGTH_;
  unsigned int QuoteEntryGrpSeqT_MSG_LENGTH_;
  unsigned int QuoteEventGrpSeqT_MSG_LENGTH_;
  unsigned int QuoteLegExecGrpSeqT_MSG_LENGTH_;
  unsigned int RBCHeaderCompT_MSG_LENGTH_;
  unsigned int RBCHeaderMECompT_MSG_LENGTH_;
  unsigned int RequestHeaderCompT_MSG_LENGTH_;
  unsigned int ResponseHeaderCompT_MSG_LENGTH_;
  unsigned int ResponseHeaderMECompT_MSG_LENGTH_;
  unsigned int AddComplexInstrumentRequestT_MSG_LENGTH_;
  unsigned int AddComplexInstrumentResponseT_MSG_LENGTH_;
  unsigned int BroadcastErrorNotificationT_MSG_LENGTH_;
  unsigned int CrossRequestT_MSG_LENGTH_;
  unsigned int CrossRequestResponseT_MSG_LENGTH_;
  unsigned int DeleteAllOrderBroadcastT_MSG_LENGTH_;
  unsigned int DeleteAllOrderNRResponseT_MSG_LENGTH_;
  unsigned int DeleteAllOrderQuoteEventBroadcastT_MSG_LENGTH_;
  unsigned int DeleteAllOrderRequestT_MSG_LENGTH_;
  unsigned int DeleteAllOrderResponseT_MSG_LENGTH_;
  unsigned int DeleteAllQuoteBroadcastT_MSG_LENGTH_;
  unsigned int DeleteAllQuoteRequestT_MSG_LENGTH_;
  unsigned int DeleteAllQuoteResponseT_MSG_LENGTH_;
  unsigned int DeleteOrderBroadcastT_MSG_LENGTH_;
  unsigned int DeleteOrderComplexRequestT_MSG_LENGTH_;
  unsigned int DeleteOrderNRResponseT_MSG_LENGTH_;
  unsigned int DeleteOrderResponseT_MSG_LENGTH_;
  unsigned int DeleteOrderSingleRequestT_MSG_LENGTH_;
  unsigned int ForcedLogoutNotificationT_MSG_LENGTH_;
  unsigned int GatewayRequestT_MSG_LENGTH_;
  unsigned int GatewayResponseT_MSG_LENGTH_;
  unsigned int HeartbeatT_MSG_LENGTH_;
  unsigned int HeartbeatNotificationT_MSG_LENGTH_;
  unsigned int InquireMMParameterRequestT_MSG_LENGTH_;
  unsigned int InquireMMParameterResponseT_MSG_LENGTH_;
  unsigned int InquireSessionListRequestT_MSG_LENGTH_;
  unsigned int InquireSessionListResponseT_MSG_LENGTH_;
  unsigned int LegalNotificationBroadcastT_MSG_LENGTH_;
  unsigned int LogonRequestT_MSG_LENGTH_;
  unsigned int LogonResponseT_MSG_LENGTH_;
  unsigned int LogoutRequestT_MSG_LENGTH_;
  unsigned int LogoutResponseT_MSG_LENGTH_;
  unsigned int MMParameterDefinitionRequestT_MSG_LENGTH_;
  unsigned int MMParameterDefinitionResponseT_MSG_LENGTH_;
  unsigned int MassQuoteRequestT_MSG_LENGTH_;
  unsigned int MassQuoteResponseT_MSG_LENGTH_;
  unsigned int ModifyOrderComplexRequestT_MSG_LENGTH_;
  unsigned int ModifyOrderNRResponseT_MSG_LENGTH_;
  unsigned int ModifyOrderResponseT_MSG_LENGTH_;
  unsigned int ModifyOrderSingleRequestT_MSG_LENGTH_;
  unsigned int ModifyOrderSingleShortRequestT_MSG_LENGTH_;
  unsigned int NewOrderComplexRequestT_MSG_LENGTH_;
  unsigned int NewOrderNRResponseT_MSG_LENGTH_;
  unsigned int NewOrderResponseT_MSG_LENGTH_;
  unsigned int NewOrderSingleRequestT_MSG_LENGTH_;
  unsigned int NewOrderSingleShortRequestT_MSG_LENGTH_;
  unsigned int NewsBroadcastT_MSG_LENGTH_;
  unsigned int OrderExecNotificationT_MSG_LENGTH_;
  unsigned int OrderExecReportBroadcastT_MSG_LENGTH_;
  unsigned int OrderExecResponseT_MSG_LENGTH_;
  unsigned int PartyEntitlementsUpdateReportT_MSG_LENGTH_;
  unsigned int QuoteActivationNotificationT_MSG_LENGTH_;
  unsigned int QuoteActivationRequestT_MSG_LENGTH_;
  unsigned int QuoteActivationResponseT_MSG_LENGTH_;
  unsigned int QuoteExecutionReportT_MSG_LENGTH_;
  unsigned int RFQRequestT_MSG_LENGTH_;
  unsigned int RFQResponseT_MSG_LENGTH_;
  unsigned int RejectT_MSG_LENGTH_;
  unsigned int RetransmitMEMessageRequestT_MSG_LENGTH_;
  unsigned int RetransmitMEMessageResponseT_MSG_LENGTH_;
  unsigned int RetransmitRequestT_MSG_LENGTH_;
  unsigned int RetransmitResponseT_MSG_LENGTH_;
  unsigned int RiskNotificationBroadcastT_MSG_LENGTH_;
  unsigned int ServiceAvailabilityBroadcastT_MSG_LENGTH_;
  unsigned int SubscribeRequestT_MSG_LENGTH_;
  unsigned int SubscribeResponseT_MSG_LENGTH_;
  unsigned int TMTradingSessionStatusBroadcastT_MSG_LENGTH_;
  unsigned int ThrottleUpdateNotificationT_MSG_LENGTH_;
  unsigned int TradeBroadcastT_MSG_LENGTH_;
  unsigned int TradingSessionStatusBroadcastT_MSG_LENGTH_;
  unsigned int UnsubscribeRequestT_MSG_LENGTH_;
  unsigned int UnsubscribeResponseT_MSG_LENGTH_;
  unsigned int UserLoginRequestT_MSG_LENGTH_;
  unsigned int UserLoginResponseT_MSG_LENGTH_;
  unsigned int UserLogoutRequestT_MSG_LENGTH_;
  unsigned int UserLogoutResponseT_MSG_LENGTH_;

 public:
  BSEMessageLength() {
    FillsGrpSeqT_MSG_LENGTH_ = sizeof(FillsGrpSeqT);
    InstrmntLegExecGrpSeqT_MSG_LENGTH_ = sizeof(InstrmntLegExecGrpSeqT);
    InstrmtLegGrpSeqT_MSG_LENGTH_ = sizeof(InstrmtLegGrpSeqT);
    LegOrdGrpSeqT_MSG_LENGTH_ = sizeof(LegOrdGrpSeqT);
    MMParameterGrpSeqT_MSG_LENGTH_ = sizeof(MMParameterGrpSeqT);
    MessageHeaderInCompT_MSG_LENGTH_ = sizeof(MessageHeaderInCompT);
    MessageHeaderOutCompT_MSG_LENGTH_ = sizeof(MessageHeaderOutCompT);
    NRBCHeaderCompT_MSG_LENGTH_ = sizeof(NRBCHeaderCompT);
    NRResponseHeaderMECompT_MSG_LENGTH_ = sizeof(NRResponseHeaderMECompT);
    NotAffectedOrdersGrpSeqT_MSG_LENGTH_ = sizeof(NotAffectedOrdersGrpSeqT);
    NotAffectedSecuritiesGrpSeqT_MSG_LENGTH_ = sizeof(NotAffectedSecuritiesGrpSeqT);
    NotifHeaderCompT_MSG_LENGTH_ = sizeof(NotifHeaderCompT);
    QuoteEntryAckGrpSeqT_MSG_LENGTH_ = sizeof(QuoteEntryAckGrpSeqT);
    QuoteEntryGrpSeqT_MSG_LENGTH_ = sizeof(QuoteEntryGrpSeqT);
    QuoteEventGrpSeqT_MSG_LENGTH_ = sizeof(QuoteEventGrpSeqT);
    QuoteLegExecGrpSeqT_MSG_LENGTH_ = sizeof(QuoteLegExecGrpSeqT);
    RBCHeaderCompT_MSG_LENGTH_ = sizeof(RBCHeaderCompT);
    RBCHeaderMECompT_MSG_LENGTH_ = sizeof(RBCHeaderMECompT);
    RequestHeaderCompT_MSG_LENGTH_ = sizeof(RequestHeaderCompT);
    ResponseHeaderCompT_MSG_LENGTH_ = sizeof(ResponseHeaderCompT);
    ResponseHeaderMECompT_MSG_LENGTH_ = sizeof(ResponseHeaderMECompT);
    AddComplexInstrumentRequestT_MSG_LENGTH_ = sizeof(AddComplexInstrumentRequestT);
    AddComplexInstrumentResponseT_MSG_LENGTH_ = sizeof(AddComplexInstrumentResponseT);
    BroadcastErrorNotificationT_MSG_LENGTH_ = sizeof(BroadcastErrorNotificationT);
    CrossRequestT_MSG_LENGTH_ = sizeof(CrossRequestT);
    CrossRequestResponseT_MSG_LENGTH_ = sizeof(CrossRequestResponseT);
    DeleteAllOrderBroadcastT_MSG_LENGTH_ = sizeof(DeleteAllOrderBroadcastT);
    DeleteAllOrderNRResponseT_MSG_LENGTH_ = sizeof(DeleteAllOrderNRResponseT);
    DeleteAllOrderQuoteEventBroadcastT_MSG_LENGTH_ = sizeof(DeleteAllOrderQuoteEventBroadcastT);
    DeleteAllOrderRequestT_MSG_LENGTH_ = sizeof(DeleteAllOrderRequestT);
    DeleteAllOrderResponseT_MSG_LENGTH_ = sizeof(DeleteAllOrderResponseT);
    DeleteAllQuoteBroadcastT_MSG_LENGTH_ = sizeof(DeleteAllQuoteBroadcastT);
    DeleteAllQuoteRequestT_MSG_LENGTH_ = sizeof(DeleteAllQuoteRequestT);
    DeleteAllQuoteResponseT_MSG_LENGTH_ = sizeof(DeleteAllQuoteResponseT);
    DeleteOrderBroadcastT_MSG_LENGTH_ = sizeof(DeleteOrderBroadcastT);
    DeleteOrderComplexRequestT_MSG_LENGTH_ = sizeof(DeleteOrderComplexRequestT);
    DeleteOrderNRResponseT_MSG_LENGTH_ = sizeof(DeleteOrderNRResponseT);
    DeleteOrderResponseT_MSG_LENGTH_ = sizeof(DeleteOrderResponseT);
    DeleteOrderSingleRequestT_MSG_LENGTH_ = sizeof(DeleteOrderSingleRequestT);
    ForcedLogoutNotificationT_MSG_LENGTH_ = sizeof(ForcedLogoutNotificationT);
    GatewayRequestT_MSG_LENGTH_ = sizeof(GatewayRequestT);
    GatewayResponseT_MSG_LENGTH_ = sizeof(GatewayResponseT);
    HeartbeatT_MSG_LENGTH_ = sizeof(HeartbeatT);
    HeartbeatNotificationT_MSG_LENGTH_ = sizeof(HeartbeatNotificationT);
    InquireMMParameterRequestT_MSG_LENGTH_ = sizeof(InquireMMParameterRequestT);
    InquireMMParameterResponseT_MSG_LENGTH_ = sizeof(InquireMMParameterResponseT);
    InquireSessionListRequestT_MSG_LENGTH_ = sizeof(InquireSessionListResponseT);
    InquireSessionListResponseT_MSG_LENGTH_ = sizeof(InquireSessionListResponseT);
    LegalNotificationBroadcastT_MSG_LENGTH_ = sizeof(LegalNotificationBroadcastT);
    LogonRequestT_MSG_LENGTH_ = sizeof(LogonRequestT);
    LogonResponseT_MSG_LENGTH_ = sizeof(LogonResponseT);
    LogoutRequestT_MSG_LENGTH_ = sizeof(LogoutRequestT);
    LogoutResponseT_MSG_LENGTH_ = sizeof(LogoutResponseT);
    MMParameterDefinitionRequestT_MSG_LENGTH_ = sizeof(MMParameterDefinitionRequestT);
    MMParameterDefinitionResponseT_MSG_LENGTH_ = sizeof(MMParameterDefinitionResponseT);
    MassQuoteRequestT_MSG_LENGTH_ = sizeof(MassQuoteRequestT);
    MassQuoteResponseT_MSG_LENGTH_ = sizeof(MassQuoteResponseT);
    ModifyOrderComplexRequestT_MSG_LENGTH_ = sizeof(ModifyOrderComplexRequestT);
    ModifyOrderNRResponseT_MSG_LENGTH_ = sizeof(ModifyOrderNRResponseT);
    ModifyOrderResponseT_MSG_LENGTH_ = sizeof(ModifyOrderResponseT);
    ModifyOrderSingleRequestT_MSG_LENGTH_ = sizeof(ModifyOrderSingleRequestT);
    ModifyOrderSingleShortRequestT_MSG_LENGTH_ = sizeof(ModifyOrderSingleShortRequestT);
    NewOrderComplexRequestT_MSG_LENGTH_ = sizeof(NewOrderComplexRequestT);
    NewOrderNRResponseT_MSG_LENGTH_ = sizeof(NewOrderNRResponseT);
    NewOrderResponseT_MSG_LENGTH_ = sizeof(NewOrderResponseT);
    NewOrderSingleRequestT_MSG_LENGTH_ = sizeof(NewOrderSingleRequestT);
    NewOrderSingleShortRequestT_MSG_LENGTH_ = sizeof(NewOrderSingleShortRequestT);
    NewsBroadcastT_MSG_LENGTH_ = sizeof(NewsBroadcastT);
    OrderExecNotificationT_MSG_LENGTH_ = sizeof(OrderExecNotificationT);
    OrderExecReportBroadcastT_MSG_LENGTH_ = sizeof(OrderExecReportBroadcastT);
    OrderExecResponseT_MSG_LENGTH_ = sizeof(OrderExecResponseT);
    PartyEntitlementsUpdateReportT_MSG_LENGTH_ = sizeof(PartyEntitlementsUpdateReportT);
    QuoteActivationNotificationT_MSG_LENGTH_ = sizeof(QuoteActivationNotificationT);
    QuoteActivationRequestT_MSG_LENGTH_ = sizeof(QuoteActivationRequestT);
    QuoteActivationResponseT_MSG_LENGTH_ = sizeof(QuoteActivationResponseT);
    QuoteExecutionReportT_MSG_LENGTH_ = sizeof(QuoteExecutionReportT);
    RFQRequestT_MSG_LENGTH_ = sizeof(RFQRequestT);
    RFQResponseT_MSG_LENGTH_ = sizeof(RFQResponseT);
    RejectT_MSG_LENGTH_ = sizeof(RejectT);
    RetransmitMEMessageRequestT_MSG_LENGTH_ = sizeof(RetransmitMEMessageRequestT);
    RetransmitMEMessageResponseT_MSG_LENGTH_ = sizeof(RetransmitMEMessageResponseT);
    RetransmitRequestT_MSG_LENGTH_ = sizeof(RetransmitRequestT);
    RetransmitResponseT_MSG_LENGTH_ = sizeof(RetransmitResponseT);
    RiskNotificationBroadcastT_MSG_LENGTH_ = sizeof(RiskNotificationBroadcastT);
    ServiceAvailabilityBroadcastT_MSG_LENGTH_ = sizeof(ServiceAvailabilityBroadcastT);
    SubscribeRequestT_MSG_LENGTH_ = sizeof(SubscribeRequestT);
    SubscribeResponseT_MSG_LENGTH_ = sizeof(SubscribeResponseT);
    TMTradingSessionStatusBroadcastT_MSG_LENGTH_ = sizeof(TMTradingSessionStatusBroadcastT);
    ThrottleUpdateNotificationT_MSG_LENGTH_ = sizeof(ThrottleUpdateNotificationT);
    TradeBroadcastT_MSG_LENGTH_ = sizeof(TradeBroadcastT);
    TradingSessionStatusBroadcastT_MSG_LENGTH_ = sizeof(TradingSessionStatusBroadcastT);
    UnsubscribeRequestT_MSG_LENGTH_ = sizeof(UnsubscribeRequestT);
    UnsubscribeResponseT_MSG_LENGTH_ = sizeof(UnsubscribeResponseT);
    UserLoginRequestT_MSG_LENGTH_ = sizeof(UserLoginRequestT);
    UserLoginResponseT_MSG_LENGTH_ = sizeof(UserLoginResponseT);
    UserLogoutRequestT_MSG_LENGTH_ = sizeof(UserLogoutRequestT);
    UserLogoutResponseT_MSG_LENGTH_ = sizeof(UserLogoutResponseT);
  }
};
}
}
