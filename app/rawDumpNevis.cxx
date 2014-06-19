#include <cstdio>
#include <iostream>
#include <stdint.h>

int main(int argc, char **argv) {

    std::cout << "number of arguments " << argc << std::endl;

    std::string fileName(argv[1]);

    FILE *finput;
    finput = fopen(fileName.c_str(),"rb");
    
    if(!finput){
        printf("Could not open.");
        return 1;
    }
    
    uint16_t word_16bit;
    
    int nSamples = 0;
    int nCollection = 0;
    int offset = 0;
    int channels = 0;
    int eventLength = 0;
    while (finput && !feof(finput) && !ferror(finput)) {
        
        int readSize = fread(&word_16bit,sizeof(uint16_t),1,finput);
        if (!readSize) break;

        unsigned int flag = (word_16bit & 0xF000) >> 12;
        unsigned int data = (word_16bit & 0x0FFF);

        if (flag == 0xf ) {
            std::cout << "offset: " << std::dec << eventLength
                      << " header: " << std::dec << offset
                      << " channel:  " << std::dec << channels
                      << " word: " << std::hex << " 0x" << word_16bit 
                      << " flag: " << "0x" << flag
                      << " data: " << std::dec << data
                      << std::endl;
            ++offset;
        }
        ++eventLength;

        if (flag == 4) {
#ifdef DUMP_CHANNELS
#undef DUMP_CHANNEL
            std::cout << "Beginning channel "
                      << std::dec << (word_16bit&0xfff) 
                      << std::hex << " (0x" << word_16bit << ")" << std::endl;
#endif
            int channel = (word_16bit&0xfff);

            ++channels;

            nSamples=0;

            if (channel==0) {
                nCollection++;
            }
            
#ifdef DUMP_CHANNELS
#undef DUMP_CHANNEL
            std::cout << "Data from Collection " 
                      << std::dec << nCollection
                      << std::dec << " Channel " << channel
                      << std::endl;
#endif

        }
        else if (flag == 5) {
#ifdef DUMP_CHANNEL
#undef DUMP_CHANNEL
            std::cout << "Ending channel "
                      << std::dec << (word_16bit&0xfff)
                      << " with " << std::dec << nSamples << " samples." 
                      << std::endl;
#endif            
        }
        else {
            nSamples++;
            /* do something here with all the data */
        }

    }
    
    return 0;
}
