#ifndef _UBOONETYPES_BEAMDATA_H
#define _UBOONETYPES_BEAMDATA_H

#include <sys/types.h>
#include <inttypes.h>
#include "evttypes.h"

#include <iostream>
#include <vector>

#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>

#include "constants.h"

namespace gov {
namespace fnal {
namespace uboone {
namespace datatypes {

using namespace gov::fnal::uboone;

class beamData {

 public:
   std::string deviceName;
   std::string units;
   std::vector<double> val;
   
   beamData();   
   
 private:

   friend class boost::serialization::access;

   template<class Archive>
     void serialize(Archive & ar, const unsigned int version)
     {
       if(version > 0)
         ar & deviceName & units & val;
     }
   
};
}  // end of namespace datatypes
}  // end of namespace uboone
}  // end of namespace fnal
}  // end of namespace gov

std::ostream & operator<<(std::ostream &os, const gov::fnal::uboone::datatypes::beamData &bd);

// This MACRO must be outside any namespaces.

BOOST_CLASS_VERSION(gov::fnal::uboone::datatypes::beamData, gov::fnal::uboone::datatypes::constants::VERSION)    

#endif /* #ifndef BOONETYPES_H */
