
#include "snmplib.h"
#include "PSKSnmp.h"
#include <msglib.h>
#include <iostream>
#include <fstream>
#include <sstream>

#define SNMP_STAT_SUCCESS 1
#define SNMP_STAT_TIMEOUT 2
#define SNMP_STAT_ERROR   3

int PSKSnmp::executeSnmpScript(const char * oidScriptName) {
  this->changeOidName(oidScriptName);
  this->initializeSNMP();
  this->startSNMPAsyncRequest();
  
  return this->getSnmpStat();
}

int PSKSnmp::getSnmpStat() {
  int snmp_stat = SNMPLib::request_state;
   
  switch(snmp_stat) {
     case 0: return SNMP_STAT_SUCCESS;
     case 2: return SNMP_STAT_TIMEOUT;
     case 1: return SNMP_STAT_ERROR;
     default: return 0;
  }
}

/* Establish Gstreamer audio pipelines 
 * for communication with Operator and Decoder units
 * Note: this is executed as a separate thread. It has
 * been done in order to have possibility to add new
 * functions and threads in future, e.g. :
 * reading GPS coords. of Operator or sending commands
 * to Decoder.
 */
void PSKSnmp::setGstStreams() {
  /* infinite loop 
   * it is executed every n sec. in thread
   */
  for(;;) {
    if(_sat_online == 1) {
	system("/usr/local/PSK200/main_start_3point_audio.sh");
	
	//if(_snr_value != 0)
	  _set_audio_stream_res = this->executeSnmpScript("NET-SNMP-EXTEND-MIB::nsExtendResult.\"start_3_point\"");
    }
    else {
	system("/usr/local/PSK200/main_start_2point_audio.sh");
	
	//if(_snr_value != 0)
	  _set_audio_stream_res = this->executeSnmpScript("NET-SNMP-EXTEND-MIB::nsExtendResult.\"start_2_point\"");
    }
    
    if(_video_state == 1) {

	  _set_video_stream_res = this->executeSnmpScript("NET-SNMP-EXTEND-MIB::nsExtendResult.\"analog\"");
    }
    else {

	  _set_video_stream_res = this->executeSnmpScript("NET-SNMP-EXTEND-MIB::nsExtendResult.\"hdmi\"");
    }
    
    boost::this_thread::sleep(boost::posix_time::seconds(2));
  }
}

void PSKSnmp::startSNMPAsyncRequest() {
  SNMPLib::asynchronous();
}

void PSKSnmp::initializeSNMP() {
  SNMPLib::initialize();
}

void PSKSnmp::changeOidName(const char *oidName) {
  SNMPLib::oids[0] = { oidName };
}

void PSKSnmp::initializeMessageQueue() {
  /* create message queue server */
  
  _msgid = MSGLib::create_message_queue();
  
  std::cout << "PSKSnmp::Message_queue_id - " << _msgid << std::cout;
}

void PSKSnmp::receiveMessages() {
    this->initializeMessageQueue();
    
    for(;;) {
      /* wait for message from client (PSK GUI) 
       * msg reciever number is 2
       */
      MSGLib::recieve_message(_msgid, 2);
      
      switch(MSGLib::message.mtype) {
	case 1: this->readSnr(); break;
	case 2: this->readParams(); break;
	case 3: this->readVideoState(); break;
	default: std::cout << "PSKSnmp::receiveMessages:Wrong_Message_Type" << std::endl;
      }
      
      boost::this_thread::sleep(boost::posix_time::seconds(1));
    }
}

void PSKSnmp::readSnr() {
  //std::cout << "PSKSnmp::readSnr():_snr_value = " << MSGLib::message.body << std::endl;
  
  _snr_value = std::atof(MSGLib::message.body);
}

void PSKSnmp::readParams() {
  std::cout << "PSKSnmp::readParams():msg = " << MSGLib::message.body << std::endl;
  boost::cmatch msg_m;
  
  if(boost::regex_match(MSGLib::message.body, msg_m, msg_reg)) {
      std::cout << "PSKSnmp::readParams():Match" << std::endl;
      
      _sat_online = std::atoi(msg_m[1].str().c_str());
      _video_state = std::atoi(msg_m[2].str().c_str());
  }
  
}

void PSKSnmp::readVideoState() {
  std::cout << "PSKSnmp::readVideoState():_video_state = " << MSGLib::message.body << std::endl;
  
  _video_state = std::atoi(MSGLib::message.body);
}



