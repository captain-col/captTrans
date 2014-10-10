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
  seb_pmt_map.clear();

  er_IO_mode = IO_GRANULARITY_CRATE;

}

//this updates all the crates and cards if necessary
void eventRecord::updateIOMode(uint8_t mode) {

  std::map<crateHeader,crateData>::iterator seb_it;
  for( seb_it = seb_map.begin(); seb_it != seb_map.end(); seb_it++){
    try {
      (seb_it->second).updateIOMode(mode);
    } catch (std::runtime_error& e) {
      int crate = (seb_it->first).getCrateNumber();
      std::string err = "Error unpacking TPC crate " + std::to_string(crate) + std::string(": ")+  e.what();
      throw std::runtime_error(err);
    }
  }

  std::map<crateHeader,crateDataPMT>::iterator seb_pmt_it;
  for( seb_pmt_it = seb_pmt_map.begin(); seb_pmt_it != seb_pmt_map.end(); seb_pmt_it++) {
    try {
      (seb_pmt_it->second).updateIOMode(mode);
    } catch (std::runtime_error& e) {
      int crate = (seb_it->first).getCrateNumber();
      std::string err = "Error unpacking PMT crate " + std::to_string(crate) + std::string(": ")+  e.what();
      throw std::runtime_error(err);
    }
  }

  er_IO_mode = mode; //eventRecords io_mode
}

//insert crateHeader,crateData pair
void eventRecord::insertSEB(crateHeader cH, crateData cD){ 
  seb_map.insert( std::pair<crateHeader,crateData>(cH,cD) ); 
}

void eventRecord::insertSEB(crateHeader cH, crateDataPMT cD){ 
  seb_pmt_map.insert( std::pair<crateHeader,crateDataPMT>(cH,cD) ); 
}

void::eventRecord::decompress(){

  if(er_IO_mode < IO_GRANULARITY_CHANNEL)
    updateIOMode(IO_GRANULARITY_CHANNEL);

  std::map<crateHeader,crateData>::iterator seb_it;
  for( seb_it = seb_map.begin(); seb_it != seb_map.end(); seb_it++)
    (seb_it->second).decompress();
  
}
