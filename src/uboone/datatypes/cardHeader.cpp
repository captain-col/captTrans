#include "cardHeader.h"

using namespace gov::fnal::uboone::datatypes;

cardHeader::cardHeader(){
  bt_card_header.id_and_module = 0;
  bt_card_header.word_count = 0;
  bt_card_header.event_number = 0;
  bt_card_header.frame_number = 0;
  bt_card_header.checksum = 0;
  bt_card_header.trig_frame_and_sample = 0;
}

uint32_t cardHeader::getID() const{
  uint32_t Crate_ID = (((bt_card_header.id_and_module>>16) & 0xfff)>>5) & 0x7f;  //was called mod_id before
  return Crate_ID;
}

uint32_t cardHeader::getModule() const{
  uint32_t Module = (bt_card_header.id_and_module>>16) & 0x1f;  //was called mod_id before
  return Module;
}

uint32_t cardHeader::getEvent() const{
  uint32_t Event = ((bt_card_header.event_number>>16) & 0xfff) + ((bt_card_header.event_number& 0xfff) <<12);
  return Event;
}

uint32_t cardHeader::getFrame() const{
  uint32_t Frame = ((bt_card_header.frame_number>>16) & 0xfff) + ((bt_card_header.frame_number & 0xfff) <<12);
  return Frame;
}

uint32_t cardHeader::getWordCount() const{
  uint32_t wc = (  ((bt_card_header.word_count>>16) & 0xfff)+((bt_card_header.word_count & 0xfff)<<12) );
  return wc;
}

size_t cardHeader::getCardDataSize() const{
  size_t DataSize = (getWordCount()+1) * sizeof(uint16_t);
  return DataSize;
}

uint32_t cardHeader::getChecksum() const{
  return ((bt_card_header.checksum &0xFFF) << 12) + ((bt_card_header.checksum >> 16)&0xFFF);
}

uint8_t cardHeader::getTrigFrameMod16() const{
  return ((bt_card_header.trig_frame_and_sample>>4) & 0xF);
}

uint32_t cardHeader::getTrigFrame() const{
  // Attempt to resolve the rollover situation: the lower 4 bits are given by the trigger frame, which should crudely match the upper bits from the course frame. 
  // Here is how I resolved it --- Nathaniel
  uint32_t frameCourse = getFrame();
  uint32_t option1 = (getFrame()&0xFFFFFFF0) | (getTrigFrameMod16());
  int32_t diff = option1-frameCourse;
  if(diff > 8) return option1 - 0x10; // Solution is too high; rollover down
  if(diff < 8) return option1 + 0x10; // Solution is too low; rollover up.
  return option1; // if within 8 ticks, this solution is fine.
}

uint32_t cardHeader::getTrigSample() const{
  uint32_t Trig_Sample = ((bt_card_header.trig_frame_and_sample >> 16) & 0xFF) + ((bt_card_header.trig_frame_and_sample & 0xF)<<8);
  return Trig_Sample;
}
