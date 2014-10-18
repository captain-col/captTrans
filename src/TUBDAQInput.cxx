#include "TUBDAQInput.hxx"

#include "datatypes/eventRecord.h"

#include "datatypes/constants.h"
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
#include <ctime>
#include <cstdlib>
#include <cstring>

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

namespace {
    std::time_t unixMkTimeIsInsane(struct tm* tmStruct) {
        // The mktime function converts a struct tm expressed in local time to
        // time_t so it's not the right way to handle UTC.  The right way to
        // convert would be to use timegm, but unfortunately, it's not a
        // standardized function.  This code uses timegm if it's available,
        // otherwise it plays the "usual" trick with the TZ environment
        // variable.
#if false // _BSD_SOURCE || _SVID_SOURCE
        return timegm(tmStruct);
#else
#warning "Using thread unsafe version UTC time conversion to replace timegm"
        time_t ret;
        char *tz;
        
        tz = std::getenv("TZ");
        if (tz) tz = strdup(tz);
        setenv("TZ", "", 1);
        tzset();
        ret = std::mktime(tmStruct);
        if (tz) {
            setenv("TZ", tz, 1);
            free(tz);
        }
        else {
            unsetenv("TZ");
        }
        tzset();
        return ret;
#endif
    }
}

CP::TUBDAQInput::TUBDAQInput(const char* name) 
    : fFilename(name) {
    fFile = new std::ifstream(fFilename.c_str(), std::ios::binary);
}

CP::TUBDAQInput::~TUBDAQInput() {
    CloseFile();
}

CP::TEvent* CP::TUBDAQInput::FirstEvent() {
    return NextEvent();
}

CP::TEvent* CP::TUBDAQInput::NextEvent(int skip) {
    gov::fnal::uboone::datatypes::eventRecord ubdaqRecord;

    boost::archive::binary_iarchive archive(*fFile);

    archive >> ubdaqRecord;
    ubdaqRecord.updateIOMode(
        gov::fnal::uboone::datatypes::IO_GRANULARITY_CHANNEL);

    // Build the event context.
    CP::TEventContext context;

    context.SetRun(ubdaqRecord.getGlobalHeader().getRunNumber());
    context.SetSubrun(ubdaqRecord.getGlobalHeader().getSubrunNumber());
    context.SetEvent(ubdaqRecord.getGlobalHeader().getEventNumber());

    // The header counts the number of seconds since midnight Jan 1, 2012 UTC
    // so it needs to be converted into a standard unix time before being
    // saved into the context.  This raise the usual UNIX problem that it's
    // time handling is insane, so bend over backwards to fix it.
    struct tm offsetTime;
    offsetTime.tm_year = 2012;
    offsetTime.tm_mon = 0;
    offsetTime.tm_mday = 1;
    offsetTime.tm_hour = 0;
    offsetTime.tm_min = 0;
    offsetTime.tm_sec = 0;
    offsetTime.tm_isdst = 0;
    ULong_t offsetTimeT= unixMkTimeIsInsane(&offsetTime);
    ULong_t offset2000 = ubdaqRecord.getGlobalHeader().getSeconds() 
        + offsetTimeT;
    context.SetTimeStamp(offset2000);

    // Fill in the number of nanoseconds since the last 1 second tick.
    int nanoseconds = 1000000*ubdaq.getGlobalheader.getMilliSeconds()
        + 1000*ubdaq.getGlobalheader.getMicroSeconds()
        + ubdaq.getGlobalheader.getNanoSeconds();
    context.SetNanoseconds(nanoseconds);

    // Define the partition for this data.
    /// \bug As of this writing, the partitions for CAPTAIN haven't been
    /// defined, and this will need to be filled in later.  For now, the
    /// partition is set to "0" so that the partition is valid and the event
    /// isn't flagged as MC.
    context.SetPartition(0);

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
            int cardNum = card_header.getModule();
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

