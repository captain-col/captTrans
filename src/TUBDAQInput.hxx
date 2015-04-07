#ifndef TUBDAQInput_hxx_seen
#define TUBDAQInput_hxx_seen

#include <ECore.hxx>
#include <TVInputFile.hxx>

#include <boost/archive/binary_iarchive.hpp>

#include <string>
#include <istream>

namespace CP {
    class TUBDAQInput;
};


/// A class to 
class  CP::TUBDAQInput : public CP::TVInputFile {
public:

    /// Open an file written in ubdaq format (microboone DAQ format).  The
    /// first and last parameters control which part of the ADC range is
    /// converted.  The default setting is often for a very long time window,
    /// and this allows the converted time window to be limited to something
    /// more reasonable.  This is controlled from the command line using the
    /// eventLoop option.  For instance, "-tubdaq" will convert the entire
    /// range, but -tubdaq(2800,3800) only converts the 500 us right around
    /// the trigger time (assuming we are using a 4.5 ms sampling period and
    /// the trigger is at sample 3200.
    TUBDAQInput(const char* fName,int first =-1, int last=-1);
    virtual ~TUBDAQInput(); 

    /// Return the first event in the input file.  If the file does not
    /// support rewind, then this should throw an exception if it is called a
    /// second time.
    virtual CP::TEvent* FirstEvent();

    /// Get the next event from the input file.  If skip is greater than zero,
    /// then skip this many events before returning.
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

    /// The input stream attached to the file.
    std::istream* fFile;

    /// The detector type
    std::string fDetector;
    
    /// The number of events read.
    int fEventsRead;

    /// The first sample to convert
    int fFirstSample;

    /// The last sample to convert
    int fLastSample;

};
#endif
