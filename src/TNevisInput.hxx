#ifndef TNevisInput_hxx_seen
#define TNevisInput_hxx_seen

#include <ECore.hxx>
#include <TVInputFile.hxx>


#define NEVIS_USE_ZLIB
#ifdef NEVIS_USE_ZLIB
#include <zlib.h>
#else
#include <cstdio>
#endif

#include <string>

namespace CP {
    class TNevisInput;

    EXCEPTION(ETruncatedNevisEvent,EInputFile);
    EXCEPTION(EOverlongNevisADC,EInputFile);
};

class  CP::TNevisInput : public CP::TVInputFile {
public:
    TNevisInput(const char* fName);
    virtual ~TNevisInput(); 

    /// Return the first event in the input file.  If the file does not
    /// support rewind, then this should throw an exception if it is called a
    /// second time.
    virtual CP::TEvent* FirstEvent();

    /// Get the next event from the input file.  If skip is greater than zero,
    /// then skip this many events before returning.
    virtual CP::TEvent* NextEvent(int skip=0);
    
    /// Return the position of the event just read inside of the file.  A
    /// position of zero is the first event.  After reading the last event,
    /// the position will be the total number of events in the file.
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

    /// Wrapper around fread or gzread to simplify the coding.
    int Read(unsigned int& flag, unsigned int& data);

    /// name of the currently open file
    std::string fFilename; 

    /// Flag for if the file needs to be byteswapped.
    bool fDoByteSwap; 

    /// The file to be read.  This can be either a zlib file, or a stdio file.
#ifdef NEVIS_USE_ZLIB
    gzFile fFile;
#else
    FILE *fFile;
#endif

};
#endif
