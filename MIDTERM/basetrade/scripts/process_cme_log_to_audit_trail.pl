#!/usr/bin/perl
use strict;
use warnings;
use feature "switch"; # for given, when

sub PrintHeader;
sub SplitKeyValue;

my $USAGE="$0 messages_logfilename ";

if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $messages_logfilename_ = $ARGV[0];

PrintHeader ( ) ;

open ( MESSAGESLOGFILE, "< $messages_logfilename_") or die "Could not open $messages_logfilename_\n";

my $transaction_number_ = 0;
while ( my $inline_ = <MESSAGESLOGFILE> )
{
    chomp ($inline_);
#    $inline_ =~ s/\,//g; # replace ',' so that we can convert to CSV
    my @words_ = split ( /\^/, $inline_ );
#    print $#words_."\n".$inline_;
    if ( $#words_ > 0 )
    {
	$transaction_number_ ++;
	my $server_proc_date_time_ = $words_[0];
	my $manual_order_identifier_ = " ";
	my $exchange_code_ = "CME";

	my %kvmap_ = ();
	for ( my $i = 1 ; $i < $#words_ ; $i ++ )
	{
	    my ( $key_, $value_ ) = SplitKeyValue ( $words_[$i] ) ;
	    $kvmap_{$key_} = $value_;
	}

	if ( exists $kvmap_{"1028"} )
	{
	    $manual_order_identifier_ = $kvmap_{"1028"} ;
	}

	my $message_direction_ = "From CME";
	if ( ( exists $kvmap_{"56"} ) &&
	     ( $kvmap_{"56"} eq "CME" ) )
	{
	    $message_direction_ = "  To CME";
	}

	my $status_ = "OK";
	my $reject_reason_ = "";
	if ( ( exists $kvmap_{"35"} ) &&
	     ( ( $kvmap_{"35"} eq "3" ) ||
	       ( $kvmap_{"35"} eq "j" ) ||
	       ( $kvmap_{"35"} eq "9" ) ||
	       ( ( $kvmap_{"35"} eq "8" ) && 
		 ( exists $kvmap_{"39"} ) &&
		 ( ( $kvmap_{"39"} eq "8" ) ||
		   ( $kvmap_{"39"} eq "H" ) ) ) ) ) 
	{
	    $status_ = "REJECT";
	    
	    if ( ( exists $kvmap_{"35"} ) &&
		 ( $kvmap_{"35"} eq "3" ) )
	    {
		if ( exists $kvmap_{"58"} ) 
		{
		    $reject_reason_ = $kvmap_{"58"} ;
		}
	    }
	}
	my $sender_location_ = "     "; # at the end since it will have a ','
	if ( exists $kvmap_{"142"} )
	{
	    $sender_location_ = $kvmap_{"142"} ;
	}
	my $sendersub_ = "DVC";
	if ( exists $kvmap_{"50"} )
	{
	    $sendersub_ = $kvmap_{"50"} ;
	}
	my $account_ = "        ";
	if ( exists $kvmap_{"1"} )
	{
	    $account_ = $kvmap_{"1"} ;
	}
	my $executing_firm_ = "998";
	my $session_id_ = "8Q3";

	my $clordid_ = " ";
	if ( exists $kvmap_{"11"} )
	{
	    $clordid_ = $kvmap_{"11"} ;
	}
	my $correlclordid_ = " ";
	if ( exists $kvmap_{"9717"} )
	{
	    $correlclordid_ = $kvmap_{"9717"} ;
	}

	my $host_order_number_ = "   ";
	if ( exists $kvmap_{"37"} )
	{
	    $host_order_number_ = $kvmap_{"37"} ;
	}

	my $message_type_str_ = "         ";
	if ( exists $kvmap_{"35"} )
	{
	    given ( $kvmap_{"35"} )
	    {
		when ("A")
		{
		    $message_type_str_ = "LOGIN    ";
		}
		when ("5")
		{
		    $message_type_str_ = "LOGOUT   ";
		}
		when ("D")
		{
		    $message_type_str_ = "NEW ORDER";
		}
		when ("F")
		{
		    $message_type_str_ = "CANCEL   ";
		}
		when ("9")
		{
		    $message_type_str_ = "CANCEL   ";
		}
		when ("8")
		{
		    $message_type_str_ = "NEW ORDER";
		    if ( exists $kvmap_{"39"} )
		    {
			given ( $kvmap_{"39"} )
			{
			    when ("0")
			    {
				$message_type_str_ = "NEW ORDER";
			    }
			    when ("4")
			    {
				$message_type_str_ = "CANCEL   ";
			    }
			    when ("C")
			    {
				$message_type_str_ = "CANCEL   ";
			    }
			    when ("5")
			    {
				$message_type_str_ = "MODIFY   ";
			    }
			    when ("1")
			    {
				$message_type_str_ = "EXECUTION";
			    }
			    when ("2")
			    {
				$message_type_str_ = "EXECUTION";
			    }
			}
		    }
		}
	    }		
	}
		
	my $buysell_ = "   ";
	if ( exists $kvmap_{"54"} )
	{
	    if ( $kvmap_{"54"} eq "1" )
	    { $buysell_ = "B"; }
	    else
	    { $buysell_ = "S"; }
	}
	my $quantity_ = "  ";
	if ( $message_type_str_ eq "EXECUTION" )
	{
	    if ( exists $kvmap_{"32"} )
	    {
		$quantity_ = $kvmap_{"32"};
	    }
	}
	else
	{
	    if ( exists $kvmap_{"38"} )
	    {
		$quantity_ = $kvmap_{"38"};
	    }
	}
	
	my $max_show_ = "  ";
	if ( exists $kvmap_{"210"} )
	{
	    $max_show_ = $kvmap_{"210"};
	}
	
	my $security_desc_ = "     ";
	if ( exists $kvmap_{"107"} )
	{
	    $security_desc_ = $kvmap_{"107"};
	}

	my $inst_grp_code_ = "  ";
	if ( exists $kvmap_{"55"} )
	{
	    $inst_grp_code_ = $kvmap_{"55"};
	}
	my $security_type_ = "   ";
	if ( exists $kvmap_{"167"} )
	{
	    $security_type_ = $kvmap_{"167"};
	}
	my $maturity_date_ = "       ";

	my $cfi_code_ = "       ";
	if ( exists $kvmap_{"461"} )
	{
	    $cfi_code_ = $kvmap_{"461"};
	}

	if ( $security_desc_ eq "8EJZ1" )
	{
	    $maturity_date_ = "2011-12";
	    $cfi_code_ = "FFDXSX";
	}
	if ( $security_desc_ eq "8ZBU1" )
	{
	    $maturity_date_ = "2011-09";
	    $cfi_code_ = "FFDXSX";
	}

	my $strike_price_ = "      ";
	if ( exists $kvmap_{"202"} )
	{
	    $strike_price_ = $kvmap_{"202"};
	}

	my $limit_price_ = "      ";
	if ( exists $kvmap_{"44"} )
	{
	    $limit_price_ = $kvmap_{"44"};
	}

	my $stop_price_ = "     ";
	if ( exists $kvmap_{"99"} )
	{
	    $stop_price_ = $kvmap_{"99"};
	}

	my $fill_price_ = "     ";
	if ( exists $kvmap_{"31"} )
	{
	    $fill_price_ = $kvmap_{"31"};
	}

	my $order_type_ = "   ";
	my $order_qualifer_ = "   ";
	if ( exists $kvmap_{"40"} )
	{
	    if ( $kvmap_{"40"} eq "1" )
	    { $order_type_ = "MKT"; }
	    else
	    { $order_type_ = "LMT"; }
	}
	if ( exists $kvmap_{"59"} )
	{
	    if ( $kvmap_{"59"} eq "0" )
	    { $order_qualifer_ = "DAY"; }
	    if ( $kvmap_{"59"} eq "1" )
	    { $order_qualifer_ = "GTC"; }
	    if ( $kvmap_{"59"} eq "3" )
	    { $order_qualifer_ = "FAK"; }
	    if ( $kvmap_{"59"} eq "6" )
	    { $order_qualifer_ = "GTD"; }
	}

	my $cti_code_ = " ";
	if ( exists $kvmap_{"9702"} )
	{
	    $cti_code_ = $kvmap_{"9702"};
	}
	my $customer_or_firm_ = " ";
	if ( exists $kvmap_{"204"} )
	{
	    $customer_or_firm_ = $kvmap_{"204"};
	}

	

	printf "%d,%s,", $transaction_number_, $server_proc_date_time_;
	printf "%s,%s,%s,", $manual_order_identifier_, $exchange_code_, $message_direction_;
	printf "%s,%s,", $status_, $reject_reason_;
	printf "%s,%s,", $sendersub_, $account_;
	printf "%s,%s,", $executing_firm_, $session_id_;
	printf "%s,%s,", $clordid_, $correlclordid_;

	printf "%s,%s,%s,%s,%s,%s,", $host_order_number_, $message_type_str_, $buysell_ ,$quantity_, $max_show_, $security_desc_;
	printf "%s,%s,%s,%s,%s,", $inst_grp_code_, $maturity_date_, $cfi_code_, $strike_price_, $limit_price_;
	printf "%s,%s,%s,%s,%s,%s,%s", $stop_price_, $fill_price_, $order_type_, $order_qualifer_, $cti_code_, $customer_or_firm_, $sender_location_;
	
	printf "\n";
    }
}

close ( MESSAGESLOGFILE );

exit ( 0 );

sub PrintHeader 
{
    printf "Server Transaction Number, Server Process Date Time,";
    printf "Manual Order Identifier, Exchange Code,Message Direction,";
    printf "Status,Reason Code/Error Code,";
    printf "TAG 50 ID,Account Number,";
    printf "Executing Firm,Session Id,";
    printf "Client Order ID,CorrelationClOrdID,";

    printf "Host Order Number, Message Type, Buy/Sell Indicator, Quantity, Max Show, Instrument/Security Decription,";
    printf "Product/Instrument Group Code, Maturity Date/Expiration Date, CFI Code, Strike Price, Limit Price,";
    printf "Stop Price, Fill Price, Order Type,Order Qualifier, Customer Type Indicator, Origin, Sender Location ID";
    
    printf "\n";
}

sub SplitKeyValue {
    my $keqv_str_ = shift;
    my $sk_="NF";
    my $sv_="NF";
    my @kvwords_ = split ( /\=/, $keqv_str_ ) ;
    if ( $#kvwords_ >= 1 )
    {
	$sk_ = $kvwords_[0];
	$sv_ = $kvwords_[1];
    }
    return ( $sk_,$sv_ );
}
