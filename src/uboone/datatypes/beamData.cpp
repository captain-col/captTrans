#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>

#include "beamData.h"

using namespace gov::fnal::uboone::datatypes;

std::ostream & operator<<(std::ostream &os, const gov::fnal::uboone::datatypes::beamData &bd)
{
  os <<"Device name: " << bd.deviceName << std::endl
     <<"Units: "<< bd.units << std::endl
     <<"Value(s): "<<bd.val.at(0);
  for (size_t i=1;i<bd.val.size();i++) os <<", "<<bd.val[i];
  os <<std::endl;

  return os;
}

beamData::beamData()
{
  deviceName="";
  units="";
  val.resize(0);
}
