#include <EventApi.hpp>
#include <iostream>
using namespace std;
using namespace MBOCHIP::SiliconMD::EApi;
using namespace MBOCHIP::SiliconMD::MApi;

int main(int argc, char** args) {
  // instantiate the EventApi with MessageApi as message api and NSEFPGAReceiver as the receiver
  EventConfig eventConfig("/home/pengine/prod/live_configs/configNSEApp.toml");
  eventConfig.events_return.mbpBookHw = true;
  eventConfig.events_return.mbpBookHwPreMatched = false;
  EventApi<MessageApi<NSEFPGAReceiver>> api(eventConfig);

  // subscribe book using tokenID
  api.subscribeBook(53428);  // FUT0 date 20221109
  api.subscribeBook(38910);  // FUT1
  api.subscribeBook(63881);  // FUT2
  api.subscribeBook(53423);
  //--------

  // application control for printing events
  bool printEvents = true;

  // start the receiving loop
  bool running = true;
  while (running) {
    // for every iteration of the loop, call the receiveEvent function
    const Event* event = api.receiveEvent();

    // test if the receiveEvent function returned an event
    if (event) {
      // if it did, print the event
      if (printEvents) std::cout << *event << std::endl << std::endl << std::endl;

      // if the event is a fatal error, something is horribly wrong, terminate the program
      if (event->type == EventMessageType::FatalError) {
        std::cout << "FATAL ERROR: " << event->channel->channelId << std::endl;
        running = false;
      }
      if (event->type == EventMessageType::MissingMessage) {
        std::cout << "Missing message " << *event->message.MissingMessage_ << std::endl;
        running = false;
      }
    }
  }
}
