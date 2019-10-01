
#include <net-snmp/net-snmp-config.h>

#include "PSKSnmp.h"

#include <boost/bind.hpp>


int main(void) {
  PSKSnmp psksnmp;
  
  boost::thread psk_gst_thread, psk_message_thread;

  psk_gst_thread = boost::thread(boost::bind(&PSKSnmp::setGstStreams, &psksnmp));
  psk_message_thread = boost::thread(boost::bind(&PSKSnmp::receiveMessages, &psksnmp));
  
  psk_gst_thread.join();
  psk_message_thread.join();
  
  return 0;
}