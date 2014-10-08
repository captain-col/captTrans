#include "cardData.h"

using namespace gov::fnal::uboone::datatypes;


char* cardData::getCardDataPtr(){
  
  if(cardData_IO_mode >= IO_GRANULARITY_CHANNEL){
    std::cout << "ERROR! Granularity is above card level." 
	      << "Cannot return pointer to card data!" << std::endl;
    return nullptr;
  }
  else{
    return card_data_ptr.get();
  }
}

void cardData::setCardDataPtr(char* ptr){

  if(cardData_IO_mode >= IO_GRANULARITY_CHANNEL){
    std::cout << "ERROR! Granularity is above card level." 
	      << "Cannot set pointer to card data!" << std::endl;
  }
  else{
    card_data_ptr.reset(ptr);
  }
}

void cardData::updateIOMode(uint8_t new_mode){

  //we are already at card granularity...so get out if that's the case
  if(new_mode <= IO_GRANULARITY_CARD)
    return;

  if(new_mode >= IO_GRANULARITY_CHANNEL && cardData_IO_mode < IO_GRANULARITY_CHANNEL){

    size_t total_data_read = 0;
    size_t size16 = sizeof(uint16_t);
    while(total_data_read < card_data_size){
      
      //get the channel header word
      std::unique_ptr<uint16_t> channel_header(new uint16_t);
      std::copy(getCardDataPtr() + total_data_read,
		getCardDataPtr() + total_data_read + size16,
		(char*)channel_header.get());
      total_data_read += size16;
      
      //std::cout << "Channel header " << std::hex << *channel_header << std::endl;
      
      //now loop until we find the channel trailer
      std::unique_ptr<uint16_t> channel_trailer(new uint16_t);
      size_t channel_data_size = 0;
      while(total_data_read < card_data_size){
	std::copy(getCardDataPtr() + total_data_read,
		  getCardDataPtr() + total_data_read + size16,
		  (char*)channel_trailer.get());
	//std::cout << "Next Word " << std::hex << *channel_trailer << std::endl;
      	total_data_read += size16;

	if(*channel_trailer & 0x5000)
	  break;
	else 
	  channel_data_size += size16;
      }//end while over channel data

      //pointer to the channel data
      std::shared_ptr<char> channel_data_ptr(new char[channel_data_size]);
      std::copy(getCardDataPtr() + total_data_read - channel_data_size - size16,
		getCardDataPtr() + total_data_read - size16,
		channel_data_ptr.get());

      //now initialise channelData object, and store in map
      channelData chD(channel_data_ptr,channel_data_size,*channel_header,*channel_trailer);
      insertChannel(chD.getChannelNumber(),chD);

    }//end while over card data size

    card_data_ptr.reset();
    cardData_IO_mode = IO_GRANULARITY_CHANNEL;
  }//endif channel granularity update


}

void cardData::insertChannel(int channel_number, channelData chD){
  channel_map.insert(std::pair<int,channelData>(channel_number,chD));
}
