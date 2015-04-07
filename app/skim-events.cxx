#include <TCaptLog.hxx>

#include <eventLoop.hxx>
#include <TEventContext.hxx>

#include <fstream>
#include <sstream>

class TDumpEvent: public CP::TEventLoopFunction {
public:
    TDumpEvent() {}

    virtual ~TDumpEvent() {};

    void Usage(void) {
        std::cout << "    -O skim=<file>   Read a skim file for events to save"
                  << std::endl;
    }

    virtual bool SetOption(std::string option,std::string value="") {
        if (option == "skim") {
            std::ifstream in(value.c_str());
            std::string line;
            while (std::getline(in,line)) {
                CaptLog("Skip: " << line);
                if (line[0] == '#') continue;
                std::istringstream lineStream(line);
                int run, event;
                lineStream >> run >> event;
                fRunEvent.push_back(std::make_pair(run,event));
            }
            in.close();
            return true;
        }
        
        return false;
    }

    bool operator () (CP::TEvent& event) {
        CP::TEventContext context = event.GetContext();
        CaptLog("Check " << context);

        // Check to see if the event was in a skim file.
        for (std::vector< std::pair<int,int> >::const_iterator re
                 = fRunEvent.begin(); re!= fRunEvent.end(); ++re) {
            if (context.GetRun() != re->first) continue;
            if (context.GetEvent() != re->second) continue;
            CaptLog("    Save " << context);
            return true;
        }

        
        
        return false;
    }

    // Do nothing... This is here to test compiler warnings.  The warning
    // can be prevented by adding

    void Finalize(CP::TRootOutput*const output) {
    }

private:

    /// A list of events to look for and save if found.
    std::vector< std::pair<int,int> > fRunEvent;

};

int main(int argc, char **argv) {
    TDumpEvent userCode;

    CP::eventLoop(argc,argv,userCode);
}

