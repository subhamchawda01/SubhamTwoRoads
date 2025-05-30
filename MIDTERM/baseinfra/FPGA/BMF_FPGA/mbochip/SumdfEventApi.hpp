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
-- Filename: SumdfEventApi.hpp
--
-- Description: 
--    
--
*/
#ifndef SUMDFEVENTAPI_HPP
#define SUMDFEVENTAPI_HPP

///@cond
#include "SumdfTypes.hpp"
#include <string>
#include <iostream>
#include "SumdfEvent.hpp"

#define FPGA_TICKS_PER_USEC 31.25

///@endcond

/*! \file SumdfEventApi.hpp
    \brief SiliconUMDF EventAPI calls.
*/

namespace SiliconUmdf {
namespace EventAPI {

    //! Opens the siliconUmdf Device.
    /*!
     * \param handler returns the device handler.
     * \param sumdfConfiguration contains the SiliconUmdf initial configuration.
     * \return Open returns status, indicating if the operation was successfully completed. Check the SiliconUMDF log in case of failure.
    */
    bool open (DeviceHandler &handler, sumdfConfiguration_t sumdfConfiguration);


    //! Closes the siliconUmdf Device.
    /*!
     * \param handler is the device handler.
     * \return CloseReturn status, indicating if the operation was successfully completed. Check the SiliconUMDF log in case of failure.
    */
    bool close (DeviceHandler &handler);
//~
//~
    //! Joins a BVMF channel.
    /*!
     * \param handler is the device handler.
     * \param sumdfChannelId number that identifies the channel. It is the position of the channel in the channelArray of sumdfConfiguration parameter passed in the open function.
     * \param instrumentFilterEnabled indicates whether the instruments filter is enabled or disabled for this channel.
     * \param numSecurityIds is the number of security ids being configured on the instrument filter. (Max 64)
     * \param securityIds[] is an array of size "numSecurityIds" of securityIds to be configured on the instrument filter. 
     * \param mdEntriesAllowed indicates which mdEntry types are allowed on this channel
//     * \return SiliconUmdf::EventAPI::JoinChannelReturn status, indicating if the operation was successfully completed. Check the SiliconUMDF log in case of failure.
    */
    JoinChannelReturn joinChannel (DeviceHandler &handler, unsigned sumdfChannelId, bool instrumentFilterEnabled, unsigned numSecurityIds, Uint64 securityIds[], MDEntriesAllowed_t mdEntriesAllowed);
            
            
    //! Leaves a BVMF channel   
    /*!
     * \param handler is the device handler.
     * \param sumdfChannelId number that identifies the channel. It is the position of the channel in the channelArray of sumdfConfiguration parameter passed in the open function.
     * \return the LeaveChannelReturn status, indicating if the operation was successfully completed. Check the SiliconUMDF log in case of failure.
    */
    bool leaveChannel (DeviceHandler &handler, unsigned sumdfChannelId);
    
    
    //! Requests the instrument list for a given channel
    /*!
     * \param handler is the device handler.
     * \param sumdfChannelId number that identifies the channel. It is the position of the channel in the channelArray of sumdfConfiguration parameter passed in the open function.
     * \return the RequestReturn status, indicating if the operation was successfully completed.  Check the SiliconUMDF log in case of failure.
    */
    bool requestInstrumentList(DeviceHandler &handler, unsigned sumdfChannelId);


    //! Waits for an Event    
    /*!
     * \param handler is the device handler.
     * \param event is the struct mapping the event returned
     * \return the WaitEventReturn status, indicating if the operation was successfully completed. Check the SiliconUMDF log in case of failure.
    */
    WaitEventsReturn waitEvents (DeviceHandler &handler, Events_t &event);
        
        
    //! Get channel status 
    /*!
     * \param handler is the device handler.
     * \param sumdfChannelId number that identifies the channel. It is the position of the channel in the channelArray of sumdfConfiguration parameter passed in the open function.
     * \param channelStatus is the structure with channelStatus information returned.
     * \return the GetChannelStatusReturn status, indicating if the operation was successfully completed. Check the SiliconUMDF log in case of failure.
    */
    bool getChannelStatus(DeviceHandler &handler, unsigned sumdfChannelId, ChannelStatus_t & channelStatus);
		
	
	//! Subscribes  on a Instrument
	/*!
	* \param handler is the device handler.
    * \param type selects the types of event to receive.
    * \param symbol is the instrument symbol. Ex.: DOLF20, PETR3
	* \return 
	*/
    SubscribeInstrumentReturn subscribeInstrument(DeviceHandler &handler, SubscriptionType type, const char symbol[33]);

	//! Unsubscribes from a Instrument
	/*!
	* \param handler is the device handler.
	* \param type It select the types of event to receive.
    * \param symbol Instrument symbol. Ex.: DOLF20, PETR3
	* \return
	*/
    UnsubscribeInstrumentReturn unsubscribeInstrument(DeviceHandler &handler, SubscriptionType type, const char symbol[33]);
	

	//! Subscribes on all instruments
	/*!
	* \param handler is the device handler.
    * \param type selects the type of events to subscribe.
	* \return
	*/
	SubscribeInstrumentReturn subscribeAll(DeviceHandler &handler, SubscriptionType type);
	
	//! Unsubscribes from all instruments
	/*!
	* \param handler is the device handler.
    * \param type selects the type of events to unsubscribe.
	* \return
	*/
	UnsubscribeInstrumentReturn unsubscribeAll(DeviceHandler &handler, SubscriptionType type);
	
    


	//! Write on the SiliconUMDF log
	/*!
	* \param handler is the device handler.
	* \param str text to write on the SiliconUMDF log.
	* \return void.
	*/
	void writeOnLog(DeviceHandler& handler, std::string str);   
    
    
} // namespace EventAPI
} // namespace SiliconUmdf
#endif
