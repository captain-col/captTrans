#ifndef TMergeInput_hxx_seen
#define TMergeInput_hxx_seen

#include <ECore.hxx>
#include <TVInputFile.hxx>
#include <HEPUnits.hxx>

#include <string>
#include <istream>

namespace CP {
    class TMergeInput;
};

/// Open input files from the TPC and PDS and merge them into a single output
/// event.  The trigger setup is such that there will be multiple PDS triggers
/// per TPC trigger.  Events are merged based on the time in the event
/// context.  The merge time is controlled from the command line using the
/// eventLoop option.  For instance, -tmerge will use the default time window,
/// while -tmerge(50ms) will use a 50 ms window.  Note: the shell will
/// probably require the "(" to be escaped, so it's -tmerge\(50ms\).
class  CP::TMergeInput : public CP::TVInputFile {
public:

    /// Open input files from the TPC and PDS and merge them into a single
    /// output event.  The trigger setup is such that there will be multiple
    /// PDS triggers per TPC trigger.  Events are merged based on the time in
    /// the event context.  The merge time is controlled from the command line
    /// using the eventLoop option.  For instance, -tmerge will use the
    /// default time window, while -tmerge(50ms) will use a 50 ms window.
    /// Note: the shell will probably require the "(" to be escaped, so it's
    /// -tmerge\(50ms\).
    TMergeInput(const char* fNames, double window = 40*unit::ms);
    virtual ~TMergeInput(); 

    /// Return the first event in the input file.  Thie merged files don't
    /// support rewind, so this is just a call to NextEvent().
    virtual CP::TEvent* FirstEvent();

    /// Get the next event from the input file.  The skip argument is ignored.
    virtual CP::TEvent* NextEvent(int skip=0);
    
    /// Return the position of the event just read inside of the file.  A
    /// position of zero is the first event.  After reading the last event,
    /// the position will be the total number of events in the file.  This is
    /// not the byte position in the file.
    virtual int GetPosition(void) const;

    /// Flag that the file is open.
    virtual bool IsOpen();

    /// Flag that the end of the file has been reached.
    virtual bool EndOfFile();

    /// Close the input file.
    virtual void CloseFile();

    /// Get the name of this file
    const char* GetFilename()  const { return fFilename.c_str();  } 

private:
    /// name of the currently open file
    std::string fFilename; 

    /// The input file to read for the TPC.
    CP::TVInputFile *fTPCFile;

    /// The input file to read for the PDS.
    CP::TVInputFile *fPDSFile;

    /// The last PDS event read (this is set to null as soon as it is used).
    CP::TEvent* fPDSEvent;
    
    /// The number of events read.
    int fEventsRead;

    /// The size of the window to merge events over in HEPUnits.
    double fWindow;
};
#endif
