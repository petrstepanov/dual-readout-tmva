#include <TSystem.h>
#include <TApplication.h>
#include <TObjString.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TF1.h>

// #include "tinyfiledialogs.h"
#include "FileUtils.h"
#include "HistUtils.h"
#include "StringUtils.h"
#include "UiUtils.h"

#include <iostream>

#define VOLTAGE_THRESHOLD -0.03
#define MIN_PEAK_POS -1E-8
#define MAX_PEAK_POS 2E-8

// Function imports all Tektronix waveforms from a directory and filters out the "bad" (noise) waveforms.
// Returns TList of "good" TH1* histograms

TList* getGoodHistogramsList(const char* dirPath){
	// Obtain Cerenkov waveform paths from a directory
	TList* cherWaveformFilenames = FileUtils::getFilePathsInDirectory(dirPath, ".csv");

	// Compose list of histograms
	TList* hists = new TList();
 	for(TObject* obj : *cherWaveformFilenames){
 		TObjString* waveformCsvPath = (TObjString*)obj;

		// Import CSV waveform into histogram
		TH1* hist = FileUtils::tekWaveformToHist(waveformCsvPath->String().Data());
		if (!hist) continue;

		// Add waveform to list of histograms that were able to read
		hists->Add(hist);

		// Optionally: save waveforms as images
		TString waveformImgPath = StringUtils::stripExtension(waveformCsvPath->String().Data()); waveformImgPath += ".png";
		UiUtils::saveHistogramAsImage(hist, waveformImgPath.Data());
 	}

	// Compose a tree with waveform parameters
 	TTree* waveformsTree = new TTree("tree_waveforms","Tree with waveforms information");
	// Writing arrays to tree:
	// https://root.cern.ch/root/htmldoc/guides/users-guide/Trees.html#cb22
 	char* fileName = new char[256];
 	waveformsTree->Branch("fileName", fileName, "fileName[256]/C");
 	// double integral;
 	// waveformsTree->Branch("integral", &integral, "integral/D");
 	// double meanV;
 	// waveformsTree->Branch("meanV", &meanV, "meanV/D");
 	double minV;
 	waveformsTree->Branch("minV", &minV, "minV/D");
 	double peakPos;
 	waveformsTree->Branch("peakPos", &peakPos, "peakPos/D");
 	// double mean;
 	// waveformsTree->Branch("m", &mean, "mean/D");
 	// double sigma;
 	// waveformsTree->Branch("sigma", &sigma, "sigma/D");

	// Compose waveform parameters tree
 	for(TObject* obj : *hists){
 		TH1* hist = (TH1*)obj;

 		// Calculate waveform parameters (for later cuts)
 		strcpy(fileName, hist->GetName());
		// integral = hist->Integral("width");
		// meanV = HistUtils::getMeanY(hist);
		minV = hist->GetMinimum();
		peakPos = hist->GetXaxis()->GetBinCenter(hist->GetMinimumBin());

		// hist->Fit("gaus");
		// mean = hist->GetFunction("gaus")->GetParameter(1);
		// dMean = peHistAll->GetFunction("gaus")->GetParError(1); // mean error
		// sigma = hist->GetFunction("gaus")->GetParameter(2);
		// dSigma = peHistAll->GetFunction("gaus")->GetParError(2); // sigma error

		// Fill tree
		waveformsTree->Fill();
 	}

	// Draw waveform properties
	waveformsTree->SetFillColor(EColor::kCyan);
	TCanvas* canvas = new TCanvas();
	canvas->SetWindowSize(canvas->GetWw()*2, canvas->GetWh());
	canvas->Divide(2,1);
	TString canvasTitle = TString::Format("Waveforms Parameters in \"%s\"", dirPath);
	TString canvasSubTitle = TString::Format("\"Good\" waveform criteria: peakVoltage < %.2f && peakPosition > %.2e && peakPosition < %.2e", VOLTAGE_THRESHOLD, MIN_PEAK_POS, MAX_PEAK_POS);
	UiUtils::addCanvasTitle(canvas, canvasTitle.Data(), canvasSubTitle.Data());

	canvas->cd(1);
	UiUtils::plotBranch(waveformsTree, "minV", "Waveform Minimum Amplitude", "Minimum Amplitude, V", "Counts", 200);
	canvas->cd(2);
	UiUtils::plotBranch(waveformsTree, "peakPos", "Waveform Peak Positions", "Peak Position, s", "Counts", 200);

	canvas->SaveAs("waveforms-parameters.png");

	// Save waveform properties
 	TFile* f = new TFile("waveforms-parameters.root", "RECREATE");
 	waveformsTree->Write();
 	f->Close();

	// Apply cut to spectra and filter out ones
 	for (TObject* obj : *hists){
 		TH1* hist = (TH1*)obj;

 		// Set minimum voltage threshold to -0.03 V and peak position -1E-8 ... 2E-8
 		Double_t minVoltage = hist->GetMinimum();
 		Double_t peakPosition = hist->GetXaxis()->GetBinCenter(hist->GetMinimumBin());
		if (minVoltage > VOLTAGE_THRESHOLD || peakPosition < MIN_PEAK_POS || peakPosition > MAX_PEAK_POS){
			hists->Remove(obj);
			std::cout << "Removing waveform \"" << hist->GetTitle() << "\"" << std::endl;
		}
 	}

 	// Debug: save good waveforms under ../*-good/ folder
 	for (TObject* obj : *hists){
 		TH1* hist = (TH1*)obj;
		TString goodWaveformPath = gSystem->GetDirName(hist->GetTitle());
		goodWaveformPath += "-good";
		// Check output directory for "good" waveforms exist
		if (gSystem->AccessPathName(goodWaveformPath.Data())){
			gSystem->mkdir(goodWaveformPath.Data());
		}
		TString goodWaveformName = FileUtils::getFileNameNoExtensionFromPath(hist->GetTitle());
		goodWaveformName += ".png";
		TString goodPathName = gSystem->ConcatFileName(goodWaveformPath.Data(), goodWaveformName.Data());

		UiUtils::saveHistogramAsImage(hist, goodPathName.Data());
 	}

 	return hists;
}

void dualReadoutTMVA(const char* cherPath = "", const char* cherScintPath = ""){
	std::cout << "Preparing input data" << std::endl;

	// TinyFileDialogs approach
	// tinyfd_forceConsole = 0; /* default is 0 */
	// tinyfd_assumeGraphicDisplay = 0; /* default is 0 */
	// TString cherWaveformsDirPath = tinyfd_selectFolderDialog("Open Cerenkov Waveform Directory", NULL);

	// Check if Chernkov and Scintillation directory paths were passed via command line paramters:

	// Specify directory for Cerenkov waveforms
	TString cherWaveformsDirPath = cherPath;
	if (cherWaveformsDirPath.Length() == 0){
		UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify Cerenkov waveform directory");
		cherWaveformsDirPath = FileUtils::getDirectoryPath();
	}

	// Obtain "good" Cerenkov waveforms for TMVA
	TList* goodCherHists = getGoodHistogramsList(cherWaveformsDirPath.Data());

	// Specify directory for Cerenkov AND Scintillation waveforms
	TString cherScintWaveformsDirPath = cherScintPath;
	if (cherScintWaveformsDirPath.Length() == 0){
		UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify Cerenkov and scintillation waveform directory");
		cherScintWaveformsDirPath = FileUtils::getDirectoryPath();
	}

	// Obtain "good" Cerenkov waveforms for TMVA
	TList* goodCherScintHists = getGoodHistogramsList(cherScintWaveformsDirPath.Data());
}

int main(int argc, char **argv) {
	// Instantiate TApplication
	TApplication* app = new TApplication("energyResolution", &argc, argv);

	// Extract command line parameters
	TString cherDirPath, cherScintDirPath;
	if (app->Argc() >= 2) cherDirPath = app->Argv()[1];
	if (app->Argc() >= 3) cherScintDirPath = app->Argv()[2];

	dualReadoutTMVA(cherDirPath.Data(), cherScintDirPath.Data());

	// Enter the event loop
	app->Run();

	// Return success
	return 0;
}
