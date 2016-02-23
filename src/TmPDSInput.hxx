#ifndef TmPDSInput_hxx_seek
#define TmPDSInput_hxx_seek

#include <TFile.h>

#include "ECore.hxx"
#include "TVInputFile.hxx"

class TTree;
class TBranch;

namespace CP {
    EXCEPTION(EPDSInputFileMissing,EInputFile);
    EXCEPTION(EPDSNoEvents,EInputFile);

    class TEvent;
    class TmPDSInput;
}

/// Attach to a miniCAPTAIN PDS DAQ file so that the photon detection system
/// events can be read.  The raw V1720 data is stored in the pmt digit
/// container, and the GPS time stamp is in a TIntegerDatum (second,ns). 
class CP::TmPDSInput : public TVInputFile {
public:
    /// Open an input file. 
    TmPDSInput(const char* fName, Option_t* option="", int compress = 1);

    /// Use a TFile that was opened someplace else.  The TmPDSInput object
    /// takes ownership of the file.  This is particularly useful if the file
    /// has been opened using the static TFile::Open method.  This method
    /// returns a pointer to a TFile derived class that is appropriate to the
    /// type of file (dist, http, &c).
    TmPDSInput(TFile* file);

    virtual ~TmPDSInput(void);
    
    /// Overload the base class filename.
    const char* GetFilename() const {return GetInputName();}

    /// Check that an input file is able to read an event.  If the
    /// file is not ready, then this will try to set up for reading.
    virtual bool IsAttached(void);

    /// Return the total number of events in this file 
    virtual Int_t GetEventsInFile(void);

    /// Get the number of events that have been read. 
    virtual Int_t GetEventsRead(void);

    /// Flag that the input file is open and ready to be read.
    virtual bool IsOpen(void);

    /// Flag that we are past an end of the file.  This is true if an attempt
    /// was made to read before the first event of the file, or after the last
    /// event in the file.
    virtual bool EndOfFile(void);

    /// Return the first event in the file.
    virtual TEvent* FirstEvent(void) {return ReadEvent(0);}

    /// Read the next event in the file.
    virtual TEvent* NextEvent(int skip = 0);

    /// Read the previous event in the file.
    virtual TEvent* PreviousEvent(int skip = 0);

    /// Return the first event in the file.
    virtual TEvent* LastEvent(void) {
        return ReadEvent(GetEventsInFile()-1);
    }

    /// Read the n'th event in the file.  The events are counted from zero.
    /// If the index is out-of-range, or no event can be read, this will
    /// return NULL.  For example, if event -1 is requested, this returns
    /// NULL.
    virtual TEvent* ReadEvent(Int_t n);

    /// Make sure that the file is closed.  This method is specific to
    /// TmPDSInput.
    virtual void Close(Option_t* opt = "");

    /// The generic way to close the file.  This is inherited from TVInputFile
    /// and must be provided for eventLoop (it's pure virtual).
    virtual void CloseFile() {Close();}

    /// Return the entry number of the event within the tree.
    virtual int GetPosition(void) const {
        if (fSequence<0) return -1;
        return fSequence;
    }

    /// Return the TFile that events are being read from.
    TFile* GetFilePointer(void) const {return fFile;}

    /// Return the file name to provide the base abstract input name class.
    virtual const char* GetInputName(void) const;

private:
    TFile* fFile;               // The file to get events from.
    Int_t fSequence;            // The sequence number of the last event read.

    TTree* fEventTree;            // the TTTree of event objects. 
    
    Int_t fEventsRead;          //! count of events read from file
    bool fAttached;             //! are we prepared to read from the file?

#ifdef PRIVATE_COPY
private:
    TmPDSInput(const TmPDSInput& aFile);
#endif

    // Declaration of leaf types
    UInt_t          event_number;
    Int_t           computer_secIntoEpoch;
    Long64_t        computer_nsIntoSec;
    UInt_t          gps_nsIntoSec;
    UInt_t          gps_secIntoDay;
    UShort_t        gps_daysIntoYear;
    UShort_t        gps_Year;
    UShort_t        gps_ctrlFlag;
#define MAXDIGITIZER 3
#define MAXCHANNELS 8
#define MAXSAMPLES 1024
#define MAXDATA MAXDIGITIZER*MAXCHANNELS*MAXSAMPLES
    UInt_t          digitizer_size[MAXDIGITIZER];
    UInt_t          digitizer_chMask[MAXDIGITIZER*MAXCHANNELS];
    UInt_t          digitizer_evNum[MAXDIGITIZER];
    UInt_t          digitizer_time[MAXDIGITIZER];
    UShort_t        digitizer_waveforms[MAXDATA];
    UInt_t          nDigitizers;
    UInt_t          nChannels;
    UInt_t          nSamples;
    UInt_t          nData;
    
    // List of branches
    TBranch        *b_event_number;   //!
    TBranch        *b_computer_secIntoEpoch;   //!
    TBranch        *b_computer_nsIntoSec;   //!
    TBranch        *b_gps_nsIntoSec;   //!
    TBranch        *b_gps_secIntoDay;   //!
    TBranch        *b_gps_daysIntoYear;   //!
    TBranch        *b_gps_Year;   //!
    TBranch        *b_gps_ctrlFlag;   //!
    TBranch        *b_digitizer_size;   //!
    TBranch        *b_digitizer_chMask;   //!
    TBranch        *b_digitizer_evNum;   //!
    TBranch        *b_digitizer_time;   //!
    TBranch        *b_digitizer_waveforms;   //!
    TBranch        *b_nDigitizers;   //!
    TBranch        *b_nChannels;   //!
    TBranch        *b_nSamples;   //!
    TBranch        *b_nData;   //!
};
#endif
