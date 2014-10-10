#include "windowHeaderPMT.h"

using namespace gov::fnal::uboone::datatypes;

windowHeaderPMT::windowHeaderPMT(){
  bt_pmt_window_header.ch_and_disc = 0;
  bt_pmt_window_header.frame_and_sample1 = 0;
  bt_pmt_window_header.sample2 = 0;
}

uint8_t windowHeaderPMT::getChannelNumber() const{
  uint8_t Channel = bt_pmt_window_header.ch_and_disc & 0x3f;
  return Channel;
}

uint8_t windowHeaderPMT::getDiscriminant() const{
  uint8_t Disc = (bt_pmt_window_header.ch_and_disc>>9) & 0x7;
  return Disc;
}

uint16_t windowHeaderPMT::getFrame() const{
  uint16_t Frame = (bt_pmt_window_header.frame_and_sample1>>5) & 0x7;
  return Frame;
}

uint32_t windowHeaderPMT::getSample() const{
  uint32_t Sample = ((bt_pmt_window_header.frame_and_sample1 & 0x1f)<<12) + (bt_pmt_window_header.sample2 & 0xfff);
  return Sample;
}

