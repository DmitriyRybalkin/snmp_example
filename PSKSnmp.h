#ifndef PSKSNMP_H
#define PSKSNMP_H

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

class PSKSnmp {
  
  public:
    PSKSnmp(): _snr_value(0), _sat_online(0), _set_audio_stream_res(0),
    msg_reg("sat_online=(\\d+);video_state=(\\d+);")
    {

    };
    
    int executeSnmpScript(const char * oidScriptName);
    void setGstStreams();
    void receiveMessages();
    
  private:
    double _snr_value;
    int _sat_online;
    int _set_audio_stream_res;
    int _set_video_stream_res;
    int _msgid;
    int _video_state;
    
    const boost::regex msg_reg;
    
    inline void startSNMPAsyncRequest();
    inline void initializeSNMP();
    inline void changeOidName(const char *oidName);
    int getSnmpStat();
    
    inline void readSnr();
    inline void readParams();
    inline void readVideoState();
    inline void initializeMessageQueue();

};

#endif