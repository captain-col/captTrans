#include "eventRecord.h"

using namespace gov::fnal::uboone::datatypes;

eventRecord::eventRecord() {

  //use default constructors here
  global_header = globalHeader();
  trigger_data = triggerData();
  gps_data = gps();

  beam_header = beamHeader();
  beam_data_vector.clear();
  
  seb_map.clear();

  er_IO_mode = IO_GRANULARITY_CRATE;

}

//this updates all the crates and cards if necessary
void eventRecord::updateIOMode(uint8_t mode) {

  std::map<crateHeader,crateData>::iterator seb_it;
  for( seb_it = seb_map.begin(); seb_it != seb_map.end(); seb_it++)
    (seb_it->second).updateIOMode(mode);

  er_IO_mode = mode; //eventRecords io_mode
}

//insert crateHeader,crateData pair
void eventRecord::insertSEB(crateHeader cH, crateData cD){ 
  seb_map.insert( std::pair<crateHeader,crateData>(cH,cD) ); 
}
