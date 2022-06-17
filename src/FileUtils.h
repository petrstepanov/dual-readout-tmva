#ifndef FileUtils_hh
#define FileUtils_hh 1

#include <TList.h>
#include <TH1.h>
#include <TFile.h>
#include <TString.h>

namespace FileUtils {

	// Obtain list of all file paths in directory
	TList* getFilePathsInDirectory(const char* dirPath = "", const char* ext = 0);

	// Import CSV waveform to ROOT histogram
	TH1* tekWaveformToHist(const char* fileName);

	// Open file with checks
	/// TFile* openFile(const char* fileName);

	// Extract file name from path
	TString getFileNameFromPath(const char* path);

	// Extract file name without extension from path
	TString getFileNameNoExtensionFromPath(const char* path);

}

#endif
