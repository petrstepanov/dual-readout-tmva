#include "./StringUtils.h"

#include <TString.h>
#include <TObjString.h>
#include <TObjArray.h>
#include <TPRegexp.h>

#include <iostream>
#include <ostream>

using namespace StringUtils;

TString StringUtils::stripExtension(const char *str) {
	TString s = str;
	Ssiz_t pos = s.Last('.');
	// If there is no extension - return filename itself
	if (pos < 0 || pos > s.Length()){
		return s;
	}
	// If there is extension - trim string: https://root.cern.ch/root/html303/examples/tstring.C.html
	TString s2 = s(0, pos);
	return s2;
}

// TODO: move to some IO utils?
void StringUtils::writeProgress(const char* s, Int_t nTimes){
    static Int_t counter = 0;
    Int_t progressPercent = ++counter*100/nTimes;

    // Flush cout
    // https://stackoverflow.com/questions/3057977/rewinding-stdcout-to-go-back-to-the-beginning-of-a-line
    std::cout << "\r" << s << ": " << progressPercent << "% (" << counter << "/" << nTimes << ") " << std::flush;
    if (counter == nTimes){
        counter = 1;
        // writeRewindEnd();
        std::cout << ", done." << std::endl;
    }
}

//void StringUtils::writeRewind(const char* s){
//    std::cout << "\r" << s << std::flush;
//}

//void StringUtils::writeRewindEnd(){
//    std::cout << ", done." << std::endl;
//}
