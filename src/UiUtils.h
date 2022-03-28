#ifndef UiUtils_hh
#define UiUtils_hh 1

#include <TH1.h>
#include <TTree.h>
#include <TCanvas.h>

namespace UiUtils {
	// Show OK message box
	void msgBoxInfo(const char* title, const char* text);

	// Save histogram as PNG image
	void saveHistogramAsImage(TH1* hist, const char* imageFilePath);

	// Plot tree branch
	void plotBranch(TTree* tree, const char* branchName, const char* title, const char* xTitle, const char* yTitle, int binning = 100);

	// Add Canvas Multi-Title
	void addCanvasTitle(TCanvas* canvas, const char* title, const char* subtitle);
}

#endif
