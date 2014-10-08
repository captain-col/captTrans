#include "datatypes/eventRecord.h"
#include "datatypes/crateData.h"
#include "datatypes/channelData.h"

#include <boost/archive/binary_oarchive.hpp>

#include <iomanip>
#include <iostream>
#include <fstream>

int main(int argc, char **argv) {
    std::ofstream outputStream("data_file.ubdaq", std::ios::binary);
    boost::archive::binary_oarchive outputArchive(outputStream);

    for (int i=0; i<1000; ++i) {
        gov::fnal::uboone::datatypes::eventRecord eventRecord;
        eventRecord.updateIOMode(0);
        eventRecord.getGlobalHeaderPtr()->setRunNumber(0xDEAD);
        eventRecord.getGlobalHeaderPtr()->setEventNumber(0x61 + i);
        std::cout << "Run " 
                  << std::hex << eventRecord.getGlobalHeader().getRunNumber()
                  << std::endl;

        outputArchive << eventRecord;
    }

    outputStream.close();

    return 0;
}
