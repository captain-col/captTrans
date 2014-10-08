#ifndef _UBOONETYPES_GLOBALHEADER_H
#define _UBOONETYPES_GLOBALHEADER_H
#include <sys/types.h>
#include <inttypes.h>
#include "evttypes.h"

#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>

#include "constants.h"

namespace gov {
namespace fnal {
namespace uboone {
namespace datatypes {

using namespace gov::fnal::uboone;

/***
 *  Global header is attached to front of each event by assembler, 
 *  in ProcessAndShipEventReader thread.
 ***/


class globalHeader {

 public:
  static const uint8_t DAQ_version_number = gov::fnal::uboone::datatypes::constants::VERSION;
  globalHeader();
  
  void setRecordType(uint8_t type) { record_type = type; }
  void setRecordOrigin(uint8_t origin) { record_origin = origin; }
  void setRunNumber(uint32_t run) { run_number = run; }
  void setEventNumber(uint32_t event) { event_number = event; }
  void setEventNumberCrate(uint32_t event) { event_number_crate = event; }

  void setSeconds(uint32_t s) { seconds = s; }
  void setMilliSeconds(uint16_t ms) { milli_seconds = ms; }
  void setMicroSeconds(uint16_t us) { micro_seconds = us; }
  void setNanoSeconds(uint16_t ns) { nano_seconds = ns; }
  void setNumberOfBytesInRecord(uint32_t size) { numberOfBytesInRecord = size; }

  uint8_t getRecordType() { return record_type; }
  uint8_t getRecordOrigin() { return record_origin; }
  uint32_t getRunNumber() { return run_number; }
  uint32_t getEventNumber() { return event_number; }
  uint32_t getEventNumberCrate() { return event_number_crate; }
  
  uint32_t getSeconds() { return seconds; }
  uint16_t getMilliSeconds() { return milli_seconds; }
  uint16_t getMicroSeconds() { return micro_seconds; }
  uint16_t getNanoSeconds() { return nano_seconds; }
  uint32_t getNumberOfBytesInRecord() { return numberOfBytesInRecord; }

  uint8_t getNumberOfSEBs() { return number_of_sebs;}
  void setNumberOfSEBs(uint8_t s) { number_of_sebs = s;}

 private:
  uint8_t record_type;   /* From event_types.h */
  uint8_t record_origin; /* DATA or MC */
  uint32_t run_number;
  uint32_t event_number;
  uint32_t event_number_crate; /* Crate's sense of the evt #. */
  
  uint32_t seconds; // GPS clock. Since Jan 1, 2012. 
  uint16_t milli_seconds;
  uint16_t micro_seconds;
  uint16_t nano_seconds;
  uint32_t numberOfBytesInRecord;

  uint8_t number_of_sebs; //put this in just to test versioning

  friend class boost::serialization::access;
  
  template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      if(version>0)
	ar & record_type & record_origin
 	   & run_number & event_number & event_number_crate
	   & seconds & milli_seconds & micro_seconds & nano_seconds
	   & numberOfBytesInRecord & number_of_sebs;

    }
  
};


}  // end of namespace sebs
}  // end of namespace uboone
}  // end of namespace fnal
}  // end of namespace gov

// This MACRO must be outside any namespaces.
BOOST_CLASS_VERSION(gov::fnal::uboone::datatypes::globalHeader, gov::fnal::uboone::datatypes::constants::VERSION)    

#endif /* #ifndef BOONETYPES_H */



