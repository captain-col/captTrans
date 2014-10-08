#include "crateData.h"

using namespace gov::fnal::uboone::datatypes;

char* crateData::getCrateDataPtr(){
  
  if(crateData_IO_mode >= IO_GRANULARITY_CARD){
    std::cout << "ERROR! Granularity is above crate level." 
	      << "Cannot return pointer to crate data!" << std::endl;
    return nullptr;
  }
  else {
    return crate_data_ptr.get();
  }
}

void crateData::setCrateDataPtr(char* ptr){

  if(crateData_IO_mode >= IO_GRANULARITY_CARD){
    std::cout << "ERROR! Granularity is above crate level." 
	      << "Cannot set pointer to crate data!" << std::endl;
  }
  else {
    crate_data_ptr.reset(ptr);
  }
}

void crateData::updateIOMode(uint8_t new_mode){

  //we are already at crate granularity...so get out if that's the case
  if(new_mode <= IO_GRANULARITY_CRATE)
    return;

  if(new_mode >= IO_GRANULARITY_CARD && crateData_IO_mode < IO_GRANULARITY_CARD){

    size_t data_read = 0;
    std::unique_ptr<event_header_t> memblkEH(new event_header_t);
    std::unique_ptr<event_trailer_t> memblkET(new event_trailer_t);

    std::copy(getCrateDataPtr() + data_read,
	      getCrateDataPtr() + data_read + sizeof(event_header_t),
	      (char*)memblkEH.get());
    event_header.setEventHeader(*memblkEH);
    data_read += sizeof(event_header_t);
    
    while(1){
      std::unique_ptr<card_header_t> memblkCardH(new card_header_t);
      std::copy(getCrateDataPtr() + data_read,
		getCrateDataPtr() + data_read + sizeof(card_header_t),
		(char*)memblkCardH.get());
      data_read += sizeof(card_header_t);
      
      cardHeader cardH(*memblkCardH);
      size_t cardDataSize = cardH.getCardDataSize();

      /*
      std::cout << "Card header ...\n"
	<< std::hex << memblkCardH->id_and_module << " " << memblkCardH->word_count << " " 
	<< memblkCardH->event_number << " " << memblkCardH->frame_number<< " " << memblkCardH->checksum << std::dec << std::endl;

      std::cout << std::hex
		<< cardH.getIDAndModuleWord() << " " << cardH.getWordCountWord() << " "
		<< cardH.getEventWord() << " " << cardH.getFrameWord() << " " << cardH.getChecksumWord() << std::endl;
      */

      std::shared_ptr<char> card_data(new char[cardDataSize]);
      std::copy(getCrateDataPtr() + data_read,
		getCrateDataPtr() + data_read + cardDataSize,
		(char*)card_data.get());
      data_read += cardDataSize;
      
      cardData cardD(card_data,cardDataSize);
      if(new_mode == IO_GRANULARITY_CHANNEL) cardD.updateIOMode(new_mode);
      insertCard(cardH,cardD);
      
      std::copy(getCrateDataPtr() + data_read,
		getCrateDataPtr() + data_read + sizeof(event_trailer_t),
		(char*)memblkET.get());
      if(memblkET->trailer == 0xe0000000) break;
    }
    
    event_trailer.setEventTrailer(*memblkET);
    data_read += sizeof(event_trailer_t);
    crate_data_ptr.reset();

    crateData_IO_mode = new_mode;
  } //endif on IO_GRANULARITY_CARD update

  if(new_mode == IO_GRANULARITY_CHANNEL && crateData_IO_mode < IO_GRANULARITY_CHANNEL){
    
    std::map<cardHeader,cardData>::iterator card_it;
    for( card_it = card_map.begin(); card_it != card_map.end(); card_it++)
      (card_it->second).updateIOMode(new_mode);
    
    crateData_IO_mode = new_mode; //eventRecords io_mode

  }//endif on IO_GRANULARITY_CHANNEL update

}

void crateData::insertCard(cardHeader cH, cardData cD){
  card_map.insert(std::pair<cardHeader,cardData>(cH,cD));
}
