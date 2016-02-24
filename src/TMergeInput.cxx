#include "TMergeInput.hxx"

#include <TEvent.hxx>
#include <TCaptLog.hxx>
#include <TEventContext.hxx>
#include <TManager.hxx>
#include <TInputManager.hxx>
#include <TUnitsTable.hxx>
#include <TPulseDigitHeader.hxx>

#include <iostream>
#include <memory>
#include <ctime>
#include <sstream>

namespace {
    class TMergeInputBuilder : public CP::TVInputBuilder {
    public:
        TMergeInputBuilder() 
            : CP::TVInputBuilder("merge", "Combine TPC and PDS files") {}
        CP::TVInputFile* Open(const char* file) const {
            std::string args = GetArguments();
            if (args.find("(") != std::string::npos) {
                CaptLog("Merge builder argument: " << args);
                
                std::string argString = args.substr(args.find_first_of('(')+1);
                argString = argString.substr(0,argString.find_first_of(",)"));
                argString = CP::TUnitsTable::Get().ConvertWithUnit(argString);
                double window;
                std::istringstream parse1(argString.c_str());
                parse1 >> window;
                double offset = 0.0;
                if (args.find(",") != std::string::npos) {
                    argString = args.substr(args.find_first_of(',')+1);
                    argString
                        = argString.substr(0,argString.find_first_of(')'));
                    if (!argString.empty()) {
                        argString
                            = CP::TUnitsTable::Get().ConvertWithUnit(argString);
                        std::istringstream parse2(argString.c_str());
                        parse2 >> offset;
                    }
                }
                CaptLog("Merge builder argument: " 
                        << CP::TUnitsTable::Get().ConvertTime(offset)
                        << " +/- "
                        << CP::TUnitsTable::Get().ConvertTime(window)
                        << " window");
                return new CP::TMergeInput(file,window,offset);
            }
            return new CP::TMergeInput(file);
        }
    };

    class TMergeInputRegistration {
    public:
        TMergeInputRegistration() {
            CP::TManager::Get().Input().Register(new TMergeInputBuilder());
        }
    };
    TMergeInputRegistration registrationObject;

}

CP::TMergeInput::TMergeInput(const char* name, double window, double offset) 
    : fFilename(name), fTPCFile(NULL),
      fPDSFile(NULL), fPDSEvent(NULL), fEventsRead(0),
      fWindow(window), fOffset(offset) {

    std::string tpcFilename
        = fFilename.substr(0,fFilename.find_first_of(','));
    
    std::string pdsFilename
        = fFilename.substr(fFilename.find_first_of(',')+1);
    
    CaptLog("Open TPC File: " << tpcFilename);
    CaptLog("Open PDS File: " << pdsFilename);

    fTPCFile = CP::TManager::Get().Input().Builder("ubdaq").Open(
        tpcFilename.c_str());
    fPDSFile = CP::TManager::Get().Input().Builder("mPDS").Open(
        pdsFilename.c_str());
}

CP::TMergeInput::~TMergeInput() {
    CloseFile();
}

CP::TEvent* CP::TMergeInput::FirstEvent() {
    return NextEvent();
}

CP::TEvent* CP::TMergeInput::NextEvent(int skip) {
    // The next TPC event is the next event.
    std::auto_ptr<CP::TEvent> newEvent;
    {
        CP::TEvent* tpcEvent = fTPCFile->NextEvent();
        if (!tpcEvent) return NULL;
        newEvent.reset(tpcEvent);
    }
    
    if (!fPDSEvent && !fPDSFile->EndOfFile()) fPDSEvent = fPDSFile->NextEvent();
    if (!fPDSEvent) return newEvent.release();

    CP::NanoStamp eventContextStamp =
        CP::TimeToNanoStamp(newEvent->GetContext().GetTimeStamp(),
                            newEvent->GetContext().GetNanoseconds());
    
    CP::NanoStamp pdsContextStamp =
        CP::TimeToNanoStamp(fPDSEvent->GetContext().GetTimeStamp(),
                            fPDSEvent->GetContext().GetNanoseconds());
    
    double timeDiff = pdsContextStamp - eventContextStamp;
    // Look for the last PDS event to consider.
    CaptNamedInfo("merge", "TPC event " << newEvent->GetContext());
    while (fPDSEvent && timeDiff < fWindow) {
        // See if this PDS event is after the start of the window, and add it
        // to the event if it is.
        if (timeDiff > -fWindow) {
            // Get the pds event container, and create it if it doesn't
            // exist.
            CP::THandle<CP::TDataVector> subEvents
                = newEvent->Get<CP::TDataVector>("~/subEvents");
            if (!subEvents) {
                newEvent->AddDatum(new CP::TDataVector("subEvents"));
                subEvents = newEvent->Get<CP::TDataVector>("~/subEvents");
            }
            subEvents->AddDatum(fPDSEvent);
            CaptNamedInfo("merge", "  PDS Match " << timeDiff/unit::second
                    << " Event " << fPDSEvent->GetContext().GetEvent());
        }
        else {
            CaptNamedInfo("merge", "  PDS Discard " << timeDiff/unit::second
                    << " Event " << fPDSEvent->GetContext().GetEvent());
        }
        fPDSEvent = NULL;
        if (!fPDSFile->EndOfFile()) fPDSEvent = fPDSFile->NextEvent();
        if (!fPDSEvent) break;
        pdsContextStamp =
            CP::TimeToNanoStamp(fPDSEvent->GetContext().GetTimeStamp(),
                                fPDSEvent->GetContext().GetNanoseconds());
        timeDiff = pdsContextStamp - eventContextStamp;
    }

    // Get the combined pmt digits container, and create it if it doesn't exist.
    CP::THandle<CP::TDigitContainer> combinedPDS 
        = newEvent->Get<CP::TDigitContainer>("~/digits/pmt");
    if (!combinedPDS) {
        CP::THandle<CP::TDataVector> dv
            = newEvent->Get<CP::TDataVector>("~/digits");
        if (!dv) {
            newEvent->AddDatum(new CP::TDataVector("digits"));
            dv = newEvent->Get<CP::TDataVector>("~/digits");
        }
        dv->AddDatum(new CP::TDigitContainer("pmt"));
        combinedPDS = newEvent->Get<CP::TDigitContainer>("~/digits/pmt");
    }

    CP::THandle<CP::TDataVector> subEvents
        = newEvent->Get<CP::TDataVector>("~/subEvents");
    if (subEvents) {
        for (CP::TDataVector::iterator e = subEvents->begin();
             e != subEvents->end(); ++e) {
            CP::TEvent *evt = dynamic_cast<CP::TEvent*>(*e);
            // Do violence to the PDS event...
            CP::THandle<CP::TDigitContainer> digits
                = evt->Get<CP::TDigitContainer>("./digits/pmt");
            if (!digits) continue;
            if (digits->size()<1) continue;
            CP::TPulseDigitHeader* header = NULL;
            if (evt->GetTimeStamp() > 0) {
                header = new TPulseDigitHeader("pds");
                header->SetBeginValid(combinedPDS->size());
                header->SetTimeStamp(evt->GetTimeStamp());
            }
            for (CP::TDigitContainer::iterator d = digits->begin();
                 d != digits->end(); ++d) {
                combinedPDS->push_back(*d);
            }
            if (header) {
                header->SetEndValid(combinedPDS->size());
                combinedPDS->AddHeader(header);
            }
            digits->clear();
        }
    }
    
    ++fEventsRead;
    return newEvent.release();
}

int  CP::TMergeInput::GetPosition() const {return fEventsRead;}

bool CP::TMergeInput::IsOpen() {
    if (!fTPCFile) return false;
    if (!fPDSFile) return false;
    if (!fTPCFile->IsOpen()) return false;
    if (!fPDSFile->IsOpen()) return false;
    return true;
}

bool CP::TMergeInput::EndOfFile() {
    return fTPCFile->EndOfFile() || fPDSFile->EndOfFile();
}

void CP::TMergeInput::CloseFile() {
    if (fTPCFile) {
        delete fTPCFile;
        fTPCFile = NULL;
    }
    if (fPDSFile) {
        delete fPDSFile;
        fPDSFile = NULL;
    }
}

