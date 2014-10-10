#include "channelData.h"

#include <vector>
#include <bitset>
#include <fstream>
#include <stdexcept>

using namespace gov::fnal::uboone::datatypes;

// The Huffman tree used here assigns the following values:
//
// Value   Code
//  +3     0000001
//  +2     00001
//  +1     001
//  +0     1
//  -1     01
//  -2     0001
//  -3     000001

void channelData::decompress(){

  uint i_bit,i_zero; // some counters we will use

  const size_t size16 = sizeof(uint16_t);
  std::unique_ptr<uint16_t> word(new uint16_t);
  std::vector<uint16_t> uncompressed_vector;
  uncompressed_vector.reserve(9600);
  
  uint16_t* words = (uint16_t*)getChannelDataPtr();
  size_t    nwords = channel_data_size/size16;
  uint16_t last_uncompressed_word = 0;

  // Dump check for debugging.
  // std::ofstream of("hufform.dump");

  for(size_t iword = 0; iword < nwords; iword++){

    uint16_t word = words[iword];
    // of << iword << "\t" << word << std::endl; // Dump for debugging.

    //if it's not a compressed word, just put it on the uncompressed vector
    if( (word & 0x8000)==0 ) {
      // std::cout << "uncompressed word " << std::hex << word << std::endl;
      last_uncompressed_word = word&0x7ff;
      //std::cout << "huff out @ " << std::dec << uncompressed_vector.size() << "  0x" << std::hex << last_uncompressed_word << std::endl;
      uncompressed_vector.push_back( last_uncompressed_word );
    
    } else {  // huffman bit on.
       // std::cout << "compressed word 0x" << std::hex << word << " b" << (std::bitset<16>) word << std::endl;


      uint16_t outword;
      size_t zero_count = 0;
      bool   non_zero_found = false;
      for(size_t index=0; index<16; ++index){
        if( !((word >> index) & 0x1) ) {
          if(non_zero_found) zero_count ++; // Count zeros IF we're past the padding in the right-hand bits.
        }else{
          if(!non_zero_found) non_zero_found= true;
          else {
            switch(zero_count) { // subst 
              case 0: outword = last_uncompressed_word;    break;
              case 1: outword = last_uncompressed_word -1; break;
              case 2: outword = last_uncompressed_word +1; break;
              case 3: outword = last_uncompressed_word -2; break;
              case 4: outword = last_uncompressed_word +2; break;
              case 5: outword = last_uncompressed_word -3; break;
              case 6: outword = last_uncompressed_word +3; break;
              default:
              // std::cout << "Huffman decompress unrecoginized bit pattern " << (std::bitset<16>) word << std::endl;
                throw std::runtime_error("Huffman decompress unrecoginized bit pattern");                
            }
            // std::cout << "huff out @ " << std::dec << uncompressed_vector.size() << "  0x" << std::hex << outword << std::endl;
            uncompressed_vector.push_back(outword);
            last_uncompressed_word = outword;   // Activite this line is delta is from last word. Comment out this line if diff is from the last EXPLICIT word, instead of the last huffman-compressed word.
            zero_count=0;
          }
        }
      }
      
    } //end else huffman bit on
  }//end for loop over data words
  // std::cout << "Decompress from " << std::dec << channel_data_size/size16 << " to " << uncompressed_vector.size() << std::endl;

  // Dump check for debugging.
  // std::ofstream of2("waveform.dump");
  // for(size_t i=0;i<uncompressed_vector.size();i++) {
  //   of2 << i << "\t" << uncompressed_vector[i] << std::endl;
  // }

  std::shared_ptr<char> newData(new char[uncompressed_vector.size()*size16]);
  std::copy(uncompressed_vector.begin(), uncompressed_vector.end(), (uint16_t*) (newData.get()) );
  channel_data_ptr.swap(newData);
  channel_data_size = uncompressed_vector.size()*size16;

}
