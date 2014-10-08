
#include <cstddef> // NULL
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/time.h>
#include "datatypes/eventRecord.h"
#include "datatypes/crateData.h"
#include "datatypes/channelData.h"

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/filesystem.hpp>

#include "share/TimeUtils.h"

#include <TROOT.h>
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <TFitResultPtr.h>
#include <TFitResult.h>
#include <TCanvas.h>
#include <TNtuple.h>

/****************************************************************************
 
  This is largely taken from code written by Georgia Karagiorgi. The main
  purpose is to read in data events and make diagnostic plots.

 ****************************************************************************/


using namespace gov::fnal::uboone::datatypes;

const int MAX_CRATES=1;
const int MAX_CARDS=12; const int CARD_OFFSET=7;
const int N_CHANNELS=64;


const float BASELINE_HIGH = 2000.;
const float BASELINE_LOW = 470.;

int main(int argc, char** argv) {

  int c;
  std::string directoryOut = "untracked_files";
  std::string fileIn = "input_subrun.dat";
  int error_threshold = 1000;
  int i_event = 1;

  while ( (c = getopt(argc, argv, "i:e:d:t:h")) != -1 ) {
    switch(c) {
    case 'e': 
      i_event = atoi(optarg)-1;
      break;
    case 'i':
      fileIn = std::string(optarg);
      break;
    case 'd': 
      directoryOut = std::string(optarg);
      break;
    case 't':
      error_threshold = atoi(optarg);
      break;
    case 'h':
      std::cout << "\nUsage:" << std::endl;
      std::cout << "\t -e ev : process event number ev (default = 1)\n";
      std::cout << "\t -t thresh : print error threshold of thresh (default = 1000)\n";
      std::cout << "\t -i fI : use input file fI (default = input_subrun.dat)\n";
      std::cout << "\t -d dO : store output in directory dO (will be created if it doesn't exist, default = untracked_files)\n";
      return 0;
    }
  }

  //make output directory
  boost::filesystem::create_directories(directoryOut.c_str());
  

  struct timeval t_start,t_end;
  
  /* Now do some declaring of variables we'll use. */
  eventRecord event_record; event_record.updateIOMode(0);
  std::map<crateHeader,crateData>::iterator seb_it;
  std::map<cardHeader,cardData>::iterator card_it;
  std::map<int,channelData>::iterator channel_it;
  
  //unsigned int event_number;
  unsigned int crate_number;
  unsigned int card_number;

  TCanvas canvas("canvas");

  //open the file at the end, and read in the caboose
  std::ifstream ifs(fileIn.c_str(), std::ios::binary | std::ios::ate);
  
  if(i_event>=0){

  //read in the end of file word first
  uint16_t end_of_file_marker;
  ifs.seekg( -1*sizeof(uint16_t) , std::ios::end);
  ifs.read( (char*)&end_of_file_marker , sizeof(uint16_t));
  
  if(end_of_file_marker != 0xe0f0){
    std::cout << "ERROR! We do not have the end of file marker! " << std::hex << end_of_file_marker << std::dec << std::endl;
    return 0;
  }
  
  //get number of events from word at end of file
  uint32_t number_of_events;
  ifs.seekg( -1*(sizeof(uint16_t)+sizeof(uint32_t)), std::ios::end);
  ifs.read( (char*)&number_of_events , sizeof(uint32_t));
    
  //now get all of the event sizes
  std::vector<std::streampos> event_locations;
  uint32_t tmp_event_size;
  std::streampos count_event_size=0;
  ifs.seekg( -1*(sizeof(uint16_t)+(number_of_events+1)*sizeof(uint32_t)), std::ios::end);
  for(uint32_t i=0; i<number_of_events; i++){
    
    // since we want the beginning, push back the event size before incrementing it
    event_locations.push_back(count_event_size);
    
    ifs.read( (char*)&tmp_event_size , sizeof(uint32_t));
    count_event_size += tmp_event_size;
    
    //std::cout << "Event " << i << " was of size " << tmp_event_size << std::endl;
  }
  
  ifs.seekg( event_locations.at(i_event) );
  }
  else{
    ifs.seekg( 0 );
  }
  gettimeofday(&t_start,NULL);
  
  boost::archive::binary_iarchive ia(ifs);
  //  const int N_COUNT=10;
  //for(int i=0; i<N_COUNT; i++){
    ia >> event_record;
    int event = (event_record.getGlobalHeaderPtr())->getEventNumber();
    gettimeofday(&t_end,NULL);
    float time_process =  utils::diff_time_milliseconds(t_end,t_start);
    std::cout << "\t\tTime = " << time_process << "(event=" << event << ")" << std::endl;
    //}
  std::unique_ptr<uint16_t> blk_chD(new uint16_t);
  
  //globalHeader *global_header = event_record.getGlobalHeaderPtr();
  //event_number = global_header->getEventNumber();
  
  event_record.updateIOMode(IO_GRANULARITY_CHANNEL);
  
  std::map<crateHeader,crateData,compareCrateHeader> seb_map = event_record.getSEBMap();
  for( seb_it = seb_map.begin(); seb_it != seb_map.end(); seb_it++){

    crateHeader crate_header = seb_it->first;
    crate_number = (unsigned int)(crate_header.getCrateNumber());
    
    std::cout << "Crate number is " << crate_number << std::endl;

    crateData *crate_data = &(seb_it->second);            
    std::map<cardHeader,cardData,compareCardHeader> card_map = crate_data->getCardMap();
    for(card_it = card_map.begin(); card_it != card_map.end(); card_it++){
      
      cardHeader card_header = (card_it->first);
      card_number = (card_header.getModule());
      
      cardData *card_data = &(card_it->second);
      
      std::map<int,channelData> channel_map = card_data->getChannelMap();
      for(channel_it = channel_map.begin(); channel_it != channel_map.end(); channel_it++){
	

	std::stringstream stream_hTitle;
	stream_hTitle << "Pulse from Crate " << crate_number 
		      << ", FEM " << card_number
		      << ", Channel Number " << channel_it->first
		      << ";Time tick;ADC Counts";
	std::stringstream stream_hFile;
	stream_hFile << directoryOut << "/pulse_" << crate_number 
		      << "_" << card_number
		      << "_" << channel_it->first
		      << ".C";


	channelData *chD = &(channel_it->second);
	int n_words = (int)(chD->getChannelDataSize()/sizeof(uint16_t));
	
	TH1F* htmp = new TH1F("htmp",(stream_hTitle.str()).c_str(),n_words,-0.5,(float)n_words+0.5);

	//get pedestal from first readout window
	std::vector<float> data_values;
	float max_value=0;
	float max_value_mid=0;
	float max_value_sub=0;
	float max_value_mid_sub=0;
	for(int d_it=0; d_it<n_words; d_it++){
	  std::copy(chD->getChannelDataPtr()+d_it*sizeof(uint16_t),
		    chD->getChannelDataPtr()+(d_it+1)*sizeof(uint16_t),
		    (char*)blk_chD.get());
	  htmp->SetBinContent(d_it+1,(float)(*blk_chD));
	  /*
	  if((float)(*blk_chD) > max_value){
	    max_value = (float)(*blk_chD);
	    if(channel_it->first < 32) 
	      max_value_sub = max_value - BASELINE_HIGH;
	    else if(channel_it->first >= 32)
	      max_value_sub = max_value - BASELINE_LOW;  
	  }
	  if((float)(*blk_chD) > max_value_mid && d_it >=33 && d_it <=66){
	    max_value_mid = (float)(*blk_chD);
	    if(channel_it->first < 32) 
	      max_value_mid_sub = max_value_mid - BASELINE_HIGH;
	    else if(channel_it->first >= 32)
	      max_value_mid_sub = max_value_mid - BASELINE_LOW;  
	  }
	  */
	}
	htmp->SetStats(0); htmp->Draw(); 
	canvas.SaveAs((stream_hFile.str()).c_str());
	/*   
	if(max_value_mid_sub < error_threshold)
	  std::cout << "ERROR: Middle frame below acceptable limit above baseline (" << max_value_mid_sub <<") --- (FEM,Channel)=(" << card_number << "," << channel_it->first << ")" << std::endl;
	if(max_value_sub < error_threshold)
	  std::cout << "ERROR: All ticks below acceptable limit above baseline (" << max_value_sub <<") --- (FEM,Channel)=(" << card_number << "," << channel_it->first << ")" << std::endl;
	*/
	delete htmp;

      } //end loop over channels
      
    } //end loop over cards/modules
    
  } // end loop over sebs/crates
  
  
  ifs.close();
  return 0;
}

