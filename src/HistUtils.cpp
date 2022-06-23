#include "./HistUtils.h"
#include "./FileUtils.h"

#include <TRandom3.h>

#include <iostream>

using namespace HistUtils;

Double_t HistUtils::getMeanY(TH1* hist){
	Double_t sum = 0;
	for (int i=1; i <= hist->GetNbinsX(); i++){
		sum += hist->GetBinContent(i);
	}
	return sum/hist->GetNbinsX();
}

TTree* HistUtils::histsToTreeLin(TList* hists, const char* treeName, const char* treeTitle){
	TTree* tree = new TTree(treeName, treeTitle);

	// Get number of bins in first histogram
	TH1* firstHist = (TH1*)hists->At(0);
	Int_t nBins = firstHist->GetNbinsX();

	// Create vector with waveform values to be written to the Tree
	// https://root.cern/doc/master/TMVA__CNN__Classification_8C.html
	std::vector<float> waveform(nBins);
	std::vector<float> *waveformPtr = &waveform;

	tree->Branch("vars", "std::vector<float>", &waveformPtr);

	for (TObject* obj : * hists){
	  TH1* hist = (TH1*) obj;
	  // Double check that all waveforms have same number of bins
	  if (nBins != hist->GetNbinsX()) {
		  std::cout << "Number of bins in waveforms is inconsistent" << std::endl;
		  exit(1);
	  }
	  for (int bin = 1; bin <= nBins; bin++){
		  waveform[bin-1] = hist->GetBinContent(bin);
	  }
	  tree->Fill();
	}

	// Return tree with one branch containing waveforms written in series
	return tree;
}

TTree* HistUtils::histsToTree(TList* hists, const char* treeName, const char* treeTitle){
	TTree* tree = new TTree(treeName, treeTitle);
	const Int_t nBranches = hists->GetSize();

	Double_t histsRandomValues[nBranches] = {0};
	// std::vector<Double_t> histsRandomValues;
	// for (int i=0; i<nBranches; i++){
	// 	histsRandomValues.push_back(0);
	// }

	// Prepare tree branches
	for (int i=0; i<nBranches; i++){
		// TString branchName = TString::Format("var%d", i+1);
		TString histTitle = ((TNamed*)hists->At(i))->GetTitle();

		TString branchName = "";
//		if (namingPattern == VarNamingPattern::fileName){
//			branchName += FileUtils::getFileNameNoExtensionFromPath(histTitle.Data());
//		}
//		else if (namingPattern == VarNamingPattern::varN){
			branchName += "var";
			branchName += i+1;
//		}
		TString branchSpecifier = branchName;
		branchSpecifier += "/D";
		tree->Branch(branchName.Data(), &(histsRandomValues[i]), branchSpecifier.Data());
	}

	// Populate with 10000 events. See example: http://root.cern/files/tmva_class_example.root
	TRandom3* rng = new TRandom3(0);
	// gRandom = new TRandom3(0);

	for (int i=0; i<10000; i++){
		for (int i=0; i<nBranches; i++){
			histsRandomValues[i] = ((TH1*)hists->At(i))->GetRandom(rng);
		}
		tree->Fill();
	}

	// Return tree with waveform distributions as branches
	return tree;
}

TTree* HistUtils::histsToTreeXY(TList* hists, const char* treeName, const char* treeTitle){
	// Create a Tree with x and y branches
	TTree* tree = new TTree(treeName, treeTitle);
	Double_t x, y;
	tree->Branch("x", &x, "x/D");
	tree->Branch("y", &y, "y/D");

	// Prepare tree branches
	for (TObject* obj : * hists){
		TH1* hist = (TH1*) obj;
		for (int i=1; i<=hist->GetNbinsX(); i++){
			x = hist->GetBinCenter(i);
			y = hist->GetBinContent(i);
			tree->Fill();
		}
	}

	// Return tree with waveform distributions as branches
	return tree;
}

TH1* HistUtils::cropHistogram(TH1* hist, Double_t minX, Double_t maxX){
	Int_t minBin = hist->GetXaxis()->FindBin(minX);
	if (minBin == 0) minBin = 1;

	Int_t maxBin = hist->GetXaxis()->FindBin(maxX);
	if (maxBin == hist->GetNbinsX()+1) maxBin = hist->GetNbinsX();

	return cropHistogram(hist, minBin, maxBin);
}

TH1* HistUtils::cropHistogram(TH1* hist, Int_t minBin, Int_t maxBin){
	TString histName = hist->GetName();
	TUUID uid = TUUID();
	TString uidSuffix = uid.AsString();
	histName += "_";
	histName += uidSuffix(0,4);

	TString histTitle = hist->GetTitle();
	histTitle += " (cropped)";
	Double_t lowEdge = hist->GetXaxis()->GetBinLowEdge(minBin);
	Double_t upEdge = hist->GetXaxis()->GetBinUpEdge(maxBin);
	TH1* cropHist = new TH1D(histName.Data(), histTitle.Data(), maxBin - minBin + 1, lowEdge, upEdge);

	for (Int_t i = 1; i <= cropHist->GetXaxis()->GetNbins(); i++){
		Double_t content = hist->GetBinContent(i+minBin-1);
		cropHist->SetBinContent(i, content);
	}

	return cropHist;
}

void HistUtils::invertHist(TH1* hist){
	for (int i=1; i<=hist->GetXaxis()->GetNbins(); i++){
		if (hist->GetBinContent(i) >= 0){
			hist->SetBinContent(i, 0);
		}
		else {
			Double_t binContent = hist->GetBinContent(i);
			hist->SetBinContent(i, -binContent);
		}
	}
}

Double_t HistUtils::rightEdgeSeconds = 300E-9; // [s]

TH1* HistUtils::prepHistForTMVA(TH1* hist){
	invertHist(hist);
	TH1* croppedHist = HistUtils::cropHistogram(hist, hist->GetXaxis()->GetBinCenter(1), rightEdgeSeconds);
	return croppedHist;
}

TList* HistUtils::prepHistsForTMVA(TList* histsList){
	TList *preppedHistsList = new TList();
	for (TObject *obj : *histsList) {
		TH1 *hist = (TH1*) obj;
		TH1 *prepedHist = HistUtils::prepHistForTMVA(hist);
		// goodCherHists->Remove(obj);
		preppedHistsList->Add(prepedHist);
	}
	return preppedHistsList;
}
