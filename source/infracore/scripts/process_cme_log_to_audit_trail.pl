#!/usr/bin/perl
use strict;
use warnings;
use feature "switch";    # for given, when

sub PrintHeader;
sub PrintNewHeader;
sub SplitKeyValue;
sub ProcessConfigFile;
sub GetMonth;
my $home                     = $ENV{"HOME"};
my $GET_EXCHANGE_SYMBOL_EXEC = $home . '/LiveExec/bin/get_exchange_symbol';
my $USAGE                    = "$0 YYYYMMDD config_file messages_logfilename server_transaction_number_start\n";

if ( $#ARGV < 3 ) { print $USAGE; exit(0); }

my $messages_logfilename_     = $ARGV[2];
my $config_filename_          = $ARGV[1];
my $transaction_number_start_ = $ARGV[3];
my $YYYYMMDD_                 = $ARGV[0];    #`date "+%Y%m%d"`; chomp($YYYYMMDD_);
my $email_body_complete_      = "";
my $send_mail_                = "";
PrintNewHeader();
my %exch_sym_exchange_code_           = ();
my %exchange_sym_account_number_      = ();
my %exchange_sym_session_id_          = ();
my %shc_to_trade_size_                = ();
my %client_order_id_to_order_flow_id_ = ();
my $order_flow_id_used_so_far_        = 1;

ProcessConfigFile($config_filename_);
open( MESSAGESLOGFILE, "< $messages_logfilename_" ) or die "Could not open $messages_logfilename_\n";

my $transaction_number_ = $transaction_number_start_;
while ( my $inline_ = <MESSAGESLOGFILE> ) {
  chomp($inline_);
  my @message_fix_string_ = split( /8=FIX.4.2/, $inline_ );
  foreach my $message_ (@message_fix_string_) {

    #    $inline_ =~ s/\,//g; # replace ',' so that we can convert to CSV

    chomp($message_);

    #      $message_ =~ s/'\x01'/' '/g;
    my @words_ = split( /\x01/, $message_ );

    if ( $#words_ > 0 ) {
      my $email_body_              = $transaction_number_ . ": \n";
      my $include_in_mail_         = "";
      my $server_timestamp_        = "";
      my $server_proc_date_time_   = $words_[0];
      my $manual_order_identifier_ = "";
      my $operator_id_             = "";
      my $exchange_code_           = "";
      my $message_direction_       = "";
      my %kvmap_                   = ();
      for ( my $i = 0 ; $i < $#words_ ; $i++ ) {
        my ( $key_, $value_ ) = SplitKeyValue( $words_[$i] );
        $kvmap_{$key_} = $value_;
      }

      my $message_type_str_ = "";
      my $order_status_     = "";
      my $reject_reason_    = "";
      if ( exists $kvmap_{"35"} ) {
        $message_direction_ = "TO CME";
        $message_type_str_  = $kvmap_{"35"};
        if (
          $message_type_str_ eq "0" or    #heartbeat
          $message_type_str_ eq "4" or    #heartbeat
          $message_type_str_ eq "2" or    #heartbeat
          $message_type_str_ eq "1" or    #heartbeat
          $message_type_str_ eq "A" or    #login
          $message_type_str_ eq "5"
          )                               #logout
        {
          next;
        }
        elsif ( $message_type_str_ eq "8" )    #execution-report
        {
          $message_direction_ = "FROM CME";
          if ( exists $kvmap_{"39"} ) {
            $order_status_ = $kvmap_{"39"};
            if (
              $order_status_ eq "0" or         #new-order-ack
              $order_status_ eq "4" or         #cancel-ack
              $order_status_ eq "C" or         #expired
              $order_status_ eq "5" or         #modify-ack
              $order_status_ eq "1" or         #partial-fill
              $order_status_ eq "2" or         #complete-fill
              $order_status_ eq "H" or         #trade-cancel
              $order_status_ eq "8"
              )                                #reject
            {
              $message_type_str_ = $message_type_str_ . "/" . $order_status_;
              if ( $order_status_ eq "8" ) {
                if ( exists $kvmap_{"58"} ) {
                  $reject_reason_ = $kvmap_{"58"};
                }
                else {
                  $email_body_      = "Could not find reject reason.\n";
                  $include_in_mail_ = "true";
                  $send_mail_       = 'true';
                }
              }
            }
          }
        }
        elsif ( $message_type_str_ eq "9" ) {    #cancel/cancel-replace reject
          $message_direction_ = "FROM CME";
          if ( exists $kvmap_{"434"} ) {
            $message_type_str_ = $message_type_str_ . "/" . $kvmap_{"434"};
          }
          if ( exists $kvmap_{"58"} ) {
            $reject_reason_ = $kvmap_{"58"};
          }
          else {
            $email_body_      = "Could not find reject reason.\n";
            $include_in_mail_ = "true";
            $send_mail_       = 'true';
          }
        }
      }

      my $market_segment_id_ = "";
      my $sendersub_         = "DVC";
      if ( exists $kvmap_{"57"} ) {
        if ( $message_direction_ eq "FROM CME" ) {
          $sendersub_ = $kvmap_{"57"};
        }
        elsif ( $message_direction_ eq "TO CME" ) {
          $market_segment_id_ = $kvmap_{"57"};    # gets filled in TO CME messages
        }
      }
      else {
        $email_body_      = $email_body_ . "==> Operator ID Does not exist.\n";
        $include_in_mail_ = "true";
      }

      if ( exists $kvmap_{"1028"} ) {
        $manual_order_identifier_ = $kvmap_{"1028"};
      }
      else {
        $email_body_      = $email_body_ . "==> Manual Order Identifier Does not exist.\n";
        $include_in_mail_ = "true";
      }

      my $security_desc_ = "";
      if ( exists $kvmap_{"107"} ) {
        $security_desc_ = $kvmap_{"107"};
      }
      else {
        $email_body_      = $email_body_ . " ==> Security description not available.\n";
        $include_in_mail_ = 'true';
      }

      #            print $exch_sym_exchange_code_{ $security_desc_ }."\n";
      if ( exists $exch_sym_exchange_code_{$security_desc_} ) {
        $exchange_code_ = $exch_sym_exchange_code_{$security_desc_};
      }

      my $account_ = "";
      if ( exists $exchange_sym_account_number_{$security_desc_} ) {
        $account_ = $exchange_sym_account_number_{$security_desc_};
      }

      my $session_id_ = "";

      if ( exists $kvmap_{"52"} ) {
        $server_timestamp_ = $kvmap_{"52"};
      }
      else {
        $email_body_      = $email_body_ . " ==> Timestamp missing for this message\n";
        $include_in_mail_ = "true";
      }

      if ( exists $kvmap_{"50"} ) {
        if ( $message_direction_ eq "TO CME" ) {
          $sendersub_ = $kvmap_{"50"};
        }
        elsif ( $message_direction_ eq "FROM CME" ) {
          $market_segment_id_ = $kvmap_{"50"};    # will be filled in FROM CME messages
        }
      }
      else {
        $email_body_      = $email_body_ . " ==> Tag 50 id missing..\n";
        $include_in_mail_ = "true";
      }

      my $self_match_prevention_id_ = "";
      if ( exists $kvmap_{"7928"} ) {
        $self_match_prevention_id_ = $kvmap_{"7928"};
      }
      else {
        #            $email_body_ = $email_body_." Self match prevention id missing..\n";
      }

      if ( exists $kvmap_{"1"} ) {
        $account_ = $kvmap_{"1"};
      }
      else {
        $email_body_      = $email_body_ . " ==> Account Number missing..\n";
        $include_in_mail_ = "true";
      }

      # This is filled while sending messages to globex
      my $executing_firm_ = "";
      if ( exists $kvmap_{"49"} ) {
        $session_id_     = substr( $kvmap_{"49"}, 0, 3 );
        $executing_firm_ = substr( $kvmap_{"49"}, 3, 3 );
        if ( $executing_firm_ eq "" ) {
          $executing_firm_ = "";
        }
      }
      else {
        $email_body_      = $email_body_ . " ==> Tag49, Session id Execution firm id missog...\n";
        $include_in_mail_ = "true";
      }

      #this is filled in received messages from globex
      if ( exists $kvmap_{"56"} and $message_direction_ eq "FROM CME" ) {
        $session_id_     = substr( $kvmap_{"56"}, 0, 3 );
        $executing_firm_ = substr( $kvmap_{"56"}, 3, 3 );
        if ( $executing_firm_ eq "" ) {
          $executing_firm_ = "DV01";
        }
      }

      my $client_order_id_ = "";
      if ( exists $kvmap_{"11"} ) {
        $client_order_id_ = $kvmap_{"11"};
      }

      my $host_order_number_ = "";
      if ( exists $kvmap_{"37"} ) {
        $host_order_number_ = $kvmap_{"37"};
      }

      my $execution_resentment_reason_ = "";    # not going to happen in our case
      if ( exists $kvmap_{"378"} ) {
        $execution_resentment_reason_ = $kvmap_{"378"};
      }

      my $buysell_ = "";
      if ( exists $kvmap_{"54"} ) {
        $buysell_ = $kvmap_{"54"};
      }
      else {
        $email_body_      = $email_body_ . " ==> Buy/.Sell missing...\n";
        $include_in_mail_ = "true";
      }

      my $quantity_ = "";
      my $filled_quantity_ = "";
      #Order Qty
      if ( exists $kvmap_{"38"} ) {
        $quantity_ = $kvmap_{"38"};
      }
      else {
        if ( exists $kvmap_{"35"} && $kvmap_{"35"} eq "D" ) {
          $email_body_      = $email_body_ . " ==> Order  Size missing\n";
          $include_in_mail_ = "true";
        }
      }
      #Filled Qty
      if ( exists $kvmap_{"32"} ) {
        $filled_quantity_ = $kvmap_{"32"};
        if ( exists $shc_to_trade_size_{$security_desc_} ) {
          $shc_to_trade_size_{$security_desc_} = $shc_to_trade_size_{$security_desc_} + $filled_quantity_;
        }
        else {
          $shc_to_trade_size_{$security_desc_} = $filled_quantity_;
        }
      }
      else {
        if ( exists $kvmap_{"39"} && ( $kvmap_{"39"} eq "1" || $kvmap_{"39"} eq "2" ) ) {
          $email_body_      = $email_body_ . " ==> Execution EXECUTION Size missing\n";
          $include_in_mail_ = "true";
        }
      }
      my $max_show_ = "";
      if ( exists $kvmap_{"210"} ) {
        $max_show_ = $kvmap_{"210"};
      }

      my $min_quantity_ = "";
      if ( exists $kvmap_{"110"} ) {
        $min_quantity_ = $kvmap_{"110"};
      }

      my $country_of_origin_ = "";
      if ( exists $kvmap_{"142"} ) {
        $country_of_origin_ = $kvmap_{"142"};
        my @country_words_ = split( ',', $country_of_origin_ ); chomp ( @country_words_ );
        if ( $#country_words_ >= 0 ) {
          if ( $country_words_[0] eq "US" ) {
            $country_of_origin_ = $country_words_[0] . $country_words_[1];
          }
          else {
            $country_of_origin_ = $country_words_[0];
          }
        }
      }

      my $cummulative_quantity_ = "";
      if ( exists $kvmap_{"14"} ) {
        $cummulative_quantity_ = $kvmap_{"14"};
      }

      my $remaining_quantity_ = "";
      if ( exists $kvmap_{"151"} ) {
        $remaining_quantity_ = $kvmap_{"151"};
      }

      my $aggressor_flag_ = "";
      if ( exists $kvmap_{"1057"} ) {
        $aggressor_flag_ = $kvmap_{"1057"};
      }

      my $cme_globex_message_id_ = "";
      if ( exists $kvmap_{"17"} ) {
        $cme_globex_message_id_ = $kvmap_{"17"};
      }
      elsif ( $message_direction_ eq "FROM CME" ) {
        $email_body_ = $email_body_ . " ==>cme-globex-message-id-missing\n";
      }

      my $ifm_flag_ = "";
      if ( exists $kvmap_{"9768"} ) {
        $ifm_flag_ = $kvmap_{"9768"};
      }

      my $spread_link_id_ = "";
      if ( exists $kvmap_{"527"} ) {
        $spread_link_id_ = $kvmap_{"527"};
      }

      my $inst_grp_code_ = "";
      if ( exists $kvmap_{"55"} ) {
        $inst_grp_code_ = $kvmap_{"55"};
      }
      else {
        $email_body_      = $email_body_ . " ==> Product grp code missing..\n";
        $include_in_mail_ = "true";
      }
      my $product_maturity_date_ = "";
      if ( exists $kvmap_{"107"} ) {
        my $year_ = "201" . substr( $kvmap_{"107"}, -1 );
        my $month_ = GetMonth( substr( $kvmap_{"107"}, -2, 1 ) );
        $product_maturity_date_ = $year_ . "-" . $month_;
      }
      else {
        $email_body_      = $email_body_ . " ==> Product Maturity date missing..\n";
        $include_in_mail_ = "true";
      }

      my $cfi_code_ = "";
      if ( exists $kvmap_{"461"} ) {
        $cfi_code_ = $kvmap_{"461"};
      }

      my $limit_price_ = "";
      if ( exists $kvmap_{"44"} ) {
        $limit_price_ = $kvmap_{"44"};
      }
      else {
        if ( exists $kvmap_{"40"} && $kvmap_{"40"} eq "2" ) {
          $email_body_      = $email_body_ . " ==> lLimit price missing..\n";
          $include_in_mail_ = "true";
        }
      }

      my $stop_price_ = "";
      if ( exists $kvmap_{"99"} ) {
        $stop_price_ = $kvmap_{"99"};
      }
      else {
        if ( exists $kvmap_{"40"} && ( $kvmap_{"40"} eq "3" || $kvmap_{"40"} eq "4" ) ) {
          $email_body_      = $email_body_ . " ==> stop pric missing...\n";
          $include_in_mail_ = "true";
        }
      }

      my $fill_price_ = "";
      if ( exists $kvmap_{"31"} ) {
        $fill_price_ = $kvmap_{"31"};
      }
      else {
        if ( exists $kvmap_{"35"}
          && $kvmap_{"35"} eq "8"
          && exists $kvmap_{"39"}
          && ( $kvmap_{"39"} eq "1" || $kvmap_{"39"} eq "2" ) )
        {
          $email_body_      = $email_body_ . " ==> Fill Price missing..\n";
          $include_in_mail_ = "true";
        }
      }

      my $order_type_     = "";
      my $order_qualifer_ = "";
      if ( exists $kvmap_{"40"} ) {
        $order_type_ = $kvmap_{"40"};
      }

      if ( exists $kvmap_{"59"} ) {
        $order_qualifer_ = $kvmap_{"59"};

        # "0"" ="DAY" #"1" = "GTC"#"3" = "FAK" #"6" = "GTD"
      }

      my $cti_code_ = "";
      if ( exists $kvmap_{"9702"} ) {
        $cti_code_ = $kvmap_{"9702"};
      }

      my $customer_or_firm_ = "";
      if ( exists $kvmap_{"204"} ) {
        $customer_or_firm_ = $kvmap_{"204"};
      }
      else {
        if ( exists $kvmap_{"35"} && ( $kvmap_{"35"} eq "D" ) )    # || $kvmap_{"35"} eq "F"))
        {
          $email_body_      = $email_body_ . "==> Customer or firm identifier missing...\n";
          $include_in_mail_ = "true";
        }
      }

      my $give_up_firm_ = "";
      if ( exists $kvmap_{"9707"} ) {
        $give_up_firm_ = $kvmap_{"9707"};
      }

      my $give_up_indicator_ = "";
      if ( exists $kvmap_{"9708"} ) {
        $give_up_indicator_ = $kvmap_{"9708"};
      }

      my $give_up_account_ = "";
      if ( exists $kvmap_{"79"} ) {
        $give_up_account_ = $kvmap_{"79"};
      }

      my $quote_request_id_ = "";
      if ( exists $kvmap_{"131"} ) {
        $quote_request_id_ = $kvmap_{"131"};
      }

      my $exchange_quote_request_id_ = "";
      if ( exists $kvmap_{"9770"} ) {
        $exchange_quote_request_id_ = $kvmap_{"9770"};
      }

      my $quote_id_ = "";
      if ( exists $kvmap_{"117"} ) {
        $quote_id_ = $kvmap_{"117"};
      }
      my $quote_set_id_ = "";
      if ( exists $kvmap_{"302"} ) {
        $quote_set_id_ = $kvmap_{"302"};
      }

      my $num_quote_set_ = "";
      if ( exists $kvmap_{"296"} ) {
        $num_quote_set_ = $kvmap_{"296"};
      }

      my $quote_entry_id_ = "";
      if ( exists $kvmap_{"299"} ) {
        $quote_entry_id_ = $kvmap_{"299"};
      }

      my $num_qoute_entries_ = "";
      if ( exists $kvmap_{"295"} ) {
        $num_qoute_entries_ = $kvmap_{"295"};
      }

      my $quote_cancel_type_ = "";
      if ( exists $kvmap_{"298"} ) {
        $quote_cancel_type_ = $kvmap_{"298"};
      }

      my $unsolicited_cancel_type_ = "";
      if ( exists $kvmap_{"9775"} ) {
        $unsolicited_cancel_type_ = $kvmap_{"9775"};
      }

      my $quote_ack_status_ = "";
      if ( exists $kvmap_{"297"} ) {
        $quote_ack_status_ = $kvmap_{"297"};
      }

      my $quote_rejection_reason_ = "";
      if ( exists $kvmap_{"300"} ) {
        $quote_rejection_reason_ = $kvmap_{"300"};
      }

      my $processed_entries_ = "";
      if ( exists $kvmap_{"9772"} ) {
        $processed_entries_ = $kvmap_{"9772"};
      }

      my $bid_price_ = "";
      if ( exists $kvmap_{"132"} ) {
        $bid_price_ = $kvmap_{"132"};
      }

      my $bid_size_ = "";
      if ( exists $kvmap_{"134"} ) {
        $bid_size_ = $kvmap_{"134"};
      }

      my $offer_price_ = "";
      if ( exists $kvmap_{"133"} ) {
        $offer_price_ = $kvmap_{"133"};
      }

      my $offer_size_ = "";
      if ( exists $kvmap_{"135"} ) {
        $offer_size_ = $kvmap_{"135"};
      }

      my $cross_id_ = "";
      if ( exists $kvmap_{"548"} ) {
        $cross_id_ = $kvmap_{"548"};
      }

      my $security_request_id_ = "";
      if ( exists $kvmap_{"320"} ) {
        $security_request_id_ = $kvmap_{"320"};
      }
      my $maturity_date_ = "";
      $transaction_number_++;

      if ( not exists $client_order_id_to_order_flow_id_{$client_order_id_} ) {
        $order_flow_id_used_so_far_++;
        $client_order_id_to_order_flow_id_{$client_order_id_} = $order_flow_id_used_so_far_;
      }

      if ( $message_direction_ eq "TO CME" ) {
        printf "%s,,%s,", $server_timestamp_, $message_direction_;
      }
      if ( $message_direction_ eq "FROM CME" ) {
        printf ",%s,%s,", $server_timestamp_, $message_direction_;
      }

      #sendersub=operator
      #TODO: Add dynamic account number and product-wise exchange codes
      printf "%s,%s,%s,%s,%s,", $sendersub_, $self_match_prevention_id_, 15038200, $session_id_, $executing_firm_;   #8
      printf "%s,%s,%s,%s,", $manual_order_identifier_, $message_type_str_, $cti_code_, $customer_or_firm_;    #12
      printf "%s,%d,", $cme_globex_message_id_, $transaction_number_;                                          #14
      printf "%s,%s,%s,", $client_order_id_to_order_flow_id_{$client_order_id_}, $spread_link_id_, $security_desc_;  #17
      printf "%s,%s,%s,%s,%s,", $market_segment_id_, $client_order_id_, $host_order_number_, $buysell_, $quantity_;  #22
      printf "%s,%s,%s,%s,", $limit_price_, $stop_price_, $order_type_, $order_qualifer_;                            #26
      printf "%s,%s,%s,%s,%s,", $ifm_flag_, $max_show_, $min_quantity_, $country_of_origin_, $fill_price_;           #31
      printf "%s,%s,%s,%s,", $filled_quantity_, $cummulative_quantity_, $remaining_quantity_, $aggressor_flag_;             #35
      printf "%s,%s,", $execution_resentment_reason_, $reject_reason_;                                               #37
      printf "%s,%s,%s,%s,%s,", $processed_entries_, $cross_id_, $quote_request_id_, $quote_id_, $quote_entry_id_;   #42
      printf "%s,%s,%s,%s", $bid_price_, $bid_size_, $offer_price_, $offer_size_;                                    #46
      printf "\n";

      if ($include_in_mail_) {
        $send_mail_           = 'true';
        $email_body_complete_ = $email_body_complete_ . $email_body_;
      }
    }
  }
}

close(MESSAGESLOGFILE);
if ($send_mail_) {
  my $email_id_ = 'diwakar@tworoads.co.in';
  open( MAIL, "|/usr/sbin/sendmail -t" );
  my $hostname_ = `hostname`;
  print MAIL "To: $email_id_\n";
  print MAIL "From: $email_id_\n";
  print MAIL "Subject: CME Audit logs $hostname_\n\n";
  print MAIL $email_body_complete_;
  close(MAIL);
}

exit(0);

sub PrintHeader {
  printf "Server_Transaction_Number,Server_Timestamp,";
  printf "Manual_Order_Identifier,Exchange_Code,Message_Direction,";
  printf "TAG_50_ID,Self_Match_Prevention_ID,Account_Number,";
  printf "Session_ID,Executing_Firm_ID,Client_Order_ID,";
  printf
"Host_Order_Number,Message_Type,Order_Status,Execution_Restatement_Reason,Reason_Code,Buy_Sell_Indicator,Quantity,Max_Show,Instrument_Decription,";
  printf "Product_Group_Code,Maturity_Date,CFI_Code,Limit_Price,";
  printf "Stop_Price,Fill_Price,Order_Type,Order_Qualifier,Customer_Type_Indicator,Origin,";
  printf "Give_Up_Firm,Give_Up_Indicator,Give_Up_Account,Quote_Request_ID,Exchange_Quote_Request_ID,Quote_ID,";
  printf "Quote_Set_ID,Number_of_Quote_Sets,Quote_Entry_ID,Number_of_Quote_Entries,Quote_Cancel_Type,";
  printf "Unsolicited_Cancel_Type,Quote_Acknowledgement_Status,Quote_Reject_Reason,";
  printf "Processed_Entries,Bid_Price,Bid_Size,Offer_Price,Offer_Size,Cross_ID,Security_Request_ID";

  printf "\n";
}

sub PrintNewHeader {
  printf "Sending_Timestamps,Receiving_Timestamps,Message_Direction,";                       #1-3
  printf "Operator_ID,Self_Match_Prevention_ID,Account_Number,";                             #4-6
  printf "Session_ID,Executing_Firm_ID,Manual_Order_Identifier,";                            #7-9
  printf "Message_Type,Customer_Type_Indicator,Origin,";                                     #10-12
  printf "CME_Globex_Message_ID,Message_Link_ID,Order_Flow_ID,Spread_Leg_Link_ID,";          #13-16
  printf "Instrument_Description,Market_segment_ID,Client_Order_ID,CME_Globex_Order_ID,";    #17-20
  printf "Buy_Sell_Indicator,Quantity,Limit_Price,Stop_Price,Order_Type,Order_Qualifier,";   #21-26
  printf "IFM_Flag,Display_Quantity,Minimum_Quantity,";                                      #27-29
  printf "Country_Of_Origin,Fill_Price,Fill_Quantity,Cumulative_Quantity,Remaining_Quantity,";#30-34
  printf "Aggressor_Flag,Source_Of_Cancellation,Reject_Reason,Processed_Quotes,";            #35-38
  printf "Cross_ID,Quote_Request_ID,Message_Quite_ID,Wquote_Entry_ID,";                      #39-42
  printf "Bid_Price,Bid_Size,Offer_Price,Offer_Size";                                        #43-46

  #printf "Server_Transaction_Number,Server_Timestamp,";
  #printf "Manual_Order_Identifier,Exchange_Code,Message_Direction,";
  #printf "TAG_50_ID,Self_Match_Prevention_ID,Account_Number,";
  #printf "Session_ID,Executing_Firm_ID,Client_Order_ID,";
  #printf
#"Host_Order_Number,Message_Type,Order_Status,Execution_Restatement_Reason,Reason_Code,Buy_Sell_Indicator,Quantity,Max_Show,Instrument_Decription,";
  #printf "Product_Group_Code,Maturity_Date,CFI_Code,Limit_Price,";
  #printf "Stop_Price,Fill_Price,Order_Type,Order_Qualifier,Customer_Type_Indicator,Origin,";
  #printf "Give_Up_Firm,Give_Up_Indicator,Give_Up_Account,Quote_Request_ID,Exchange_Quote_Request_ID,Quote_ID,";
  #printf "Quote_Set_ID,Number_of_Quote_Sets,Quote_Entry_ID,Number_of_Quote_Entries,Quote_Cancel_Type,";
  #printf "Unsolicited_Cancel_Type,Quote_Acknowledgement_Status,Quote_Reject_Reason,";
  #printf "Processed_Entries,Bid_Price,Bid_Size,Offer_Price,Offer_Size,Cross_ID,Security_Request_ID";

  printf "\n";
}

sub SplitKeyValue {
  my $keqv_str_ = shift;
  my $sk_       = "NF";
  my $sv_       = "NF";
  my @kvwords_  = split( /\=/, $keqv_str_ );
  if ( $#kvwords_ >= 1 ) {
    $sk_ = $kvwords_[0];
    $sv_ = $kvwords_[1];
  }
  return ( $sk_, $sv_ );
}

sub ProcessConfigFile {
  my ($config_filename_) = @_;
  open( CONFIGFILE, "< $config_filename_" ) or die "Could not open $config_filename_\n";
  while ( my $inline_ = <CONFIGFILE> ) {
    my @line_words_ = split( / /, $inline_ );
    if ( $#line_words_ > 0 ) {
      my $shortcode_  = $line_words_[0];
      my $extra_info_ = $line_words_[1];

      #     print $extra_info_."\n";
      my $exchange_sym_ = `$GET_EXCHANGE_SYMBOL_EXEC $shortcode_ $YYYYMMDD_`;
      chomp($exchange_sym_);

      #my @extra_info_words_ = split(' ', $extra_info_);
      #     print "size: ".$#extra_info_words_."\n";
      #     if ($#extra_info_words_ > 0)
      #     {
      $exch_sym_exchange_code_{$exchange_sym_} = $extra_info_;

      #       print " ".$extra_info_words_[0];
      #     }
    }

  }
  close(CONFIGFILE);
}

sub GetMonth {
  my ($code_) = @_;
  given ($code_) {
    when ("F") {
      return "01";
    }
    when ("G") {
      return "02";
    }
    when ("H") {
      return "03";
    }
    when ("J") {
      return "04";
    }
    when ("K") {
      return "05";
    }
    when ("M") {
      return "06";
    }
    when ("N") {
      return "07";
    }
    when ("Q") {
      return "08";
    }
    when ("U") {
      return "09";
    }
    when ("V") {
      return "10";
    }
    when ("X") {
      return "11";
    }
    when ("Z") {
      return "12";
    }
  }
}
