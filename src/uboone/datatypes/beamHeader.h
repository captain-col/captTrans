#ifndef _UBOONETYPES_BEAMHEADER_H
#define _UBOONETYPES_BEAMHEADER_H

#include <sys/types.h>
#include <inttypes.h>
#include "evttypes.h"

#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>

#include "constants.h"

#include <iostream>

namespace gov {
namespace fnal {
namespace uboone {
namespace datatypes {

using namespace gov::fnal::uboone;

class beamHeader {

 public:
   uint8_t record_type;
   std::string event_signal;
   uint32_t seconds; // GPS clock. Since Jan 1, 2012. 
   uint16_t milli_seconds;
   uint8_t  number_of_devices;
   uint32_t number_of_bytes_in_record;

   beamHeader();

 private:

   friend class boost::serialization::access;

   template<class Archive>
     void serialize(Archive & ar, const unsigned int version)
     {

       if(version > 0)
	 ar & record_type & event_signal & seconds & milli_seconds & number_of_devices & number_of_bytes_in_record;

     }
  
};
}  // end of namespace datatypes
}  // end of namespace uboone
}  // end of namespace fnal
}  // end of namespace gov

std::ostream & operator<<(std::ostream &os, const gov::fnal::uboone::datatypes::beamHeader &bh);

// This MACRO must be outside any namespaces.

BOOST_CLASS_VERSION(gov::fnal::uboone::datatypes::beamHeader, gov::fnal::uboone::datatypes::constants::VERSION)    


#endif /* #ifndef BOONETYPES_H */
