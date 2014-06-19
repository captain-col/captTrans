#include "TNevisInput.hxx"
#include "TEvent.hxx"
#include "TEventContext.hxx"

#include "TManager.hxx"
#include "TPulseDigit.hxx"
#include "TInputManager.hxx"
#include "TMCChannelId.hxx"

#include <stdint.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

namespace {
    class TNevisInputBuilder : public CP::TVInputBuilder {
    public:
        TNevisInputBuilder() 
            : CP::TVInputBuilder("nevis", "Read a captEvent NEVIS file") {}
        CP::TVInputFile* Open(const char* file) const {
            return new CP::TNevisInput(file);
        }
    };

    class TNevisInputRegistration {
    public:
        TNevisInputRegistration() {
            CP::TManager::Get().Input().Register(new TNevisInputBuilder());
        }
    };
    TNevisInputRegistration registrationObject;
}

CP::TNevisInput::TNevisInput(const char* name) 
    : fFilename(name) {
    int32_t endian = 0x12345678;

    fDoByteSwap = *(char*)(&endian) != 0x78;

#ifdef NEVIS_USE_ZLIB
    fFile = gzopen(fFilename.c_str(),"rb");
#else
    fFile = fopen(fFilename.c_str(),"rb");
#endif

}

CP::TNevisInput::~TNevisInput() {
    CloseFile();
}

CP::TEvent* CP::TNevisInput::FirstEvent() {
    return NextEvent();
}

int CP::TNevisInput::Read(unsigned int& flag, unsigned int& data) {
    uint16_t word16;
    
#ifdef NEVIS_USE_ZLIB
    int result = gzread(fFile,&word16,sizeof(uint16_t));
#else
    int result = fread(&word16,sizeof(uint16_t),1,fFile);
#endif

    flag = (word16 & 0xF000) >> 12;
    data = (word16 & 0x0FFF);

    if (result<1) {
        CaptError("Read Error " << result);
        throw ETruncatedNevisEvent();
    };

#ifdef DUMP
    CaptLog("Read 0x" << std::hex << flag
            << " 0x" << std::hex << data
            << " (" << std::dec << data << ")");
#endif

    return result;
}

CP::TEvent* CP::TNevisInput::NextEvent(int skip) {
    unsigned int flag;
    unsigned int data;
    
    CP::TEventContext context;

    /// Read the event barrier at the beginning of the event.  (three words).  
    Read(flag,data);
    if (flag != 0xf || data != 0xfff) {
        CaptError("Error in input file.");
        throw ETruncatedNevisEvent();
    } 

    Read(flag,data);
    if (flag != 0xf || data != 0xfff) {
        CaptError("Error in input file.");
        throw ETruncatedNevisEvent();
    } 

    Read(flag,data);
    if (flag != 0xf || data != 0xfff) {
        CaptError("Error in input file.");
        throw ETruncatedNevisEvent();
    } 

    // Not sure what this word is for.  I think it must be some status flags
    // for the event.
    Read(flag,data);

    // Read the event size.  I'm not sure exactly how this relates to the
    // event ize, but it's get very close to the number of words that are
    // saved.
    Read(flag,data);
    int eventSize = data;
    Read(flag,data);
    eventSize = 4095*eventSize + data;

    // Read the event number
    Read(flag,data);
    int eventNumber = data;
    Read(flag,data);
    eventNumber = 4095*eventNumber + data;

    context.SetEvent(eventNumber);

    // Read the run number.
    Read(flag,data);
    int eventRun = data;
    Read(flag,data);
    eventRun = 4095*eventRun + data;

    context.SetRun(eventRun);

    // Not sure what these two words are.
    Read(flag,data);
    Read(flag,data);

    std::cout << "Event " << context << std::endl;

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

    // Read the channel data.
    for (int i=0; i<99999999; ++i) {
        Read(flag,data);
        if (flag == 0x4) {
            int channelNumber = data;
            /// \bug The Nevis DAQ numbers the channels in some unknown way,
            /// so for now, just hack the number into an TMCChannelId.  This
            /// is a kludge!
            CP::TMCChannelId channel(0,0,channelNumber);
            // Read the ADC data.
            CP::TPulseDigit::Vector adc;
            adc.clear();
            do {
                Read(flag,data);
                adc.push_back(data);
                if (adc.size() > 50000) {
                    CaptError("Over-long ADC read");
                    throw EOverlongNevisADC();
                }
            } while (flag == 0x0);
            // Create the digit.
            CP::TPulseDigit* digit = new TPulseDigit(channel,0, adc);
            drift->push_back(digit);

            if (flag == 0x5) continue;
            if (flag == 0xe) break;
        }
        if (flag == 0x0) continue;
        if (flag == 0xe) break;

        CaptError("Unexpected flag in Nevis DAQ file" << std::hex << flag 
                  << " " << data);
    }

    return newEvent.release();
}

int  CP::TNevisInput::GetPosition() const {return 0;}

bool CP::TNevisInput::IsOpen() {return fFile;}

bool CP::TNevisInput::EndOfFile() {

#ifdef NEVIS_USE_ZLIB
    return gzeof(fFile);
#else
    return feof(fFile);
#endif

}

void CP::TNevisInput::CloseFile() {

    if (!fFile) return;

#ifdef NEVIS_USE_ZLIB
    gzclose(fFile);
#else
    fclose(fFile);
#endif
    fFile = NULL;

}

