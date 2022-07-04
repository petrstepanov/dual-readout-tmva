#ifndef StringUtils_hh
#define StringUtils_hh 1

#include <TList.h>
#include <TH1.h>

namespace StringUtils {
	// Extract filename from path
	TString stripExtension(const char *str);

	// TODO: move to some IO utils?
	void writeProgress(const char* s, Int_t nTimes);

    void writeRewind(const char* s);
    void writeRewindEnd();
}

#endif
