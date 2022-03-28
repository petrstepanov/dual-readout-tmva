#ifndef StringUtils_hh
#define StringUtils_hh 1

#include <TList.h>
#include <TH1.h>

namespace StringUtils {
	// Extract filename from path
	TString stripExtension(const char *str);
}

#endif
