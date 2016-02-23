//
// Implement a class to read a miniCAPTAIN photon detection system event from
// a file.
// 
#include "TmPDSInput.hxx"

#include "TEvent.hxx"
#include "TManager.hxx"
#include "TInputManager.hxx"
#include "TCaptLog.hxx"
#include "TIntegerDatum.hxx"
#include "TPulseDigit.hxx"
#include "TPDSChannelId.hxx"

#include <TFile.h>
#include <TTree.h>
#include <TFolder.h>

#include <iostream>
#include <memory>
#include <ctime>

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

namespace {
    class TmPDSInputBuilder : public CP::TVInputBuilder {
    public:
        TmPDSInputBuilder() 
            : CP::TVInputBuilder("mPDS", "Read a miniCAPTAIN PDS DAQ file") {}
        CP::TVInputFile* Open(const char* file) const {
            return new CP::TmPDSInput(file,"OLD");
        }
    };

    class TmPDSInputRegistration {
    public:
        TmPDSInputRegistration() {
            CP::TManager::Get().Input().Register(new TmPDSInputBuilder());
        }
    };
    TmPDSInputRegistration registrationObject;

}

CP::TmPDSInput::TmPDSInput(const char* name, Option_t* option, Int_t compress) 
    : fFile(NULL), fSequence(0), fEventTree(NULL), 
      fEventsRead(0), fAttached(false) {
    fFile = new TFile(name, option, "PDS Input File", compress);
    if (!fFile || !fFile->IsOpen()) {
        throw CP::EPDSInputFileMissing();
    }
    IsAttached();
    CaptVerbose("Input file " << fFile->GetName() << " is open");
}

CP::TmPDSInput::TmPDSInput(TFile* file) 
    : fFile(file), fSequence(0), fEventTree(NULL),
      fEventsRead(0), fAttached(false) {
    if (!IsOpen()) {
        throw CP::ENoInputFile();
    }
    IsAttached();
    
    CaptVerbose("PDS Input file " << fFile->GetName() << " is open");
}

CP::TmPDSInput::~TmPDSInput(void) {
    Close();
    if (fFile) delete fFile;
}

#ifdef PRIVATE_COPY
CP::TmPDSInput::TmPDSInput(const CP::TmPDSInput& aFile) {
    // Copy constructor not implemented
    MayNotUse("Copy Constructor");
    return;
}
#endif

const char* CP::TmPDSInput::GetInputName() const {
    if (fFile) return fFile->GetName();
    return NULL;
}

bool CP::TmPDSInput::IsAttached(void) {
    if (!IsOpen()) return false;

    // Make sure the **** global file pointer is pointing to this file.
    if (gFile != fFile) {
        if (gFile) {
            CaptDebug("Changing current file from " << gFile->GetName()
                       << " to " << fFile->GetName() << " to read.");
        }
        fFile->cd();
    }

    // Make sure that we have the event tree.
    if (!fEventTree) {
        fEventTree = dynamic_cast<TTree*>(fFile->Get("pmt_tree"));
        if (!fEventTree) throw EPDSNoEvents();
    }

    fEventTree->SetBranchAddress("event_number",
                                 &event_number,
                                 &b_event_number);
    fEventTree->SetBranchAddress("computer_secIntoEpoch",
                                 &computer_secIntoEpoch,
                                 &b_computer_secIntoEpoch);
    fEventTree->SetBranchAddress("computer_nsIntoSec",
                                 &computer_nsIntoSec,
                                 &b_computer_nsIntoSec);
    fEventTree->SetBranchAddress("gps_nsIntoSec",
                                 &gps_nsIntoSec,
                                 &b_gps_nsIntoSec);
    fEventTree->SetBranchAddress("gps_secIntoDay",
                                 &gps_secIntoDay,
                                 &b_gps_secIntoDay);
    fEventTree->SetBranchAddress("gps_daysIntoYear",
                                 &gps_daysIntoYear,
                                 &b_gps_daysIntoYear);
    fEventTree->SetBranchAddress("gps_Year",
                                 &gps_Year,
                                 &b_gps_Year);
    fEventTree->SetBranchAddress("gps_ctrlFlag",
                                 &gps_ctrlFlag,
                                 &b_gps_ctrlFlag);
    fEventTree->SetBranchAddress("digitizer_size",
                                 digitizer_size,
                                 &b_digitizer_size);
    fEventTree->SetBranchAddress("digitizer_chMask",
                                 digitizer_chMask,
                                 &b_digitizer_chMask);
    fEventTree->SetBranchAddress("digitizer_evNum",
                                 digitizer_evNum,
                                 &b_digitizer_evNum);
    fEventTree->SetBranchAddress("digitizer_time",
                                 digitizer_time,
                                 &b_digitizer_time);
    fEventTree->SetBranchAddress("digitizer_waveforms",
                                 digitizer_waveforms,
                                 &b_digitizer_waveforms);
    fEventTree->SetBranchAddress("nDigitizers",
                                 &nDigitizers,
                                 &b_nDigitizers);
    fEventTree->SetBranchAddress("nChannels",
                                 &nChannels,
                                 &b_nChannels);
    fEventTree->SetBranchAddress("nSamples",
                                 &nSamples,
                                 &b_nSamples);
    fEventTree->SetBranchAddress("nData",
                                 &nData,
                                 &b_nData);
    
    return true;
}

Int_t CP::TmPDSInput::GetEventsInFile(void) {
    // Returns number of events in this file that can be read.
    if (!IsAttached()) return 0;
    return static_cast<Int_t>(fEventTree->GetEntries());
}

Int_t CP::TmPDSInput::GetEventsRead(void) {
    // Returns number of events read from this file
    return fEventsRead;
}

bool CP::TmPDSInput::IsOpen(void) {
    if (!fFile) return false;
    return fFile->IsOpen();
}

bool CP::TmPDSInput::EndOfFile(void) {
    // Flag that we are past an end of the file.  This is true if the next
    // event to be read is before the first event, or after the last event.  
    if (fSequence<0) {
        fSequence = -1;
        return true;
    }
    if (GetEventsInFile()<=fSequence) {
        fSequence = GetEventsInFile();
        return true;
    }
    return false;
}

CP::TEvent* CP::TmPDSInput::NextEvent(int skip) {
    if (skip>0) fSequence += skip;
    return ReadEvent(++fSequence);
}

CP::TEvent* CP::TmPDSInput::PreviousEvent(int skip) {
    if (skip>0) fSequence -= skip;
    return ReadEvent(--fSequence);
}

CP::TEvent* CP::TmPDSInput::ReadEvent(Int_t n) {
    // Read the n'th event (starting from 0) in the file
    fSequence = n;
    if (fSequence<0) {
        fSequence = -1;
        return NULL;
    }

    if (!IsAttached()) return NULL;
 
    // Read the new event from the tree.
    int nBytes = fEventTree->GetEntry(fSequence);
    if (nBytes > 0) {
        ++fEventsRead;
    } else {
        fSequence = GetEventsInFile();
    }

    // Create the context.
    CP::TEventContext context;
    context.SetEvent(event_number);
    context.SetRun(0);
    context.SetPartition(CP::TEventContext::kmCAPTAIN);
    context.SetTimeStamp(computer_secIntoEpoch);
    context.SetNanoseconds(computer_nsIntoSec);
    
    // Create the event.
    std::auto_ptr<CP::TEvent> newEvent(new CP::TEvent(context));

    // Add the GPS time into an integer datum.
    if (gps_Year > 0) {
        std::auto_ptr<CP::TIntegerDatum> gpsTime(
            new CP::TIntegerDatum("gpsTime"));
        struct tm gpsOffset;
        gpsOffset.tm_year = gps_Year+96;
        gpsOffset.tm_mon = 0;
        gpsOffset.tm_mday = 1;
        gpsOffset.tm_hour = 0;
        gpsOffset.tm_min = 0;
        gpsOffset.tm_sec = 0;
        gpsOffset.tm_isdst = 0;
        std::time_t gpsSeconds = unixMkTimeIsInsane(&gpsOffset);
        gpsSeconds += 3600*24*gps_daysIntoYear;
        gpsSeconds += gps_secIntoDay;
        gpsTime->push_back(gpsSeconds);
        gpsTime->push_back(gps_nsIntoSec);
        gpsTime->push_back(gps_ctrlFlag);
        newEvent->AddDatum(gpsTime.release());
    }
    
    if (nData > MAXDATA) {
        CaptError("Malformed data file");
        throw;
    }
    
    // Get the digits container, and create it if it doesn't exist.
    CP::THandle<CP::TDigitContainer> pmt 
        = newEvent->Get<CP::TDigitContainer>("~/digits/pmt");
    if (!pmt) {
        CP::THandle<CP::TDataVector> dv
            = newEvent->Get<CP::TDataVector>("~/digits");
        if (!dv) {
            newEvent->AddDatum(new CP::TDataVector("digits"));
            dv = newEvent->Get<CP::TDataVector>("~/digits");
        }
        dv->AddDatum(new CP::TDigitContainer("pmt"));
        pmt = newEvent->Get<CP::TDigitContainer>("~/digits/pmt");
    }

    for (int i = 0; i< nDigitizers; ++i) {
        for (int j=0; j<nChannels; ++j) {
            // Get the digits from the event.
            CP::TPulseDigit::Vector adc(nSamples);
            for (int k=0; k<nSamples; ++k) {
                adc[k] = digitizer_waveforms[i*nChannels*nSamples+j*nSamples+k];
            }

            // Create the digit.
            CP::TPDSChannelId chanId(0,i,j);
            CP::TPulseDigit* digit = new TPulseDigit(chanId,0,adc);
            pmt->push_back(digit);
        }
    }
    
    return newEvent.release();
}

void CP::TmPDSInput::Close(Option_t* opt) {
    if (!IsOpen()) return;
    fFile->Close(opt);
}

