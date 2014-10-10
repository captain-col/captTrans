#include "crateData.h"
#include <stdexcept>

using namespace gov::fnal::uboone::datatypes;

char* crateData::getCrateDataPtr(){
  
  if(crateData_IO_mode >= IO_GRANULARITY_CARD){
    throw std::runtime_error("crateData::getCardDataPtr() ERROR! Granularity is above crate level.");
    return nullptr;
  }
  else {
    return crate_data_ptr.get();
  }
}

const char* crateData::getCrateDataPtr() const{
  if(crateData_IO_mode >= IO_GRANULARITY_CARD){
    throw std::runtime_error("crateData::getCrateDataPtr() ERROR! Granularity is above crate level.");
    return nullptr;
  }
  else {
    return crate_data_ptr.get();
  }
}

void crateData::setCrateDataPtr(char* ptr){

  if(crateData_IO_mode >= IO_GRANULARITY_CARD){
    throw std::runtime_error("crateData::setCrateDataPtr() ERROR! Granularity is above crate level.");
  }
  else {
    crate_data_ptr.reset(ptr);
  }
}

void crateData::updateIOMode(uint8_t new_mode){

  //we are already at crate granularity...so get out if that's the case
  if(new_mode <= IO_GRANULARITY_CRATE)
    return;

  const size_t size16 = sizeof(uint16_t);

  if(new_mode >= IO_GRANULARITY_CARD && crateData_IO_mode < IO_GRANULARITY_CARD){
    // Current granularity is crate, wanted is card or channel.
    
    size_t data_read = 0;
    const char* ptr = getCrateDataPtr();

    // Read the crate header, make sure it looks right.
    event_header_t* header = (event_header_t*)ptr;
    event_header.setEventHeader(*header);
    data_read += sizeof(event_header_t);
    if(event_header.getHeader() != 0xffffffff) throw std::runtime_error("Bad tpc crate event_header word.");
    
    int cards_read = 0;
    bool done = false;    
    while(!done){
      // Sanity check: is there enough data left in the buffer to read a single card_header?
      if(crate_data_size - data_read < sizeof(card_header_t))  throw std::runtime_error("TPC data error - not enough data to form a card header.");
      

      // Copy the card header into a datatypes class:
      card_header_t* my_card_header = (card_header_t*)(ptr+data_read);
      data_read += sizeof(card_header_t);
      
      cardHeader cardH(*my_card_header);
      size_t cardDataSize = cardH.getCardDataSize();
      
      // Sanity check. 
      if(data_read + cardDataSize > crate_data_size) throw std::runtime_error("TPC cardDataSize error - card data bigger than remaining crate data.");

      std::shared_ptr<char> card_data(new char[cardDataSize]);
      std::copy(getCrateDataPtr() + data_read,
                getCrateDataPtr() + data_read + cardDataSize,
                (char*)card_data.get());
      //wait to increment data_read until after updating channel granularity

      cardData cardD(card_data,cardDataSize);
      if(new_mode == IO_GRANULARITY_CHANNEL) {
        // We've unpacked the card, now unpack the channel.
        
        int channel_data_size = -1; // By default, assume variable-length channel records, i.e. huffman-encoded.

        // Check to see if data is huffman or flat.
        // NJT - Alas, this check doesn't work. It's entirely possible the huffman bit isn't in that word,
        // and in fact it's even LIKELY for noisy data. We could go through all the words looking for the huffman
        // bit, but without a clue from the electronics the safest thing is to put everything through
 
        //for non-Huffman coded data, we can directly get the channel data size
        // std::cout << "Data word is " << std::hex << *data_word << std::dec << std::endl;

        // std::unique_ptr<uint16_t> data_word(new uint16_t);
        //  std::copy(getCrateDataPtr() + data_read + size16*3, //we want the 3rd word in channel
        //            getCrateDataPtr() + data_read + size16*4,
        //            (char*)data_word.get());
        //  if( !(*data_word & 0x8000) )
        //    channel_data_size = cardDataSize/64; //64 channels in each FEM
        

        // std::cout << "Channel data size is " << channel_data_size << std::endl;        
        cardD.updateIOMode(new_mode,channel_data_size);
      }

      //now increment the data_read variable
      data_read += cardDataSize;

      // Got a header and data object for this card. Put it in the map.
      insertCard(cardH,cardD);
      cards_read++;


      // Do we have enough space left to see another card header?
      if(data_read + sizeof(card_header_t) > crate_data_size) {

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
    }

    if(event_trailer.getTrailer() != 0xe0000000) throw std::runtime_error("Bad event_trailer word."); // shouldn't ever happen.
    
    // std::cout << "crateData.cpp read " << std::dec << cards_read << " cards with " << data_read << " bytes." << std::endl;    
    crate_data_ptr.reset();

    crateData_IO_mode = new_mode;
  } //endif on IO_GRANULARITY_CARD update

  if(new_mode == IO_GRANULARITY_CHANNEL && crateData_IO_mode < IO_GRANULARITY_CHANNEL){
    // this code activated when current granularity is card, wanted is channel
    std::map<cardHeader,cardData>::iterator card_it;
    for( card_it = card_map.begin(); card_it != card_map.end(); card_it++){

      size_t cardDataSize = (card_it->second).getCardDataSize();

      int channel_data_size = -1; // Assume variable-length / huffman-compressed

      // check for Huffman coding
      // NJT - Alas, this check doesn't work. It's entirely possible the huffman bit isn't in that word,
      // and in fact it's even LIKELY for noisy data. We could go through all the words looking for the huffman
      // bit, but without a clue from the electronics the safest thing is to put everything through
      //
      // std::unique_ptr<uint16_t> data_word(new uint16_t);
      // std::copy((card_it->second).getCardDataPtr() + size16*2, //we want the 3rd word in channel
      //           (card_it->second).getCardDataPtr() + size16*3,
      //           (char*)data_word.get());
      // 
      // //for non-Huffman coded data, we can directly get the channel data size      
      // if( !(*data_word & 0x8000) )
      //   channel_data_size = cardDataSize/64; //64 channels in each FEM
              
      (card_it->second).updateIOMode(new_mode,channel_data_size);
      
    }

    crateData_IO_mode = new_mode; //eventRecords io_mode

  }//endif on IO_GRANULARITY_CHANNEL update

}

void crateData::insertCard(cardHeader cH, cardData cD){
  card_map.insert(std::pair<cardHeader,cardData>(cH,cD));
}

void crateData::decompress(){

  if(crateData_IO_mode < IO_GRANULARITY_CHANNEL)
    updateIOMode(IO_GRANULARITY_CHANNEL);

  std::map<cardHeader,cardData>::iterator i_card;
  for (i_card=card_map.begin(); i_card!=card_map.end(); i_card++)
    (i_card->second).decompress();

}
