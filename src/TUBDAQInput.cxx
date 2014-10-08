#include "TUBDAQInput.hxx"

#include "datatypes/eventRecord.h"
#
#include "datatypes/crateData.h"
#include "datatypes/channelData.h"

#include "TEvent.hxx"
#include "TEventContext.hxx"
#include "TManager.hxx"
#include "TPulseDigit.hxx"
#include "TInputManager.hxx"
#include "TTPCChannelId.hxx"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>

namespace {
    class TUBDAQInputBuilder : public CP::TVInputBuilder {
    public:
        TUBDAQInputBuilder() 
            : CP::TVInputBuilder("ubdaq", "Read a microboone DAQ file") {}
        CP::TVInputFile* Open(const char* file) const {
            return new CP::TUBDAQInput(file);
        }
    };

    class TUBDAQInputRegistration {
    public:
        TUBDAQInputRegistration() {
            CP::TManager::Get().Input().Register(new TUBDAQInputBuilder());
        }
    };
    TUBDAQInputRegistration registrationObject;
}

CP::TUBDAQInput::TUBDAQInput(const char* name) 
    : fFilename(name) {
    fFile = new std::ifstream(fFilename.c_str(), std::ios::binary);
    fArchive = new boost::archive::binary_iarchive(*fFile);
}

CP::TUBDAQInput::~TUBDAQInput() {
    CloseFile();
}

CP::TEvent* CP::TUBDAQInput::FirstEvent() {
    return NextEvent();
}

CP::TEvent* CP::TUBDAQInput::NextEvent(int skip) {
    gov::fnal::uboone::datatypes::eventRecord ubdaqRecord;
    ubdaqRecord.updateIOMode(0);

    CaptError("Read Event");

    (*fArchive) >> ubdaqRecord;

    CaptError("Build Event" 
              << std::hex << ubdaqRecord.getGlobalHeader().getRunNumber());

    // Build the event context.
    CP::TEventContext context;
    
    context.SetRun(ubdaqRecord.getGlobalHeader().getRunNumber());
    context.SetEvent(ubdaqRecord.getGlobalHeader().getEventNumber());
    context.SetTimeStamp(ubdaqRecord.getGlobalHeader().getSeconds());

    // Create the event.
    std::auto_ptr<CP::TEvent> newEvent(new CP::TEvent(context));

    // Get the digits container, and create it if it doesn't exist.
    CP::THandle<CP::TDigitContainer> drift 
        = newEvent->Get<CP::TDigitContainer>("~/digits/drift");
    if (!drift) {
        CP::THandle<CP::TDataVector> dv
            = newEvent->Get<CP::TDataVector>("~/digits");
        if (!dv) {
            newEvent->AddDatum(new CP::TDataVector("digits"));
            dv = newEvent->Get<CP::TDataVector>("~/digits");
        }
        dv->AddDatum(new CP::TDigitContainer("drift"));
        drift = newEvent->Get<CP::TDigitContainer>("~/digits/drift");
    }

    // Get the digits from the event.
    typedef std::map<gov::fnal::uboone::datatypes::crateHeader,
                     gov::fnal::uboone::datatypes::crateData,
                     gov::fnal::uboone::datatypes::compareCrateHeader> crateMap;
    typedef std::map<gov::fnal::uboone::datatypes::cardHeader,
                     gov::fnal::uboone::datatypes::cardData,
                     gov::fnal::uboone::datatypes::compareCardHeader> cardMap;
    typedef std::map<int,
                     gov::fnal::uboone::datatypes::channelData> channelMap;

    crateMap crates = ubdaqRecord.getSEBMap();
    for (crateMap::iterator crate = crates.begin(); 
         crate != crates.end();
         ++crate) {
        crateMap::key_type crate_header = crate->first;
        int crateNum = crate_header.getCrateNumber();
        crateMap::mapped_type crate_data = crate->second;
        cardMap cards = crate_data.getCardMap();
        for (cardMap::iterator card = cards.begin();
             card != cards.end();
             ++card) {
            cardMap::key_type card_header = card->first;
            int cardNum = card_header.getIDAndModuleWord();  // PROBABLY WRONG
            cardMap::mapped_type card_data = card->second;
            channelMap channels = card_data.getChannelMap();
            for (channelMap::iterator channel = channels.begin();
                 channel != channels.end();
                 ++channel) {
                int channelNum = channel->second.getChannelNumber();
                int nSamples
                    = channel->second.getChannelDataSize()/sizeof(UShort_t);
                Char_t* samples = channel->second.getChannelDataPtr();

                CP::TTPCChannelId chanId(crateNum,cardNum,channelNum);
                // Read the ADC data.  This could be more efficient, but as
                // long as it's not a bottle neck, I'm keeping it bog simple.
                CP::TPulseDigit::Vector adc;
                adc.clear();
                for (int i=0; i<nSamples; ++i) {
                    UShort_t sample;
                    // A copy is used since the ADC samples (uint16_t) are
                    // saved in an array of uint8_t and may not be aligned
                    // with shorts.  The copy works even if the alignment is
                    // wrong.
                    std::copy(samples+(i)*sizeof(UShort_t),
                              samples+(i+1)*sizeof(UShort_t),
                              (char*)(&sample));
                    adc.push_back(sample);
                }

                // Create the digit.
                CP::TPulseDigit* digit = new TPulseDigit(chanId,0, adc);
                drift->push_back(digit);
            }
        }
    }

    return newEvent.release();
}

int  CP::TUBDAQInput::GetPosition() const {return fFile->tellg();}

bool CP::TUBDAQInput::IsOpen() {return fFile;}

bool CP::TUBDAQInput::EndOfFile() {
    return fFile->eof() || fFile->fail();
}

void CP::TUBDAQInput::CloseFile() {
    if (fFile) {
        delete fFile;
        fFile = NULL;
    }
}

bool CP::TUBDAQInput::FindEvent() {
    const char* signature = "serialization::archive\0";

    CaptError("FindEvent");
    const char* checker = signature;
    while (!fFile->eof()) {
        char c = fFile->get();
        // CaptError("Read " << (int) c);
        if (c == *checker) {
            ++checker;
            // CaptError("Checking " <<c);
        }
        else {
            // CaptError("Failed");
            checker = signature;
        }
        if (*checker == 0) {
            // we found the signature
            CaptError("success");
            unsigned int pos = fFile->tellg();
            pos -= 22;   // The length of the signature.
            fFile->seekg(pos);
            return true;
        }
    }
    return false;
}
