#include "./StringUtils.h"

#include <TString.h>
#include <TObjString.h>
#include <TObjArray.h>
#include <TPRegexp.h>

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
