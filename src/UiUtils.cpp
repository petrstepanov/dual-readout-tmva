#include "UiUtils.h"

#include <TGMsgBox.h>
#include <TCanvas.h>
#include <TROOT.h>

void UiUtils::msgBoxInfo(const char* title, const char* text){
  new TGMsgBox(gClient->GetRoot(), NULL, title, text,
			   EMsgBoxIcon::kMBIconAsterisk, EMsgBoxButton::kMBOk);
}

void UiUtils::saveHistogramAsImage(TH1* hist, const char* imageFilePath){
	// Set ROOT to batch
	Bool_t isBatch = gROOT->IsBatch();
	gROOT->SetBatch(kTRUE);

	// Plot and save histograms as PNG
	TCanvas c("waveformTempCanvas");
 	c.SetWindowSize(c.GetWw()*1.5, c.GetWh()*1.5);
	c.Divide(1,2);
	c.cd(1);
	hist->Draw();

	c.cd(2);
	TH1D* trimHist = new TH1D();
	hist->Copy(*trimHist);

	// Trim histogram to 400 ns
	trimHist->GetXaxis()->SetRangeUser(trimHist->GetXaxis()->GetBinLowEdge(1), 400E-9);
	trimHist->Draw();
	c.SaveAs(imageFilePath);

	// Revert ROOT batch mode
	gROOT->SetBatch(isBatch);
}
