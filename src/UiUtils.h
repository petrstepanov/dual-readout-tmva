#ifndef UiUtils_hh
#define UiUtils_hh 1

#include <TH1.h>

namespace UiUtils {
	// Show OK message box
	void msgBoxInfo(const char* title, const char* text);

	// Save histogram as PNG image
	void saveHistogramAsImage(TH1* hist, const char* imageFilePath);
}

#endif
