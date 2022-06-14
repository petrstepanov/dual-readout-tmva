#include "./FileUtils.h"
#include "./StringUtils.h"

#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TGClient.h>
#include <TGFileDialog.h>
#include <TObjString.h>
#include <TUUID.h>
// #include <TCanvas.h>

#include <iostream>
#include <fstream>

using namespace FileUtils;

TString FileUtils::getDirectoryPath() {
	TGFileInfo fi;
	TGFileDialog* fileDialog = new TGFileDialog(gClient->GetRoot(), 0, EFileDialogMode::kDOpen, &fi);
	TString dirPath = fi.fIniDir;
	return dirPath;
}

// This function returns a list of filenames in directory with certain extension
TList* FileUtils::getFilePathsInDirectory(const char *dirPath, const char* ext) {
	// Create list of filenames to return
	TList *fileNames = new TList();

	// Open system directory
	TSystemDirectory *dir = new TSystemDirectory(dirPath, dirPath);
	if (!dir->IsDirectory()) {
		std::cerr << "\"" << dirPath << "\" is not a directory";
		return fileNames;
	}

	// Obtain list of files in the directory
	TList *files = dir->GetListOfFiles();
	files->Sort();
	TIter next(files);
	while (TSystemFile *file = (TSystemFile*) next()) {
		TString fileName = file->GetName();
		if (!file->IsDirectory() && (ext == 0 || (ext!=0 && fileName.EndsWith(ext)))){
			std::cout << "* " << fileName << std::endl;
			TString filePath = file->GetTitle();
			TString fileNamePath = gSystem->ConcatFileName(filePath.Data(), fileName.Data());
			TObjString *objFileName = new TObjString(fileNamePath.Data());
			fileNames->Add(objFileName);
		} else {
			std::cout << "  " << fileName << std::endl;
		}
	}

	return fileNames;
}

TH1* FileUtils::tekWaveformToHist(const char* filePath){
	// Open waveform file
	std::ifstream myfile(filePath);

	// Check file opened successully
	if(!myfile){
		std::cerr << "Error: file could not be opened" << std::endl;
		exit(1);
	}

	// Skip header
	for (int i=1; i<=21; i++){
		std::string line;
		std::getline (myfile, line);
		if (myfile.fail()){
			break; // exit if not successful read
		}
	}

	// Start reading values until EOF
	std::vector<double> time;
	std::vector<double> ch1;
	while(!myfile.eof()){
		// Read first two columns
		double col1, col2;
		char comma;
		myfile >> col1 >> comma >> col2;
		if (myfile.fail()){
			break; // exit if not successful read
		}

		time.push_back(col1);
		ch1.push_back(col2);

		// Skip to next line
		myfile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	myfile.close();

	// Create histogram
	TString histName = FileUtils::getFileNameNoExtensionFromPath(filePath);
	TUUID uid = TUUID();
	TString uidSuffix = uid.AsString();
	histName += "_";
	histName += uidSuffix(0,4);

	if (time.size() <= 2){
		return nullptr;
	}

	double binWidth = time[1] - time[0];
	double leftEdge = time[1] - binWidth/2;
	double rightEdge = time.back() + binWidth/2;
	int nBins = (int)time.size();

	TH1* hist = new TH1D(histName.Data(), filePath, nBins, leftEdge, rightEdge);

	// Fill histogram
	for (unsigned int i = 1; i <= time.size(); i++){
	    hist->SetBinContent(i, ch1[i-1]);
	}

	hist->GetXaxis()->SetTitle("Time, s");
	hist->GetYaxis()->SetTitle("Amplitude, V");
	return hist;
}

//TFile* FileUtils::openFile(const char* fileName){
//  std::cout << "Opening file \"" << fileName << "\"..." << std::endl;
//  TFile *file = new TFile(fileName);
//  if (file->IsZombie()){
//    file->Close();
//    std::cerr << "Error opening file \"" << fileName << "\". Aborting." << std::endl;
//    exit(1);
//  }
//  return file;
//}

TString FileUtils::getFileNameFromPath(const char* fileNamePath){
	// Get file path directory
	TString dirName = gSystem->DirName(fileNamePath);

	// Remove directory from path
	TString fileNamePathString = fileNamePath;
	fileNamePathString.ReplaceAll(dirName, "");

	// Remove leading '/' or '\'
	TString str = fileNamePathString(1, fileNamePathString.Length()-1);

	return str;
}

TString FileUtils::getFileNameNoExtensionFromPath(const char* fileNamePath){
	// Get file path directory
	TString fileNameWithExt = FileUtils::getFileNameFromPath(fileNamePath);
	TString fileNameNoExt = StringUtils::stripExtension(fileNameWithExt.Data());
	return fileNameNoExt;
}

// Concatenate a directory and a file name. User must delete returned string. More...
// ConcatFileName (const char *dir, const char *name)

// Check file exists (also can check if readable and other permiissions)
// gSystem->AccessPathName(fileNamePath.Data()) == kFALSE)


// DirName()
