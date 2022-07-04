#include <TSystem.h>
#include <TApplication.h>
#include <TObjString.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TROOT.h>
#include <TF1.h>
#include <TH1.h>
#include <TLeaf.h>
#include <TVectorD.h>
#include <TMacro.h>
#include <TList.h>
#include <TError.h>
#include <TROOT.h>
#include <TObjString.h>
#include <TString.h>

#include <TMVA/Types.h>
#include <TMVA/DataLoader.h>
#include <TMVA/Factory.h>
#include <TMVA/TMVAGui.h>
#include <TMVA/Reader.h>
#include <TMVA/Tools.h>
#include <TMVA/PyMethodBase.h>
// #include "tinyfiledialogs.h"
#include "./FileUtils.h"
#include "./HistUtils.h"
#include "./StringUtils.h"
#include "./UiUtils.h"

#include <iostream>
#include <iomanip>
#include <set>
#include <vector>
#include "cxxopts.hpp"

#define N_BINS 10000
#define VOLTAGE_THRESHOLD -0.03
#define MIN_PEAK_POS -1E-8
#define MAX_PEAK_POS 2E-8

// Function imports all Tektronix waveforms from a directory and filters out the "bad" (noise) waveforms.
// Returns TList of "good" TH1* histograms

TList* getGoodHistogramsList(const char *dirPath, bool saveWaveformImages = kFALSE) {
    // Obtain Cerenkov waveform paths from a directory
    TList *waveformFilenames = FileUtils::getFilePathsInDirectory(dirPath, ".csv");

    // Compose list of histograms
    TList *hists = new TList();
//    Int_t counter = 0;
//    Int_t nHists = ;
    for (TObject *obj : *waveformFilenames) {
        StringUtils::writeProgress("Converting waveforms to ROOT histograms", waveformFilenames->GetSize());
//        std::cout << "\r: " progress*100 << "% (" << counter << "/" << nHists << ") ";
        //        std::cout << "\rConverting waveforms to ROOT histograms: " progress*100 << "% (" << counter << "/" << nHists << ") ";
        TObjString *waveformCsvPath = (TObjString*) obj;

        // Import CSV waveform into histogram
        TH1 *hist = FileUtils::tekWaveformToHist(waveformCsvPath->String().Data());
        if (!hist)
            continue;

        // Add waveform to list of histograms that were able to read
        hists->Add(hist);

        // Optionally: save waveforms as images
        TString waveformImgPath("");
        waveformImgPath = StringUtils::stripExtension(waveformCsvPath->String().Data());
        waveformImgPath += ".png";
        if (saveWaveformImages){
            UiUtils::saveHistogramAsImage(hist, waveformImgPath.Data());
        }
    }
    std::cout << ", done." << std::endl; // all done

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

    {
        TVirtualPad* pad = canvas->cd(1);
        UiUtils::plotBranch(waveformsTree, "minV", "Waveform Minimum Amplitude", "Minimum Amplitude, V", "Counts", 100);
        pad->SetLogy(kTRUE);
    }
    {
        TVirtualPad* pad = canvas->cd(2);
        UiUtils::plotBranch(waveformsTree, "peakPos", "Waveform Peak Positions", "Peak Position, s", "Counts", 100);
        pad->SetLogy(kTRUE);
    }

    TString wfPngFilePath = gSystem->ConcatFileName(dirPath, "waveforms-parameters.png");
    canvas->SaveAs(wfPngFilePath.Data());

    // Save waveform properties
    TString wfRootFilePath = gSystem->ConcatFileName(dirPath, "waveforms-parameters.root");
    TFile *f = new TFile(wfRootFilePath.Data(), "RECREATE");
    waveformsTree->Write();
    canvas->Write();
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
            StringUtils::writeProgress("Identifying \"noise\" waveforms", hists->GetSize());
        }
    }
    Int_t goodPercent = hists->GetSize()*100/waveformFilenames->GetSize();
    Info("Identified %d \"good\" waveforms (%d%%), %d noise waveforms (%d%%).", hists->GetSize(), goodPercent, waveformFilenames->GetSize() - hists->GetSize(), 100-goodPercent);
    // Debug: save good waveforms under ../*-good/ folder
    //if (saveWaveformImages) {
    //    for (TObject *obj : *hists) {
    //        TH1 *hist = (TH1*) obj;
    //        TString goodWaveformPath = dirPath; // gSystem->GetDirName(hist->GetTitle());
    //        goodWaveformPath += "-good";
    //        // Check output directory for "good" waveforms exist
    //        if (gSystem->AccessPathName(goodWaveformPath.Data())) {
    //            gSystem->mkdir(goodWaveformPath.Data());
    //        }
    //        TString goodWaveformName = FileUtils::getFileNameNoExtensionFromPath(hist->GetTitle());
    //        goodWaveformName += ".png";
    //        TString goodPathName = gSystem->ConcatFileName(goodWaveformPath.Data(), goodWaveformName.Data());
    //
    //        UiUtils::saveHistogramAsImage(hist, goodPathName.Data());
    //    }
    //}

    return hists;
}

enum class MLFileType {
    Linear,    // https://root.cern/doc/master/TMVA__CNN__Classification_8C.html
    PonitsXY,  // https://root.cern/doc/master/TMVAMinimalClassification_8C.html
    PDF         // https://root.cern/doc/master/TMVAClassification_8C.html
};

void createROOTFileForLearning(const char *cherPath, const char *cherScintPath, Bool_t saveWaveformImages = kFALSE, MLFileType rootFileType = MLFileType::Linear) {
    // TinyFileDialogs approach
    // tinyfd_forceConsole = 0; /* default is 0 */
    // tinyfd_assumeGraphicDisplay = 0; /* default is 0 */
    // TString cherWaveformsDirPath = tinyfd_selectFolderDialog("Open Cerenkov Waveform Directory", NULL);

    // Check if Chernkov and Scintillation directory paths were passed via command line paramters:

    // Specify directory for Cerenkov waveforms
    TString cherWaveformsDirPath = cherPath;
    if (cherWaveformsDirPath.Length() == 0) {
        Error("createROOTFileForLearning", "Directory with background spectra (Cube6, Cerenkov) not provided");
        exit(1);
    }

    // Obtain "good" Cerenkov waveforms for TMVA
    TList *goodCherHists = getGoodHistogramsList(cherWaveformsDirPath.Data(), saveWaveformImages);
    TList *goodCherHistsPrepared = HistUtils::prepHistsForTMVA(goodCherHists);
    Info("createROOTFileForLearning", "\"Good\" background histograms processed (invert, crop)");

    // Specify directory for Cerenkov AND Scintillation waveforms
    TString cherScintWaveformsDirPath = cherScintPath;
    if (cherScintWaveformsDirPath.Length() == 0) {
        Error("createROOTFileForLearning", "Directory with background spectra (Cube6, Cerenkov) not provided");
        exit(1);
    }

    // Obtain "good" Cerenkov and Scintillation waveforms for TMVA
    TList *goodCherScintHists = getGoodHistogramsList(cherScintWaveformsDirPath.Data(), saveWaveformImages);
    TList *goodCherScintHistsPrepared = HistUtils::prepHistsForTMVA(goodCherScintHists);
    Info("createROOTFileForLearning", "\"Good\" signal histograms processed (invert, crop)");

    // Prepare trees
    TTree *treeBackground;
    TTree *treeSignal;
    if (rootFileType == MLFileType::Linear) {
        treeBackground = HistUtils::histsToTreeLin(goodCherHistsPrepared, "treeB", "Background Tree - Cerenkov");
        Info("createROOTFileForLearning", "Background Tree Created");
        treeSignal = HistUtils::histsToTreeLin(goodCherScintHistsPrepared, "treeS", "Signal Tree - Cerenkov and scintillation");
        Info("createROOTFileForLearning", "Signal Tree Created");
    }
//	else if (rootFileType == MLFileType::PDF){
//		treeBackground = HistUtils::histsToTree(goodCherHistsPrepared, "treeB", "Background Tree - Cerenkov");
//		treeSignal = HistUtils::histsToTree(goodCherScintHistsPrepared, "treeS", "Signal Tree - Cerenkov and scintillation");
//	} else {
//		treeBackground = HistUtils::histsToTreeXY(goodCherHistsPrepared, "treeS", "Background Tree - Cerenkov");
//		treeSignal = HistUtils::histsToTreeXY(goodCherScintHistsPrepared, "treeB", "Signal Tree - Cerenkov and scintillation");
//	}

    // Write trees to file for training
    TString workingDirectory = gSystem->GetWorkingDirectory().c_str();
    TString tmvaFileName = "tmva-input.root";
    TString tmvaFileNamePath = gSystem->ConcatFileName(workingDirectory.Data(), tmvaFileName.Data());
    TFile *tmvaFile = new TFile(tmvaFileNamePath.Data(), "RECREATE");
    treeBackground->Write();
    treeSignal->Write();

    // Write histograms number of bins - need for TMVA reading later if waveforms
    // are written in series

    Int_t backgroundBins = ((TH1*) goodCherHistsPrepared->At(0))->GetNbinsX();
    Int_t signalBins = ((TH1*) goodCherScintHistsPrepared->At(0))->GetNbinsX();
    if (backgroundBins != signalBins) {
        std::cout << "Signal bins not equal to background bins." << std::endl;
        exit(1);
    }
    TVectorD bins(1);
    bins[0] = backgroundBins;
    bins.Write("bins");

    tmvaFile->Close();
    Info("createROOTFileForLearning", "File \"%s\" created", tmvaFileNamePath.Data());
}

/*
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
 */

void trainTMVA_CNN(const char *trainingFileURI, std::set<TMVA::Types::EMVA> tmvaMethodOnly) {
    // Petr Stepanov: refer to: https://root.cern/doc/master/TMVA__CNN__Classification_8C.html

    int num_threads = 0;    // use default threads
    TMVA::Tools::Instance();
    // Enable MT running
    if (num_threads >= 0) {
        ROOT::EnableImplicitMT(num_threads);
        if (num_threads > 0)
            gSystem->Setenv("OMP_NUM_THREADS", TString::Format("%d", num_threads));
    } else {
        gSystem->Setenv("OMP_NUM_THREADS", "1");
    }
    std::cout << "Running with nthreads  = " << ROOT::GetThreadPoolSize() << std::endl;

    // Open output file
    TFile *outputFile = nullptr;
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

    TMVA::Factory factory("TMVA_CNN_Classification", outputFile, "!V:ROC:!Silent:Color:AnalysisType=Classification:Transformations=None:!Correlations");

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
        Error("trainTMVA_CNN", "Input file %s not found", trainingFileURI);
        exit(1);
    }

    // --- Register the training and test trees

    TTree *signalTree = (TTree*) inputFile->Get("treeS");
    TTree *backgroundTree = (TTree*) inputFile->Get("treeB");

    int nEventsSig = signalTree->GetEntries();
    int nEventsBkg = backgroundTree->GetEntries();

    // global event weights per tree (see below for setting event-wise weights)
    Double_t signalWeight = 1.0;
    Double_t backgroundWeight = 1.0;

    // You can add an arbitrary number of signal or background trees
    loader->AddSignalTree(signalTree, signalWeight);
    loader->AddBackgroundTree(backgroundTree, backgroundWeight);

    // Read histogram size from file
    TVectorD *bins = inputFile->Get<TVectorD>("bins");
    Double_t b = (*bins)[0];

    // add event variables (image)
    // use new method (from ROOT 6.20 to add a variable array for all image data)
    // issue: https://github.com/root-project/root/pull/10780
    // loader->AddVariablesArray("vars", (Int_t)b);

    // Petr Stepanov: need to revert to old method becauyse of the issue above
    for (int i = 0; i < b; i++) {
        TString expression = "var";
        expression += i;
        loader->AddVariable(expression);
    }

    // TODO: try old method with vars[0] ?

    // Set individual event weights (the variables must exist in the original TTree)
    //    for signal    : factory->SetSignalWeightExpression    ("weight1*weight2");
    //    for background: factory->SetBackgroundWeightExpression("weight1*weight2");
    // loader->SetBackgroundWeightExpression( "weight" );

    // Apply additional cuts on the signal and background samples (can be different)
    TCut mycuts = "";    // for example: TCut mycuts = "abs(var1)<0.5 && abs(var2-0.5)<1";
    TCut mycutb = "";    // for example: TCut mycutb = "abs(var1)<0.5";

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
    TString prepareOptions = TString::Format("nTrain_Signal=%d:nTrain_Background=%d:SplitMode=Random:SplitSeed=100:NormMode=NumEvents:!V:!CalcCorrelations",
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
    if (tmvaMethodOnly.size() == 0 || tmvaMethodOnly.count(TMVA::Types::kBDT)) {
        TString methodTitle = TMVA::Types::Instance().GetMethodName(TMVA::Types::kBDT);
        factory.BookMethod(loader, TMVA::Types::kBDT, methodTitle, "!V:NTrees=400:MinNodeSize=2.5%:MaxDepth=2:BoostType=AdaBoost:AdaBoostBeta=0.5:"
                "UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20");
    }
    /**

     #### Booking Deep Neural Network

     Here we book the DNN of TMVA. See the example TMVA_Higgs_Classification.C for a detailed description of the
     options

     **/

    // Check if ROOT is built with TMVA CNN CPU or GPU support
#ifndef R__HAS_TMVACPU
#ifndef R__HAS_TMVAGPU
    Warning("trainTMVA_CNN", "TMVA is not build with GPU or CPU multi-thread support. Cannot use TMVA Deep Learning for Convolutional Neural Network (CNN)");
    tmvaMethodOnly.erase(TMVA::Types::kDNN);
#endif
#endif

    if (tmvaMethodOnly.size() == 0 || tmvaMethodOnly.count(TMVA::Types::kDNN)) {

        TString layoutString("Layout=DENSE|100|RELU,BNORM,DENSE|100|RELU,BNORM,DENSE|100|RELU,BNORM,DENSE|100|RELU,DENSE|1|LINEAR");

        // Training strategies
        // one can catenate several training strings with different parameters (e.g. learning rates or regularizations
        // parameters) The training string must be concatenates with the `|` delimiter
        TString trainingString1("LearningRate=1e-3,Momentum=0.9,Repetitions=1,"
                "ConvergenceSteps=5,BatchSize=100,TestRepetitions=1,"
                "MaxEpochs=20,WeightDecay=1e-4,Regularization=None,"
                "Optimizer=ADAM,DropConfig=0.0+0.0+0.0+0.");

        TString trainingStrategyString("TrainingStrategy=");
        trainingStrategyString += trainingString1;        // + "|" + trainingString2 + ....

        // Build now the full DNN Option string

        TString dnnOptions("!H:V:ErrorStrategy=CROSSENTROPY:VarTransform=None:"
                "WeightInitialization=XAVIER");
        dnnOptions.Append(":");
        dnnOptions.Append(layoutString);
        dnnOptions.Append(":");
        dnnOptions.Append(trainingStrategyString);

        TString methodTitle = TMVA::Types::Instance().GetMethodName(TMVA::Types::kDNN);
        // use GPU if available
#ifdef R__HAS_TMVAGPU
        dnnOptions += ":Architecture=GPU";
        // dnnMethodName += "_GPU";
#elif defined(R__HAS_TMVACPU)
        dnnOptions += ":Architecture=CPU";
        // dnnMethodName += "_CPU";
#endif

        factory.BookMethod(loader, TMVA::Types::kDL, methodTitle, dnnOptions);
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

    /*
     if (Use.count(TMVA::Types::kPyTorch)) {

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
     */

    // Train Methods
    factory.TrainAllMethods();

    // Test and Evaluate Methods
    factory.TestAllMethods();
    factory.EvaluateAllMethods();

    // Plot ROC Curve
    if (!gROOT->IsBatch()) {
        TCanvas *canvas = factory.GetROCCurve(loader);
        canvas->Draw();
    }

    // Close output file
    outputFile->Close();

    // Launch the GUI for the root macros
    if (!gROOT->IsBatch()) {
        TMVA::TMVAGui("TMVA_ClassificationOutput.root");
    }

    Info("trainTMVA_CNN", "Training completed");
}

std::map<std::string, float> classifyWaveform_Linear(const char *weightDirPath, const char *testDirPath) {
    // Read "good" waveforms to be tested
    TList *goodTestHists = getGoodHistogramsList(testDirPath);
    TList *goodTestHistsPrepared = HistUtils::prepHistsForTMVA(goodTestHists);  // TODO: Crop and invert histograms (required for the hist->GetRandom() to work)
    if (goodTestHistsPrepared->GetSize() < 1) {

        std::map<std::string, float> map { };
        return map;
    }
    // Remember number of bins in first good histogram
    Int_t nBins = ((TH1*) (goodTestHistsPrepared->At(0)))->GetNbinsX();

    // Create a set of variables and declare them to the reader
    // - the variable names MUST corresponds in name and type to those given in the weight file(s) used
    std::vector<float> fValues(nBins);
    // std::vector<float> *fValuesPtr = new std::vector<float>(nBins);

    // Instantiate the reader
    // Taken from: https://root.cern/doc/master/TMVAClassificationApplication_8C.html
    TMVA::Reader *reader = new TMVA::Reader("!Color:!Silent");

    // Hint. We represent each spectrum bin for the reader as separate variable vars[0], vars[1],...
    // Check the input "...weight.xml" files. Variables there are named like above
    // for (std::size_t i = 0; i < nBins; i++) {
    // Petr Stepanov: this taken from RReader.hxx, ROOT sources
    // TString varExpression = TString::Format("vars[%d]", i);
    // reader->AddVariable(varExpression, &fValues[i]);

    // Not working...

    //}
    // Petr Stepanov: &fValues is of wrong type. We need to pass float* which is &fvalues[0]
    //                issue with the AddVariablesArray() method: https://github.com/root-project/root/pull/10780
    // reader->DataInfo().AddVariablesArray("vars", nBins, "", "", 0, 0, 'F', kFALSE, &fValues[0]); // TODO: one of parameters is normalized. Use it?
    for (int i = 0; i < nBins; i++) {
        TString expression = "var";
        expression += i;
        reader->AddVariable(expression, &fValues[i]);
    }

    // Book the MVA methods
    // TString dir = weightDirPath;
    // TString prefix = "TMVA_CNN_Classification_";

    // Book method(s)
    // Search
    // std::map<std::string, int> Use {{"BDT", 1}, {"DNN_CPU", 1}};
    // std::set<TMVA::Types::EMVA> foundMethods {};

    // Loop through all weight files in given weight directory
    TList *fileNames = FileUtils::getFilePathsInDirectory(weightDirPath, ".xml");
    std::list<TH1F*> histograms { };
    for (TObject *obj : *fileNames) {
        TObjString *fileNameObjString = (TObjString*) obj;
        if (fileNameObjString) {
            TString filePath = fileNameObjString->GetString();
            TString methodType = FileUtils::getFileNameNoExtensionFromPath(filePath);

            // Book TMVA method in the reader
            reader->BookMVA(methodType, filePath);

            // Prepare output histogram
            TH1F *hist = new TH1F(methodType, methodType, 100, -1.0, 1.0);
            histograms.push_back(hist);
        }
    }

//	for (std::set<std::string, Bool_t>::iterator it = Use.begin(); it != Use.end(); it++) {
//		if (it->second) {
//			TString methodName = TString(it->first) + TString(" method");
//			TString weightfile = dir + prefix + TString(it->first) + TString(".weights.xml");
//
//		}
//	}

    // Book Output histograms
    // https://root.cern/doc/master/TMVAClassificationApplication_8C.html
//	for (TObject* obj : *fileNames){
//		TObjString* fileNameObjString = (TObjString*) obj;
//		if (fileNameObjString){
//			TString filePath = fileNameObjString->GetString();
//			TString methodType = FileUtils::getFileNameNoExtensionFromPath(filePath);
//			reader->BookMVA(methodType, filePath);
//		}
//	}
//
//	TH1F *histBdtF(0);
//	TH1F *histDnnCpu(0);
//	histBdtF = new TH1F("MVA_BDT", "MVA_BDT", nbin, -1.0, 1.0 );
//	histDnnCpu = new TH1F("MVA_DNN_CPU", "MVA_DNN_CPU", nbin, -0.1, 1.1);

    // Prepare input tree (this must be replaced by your data source)
    // in this example, there is a toy tree with signal and one with background events
    // we'll later on use only the "signal" events for the test in this example.

    // Prepare trees
    TTree *treeTest;
    // if (rootFileType == MLFileType::Linear){
    treeTest = HistUtils::histsToTreeLin(goodTestHistsPrepared, "tree", "Tree for Classification");
    Info("classifyWaveform_Linear", "Test Tree Created");
    // }

    // Read test tree
    // std::vector<float> fV(nBins);
    // std::vector<float>* fVPtr = &fValues;

    // Petr Stepanov: address of the "vars" should be address of a pointer
    // How to read write vector to Tree: https://gist.github.com/jiafulow/8877081e032158471578
    // Petr Stepanov: current GitHub issue prevents passing a vector to the Reader
    // std::vector<float> *fValuesPtr = &fValues;
    // treeTest->SetBranchAddress("vars", &fValuesPtr);

    for (int i = 0; i < nBins; i++) {
        TString expr = "var";
        expr += i;
        treeTest->SetBranchAddress(expr.Data(), &fValues[i]);
    }

    Long64_t nEntries = treeTest->GetEntries();
    std::map<std::string, float> map;
    for (Long64_t ievt = 0; ievt < nEntries; ievt++) {
        treeTest->GetEntry(ievt);

//		TString hName = TString::Format("h%d", ievt);
//		TH1F* h = new TH1F(hName.Data(), hName.Data(), nBins, 0, nBins);
//		int i = 1;
//		for (float val : *fValuesPtr){
//			h->SetBinContent(i, val);
//			i++;
//		}
//		new TCanvas();
//		h->Draw();

        // Get spectrunm name
        TString spectrumName = "";
        TObject *obj = goodTestHistsPrepared->At(ievt);
        TH1 *spectrumHist = (TH1*) obj;
        if (spectrumHist)
            spectrumName = spectrumHist->GetName();
        std::cout << "Entry: " << ievt << std::endl;
        std::cout << "Filename: " << spectrumName << std::endl;
        for (TH1F *hist : histograms) {
            // Get response from MVA - passing method name (here - hist name)
            Double_t val = reader->EvaluateMVA(hist->GetName());
            // Fill histogram with responce
            hist->Fill(val);
            map[hist->GetName()] = val;
            // Output to screen
            std::cout << "MVA response for \"" << hist->GetName() << "\": " << val << std::endl;
        }
        std::cout << std::endl;

    }

//	histBdtF->Fill( reader->EvaluateMVA("BDT method" ));
//	histDnnCpu->Fill(reader->EvaluateMVA("DNN_CPU method"));

    // Write histograms
    TFile *target = new TFile("TMVApp.root", "RECREATE");
    for (TH1F *hist : histograms) {
        hist->Write();
        TCanvas *c = new TCanvas();
        hist->Draw();
    }
//	histBdtF->Write();
//	histDnnCpu->Write();

    target->Close();
    std::cout << "--- Created root file: \"TMVApp.root\" containing the MVA output histograms" << std::endl;

    delete reader;

    Info("classifyWaveform_Linear", "Classification completed");

    return map;
}

int main(int argc, char *argv[]) {
    // Instantiate TApplication
    TApplication *app = new TApplication("energyResolution", &argc, argv);

    // Initialize command-line parameters options
    // https://github.com/jarro2783/cxxopts
    cxxopts::Options options("Dual Readout TMVA", "ROOT Machine Learning (ML) approach to categorize waveforms upon their shape.");

    // Add command-line options
    options.allow_unrecognised_options().add_options()    //
    ("mode", "Program mode ('prepare', 'train', 'tmva-gui', 'classify')", cxxopts::value<std::string>())    //
    ("save-waveform-img", "Save .png waveforms images('prepare')", cxxopts::value<bool>()->default_value("false"))    //
    ("background", "Directory path for background .csv waveforms ('prepare')", cxxopts::value<std::string>())    //
    ("signal", "Directory path for signal .csv waveforms ('prepare')", cxxopts::value<std::string>())    //
    ("weight", "Machine learning weight file path ('classify')", cxxopts::value<std::string>())    //
    ("test", "Directory path with .csv waveforms for classifying ('test')", cxxopts::value<std::string>())    //
    ("bdt", "Use only Boosted Decision Trees (BDT) for training", cxxopts::value<bool>()->default_value("false"))    //
    ("dnn", "Use only Deep Neural Network (DNN) for training", cxxopts::value<bool>()->default_value("false"))("help", "Print usage");    //

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
    std::string weightDirPath;
    if (result.count("weight"))
        weightDirPath = result["weight"].as<std::string>();
    std::string testDirPath;
    if (result.count("test"))
        testDirPath = result["test"].as<std::string>();

    Bool_t saveWaveformImages = kFALSE;
    if (result["save-waveform-img"].as<bool>()) {
        saveWaveformImages = kTRUE;
    }
    // By default, training is performed for all possible methods (currently implemented kBDT and kDNN)
    // However, user can specify only certain training methods in particular via command-line parameters
    std::set<TMVA::Types::EMVA> tmvaMethodsOnly { };
    if (result["bdt"].as<bool>()) {
        tmvaMethodsOnly.insert(TMVA::Types::EMVA::kBDT);
    }
    if (result["dnn"].as<bool>()) {
        tmvaMethodsOnly.insert(TMVA::Types::EMVA::kDNN);
    }

    if (mode == "prepare") {
        // Step 1. Process CSV waveforms into a ROOT file with trees for learning

        // Check if backgroud directory passed via command line
        if (backgroundDir.size() == 0) {
            // Use GUI picker if 'testDirPath' command line parameter not passed
            UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify directory with background spectra (Cube6, Cerenkov)");
            TString dir = UiUtils::getDirectoryPath();
            backgroundDir = dir.Data();
        }
        if (signalDir.size() == 0) {
            // Use GUI picker if 'testDirPath' command line parameter not passed
            UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify directory with signal spectra (Cube9, Cerenkov+scintillation)");
            TString dir = UiUtils::getDirectoryPath();
            signalDir = dir.Data();
        }
        createROOTFileForLearning(backgroundDir.c_str(), signalDir.c_str(), saveWaveformImages);
    } else if (mode == "train") {
        // Step 2. Learn ROOT TMVA to categorize the
        std::vector<std::string> unmatched = result.unmatched();
        if (unmatched.size() == 0) {
            // Use GUI picker if 'testDirPath' command line parameter not passed
            UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify training file path");
            TString filePath = UiUtils::getFilePath();
            unmatched.push_back(filePath.Data());
        }
        // trainTMVA(unmatched[0].c_str());
        trainTMVA_CNN(unmatched[0].c_str(), tmvaMethodsOnly);
    } else if (mode == "tmva-gui") {
        // View training output
        std::vector<std::string> unmatched = result.unmatched();
        if (unmatched.size() == 0) {
            // Use GUI picker if 'testDirPath' command line parameter not passed
            UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify ROOT training result file");
            TString filePath = UiUtils::getFilePath();
            unmatched.push_back(filePath.Data());
        }
        TMVA::TMVAGui(unmatched[0].c_str());
    } else if (mode == "classify") {
        // Step 3. Use TMVA to categorize the
        if (weightDirPath.size() == 0) {
            // Use GUI picker if 'testDirPath' command line parameter not passed
            UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify directory with weight files");
            TString dir = UiUtils::getDirectoryPath();
            weightDirPath = dir.Data();
        }
        if (testDirPath.size() == 0) {
            // Use GUI picker if 'testDirPath' command line parameter not passed
            UiUtils::msgBoxInfo("Dual Readout TMVA", "Specify directory with test spectra");
            TString dir = UiUtils::getDirectoryPath();
            testDirPath = dir.Data();
        }
        classifyWaveform_Linear(weightDirPath.c_str(), testDirPath.c_str());
    }

    // Enter the event loop
    app->Run();

    // Return success
    gSystem->Exit(0);
}
