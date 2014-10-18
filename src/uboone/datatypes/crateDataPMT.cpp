#include "crateDataPMT.h"
#include <iostream> // for debugging
#include <stdexcept>

using namespace gov::fnal::uboone::datatypes;

char* crateDataPMT::getCrateDataPtr(){
  
  if(crateData_IO_mode >= IO_GRANULARITY_CARD){
    throw std::runtime_error("crateDataPMT::getCardDataPtr() ERROR! Granularity is above crate level.");
    return NULL;
  }
  else {
    return crate_data_ptr.get();
  }
}

const char* crateDataPMT::getCrateDataPtr() const{
  
  if(crateData_IO_mode >= IO_GRANULARITY_CARD){
    throw std::runtime_error("crateDataPMT::getCardDataPtr() ERROR! Granularity is above crate level.");
    return NULL;
  }
  else {
    return crate_data_ptr.get();
  }
}

void crateDataPMT::setCrateDataPtr(char* ptr){

  if(crateData_IO_mode >= IO_GRANULARITY_CARD){
    throw std::runtime_error("crateDataPMT::setCardDataPtr() ERROR! Granularity is above crate level.");
  }
  else {
    crate_data_ptr.reset(ptr);
  }
}

void crateDataPMT::updateIOMode(uint8_t new_mode){

  //we are already at crate granularity...so get out if that's the case
  if(new_mode <= IO_GRANULARITY_CRATE)
    return;

  const size_t size16 = sizeof(uint16_t);

  if(new_mode >= IO_GRANULARITY_CARD && crateData_IO_mode < IO_GRANULARITY_CARD){

    const char* ptr = getCrateDataPtr();
    size_t data_read = 0;
    
    // Read the crate header, make sure it looks right.
    event_header_t* header = (event_header_t*)ptr;
    event_header.setEventHeader(*header);
    data_read += sizeof(event_header_t);
    if(event_header.getHeader() != 0xffffffff) throw std::runtime_error("Bad pmt crate event_header word.");
    
    int cards_read = 0;
    bool done = false;
    while(!done){
      // Sanity check: is there enough data left in the buffer to read a single card_header?
      if(crate_data_size - data_read < sizeof(pmt_card_header_t))  throw std::runtime_error("PMT data error - not enough data to form a card header.");

      // Copy the card header into a datatypes class:
      pmt_card_header_t* my_card_header = (pmt_card_header_t*)(ptr+data_read);
      data_read += sizeof(pmt_card_header_t);
      
      cardHeaderPMT cardH(*my_card_header);
      size_t cardDataSize = cardH.getCardDataSize();

      // Sanity check. 
      if(data_read + cardDataSize > crate_data_size) throw std::runtime_error("PMT cardDataSize error - card data bigger than remaining crate data.");

        // std::cout << "Card header ...\n"
  // << std::hex << memblkCardH->id_and_module << " " << memblkCardH->word_count << " " 
  // << memblkCardH->event_number << " " << memblkCardH->frame_number<< " " << memblkCardH->checksum << std::dec << std::endl;


      std::shared_ptr<char> card_data(new char[cardDataSize]);
      std::copy(ptr + data_read,
                ptr + data_read + cardDataSize,
                (char*)card_data.get());
      //wait to increment data_read until after updating channel granularity

      cardDataPMT cardD(card_data,cardDataSize);
      if(new_mode == IO_GRANULARITY_CHANNEL)
             cardD.updateIOMode(new_mode);

      //now increment the data_read variable
      data_read += cardDataSize;

      // Got a header and data object for this card. Put it in the map.
      insertCard(cardH,cardD);
      cards_read++;


      // Do we have enough space left to see another card header?
      if(data_read + sizeof(pmt_card_header_t) > crate_data_size) {

        // No. Let's see if there's an end-of-event record in here.
        // std::cout << "Check for end. crate_data_size: " << crate_data_size << " data_read: " << data_read << std::endl;
        for( size_t offset = 0; offset < (crate_data_size-data_read); offset+= size16){
          event_trailer_t* trailer = ((event_trailer_t*)(ptr + data_read+ offset));
          // std::cout << "trailer: " << trailer->trailer << std::endl;
          if(trailer->trailer==0xe0000000){
            event_trailer.setEventTrailer(*trailer);
            data_read += offset + sizeof(event_trailer_t);
            done = true;
            break;
          }
        }
        if(!done)     // Hmm. There's not enough room for another card header, but we didn't find a trailer. Problem!  
          throw std::runtime_error("Could not find event_trailer word.");
        
      }


    } // done looping cards; done was true.
    
    if(event_trailer.getTrailer() != 0xe0000000) throw std::runtime_error("Bad pmt crate event_trailer word."); // shouldn't ever happen.
    crate_data_ptr.reset();

    crateData_IO_mode = new_mode;
  } //endif on IO_GRANULARITY_CARD update

  if(new_mode == IO_GRANULARITY_CHANNEL && crateData_IO_mode < IO_GRANULARITY_CHANNEL){

    // this code activated when current granularity is card, wanted is channel
    std::map<cardHeaderPMT,cardDataPMT>::iterator card_it;
    for( card_it = card_map.begin(); card_it != card_map.end(); card_it++){
      (card_it->second).updateIOMode(new_mode);      
    }

    crateData_IO_mode = new_mode; //eventRecords io_mode

  }//endif on IO_GRANULARITY_CHANNEL update

}

void crateDataPMT::insertCard(cardHeaderPMT cH, cardDataPMT cD){
  card_map.insert(std::pair<cardHeaderPMT,cardDataPMT>(cH,cD));
}
