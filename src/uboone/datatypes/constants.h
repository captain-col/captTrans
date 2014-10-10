#ifndef _UBOONETYPES_CONSTANTS_H
#define _UBOONETYPES_CONSTANTS_H

namespace gov {
namespace fnal {
namespace uboone {
namespace datatypes {

  //Readout options for levels of detail in the data readout/readin.
  enum {
    IO_GRANULARITY_CRATE,
    IO_GRANULARITY_CARD,
    IO_GRANULARITY_CHANNEL
  };
  
  //Crate types.
  enum {
    TPC_CRATE,
    PMT_CRATE
  };

namespace constants
{
  const int VERSION = 5; // A dB query eventually. // NJT: shouldn't this match major version number for datatypes package?
    // ... other related constants

} // namespace constants


}
}
}
}

#endif // _UBOONETYPES_CONSTANTS_H
