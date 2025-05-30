#include <ManagerApi.hpp>
#include <iostream>
using namespace std;
using namespace MBOCHIP::SiliconMD::EApi;
using namespace MBOCHIP::SiliconMD::MApi;

int main(int argc, char** args) {

    /**********************************************************************************
    * Event API and channels configuration
    **********************************************************************************/
    ChannelConfig config{};
    config.recovery = true;
    EventConfig eventConfig("/home/pengine/prod/live_configs/configNSE.toml");

    /**********************************************************************************
    * Initialize the API
    **********************************************************************************/
    ManagerApi<MessageApi<NSEFPGAReceiver>> api(eventConfig);

    /**********************************************************************************
    * Instruments filtering
    **********************************************************************************/
    ChannelFilter channel011Filter{};
    channel011Filter.instruments.push_back(57659);
    channel011Filter.instruments.push_back(50411);
    api.enableChannel("011", config, channel011Filter);

    /**********************************************************************************
    * Application logic
    **********************************************************************************/
    /* Control for printing events */
    bool printEvents = false;

    /* Start the receiving loop */
    bool running = true;
    while (running) {
        /* For every iteration of the loop, call the receiveEvent function */
        const Event* event = api.receiveEvent();
        if (event) {
            if (printEvents)
                std::cout << *event << std::endl << std::endl;
            switch (event->type) {
                case MBOCHIP::SiliconMD::EApi::EventMessageType::FatalError:
                    std::cout << "FATAL ERROR: " << event->channel->channelId << std::endl;
                    running = false;
                    break;
                case MBOCHIP::SiliconMD::EApi::EventMessageType::SnapshotError:
                    std::cout << "SNAPSHOT ERROR: " << event->channel->channelId << std::endl;
                    break;
                case MBOCHIP::SiliconMD::EApi::EventMessageType::MissingMessage:
                    std::cout << "MISSING MESSAGE: " << event->channel->channelId << std::endl;
                    running = false;
                    break;
                default:
                    break;
            }
        }
    }
}
