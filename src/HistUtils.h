#ifndef HistUtils_hh
#define HistUtils_hh 1

#include <TH1.h>
#include <TList.h>
#include <TTree.h>

//enum class VarNamingPattern {
//	varN,
//	fileName
//};

namespace HistUtils {
	// Get Mean Y value
	Double_t getMeanY(TH1* hist);

	// Convert histogram into a Tree branch for TMVA
	// TTree* histsToTree(TList* hists, const char* treeName, const char* treeTitle, VarNamingPattern namingPattern = VarNamingPattern::varN);
	TTree* histsToTreeLin(TList* hists, const char* treeName, const char* treeTitle);
	TTree* histsToTree(TList* hists, const char* treeName, const char* treeTitle);
	TTree* histsToTreeXY(TList* hists, const char* treeName, const char* treeTitle);

	// Invert histogram
	void invertHist(TH1* hist);

	// Crop histogram
	TH1* cropHistogram(TH1* hist, Double_t minX, Double_t maxX);
	TH1* cropHistogram(TH1* hist, Int_t minBin, Int_t maxBin);

	extern Double_t rightEdgeSeconds; // [s]

	// Prepare histogram for machine learning analysis (invert and crop)
	TH1* prepHistForTMVA(TH1* hist);
	TList* prepHistsForTMVA(TList* histsList);
}

#endif
