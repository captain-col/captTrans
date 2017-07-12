#include "TUBDAQInput.hxx"

#include "datatypes/eventRecord.h"

#include "datatypes/constants.h"
#include "datatypes/crateData.h"
#include "datatypes/channelData.h"

#include "TEvent.hxx"
#include "TCaptLog.hxx"
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

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace {
    class TUBDAQInputBuilder : public CP::TVInputBuilder {
    public:
        TUBDAQInputBuilder() 
            : CP::TVInputBuilder("ubdaq",
                                 "Read a uboone DAQ file"
                                 " [ubdaq(temp[=n]) to not save digits]" ) {}
        CP::TVInputFile* Open(const char* file) const {
            std::string args = GetArguments();
            if (args.find("(") != std::string::npos) {
                CaptLog("UBDAQ builder argument: " << args);
                std::string argument = args.substr(
                    args.find_first_of('(')+1);
                int first=-1;
                int last = -1;
                std::istringstream parse(argument);
                parse >> first;
                if (parse.good()) {
                    char sep;
                    do {
                        parse >> sep;
                    } while (sep != ',' && parse.good());
                    parse >> last;
                }
                else {
                    first = last = -1;
                }
                if (first > 0) {
                    CaptLog("UBDAQ builder argument: " << args 
                            << " --> " << first
                            << " to " << last << " sample will be calibrated");
                }
                if (args.find("temp") != std::string::npos) {
                    int scaling = 200;
                    std::size_t pos = args.find("temp=");
                    if (pos != std::string::npos) {
                        std::string tempArg = args.substr(pos+5);
                        std::istringstream parseTemp(tempArg);
                        parseTemp >> scaling;
                    }
                    CaptLog("UBDAQ builder argument: " << args
                            << " --> Digits scaled in output file by "
                            << scaling);
                    return new CP::TUBDAQInput(file,first,last,scaling);
                }
                else {
                    return new CP::TUBDAQInput(file,first,last,-1);
                }
            }
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
        // variable.  There are similar functions in captDBI, but this class
        // can't depend on that so here it is.
#if defined(_DEFAULT_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
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

CP::TUBDAQInput::TUBDAQInput(const char* name, int first, int last, int scale) 
    : fFilename(name), fFirstSample(first), fLastSample(last),
      fScaledDigitSave(scale) {

    if (fFilename.rfind(".gz") != std::string::npos) {
        std::ifstream *compressed
            = new std::ifstream(fFilename.c_str(),
                                std::ios::in | std::ios::binary);
        boost::iostreams::filtering_istreambuf *input 
            = new boost::iostreams::filtering_istreambuf();
        input->push(boost::iostreams::gzip_decompressor());
        input->push(*compressed);
        fFile = new std::istream(input);
    }
    else {
        fFile = new std::ifstream(fFilename.c_str(),
                                  std::ios::in | std::ios::binary);
    }

    // Determine the detector type being converted so that the partition can
    // be correctly set.  This depends on the file naming convention, but the
    // DAQ doesn't provide any other status header to determine the detector.
    if (fFilename.find("mCAPTAIN") != std::string::npos) {
        fDetector = "mCAPTAIN";
    }
    else {
        fDetector = "CAPTAIN";
    }
    
    fEventsRead = 0;
}

CP::TUBDAQInput::~TUBDAQInput() {
    CloseFile();
}

CP::TEvent* CP::TUBDAQInput::FirstEvent() {
    return NextEvent();
}

CP::TEvent* CP::TUBDAQInput::NextEvent(int skip) {
    typedef std::map<gov::fnal::uboone::datatypes::crateHeader,
                     gov::fnal::uboone::datatypes::crateData,
                     gov::fnal::uboone::datatypes::compareCrateHeader> crateMap;
    typedef std::map<gov::fnal::uboone::datatypes::cardHeader,
                     gov::fnal::uboone::datatypes::cardData,
                     gov::fnal::uboone::datatypes::compareCardHeader> cardMap;
    typedef std::map<int,
                     gov::fnal::uboone::datatypes::channelData> channelMap;

    gov::fnal::uboone::datatypes::eventRecord ubdaqRecord;

    boost::archive::binary_iarchive archive(*fFile);

    archive >> ubdaqRecord;
    ubdaqRecord.updateIOMode(
        gov::fnal::uboone::datatypes::IO_GRANULARITY_CHANNEL);

    // Build the event context.
    CP::TEventContext context;

    context.SetRun(ubdaqRecord.getGlobalHeader().getRunNumber());
    context.SetSubRun(ubdaqRecord.getGlobalHeader().getSubrunNumber());
    context.SetEvent(ubdaqRecord.getGlobalHeader().getEventNumber());

    do {
        // Check if the global header clock is "reasonable".  If it is, then
        // correct for the offset.
        std::time_t offset2000 = ubdaqRecord.getGlobalHeader().getSeconds();
        if (offset2000 != 0xFFFFFFFF) {
            // The header counts the number of seconds since midnight Jan 1,
            // 2012 UTC so it needs to be converted into a standard unix time
            // before being saved into the context.  This raises the usual
            // UNIX problem that it's time handling is insane, so bend over
            // backwards to fix it.
            struct tm offsetTime;
            offsetTime.tm_year = 112;
            offsetTime.tm_mon = 0;
            offsetTime.tm_mday = 1;
            offsetTime.tm_hour = 0;
            offsetTime.tm_min = 0;
            offsetTime.tm_sec = 0;
            offsetTime.tm_isdst = 0;
            std::time_t offsetTimeT= unixMkTimeIsInsane(&offsetTime);
            unsigned int seconds = offset2000 + offsetTimeT;
            unsigned int milliseconds
                = ubdaqRecord.getGlobalHeader().getMilliSeconds();
            unsigned int microseconds
                = ubdaqRecord.getGlobalHeader().getMicroSeconds();
            unsigned int nanoseconds
                = ubdaqRecord.getGlobalHeader().getNanoSeconds()
                + 1000*microseconds + 1000000*milliseconds;
            context.SetTimeStamp(seconds);
            context.SetNanoseconds(nanoseconds);
            break;
        }
        
        // The number of seconds hasn't been initialized, so try the SEB
        // clocks.
        crateMap &crates = ubdaqRecord.getSEBMap();
        unsigned int seconds = 0xFFFFFFFF;
        unsigned int nanoseconds = 0xFFFFFFFF;
        for (crateMap::iterator crate = crates.begin(); 
             crate != crates.end();
             ++crate) {
            crateMap::key_type crate_header = crate->first;
            unsigned int crateSec = crate_header.getSebTimeSec();
            unsigned int crateUsec = crate_header.getSebTimeUsec();
            if ((crateSec < seconds) ||
                (crateSec == seconds && 1000*crateUsec < nanoseconds)) {
                seconds = crateSec;
                nanoseconds = 1000*crateUsec;
                context.SetTimeStamp(seconds);
                context.SetNanoseconds(nanoseconds);
            }
        }

        if (seconds != 0xFFFFFFFF) break;
        CaptError("Event " << context.GetRun() 
                  << "." << context.GetEvent() 
                  << ": No time for DAQ.");

    } while (false);

    // Define the partition for this data.
    if (fDetector == "mCAPTAIN") {
        context.SetPartition(CP::TEventContext::kmCAPTAIN);
    }
    else if (fDetector == "CAPTAIN") {
        context.SetPartition(CP::TEventContext::kCAPTAIN);
    }
    else {
        context.SetPartition(CP::TEventContext::kCAPTAIN);
        static int errorThrottle=100;
        if (--errorThrottle>0) {
            CaptError("Detector type not set for " << context.GetRun() 
                      << "." << context.GetEvent() 
                      << ": Defaulting to CAPTAIN.");
        }
    }        
        
    // Create the event.
    std::auto_ptr<CP::TEvent> newEvent(new CP::TEvent(context));
    newEvent->SetTimeStamp(context.GetTimeStamp(), context.GetNanoseconds());
        
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
        if (0<fScaledDigitSave && 0 != (fEventsRead % fScaledDigitSave)) {
            dv->AddTemporary(new CP::TDigitContainer("drift"));
        }
        else {
            dv->AddDatum(new CP::TDigitContainer("drift"));
        }
        drift = newEvent->Get<CP::TDigitContainer>("~/digits/drift");
    }

    // Get the digits from the event.
    CP::TPulseDigit::Vector adc(30000);
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
                CP::TTPCChannelId chanId(crateNum,cardNum,channelNum);

                int nSamples
                    = channel->second.getChannelDataSize()/sizeof(UShort_t);
                Char_t* samples = channel->second.getChannelDataPtr();
                int beginSamples = 0;

                // Possibly truncate some of the samples at the beginning.
                if (fFirstSample > 0 && fFirstSample < nSamples) {
                    beginSamples = fFirstSample;
                }

                // Possibly truncate some of the samples at the end.
                if (fLastSample > fFirstSample && fLastSample < nSamples) {
                    nSamples = fLastSample;
                }

                // Protect against data-mangling...
                if (nSamples > 9596) {
                    CaptError("Truncate digit length"
                              << " from " << nSamples
                              << " for " << chanId);
                    nSamples = 9596;
                }

                // Read the ADC data.  This could be more efficient, but as
                // long as it's not a bottle neck, I'm keeping it bog simple.
                adc.clear();
                for (int i=beginSamples; i<nSamples; ++i) {
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
                CP::TPulseDigit* digit = new TPulseDigit(chanId,
                                                         beginSamples,
                                                         adc);
                drift->push_back(digit);
            }
        }
    }

    ++fEventsRead;
    return newEvent.release();
}

int  CP::TUBDAQInput::GetPosition() const {return fEventsRead;}

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

