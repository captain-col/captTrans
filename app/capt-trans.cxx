#include <TROOT.h>

#include <eventLoop.hxx>

class TDumpNevis: public CP::TEventLoopFunction {
public:
    TDumpNevis() {
        fLSOption = "";
        fQuiet = true;
    }

    virtual ~TDumpNevis() {};

    void Usage(void) {
        std::cout << "    -O list     List events in the file"
                  << std::endl;
        std::cout << "    -O <option> Set the option for TObject::ls()"
                  << std::endl;
    }

    virtual bool SetOption(std::string option,std::string value="") {
        if (value != "") return false;
        if (option == "list") fQuiet =false;
        else fLSOption = option;
        return true;
    }

    bool operator () (CP::TEvent& event) {
        if (CP::TCaptLog::GetLogLevel()>CP::TCaptLog::QuietLevel && !fQuiet) {
            event.ls(fLSOption.c_str());
        }
        return true;
    }

    // Do nothing... This is here to test compiler warnings.  The warning
    // can be prevented by adding
    //
    // using CP::TEventLoopFunction::Finalize;
    void Finalize(CP::TRootOutput*const output) {
    }

private:
    std::string fLSOption;
    bool fQuiet;
};

int main(int argc, char **argv) {
    TDumpNevis userCode;
    CP::eventLoop(argc,argv,userCode);
}

