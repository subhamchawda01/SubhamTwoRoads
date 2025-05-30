/**
    \file OrderRoutingServer/ors_control_exec.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
*/

#include <iostream>
#include <stdlib.h>

#include <boost/program_options.hpp>

#include "dvccode/Utils/tcp_client_socket.hpp"
#include "infracore/BasicOrderRoutingServer/control_command.hpp"

#include "infracore/OrderRoutingServer/ors_control_exec.hpp"

int main(int argc, char** argv) {
  std::vector<std::string> control_command_text_;

  // boost::program_options::options_description desc("Allowed options");

  // desc.add_options()
  //   ("command", boost::program_options::value<std::vector<std::string> >(&control_command_text_)->multitoken(),
  //   "command")
  //   ;

  // boost::program_options::positional_options_description pod_;
  // pod_.add("command", -1);

  // boost::program_options::variables_map vm;
  // // boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  // boost::program_options::store(boost::program_options::command_line_parser(argc,
  // argv).options(desc).positional(pod_).run(), vm);
  // boost::program_options::notify(vm);

  // if (vm.count("command") <= 0)
  //   {
  //     return 0;
  //   }

  if (argc <= 1) return 0;

  std::string control_ip = "127.0.0.1";
  int control_port = 10003;

  if (argc > 1) control_port = atoi(argv[1]);

  for (int i = 2; i < argc; i++) {
    control_command_text_.push_back(argv[i]);
  }

  if (control_command_text_.size() <= 0) return 0;

  HFSAT::TCPClientSocket tcp_client_socket_;
  tcp_client_socket_.Connect(control_ip, control_port);

  // THESE CHECKS ARE NOT NEEDED HERE.
  // THE ORS WILL REJECT MALFORMED COMMANDS.
  // LEFT HERE IN CASE SOMEDAY SOMEONE DECIDES TO REENABLE CHECKING HERE

  // bool syntax_okay_ = false;
  // if ( ( control_command_text_[0].compare ( "START" ) == 0 ) ||
  //      ( control_command_text_[0].compare ( "STOP" ) == 0 ) ||
  //      ( control_command_text_[0].compare ( "LOGIN" ) == 0 ) ||
  //      ( control_command_text_[0].compare ( "LOGOUT" ) == 0 ) ||
  //      ( control_command_text_[0].compare ( "GENERATEHEARTBEAT" ) == 0 ) ||
  //      ( control_command_text_[0].compare ( "TMXDESYNC" ) == 0 ) ||
  //      ( control_command_text_[0].compare ( "TMXSYNC" ) == 0 ) ||
  //      ( control_command_text_[0].compare ( "TMXREGISTERACCT" ) == 0 ) ||
  //      ( control_command_text_[0].compare ( "TMXDELETEACCT" ) == 0 ) ||
  //      ( control_command_text_[0].compare ( "TMXPOSITIONDELIMITER" ) == 0 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( ( control_command_text_[0].compare ( "SETMAXPOS" ) == 0 ) ||
  // 	      ( control_command_text_[0].compare ( "SETMAXSIZE" ) == 0 ) ||
  // 	      ( control_command_text_[0].compare ( "SETWORSTCASEPOS" ) == 0 )
  // 	      ) &&
  // 	    ( control_command_text_.size ( ) >= 2 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "ADDTRADINGSYMBOL" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 5 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "UPDATESETTINGS" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 3 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "SENDORDER" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 6 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "SENDTIFORDER" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 7 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "CANCELORDER" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 2 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "MODIFYORDER" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 6 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "TMXACCOUNTTYPE" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 2 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "TMXCLEARINGTRADETYPE" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 2 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "TMXMODIFYORDERMODE" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 2 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "TMXSENDRFQ" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 3 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "TMXREGISTERACCT" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 1 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "TMXDELETEACCT" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 1 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "TMXPOSITIONENTRY" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 3 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "TMXPOSITIONDELETE" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 2 ) )
  //   { syntax_okay_ = true; }
  // else if ( ( control_command_text_[0].compare ( "TMXPOSITIONDELIMITER" ) == 0 ) &&
  // 	    ( control_command_text_.size ( ) >= 1 ) )
  //   { syntax_okay_ = true; }

  std::string send_text = std::string("CONTROLCOMMAND ") + HFSAT::ORS::JoinText(" ", control_command_text_);
  tcp_client_socket_.WriteN(send_text.length(), send_text.c_str());

  tcp_client_socket_.Close();
}
