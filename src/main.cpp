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

void dualReadoutTMVA(){
	std::cout << "Preparing input data" << std::endl;

	// TinyFileDialogs approach
	// tinyfd_forceConsole = 0; /* default is 0 */
	// tinyfd_assumeGraphicDisplay = 0; /* default is 0 */
	// TString cherWaveformsDirPath = tinyfd_selectFolderDialog("Open Cerenkov Waveform Directory", NULL);

	// Specify directory for Cerenkov waveforms
	UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify Cerenkov waveform directory");
	TString cherWaveformsDirPath = FileUtils::getDirectoryPath();

	// Obtain Cerenkov waveform paths from a directory
	TList* cherWaveformFilenames = FileUtils::getFilePathsInDirectory(cherWaveformsDirPath, ".csv");

	// Compose list of histograms
	TList* cherHists = new TList();
 	for(TObject* obj : *cherWaveformFilenames){
 		TObjString* waveformCsvPath = (TObjString*)obj;

		// Import CSV waveform into histogram
		TH1* hist = FileUtils::tekWaveformToHist(waveformCsvPath->String().Data());
		cherHists->Add(hist);

		// Optionally: save waveforms as images
		// TString waveformImgPath = StringUtils::stripExtension(waveformCsvPath->String().Data()); waveformImgPath += ".png";
		// UiUtils::saveHistogramAsImage(hist, waveformImgPath.Data());
 	}

	// Compose a tree with waveform parameters
 	TTree* waveformsTree = new TTree("tree_waveforms","Tree with waveforms information");
	// Writing arrays to tree:
	// https://root.cern.ch/root/htmldoc/guides/users-guide/Trees.html#cb22
 	char* fileName = new char[256];
 	waveformsTree->Branch("fileName", fileName, "fileName[256]/C");
 	double integral;
 	waveformsTree->Branch("integral", &integral, "integral/D");
 	double meanV;
 	waveformsTree->Branch("meanV", &meanV, "meanV/D");
 	double minV;
 	waveformsTree->Branch("minV", &minV, "minV/D");
 	double timeAtMinV;
 	waveformsTree->Branch("timeAtMaxV", &timeAtMinV, "timeAtMaxV/D");
 	// double mean;
 	// waveformsTree->Branch("m", &mean, "mean/D");
 	// double sigma;
 	// waveformsTree->Branch("sigma", &sigma, "sigma/D");

	// Compose waveform parameters tree
 	for(TObject* obj : *cherHists){
 		TH1* hist = (TH1*)obj;

 		// Calculate waveform parameters (for later cuts)
 		strcpy(fileName, hist->GetName());
		integral = hist->Integral("width");
		meanV = HistUtils::getMeanY(hist);
		minV = hist->GetMinimum();
		timeAtMinV = hist->GetXaxis()->GetBinCenter(hist->GetMinimumBin());

		// hist->Fit("gaus");
		// mean = hist->GetFunction("gaus")->GetParameter(1);
		// dMean = peHistAll->GetFunction("gaus")->GetParError(1); // mean error
		// sigma = hist->GetFunction("gaus")->GetParameter(2);
		// dSigma = peHistAll->GetFunction("gaus")->GetParError(2); // sigma error

		// Fill tree
		waveformsTree->Fill();
 	}

 	// TCanvas* wfIntCanvas = new TCanvas("wf_int");
 	// waveformsTree->Draw("integral");

 	waveformsTree->SetFillColor(EColor::kCyan);
 	TCanvas* canvas = new TCanvas("canvas", "Waveforms Parameters");
 	canvas->SetWindowSize(canvas->GetWw(), canvas->GetWh()*2);
 	canvas->Divide(1,3);

 	TH1* htemp;
 	int i=0;

 	// Draw distribution of the absolute maximum values
 	canvas->cd(++i);
 	waveformsTree->Draw("meanV");
 	htemp = (TH1F*)gPad->GetPrimitive("htemp");
 	htemp->SetTitle("Waveform Mean Voltage");
 	htemp->GetXaxis()->SetTitle("Voltage, V");
 	htemp->GetYaxis()->SetTitle("Counts");

 	// Draw distribution of the absolute maximum values
 	canvas->cd(++i);
 	waveformsTree->Draw("minV");
 	htemp = (TH1F*)gPad->GetPrimitive("htemp");
 	htemp->SetTitle("Waveform Minimum Amplitude");
 	htemp->GetXaxis()->SetTitle("Minimum Amplitude, V");
 	htemp->GetYaxis()->SetTitle("Counts");

 	// Draw distribution of the peak positions
 	canvas->cd(++i);
 	waveformsTree->Draw("timeAtMaxV");
 	htemp = (TH1F*)gPad->GetPrimitive("htemp");
 	htemp->SetTitle("Waveform Peak Positions");
 	htemp->GetXaxis()->SetTitle("Peak Position, s");
 	htemp->GetYaxis()->SetTitle("Counts");

 	canvas->SaveAs("waveforms-parameters.png");

 	TFile* f = new TFile("waveforms-parameters.root", "RECREATE");
 	waveformsTree->Write();
 	f->Close();
	// TODO: Apply cut to spectra and filter out ones
}

int main(int argc, char **argv) {
	// Instantiate TApplication
	TApplication* app = new TApplication("energyResolution", &argc, argv);

	dualReadoutTMVA();

	// Enter the event loop
	app->Run();

	// Return success
	return 0;
}
