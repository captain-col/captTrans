#ifndef _UBOONETYPES_CARDHEADERPMT_H
#define _UBOONETYPES_CARDHEADERPMT_H
#include <memory>
#include <algorithm>
#include <sys/types.h>
#include <inttypes.h>
#include "evttypes.h"
#include "share/boonetypes.h"

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
   Note: this is the serialization class for the hardcoded card_header_t struct.
   If there is a change there, it needs to be made here too. If there is a 
   change made here, it may do no good without a change there.
***/


class cardHeaderPMT {
 public:
  static const uint8_t DAQ_version_number = gov::fnal::uboone::datatypes::constants::VERSION;
  cardHeaderPMT();
  cardHeaderPMT(pmt_card_header_t cardH) { bt_pmt_card_header = cardH; }

  uint32_t getIDAndModuleWord() const { return bt_pmt_card_header.id_and_module; }
  uint32_t getWordCountWord()  const { return bt_pmt_card_header.word_count; }
  uint32_t getEventWord()  const { return bt_pmt_card_header.event_number; }
  uint32_t getFrameWord()  const { return bt_pmt_card_header.frame_number; }  
  uint32_t getChecksumWord()  const { return bt_pmt_card_header.checksum; }
  uint32_t getTrigFrameAndSampleWord()  const { return bt_pmt_card_header.trig_frame_and_sample; }
  
  void setIDAndModuleWord(uint32_t word) { bt_pmt_card_header.id_and_module = word; }
  void setWordCountWord(uint32_t word) { bt_pmt_card_header.word_count = word; }
  void setEventNumberWord(uint32_t word) { bt_pmt_card_header.event_number = word; }
  void setFrameNumberWord(uint32_t word) { bt_pmt_card_header.frame_number = word; }  
  void setChecksumWord(uint32_t word) { bt_pmt_card_header.checksum = word; }
  void setTrigFramAndSampleWord(uint32_t word) { bt_pmt_card_header.trig_frame_and_sample = word; }

  pmt_card_header_t getCardHeader()  const { return bt_pmt_card_header; }
  void setCardHeader(pmt_card_header_t cardH) { bt_pmt_card_header = cardH; }

  uint32_t getID() const;
  uint32_t getModule() const;
  uint32_t getEvent() const;
  uint32_t getFrame() const;
  uint32_t getChecksum() const;
  uint32_t getWordCount() const;
  uint32_t getTrigFrame() const;
  uint8_t  getTrigFrameMod16() const;
  uint32_t getTrigSample() const;
 
  size_t getCardDataSize() const;

 private:
  pmt_card_header_t bt_pmt_card_header;

  friend class boost::serialization::access;
  
  template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      if(version>0)
	ar & bt_pmt_card_header.id_and_module
	   & bt_pmt_card_header.word_count
	   & bt_pmt_card_header.event_number
 	   & bt_pmt_card_header.frame_number
	   & bt_pmt_card_header.checksum
           & bt_pmt_card_header.trig_frame_and_sample;
    }

};
}  // end of namespace datatypes
}  // end of namespace uboone
}  // end of namespace fnal
}  // end of namespace gov

// This MACRO must be outside any namespaces.
BOOST_CLASS_VERSION(gov::fnal::uboone::datatypes::cardHeaderPMT, gov::fnal::uboone::datatypes::constants::VERSION)    
#endif /* #ifndef BOONETYPES_H */



