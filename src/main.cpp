#include <TSystem.h>
#include <TApplication.h>
#include <TObjString.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TROOT.h>
#include <TF1.h>

#include <TMVA/DataLoader.h>
#include <TMVA/Factory.h>
#include <TMVA/TMVAGui.h>
// #include "tinyfiledialogs.h"
#include "FileUtils.h"
#include "HistUtils.h"
#include "StringUtils.h"
#include "UiUtils.h"

#include <iostream>
#include "cxxopts.hpp"

#define VOLTAGE_THRESHOLD -0.03
#define MIN_PEAK_POS -1E-8
#define MAX_PEAK_POS 2E-8

// Function imports all Tektronix waveforms from a directory and filters out the "bad" (noise) waveforms.
// Returns TList of "good" TH1* histograms

TList* getGoodHistogramsList(const char *dirPath) {
	// Obtain Cerenkov waveform paths from a directory
	TList *cherWaveformFilenames = FileUtils::getFilePathsInDirectory(dirPath, ".csv");

	// Compose list of histograms
	TList *hists = new TList();
	for (TObject *obj : *cherWaveformFilenames) {
		TObjString *waveformCsvPath = (TObjString*) obj;

		// Import CSV waveform into histogram
		TH1 *hist = FileUtils::tekWaveformToHist(waveformCsvPath->String().Data());
		if (!hist)
			continue;

		// Add waveform to list of histograms that were able to read
		hists->Add(hist);

		// Optionally: save waveforms as images
		TString waveformImgPath = StringUtils::stripExtension(waveformCsvPath->String().Data());
		waveformImgPath += ".png";
		UiUtils::saveHistogramAsImage(hist, waveformImgPath.Data());
	}

	// Compose a tree with waveform parameters
	TTree *waveformsTree = new TTree("tree_waveforms", "Tree with waveforms information");
	// Writing arrays to tree:
	// https://root.cern.ch/root/htmldoc/guides/users-guide/Trees.html#cb22
	char *fileName = new char[256];
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
	for (TObject *obj : *hists) {
		TH1 *hist = (TH1*) obj;

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
	TCanvas *canvas = new TCanvas();
	canvas->SetWindowSize(canvas->GetWw() * 2, canvas->GetWh());
	canvas->Divide(2, 1);
	TString canvasTitle = TString::Format("Waveforms Parameters in \"%s\"", dirPath);
	TString canvasSubTitle = TString::Format("\"Good\" waveform criteria: peakVoltage < %.2f V && peakPosition > %.2e s && peakPosition < %.2e s",
	VOLTAGE_THRESHOLD, MIN_PEAK_POS, MAX_PEAK_POS);
	UiUtils::addCanvasTitle(canvas, canvasTitle.Data(), canvasSubTitle.Data());

	canvas->cd(1);
	UiUtils::plotBranch(waveformsTree, "minV", "Waveform Minimum Amplitude", "Minimum Amplitude, V", "Counts", 200);
	canvas->cd(2);
	UiUtils::plotBranch(waveformsTree, "peakPos", "Waveform Peak Positions", "Peak Position, s", "Counts", 200);

	TString wfPngFilePath = gSystem->ConcatFileName(dirPath, "waveforms-parameters.png");
	canvas->SaveAs(wfPngFilePath.Data());

	// Save waveform properties
	TString wfRootFilePath = gSystem->ConcatFileName(dirPath, "waveforms-parameters.root");
	TFile *f = new TFile(wfRootFilePath.Data(), "RECREATE");
	waveformsTree->Write();
	f->Close();

	// Apply cut to spectra and filter out ones
	for (TObject *obj : *hists) {
		TH1 *hist = (TH1*) obj;

		// Set minimum voltage threshold to -0.03 V and peak position -1E-8 ... 2E-8
		Double_t minVoltage = hist->GetMinimum();
		Double_t peakPosition = hist->GetXaxis()->GetBinCenter(hist->GetMinimumBin());
		if (minVoltage > VOLTAGE_THRESHOLD || peakPosition < MIN_PEAK_POS || peakPosition > MAX_PEAK_POS) {
			hists->Remove(obj);
			std::cout << "Removing waveform \"" << hist->GetTitle() << "\"" << std::endl;
		}
	}

	// Debug: save good waveforms under ../*-good/ folder
	for (TObject *obj : *hists) {
		TH1 *hist = (TH1*) obj;
		TString goodWaveformPath = dirPath; // gSystem->GetDirName(hist->GetTitle());
		goodWaveformPath += "-good";
		// Check output directory for "good" waveforms exist
		if (gSystem->AccessPathName(goodWaveformPath.Data())) {
			gSystem->mkdir(goodWaveformPath.Data());
		}
		TString goodWaveformName = FileUtils::getFileNameNoExtensionFromPath(hist->GetTitle());
		goodWaveformName += ".png";
		TString goodPathName = gSystem->ConcatFileName(goodWaveformPath.Data(), goodWaveformName.Data());

		UiUtils::saveHistogramAsImage(hist, goodPathName.Data());
	}

	return hists;
}

void createROOTFileForLearning(const char *cherPath, const char *cherScintPath) {
	// TinyFileDialogs approach
	// tinyfd_forceConsole = 0; /* default is 0 */
	// tinyfd_assumeGraphicDisplay = 0; /* default is 0 */
	// TString cherWaveformsDirPath = tinyfd_selectFolderDialog("Open Cerenkov Waveform Directory", NULL);

	// Check if Chernkov and Scintillation directory paths were passed via command line paramters:

	// Specify directory for Cerenkov waveforms
	TString cherWaveformsDirPath = cherPath;
	if (cherWaveformsDirPath.Length() == 0) {
		UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify Cerenkov waveform directory");
		cherWaveformsDirPath = FileUtils::getDirectoryPath();
	}

	// Obtain "good" Cerenkov waveforms for TMVA
	TList *goodCherHists = getGoodHistogramsList(cherWaveformsDirPath.Data());

	// Specify directory for Cerenkov AND Scintillation waveforms
	TString cherScintWaveformsDirPath = cherScintPath;
	if (cherScintWaveformsDirPath.Length() == 0) {
		UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify Cerenkov and scintillation waveform directory");
		cherScintWaveformsDirPath = FileUtils::getDirectoryPath();
	}

	// Obtain "good" Cerenkov waveforms for TMVA
	TList *goodCherScintHists = getGoodHistogramsList(cherScintWaveformsDirPath.Data());

	// TODO: Crop and invert histograms is required for the hist->GetRandom() to work?
	TList *goodCherHistsPrepared = new TList();
	for (TObject *obj : *goodCherHists) {
		TH1 *hist = (TH1*) obj;
		TH1 *prepedHist = HistUtils::prepHistForTMVA(hist);
		// goodCherHists->Remove(obj);
		goodCherHistsPrepared->Add(prepedHist);
	}
	TList *goodCherScintHistsPrepared = new TList();
	for (TObject *obj : *goodCherScintHists) {
		TH1 *hist = (TH1*) obj;
		TH1 *prepedHist = HistUtils::prepHistForTMVA(hist);
		// goodCherScintHists->Remove(obj);
		goodCherScintHistsPrepared->Add(prepedHist);
	}

	// Prepare trees
	// TTree *backgroundTree = HistUtils::histsToTree(goodCherHists, "treeB", "Background Tree - Cerenkov");
	// TTree *signalTree = HistUtils::histsToTree(goodCherHists, "treeS", "Signal Tree - Cerenkov and scintillation");
	TTree *treeSignal = HistUtils::histsToTreeXY(goodCherHistsPrepared, "treeB", "Background Tree - Cerenkov");
	TTree *treeBackground = HistUtils::histsToTreeXY(goodCherScintHistsPrepared, "treeS", "Signal Tree - Cerenkov and scintillation");

	// Write trees to file for training
	TString workingDirectory = gSystem->GetWorkingDirectory().c_str();
	TString tmvaFileName = "tmva-input.root";
	TString tmvaFileNamePath = gSystem->ConcatFileName(workingDirectory.Data(), tmvaFileName.Data());
	TFile *tmvaFile = new TFile(tmvaFileNamePath.Data(), "RECREATE");
	treeBackground->Write();
	treeSignal->Write();
	tmvaFile->Close();
}

void trainTMVA(const char *trainingFileURI) {
	// Open teaching ROOT file and extract signal and backgroud trees
	TFile *trainingFile = TFile::Open(trainingFileURI);
	if (!trainingFile) {
		exit(1);
	}

	// Minimal classification example:
	// https://root.cern/doc/master/TMVAMinimalClassification_8C.html
	TTree *treeB = trainingFile->Get<TTree>("treeB");
	TTree *treeS = trainingFile->Get<TTree>("treeS");

	// Create output ROOT file
	// Refer here: https://root.cern/doc/master/TMVAClassification_8C.html
	TString outputFileName("tmva-output.root");
	TFile *outputFile = TFile::Open(outputFileName, "RECREATE");

	// Create the factory object. Later you can choose the methods
	TMVA::Factory *factory = new TMVA::Factory("TMVAClassification", outputFile, "AnalysisType=Classification");

	// Data specification
	TMVA::DataLoader *dataloader = new TMVA::DataLoader("dataset");
	dataloader->AddVariable("x", 'D');
	dataloader->AddVariable("y", 'D');

	dataloader->AddSignalTree(treeS);       // TODO: specify ETreeType::kTraining,
	dataloader->AddBackgroundTree(treeB);

	// Apply additional cuts on the signal and background samples (can be different)
	TCut signalCut = "";
	TCut backgroundCut = "";
	TString datasetOptions = "SplitMode=Random";
	dataloader->PrepareTrainingAndTestTree(signalCut, backgroundCut, datasetOptions);

	// Method specification
	factory->BookMethod(dataloader, TMVA::Types::kBDT, "BDT", "");
	// factory->BookMethod(dataLoader, TMVA::Types::kFisher, "Fisher", "!H:!V:Fisher");
	// factory->BookMethod(dataloader, TMVA::Types::kLikelihood, "Likelihood", "!H:!V:TransformOutput:PDFInterpol=Spline2:NSmoothSig[0]=20:NSmoothBkg[0]=20:NSmoothBkg[1]=10:NSmooth=1:NAvEvtPerBin=50");

	// Now you can tell the factory to train, test, and evaluate the MVAs
	factory->TrainAllMethods();
	factory->TestAllMethods();
	factory->EvaluateAllMethods();

	// Clean up
	outputFile->Close();

	delete outputFile;
	delete treeS;
	delete treeB;

	// Clean up
	delete factory;
	delete dataloader;

	// Launch the GUI for the root macros
	if (!gROOT->IsBatch()) TMVA::TMVAGui(outputFileName);
}

void classifyWaveform(const char *weightFile, const char *testDir) {
	// TODO: classifying phase
}

int main(int argc, char *argv[]) {
	// Instantiate TApplication
	TApplication *app = new TApplication("energyResolution", &argc, argv);

	// Initialize command-line parameters options
	// https://github.com/jarro2783/cxxopts
	cxxopts::Options options("Dual Readout TMVA", "ROOT Machine Learning (ML) approach to categorize waveforms upon their shape.");

	// Add command-line options
	options.allow_unrecognised_options().add_options()("m,mode", "Program mode ('prepare', 'train', 'classify')", cxxopts::value<std::string>()) //
	("b,background", "Directory path for background .csv waveforms ('prepare')", cxxopts::value<std::string>()) //
	("s,signal", "Directory path for signal .csv waveforms ('prepare')", cxxopts::value<std::string>()) //
	("w,weight", "Machine learning weight file path ('classify')", cxxopts::value<std::string>()) //
	// ("t,test", "Directory path with .csv waveforms for classifying ('classify')", cxxopts::value<std::string>()) //
	// ("g,gui", "Start with ROOT GUI", cxxopts::value<bool>()->default_value("false"))
	("h,help", "Print usage"); //

	auto result = options.parse(app->Argc(), app->Argv());

	if (result.count("help")) {
		std::cout << options.help() << std::endl;
		exit(0);
	}
	std::string mode;
	if (result.count("mode"))
		mode = result["mode"].as<std::string>();
	std::string backgroundDir;
	if (result.count("background"))
		backgroundDir = result["background"].as<std::string>();
	std::string signalDir;
	if (result.count("signal"))
		signalDir = result["signal"].as<std::string>();
	std::string weightFile;
	if (result.count("weight"))
		weightFile = result["weight"].as<std::string>();
	std::string testDir;
	// if (result.count("test"))
	// testDir = result["test"].as<std::string>();
	// bool gui = result["gui"].as<bool>();

	if (mode == "prepare") {
		// Step 1. Process CSV waveforms into a ROOT file with trees for learning
		createROOTFileForLearning(backgroundDir.c_str(), signalDir.c_str());
	} else if (mode == "train") {
		// Step 2. Learn ROOT TMVA to categorize the
		std::vector<std::string> unmatched = result.unmatched();
		trainTMVA(unmatched[0].c_str());
	} else if (mode == "classify") {
		// Step 3. Use TMVA to categorize the
		classifyWaveform(weightFile.c_str(), testDir.c_str());
	}

	// Enter the event loop
	app->Run();

	// Return success
	return 0;
}
