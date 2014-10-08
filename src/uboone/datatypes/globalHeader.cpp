#include "globalHeader.h"


using namespace gov::fnal::uboone::datatypes;

globalHeader::globalHeader() {

  record_type = RESERVED;
  record_origin = 2;
  run_number = 0xdadadada;
  event_number = 0xdadadada;
  event_number_crate = 0xdadadada; 
  
  seconds = 0xdadadada; // GPS clock. Since Jan 1, 2012. 
  // Do we need to worry about Leap seconds?
  milli_seconds = 0xdada;
  micro_seconds = 0xdada;
  nano_seconds = 0xdada;
  numberOfBytesInRecord = 0xdadadada;

  number_of_sebs = 0xd0;

}

