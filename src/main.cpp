#include <TSystem.h>
#include <TApplication.h>
#include <TObjString.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TROOT.h>
#include <TF1.h>
#include <TLeaf.h>
#include <TVectorD.h>
#include <TMacro.h>

#include <TMVA/DataLoader.h>
#include <TMVA/Factory.h>
#include <TMVA/TMVAGui.h>
#include <TMVA/Reader.h>
#include <TMVA/Tools.h>
#include <TMVA/PyMethodBase.h>
// #include "tinyfiledialogs.h"
#include "FileUtils.h"
#include "HistUtils.h"
#include "StringUtils.h"
#include "UiUtils.h"

#include <iostream>
#include "cxxopts.hpp"

#define N_BINS 10000
#define VOLTAGE_THRESHOLD -0.03
#define MIN_PEAK_POS -1E-8
#define MAX_PEAK_POS 2E-8

// Function imports all Tektronix waveforms from a directory and filters out the "bad" (noise) waveforms.
// Returns TList of "good" TH1* histograms

TList* getGoodHistogramsList(const char *dirPath) {
	// Obtain Cerenkov waveform paths from a directory
	TList *waveformFilenames = FileUtils::getFilePathsInDirectory(dirPath, ".csv");

	// Compose list of histograms
	TList *hists = new TList();
	for (TObject *obj : *waveformFilenames) {
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
		// UiUtils::saveHistogramAsImage(hist, waveformImgPath.Data());
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
		Int_t nBins = hist->GetNbinsX();
		if (minVoltage > VOLTAGE_THRESHOLD || peakPosition < MIN_PEAK_POS || peakPosition > MAX_PEAK_POS || nBins != N_BINS) {
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

		// UiUtils::saveHistogramAsImage(hist, goodPathName.Data());
	}

	return hists;
}

enum class MLFileType {
	Linear,     // https://root.cern/doc/master/TMVA__CNN__Classification_8C.html
	PonitsXY,   // https://root.cern/doc/master/TMVAMinimalClassification_8C.html
	PDF         // https://root.cern/doc/master/TMVAClassification_8C.html
};

void createROOTFileForLearning(const char *cherPath, const char *cherScintPath, MLFileType rootFileType = MLFileType::Linear) {
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
	Info("createROOTFileForLearning", "\"Good\" background histograms selected");

	// Specify directory for Cerenkov AND Scintillation waveforms
	TString cherScintWaveformsDirPath = cherScintPath;
	if (cherScintWaveformsDirPath.Length() == 0) {
		UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify Cerenkov and scintillation waveform directory");
		cherScintWaveformsDirPath = FileUtils::getDirectoryPath();
	}

	// Obtain "good" Cerenkov and Scintillation waveforms for TMVA
	TList *goodCherScintHists = getGoodHistogramsList(cherScintWaveformsDirPath.Data());
	Info("createROOTFileForLearning", "\"Good\" signal histograms selected");

	// TODO: Crop and invert histograms is required for the hist->GetRandom() to work?
	TList *goodCherHistsPrepared = new TList();
	for (TObject *obj : *goodCherHists) {
		TH1 *hist = (TH1*) obj;
		TH1 *prepedHist = HistUtils::prepHistForTMVA(hist);
		// goodCherHists->Remove(obj);
		goodCherHistsPrepared->Add(prepedHist);
		Info("createROOTFileForLearning", "\"Good\" background histograms processed (invert, crop)");
	}
	TList *goodCherScintHistsPrepared = new TList();
	for (TObject *obj : *goodCherScintHists) {
		TH1 *hist = (TH1*) obj;
		TH1 *prepedHist = HistUtils::prepHistForTMVA(hist);
		// goodCherScintHists->Remove(obj);
		goodCherScintHistsPrepared->Add(prepedHist);
		Info("createROOTFileForLearning", "\"Good\" signal histograms processed (invert, crop)");
	}

	// Prepare trees
	TTree* treeBackground;
	TTree* treeSignal;
	if (rootFileType == MLFileType::Linear){
		treeBackground = HistUtils::histsToTreeLin(goodCherHistsPrepared, "treeB", "Background Tree - Cerenkov");
		Info("createROOTFileForLearning", "Background Tree Created");
		treeSignal = HistUtils::histsToTreeLin(goodCherScintHistsPrepared, "treeS", "Signal Tree - Cerenkov and scintillation");
		Info("createROOTFileForLearning", "Signal Tree Created");
	} else if (rootFileType == MLFileType::PDF){
		treeBackground = HistUtils::histsToTree(goodCherHistsPrepared, "treeB", "Background Tree - Cerenkov");
		treeSignal = HistUtils::histsToTree(goodCherScintHistsPrepared, "treeS", "Signal Tree - Cerenkov and scintillation");
	} else {
		treeBackground = HistUtils::histsToTreeXY(goodCherHistsPrepared, "treeS", "Background Tree - Cerenkov");
		treeSignal = HistUtils::histsToTreeXY(goodCherScintHistsPrepared, "treeB", "Signal Tree - Cerenkov and scintillation");
	}

	// Write trees to file for training
	TString workingDirectory = gSystem->GetWorkingDirectory().c_str();
	TString tmvaFileName = "tmva-input.root";
	TString tmvaFileNamePath = gSystem->ConcatFileName(workingDirectory.Data(), tmvaFileName.Data());
	TFile *tmvaFile = new TFile(tmvaFileNamePath.Data(), "RECREATE");
	treeBackground->Write();
	treeSignal->Write();

	// Write histograms number of bins - need for TMVA reading later if waveforms
	// are written in series

	Int_t backgroundBins = ((TH1*)goodCherHistsPrepared->At(0))->GetNbinsX();
	Int_t signalBins = ((TH1*)goodCherScintHistsPrepared->At(0))->GetNbinsX();
	if (backgroundBins != signalBins){
		std::cout << "Signal bins not equal to background bins." << std::endl;
		exit(1);
	}
	TVectorD bins(1);
	bins[0] = backgroundBins;
	bins.Write("bins");

	tmvaFile->Close();
	Info("createROOTFileForLearning", "File \"%s\" created", tmvaFileNamePath.Data());
}

void trainTMVA(const char *trainingFileURI, MLFileType rootFileType = MLFileType::Linear) {
	// Enable ROOT Multi-Threading
	TMVA::Tools::Instance();
	int nThreads = 0;  // use default threads
	ROOT::EnableImplicitMT(nThreads);
	gSystem->Setenv("OMP_NUM_THREADS", nThreads == 0 ? "1" : TString::Format("%d", nThreads).Data());
	std::cout << "Running with nThreads = " << ROOT::GetThreadPoolSize() << std::endl;

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
	// https://root.cern/doc/master/TMVAMinimalClassification_8C.html
	TMVA::Factory *factory = new TMVA::Factory("TMVAClassification", outputFile, "AnalysisType=Classification");

	// https://root.cern/doc/master/TMVA__CNN__Classification_8C.html
	// TMVA::Factory *factory = new TMVA::Factory("TMVAClassification", outputFile, "!V:ROC:!Silent:Color:AnalysisType=Classification:Transformations=None:!Correlations");

	// Data specification
	TMVA::DataLoader *dataloader = new TMVA::DataLoader("dataset");

	// Add signal and background trees
	dataloader->AddSignalTree(treeS);       // TODO: specify ETreeType::kTraining,
	dataloader->AddBackgroundTree(treeB);

	// Specify variables
	if (rootFileType == MLFileType::Linear){
		// Read histogram size from file
		TVectorD* bins = trainingFile->Get<TVectorD>("bins");
		Double_t b = (*bins)[0];
		// Use new method (from ROOT 6.20 to add a variable array for all image data)
		dataloader->AddVariablesArray("vars", (Int_t)b);
	}
	else if (rootFileType == MLFileType::PonitsXY){
		dataloader->AddVariable("x", 'D');
		dataloader->AddVariable("y", 'D');
	}
	else if (rootFileType == MLFileType::PDF){
		TObjArray* listOfLeaves = treeB->GetListOfLeaves();
		for (TObject* obj : * listOfLeaves){
			TLeaf* leaf = (TLeaf*) obj;
			dataloader->AddVariable( leaf->GetName(), leaf->GetName(), "units", 'D' );
		}
	}
	else {
		std::cout << "Unspecified input data type." << std::endl;
		exit(0);
	}

	// Apply additional cuts on the signal and background samples (can be different)
	TCut signalCut = "";
	TCut backgroundCut = "";

	int nEventsBkg = treeB->GetEntries();
	int nEventsSig = treeS->GetEntries();
	int nTrainSig = 0.8 * nEventsSig;
	int nTrainBkg = 0.8 * nEventsBkg;

	// Build the string options for DataLoader::PrepareTrainingAndTestTree
	TString prepareOptions = TString::Format("nTrain_Signal=%d:nTrain_Background=%d:SplitMode=Random:SplitSeed=100:NormMode=NumEvents:!V:!CalcCorrelations",
			                                  nTrainSig, nTrainBkg);

	// TString prepareOptions = "SplitMode=Random";
	dataloader->PrepareTrainingAndTestTree(signalCut, backgroundCut, prepareOptions);

	// Method specification
	// BDT = Boosted Decigion Trees
	factory->BookMethod(dataloader, TMVA::Types::kBDT, "BDT", "");
	// factory->BookMethod(dataLoader, TMVA::Types::kFisher, "Fisher", "!H:!V:Fisher");
	// factory->BookMethod(dataloader, TMVA::Types::kLikelihood, "Likelihood", "!H:!V:TransformOutput:PDFInterpol=Spline2:NSmoothSig[0]=20:NSmoothBkg[0]=20:NSmoothBkg[1]=10:NSmooth=1:NAvEvtPerBin=50");

	// Now you can tell the factory to train, test, and evaluate the MVAs
	factory->TrainAllMethods();
	factory->TestAllMethods();
	factory->EvaluateAllMethods();

	// Plot ROC Curve
	TCanvas* c1 = factory->GetROCCurve(dataloader);
	c1->Draw();

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

void trainTMVA_CNN(const char *trainingFileURI, Bool_t useTMVACNN, Bool_t useTMVADNN, Bool_t useTMVABDT, Bool_t usePyTorchCNN){
		#ifndef R__HAS_TMVACPU
		#ifndef R__HAS_TMVAGPU
		   Warning("TMVA_CNN_Classification",
				   "TMVA is not build with GPU or CPU multi-thread support. Cannot use TMVA Deep Learning for CNN");
		   useTMVACNN = false;
		#endif
		#endif

	   bool writeOutputFile = true;

	   int num_threads = 0;  // use default threads

	   TMVA::Tools::Instance();

	   // do enable MT running
	   if (num_threads >= 0) {
	      ROOT::EnableImplicitMT(num_threads);
	      if (num_threads > 0) gSystem->Setenv("OMP_NUM_THREADS", TString::Format("%d",num_threads));
	   }
	   else
	      gSystem->Setenv("OMP_NUM_THREADS", "1");

	   std::cout << "Running with nthreads  = " << ROOT::GetThreadPoolSize() << std::endl;

	   TFile *outputFile = nullptr;
	   if (writeOutputFile)
	      outputFile = TFile::Open("TMVA_CNN_ClassificationOutput.root", "RECREATE");

	   /***
	       ## Create TMVA Factory

	    Create the Factory class. Later you can choose the methods
	    whose performance you'd like to investigate.

	    The factory is the major TMVA object you have to interact with. Here is the list of parameters you need to pass

	    - The first argument is the base of the name of all the output
	    weightfiles in the directory weight/ that will be created with the
	    method parameters

	    - The second argument is the output file for the training results

	    - The third argument is a string option defining some general configuration for the TMVA session.
	      For example all TMVA output can be suppressed by removing the "!" (not) in front of the "Silent" argument in the
	   option string

	    - note that we disable any pre-transformation of the input variables and we avoid computing correlations between
	   input variables
	   ***/

	   TMVA::Factory factory(
	      "TMVA_CNN_Classification", outputFile,
	      "!V:ROC:!Silent:Color:AnalysisType=Classification:Transformations=None:!Correlations");

	   /***

	       ## Declare DataLoader(s)

	       The next step is to declare the DataLoader class that deals with input variables

	       Define the input variables that shall be used for the MVA training
	       note that you may also use variable expressions, which can be parsed by TTree::Draw( "expression" )]

	       In this case the input data consists of an image of 16x16 pixels. Each single pixel is a branch in a ROOT TTree

	   **/

	   TMVA::DataLoader *loader = new TMVA::DataLoader("dataset");

	   /***

	       ## Setup Dataset(s)

	       Define input data file and signal and background trees

	   **/

		// Open teaching ROOT file and extract signal and backgroud trees
		TFile *inputFile = TFile::Open(trainingFileURI);
		if (!inputFile) {
			Error("TMVA_CNN_Classification", "Input file %s not found", trainingFileURI);
			exit(1);
		}

	   // --- Register the training and test trees

	   TTree *signalTree = (TTree *)inputFile->Get("treeS");
	   TTree *backgroundTree = (TTree *)inputFile->Get("treeB");

	   int nEventsSig = signalTree->GetEntries();
	   int nEventsBkg = backgroundTree->GetEntries();

	   // global event weights per tree (see below for setting event-wise weights)
	   Double_t signalWeight = 1.0;
	   Double_t backgroundWeight = 1.0;

	   // You can add an arbitrary number of signal or background trees
	   loader->AddSignalTree(signalTree, signalWeight);
	   loader->AddBackgroundTree(backgroundTree, backgroundWeight);

		// Read histogram size from file
		TVectorD* bins = inputFile->Get<TVectorD>("bins");
		Double_t b = (*bins)[0];

	   /// add event variables (image)
	   /// use new method (from ROOT 6.20 to add a variable array for all image data)
	   loader->AddVariablesArray("vars", (Int_t)b);

	   // Set individual event weights (the variables must exist in the original TTree)
	   //    for signal    : factory->SetSignalWeightExpression    ("weight1*weight2");
	   //    for background: factory->SetBackgroundWeightExpression("weight1*weight2");
	   // loader->SetBackgroundWeightExpression( "weight" );

	   // Apply additional cuts on the signal and background samples (can be different)
	   TCut mycuts = ""; // for example: TCut mycuts = "abs(var1)<0.5 && abs(var2-0.5)<1";
	   TCut mycutb = ""; // for example: TCut mycutb = "abs(var1)<0.5";

	   // Tell the factory how to use the training and testing events
	   //
	   // If no numbers of events are given, half of the events in the tree are used
	   // for training, and the other half for testing:
	   //    loader->PrepareTrainingAndTestTree( mycut, "SplitMode=random:!V" );
	   // It is possible also to specify the number of training and testing events,
	   // note we disable the computation of the correlation matrix of the input variables

	   int nTrainSig = 0.8 * nEventsSig;
	   int nTrainBkg = 0.8 * nEventsBkg;

	   // build the string options for DataLoader::PrepareTrainingAndTestTree
	   TString prepareOptions = TString::Format(
	      "nTrain_Signal=%d:nTrain_Background=%d:SplitMode=Random:SplitSeed=100:NormMode=NumEvents:!V:!CalcCorrelations",
	      nTrainSig, nTrainBkg);

	   loader->PrepareTrainingAndTestTree(mycuts, mycutb, prepareOptions);

	   /***

	       DataSetInfo              : [dataset] : Added class "Signal"
	       : Add Tree sig_tree of type Signal with 10000 events
	       DataSetInfo              : [dataset] : Added class "Background"
	       : Add Tree bkg_tree of type Background with 10000 events



	   **/

	   // signalTree->Print();

	   /****
	        # Booking Methods

	        Here we book the TMVA methods. We book a Boosted Decision Tree method (BDT)

	   **/

	   // Boosted Decision Trees
	   if (useTMVABDT) {
	      factory.BookMethod(loader, TMVA::Types::kBDT, "BDT",
	                         "!V:NTrees=400:MinNodeSize=2.5%:MaxDepth=2:BoostType=AdaBoost:AdaBoostBeta=0.5:"
	                         "UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20");
	   }
	   /**

	      #### Booking Deep Neural Network

	      Here we book the DNN of TMVA. See the example TMVA_Higgs_Classification.C for a detailed description of the
	   options

	   **/

	   if (useTMVADNN) {

	      TString layoutString(
	         "Layout=DENSE|100|RELU,BNORM,DENSE|100|RELU,BNORM,DENSE|100|RELU,BNORM,DENSE|100|RELU,DENSE|1|LINEAR");

	      // Training strategies
	      // one can catenate several training strings with different parameters (e.g. learning rates or regularizations
	      // parameters) The training string must be concatenates with the `|` delimiter
	      TString trainingString1("LearningRate=1e-3,Momentum=0.9,Repetitions=1,"
	                              "ConvergenceSteps=5,BatchSize=100,TestRepetitions=1,"
	                              "MaxEpochs=20,WeightDecay=1e-4,Regularization=None,"
	                              "Optimizer=ADAM,DropConfig=0.0+0.0+0.0+0.");

	      TString trainingStrategyString("TrainingStrategy=");
	      trainingStrategyString += trainingString1; // + "|" + trainingString2 + ....

	      // Build now the full DNN Option string

	      TString dnnOptions("!H:V:ErrorStrategy=CROSSENTROPY:VarTransform=None:"
	                         "WeightInitialization=XAVIER");
	      dnnOptions.Append(":");
	      dnnOptions.Append(layoutString);
	      dnnOptions.Append(":");
	      dnnOptions.Append(trainingStrategyString);

	      TString dnnMethodName = "TMVA_DNN_CPU";
	// use GPU if available
	#ifdef R__HAS_TMVAGPU
	      dnnOptions += ":Architecture=GPU";
	      dnnMethodName = "TMVA_DNN_GPU";
	#elif defined(R__HAS_TMVACPU)
	      dnnOptions += ":Architecture=CPU";
	#endif

	      factory.BookMethod(loader, TMVA::Types::kDL, dnnMethodName, dnnOptions);
	   }

	   /***
	    ### Book Convolutional Neural Network in TMVA

	    For building a CNN one needs to define

	    -  Input Layout :  number of channels (in this case = 1)  | image height | image width
	    -  Batch Layout :  batch size | number of channels | image size = (height*width)

	    Then one add Convolutional layers and MaxPool layers.

	    -  For Convolutional layer the option string has to be:
	       - CONV | number of units | filter height | filter width | stride height | stride width | padding height | paddig
	   width | activation function

	       - note in this case we are using a filer 3x3 and padding=1 and stride=1 so we get the output dimension of the
	   conv layer equal to the input

	      - note we use after the first convolutional layer a batch normalization layer. This seems to help significatly the
	   convergence

	     - For the MaxPool layer:
	        - MAXPOOL  | pool height | pool width | stride height | stride width

	    The RESHAPE layer is needed to flatten the output before the Dense layer


	    Note that to run the CNN is required to have CPU  or GPU support

	   ***/

	   if (useTMVACNN) {
		  // Fatal in <calculateDimension>: Not compatible hyper parameters for layer - (imageDim, filterDim, padding, stride) 1, 2, 0, 1

	      TString inputLayoutString("InputLayout=1|1065");

	      // Batch Layout
	      TString layoutString("Layout=CONV|10|3|3|1|1|1|1|RELU,BNORM,CONV|10|3|3|1|1|1|1|RELU,MAXPOOL|2|2|1|1,"
	                           "RESHAPE|FLAT,DENSE|100|RELU,DENSE|1|LINEAR");

	      // Training strategies.
	      TString trainingString1("LearningRate=1e-3,Momentum=0.9,Repetitions=1,"
	                              "ConvergenceSteps=5,BatchSize=100,TestRepetitions=1,"
	                              "MaxEpochs=20,WeightDecay=1e-4,Regularization=None,"
	                              "Optimizer=ADAM,DropConfig=0.0+0.0+0.0+0.0");

	      TString trainingStrategyString("TrainingStrategy=");
	      trainingStrategyString +=
	         trainingString1; // + "|" + trainingString2 + "|" + trainingString3; for concatenating more training strings

	      // Build full CNN Options.
	      TString cnnOptions("!H:V:ErrorStrategy=CROSSENTROPY:VarTransform=None:"
	                         "WeightInitialization=XAVIER");

	      cnnOptions.Append(":");
	      cnnOptions.Append(inputLayoutString);
	      cnnOptions.Append(":");
	      cnnOptions.Append(layoutString);
	      cnnOptions.Append(":");
	      cnnOptions.Append(trainingStrategyString);

	      //// New DL (CNN)
	      TString cnnMethodName = "TMVA_CNN_CPU";
	// use GPU if available
	#ifdef R__HAS_TMVAGPU
	      cnnOptions += ":Architecture=GPU";
	      cnnMethodName = "TMVA_CNN_GPU";
	#else
	      cnnOptions += ":Architecture=CPU";
	      cnnMethodName = "TMVA_CNN_CPU";
	#endif

	      factory.BookMethod(loader, TMVA::Types::kDL, cnnMethodName, cnnOptions);
	   }

	   if (usePyTorchCNN) {

	      Info("TMVA_CNN_Classification", "Using Convolutional PyTorch Model");
	      TString pyTorchFileName = gROOT->GetTutorialDir() + TString("/tmva/PyTorch_Generate_CNN_Model.py");
	      // check that pytorch can be imported and file defining the model and used later when booking the method is existing
	      if (gSystem->Exec("python -c 'import torch'")  || gSystem->AccessPathName(pyTorchFileName) ) {
	         Warning("TMVA_CNN_Classification", "PyTorch is not installed or model building file is not existing - skip using PyTorch");
	      }
	      else {
	         // book PyTorch method only if PyTorch model could be created
	         Info("TMVA_CNN_Classification", "Booking PyTorch CNN model");
	         TString methodOpt = "H:!V:VarTransform=None:FilenameModel=PyTorchModelCNN.pt:"
	                             "FilenameTrainedModel=PyTorchTrainedModelCNN.pt:NumEpochs=20:BatchSize=100";
	         methodOpt += TString(":UserCode=") + pyTorchFileName;
	         factory.BookMethod(loader, TMVA::Types::kPyTorch, "PyTorch", methodOpt);
	      }
	   }


	   ////  ## Train Methods

	   factory.TrainAllMethods();

	   /// ## Test and Evaluate Methods

	   factory.TestAllMethods();

	   factory.EvaluateAllMethods();

	   /// ## Plot ROC Curve

	   auto c1 = factory.GetROCCurve(loader);
	   c1->Draw();

	   // close outputfile to save output file
	   outputFile->Close();

	   // Launch the GUI for the root macros
	   if (!gROOT->IsBatch()) TMVA::TMVAGui("TMVA_CNN_ClassificationOutput.root");
}

void classifyWaveform_XY(const char *weightFile, const char *testDir) {
/*
	// Taken from:
	TMVA::Reader *reader = new TMVA::Reader("!Color:!Silent");

	// Create a set of variables and declare them to the reader
	// - the variable names MUST corresponds in name and type to those given in the weight file(s) used
	Float_t x, y;
	reader->AddVariable("x", &x);
	reader->AddVariable("y", &y);

	// Spectator variables declared in the training have to be added to the reader, too
	Float_t spec1, spec2;
	reader->AddSpectator("spec1 := var1*2", &spec1);
	reader->AddSpectator("spec2 := var1*3", &spec2);

	Float_t Category_cat1, Category_cat2, Category_cat3;
	if (Use["Category"]) {
		// Add artificial spectators for distinguishing categories
		reader->AddSpectator("Category_cat1 := var3<=0", &Category_cat1);
		reader->AddSpectator("Category_cat2 := (var3>0)&&(var4<0)", &Category_cat2);
		reader->AddSpectator("Category_cat3 := (var3>0)&&(var4>=0)", &Category_cat3);
	}

	// Book the MVA methods

	TString dir = "dataset/weights/";
	TString prefix = "TMVAClassification";

	// Book method(s)
	for (std::map<std::string, int>::iterator it = Use.begin(); it != Use.end(); it++) {
		if (it->second) {
			TString methodName = TString(it->first) + TString(" method");
			TString weightfile = dir + prefix + TString("_") + TString(it->first) + TString(".weights.xml");
			reader->BookMVA(methodName, weightfile);
		}
	}
*/
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
	("c,cnn",  "Use TMVA Convolutional Neural Network (CNN) for training", cxxopts::value<bool>()->default_value("false"))
	("p,cnnp", "Use TMVA Convolutional PyTorch Model for training", cxxopts::value<bool>()->default_value("false"))
	("d,dnn",  "Use TMVA Deep Neural Network (DNN) for training", cxxopts::value<bool>()->default_value("false"))
	("t,bdt",  "Use TMVA Boosted Decision Trees (BDT) for training", cxxopts::value<bool>()->default_value("true"))
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
	bool cnn = result["cnn"].as<bool>();
	bool cnnp = result["cnnp"].as<bool>();
	bool dnn = result["dnn"].as<bool>();
	bool bdt = result["bdt"].as<bool>();

	if (mode == "prepare") {
		// Step 1. Process CSV waveforms into a ROOT file with trees for learning
		createROOTFileForLearning(backgroundDir.c_str(), signalDir.c_str());
	} else if (mode == "train") {
		// Step 2. Learn ROOT TMVA to categorize the
		std::vector<std::string> unmatched = result.unmatched();
		// trainTMVA(unmatched[0].c_str());
		trainTMVA_CNN(unmatched[0].c_str(), cnn, dnn, bdt, cnnp);
	} else if (mode == "classify") {
		// Step 3. Use TMVA to categorize the
		classifyWaveform_XY(weightFile.c_str(), testDir.c_str());
	}

	// Enter the event loop
	app->Run();

	// Return success
	return 0;
}
