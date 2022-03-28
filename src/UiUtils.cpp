#include "UiUtils.h"

#include <TGMsgBox.h>
#include <TROOT.h>
#include <TText.h>
#include <TEnv.h>

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
 	c.SetWindowSize(c.GetWw(), c.GetWh()*2);
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

void UiUtils::plotBranch(TTree* tree, const char* branchName, const char* title, const char* xTitle, const char* yTitle, int binning){
	// Temporary change default Draw() binning and draw histogram
	// https://root-forum.cern.ch/t/ttree-draw-how-to-change-the-default-number-of-bins/797/2
	Int_t binningOld = gEnv->GetValue("Hist.Binning.1D.x", 0);
	gEnv->SetValue("Hist.Binning.1D.x", binning);
	tree->Draw(branchName);
	gEnv->SetValue("Hist.Binning.1D.x", binningOld);

	// Access histogram plotted from Tree
	// https://root.cern.ch/root/htmldoc/guides/users-guide/Trees.html#accessing-the-histogram-in-batch-mode
 	TH1* htemp = (TH1F*)gPad->GetPrimitive("htemp");
 	htemp->SetTitle(title);
 	htemp->GetXaxis()->SetTitle(xTitle);
 	htemp->GetYaxis()->SetTitle(yTitle);
}

void UiUtils::addCanvasTitle(TCanvas* canvas, const char* title, const char* subtitle){
  // Define heights for title and subtitle (in pixels)
  const Int_t titleHeightPx = 40;
  const Int_t subtitleHeightPx = 25;

  Int_t titlePadHeight = titleHeightPx + (subtitle == 0 ? 0 : subtitleHeightPx);

  Int_t canvasHeightPx = canvas->GetWh();
  Double_t titlePadHeightNDC = (Double_t)titlePadHeight/(Double_t)canvasHeightPx;

  // If canvas has no child pads - create child pad and copy canvas into its child pad
  if (canvas->GetPad(1)==NULL){
    canvas->Divide(1,1);
    TVirtualPad* pad = canvas->cd(1);
    pad->SetPad(0, 0, 1, 1); // Remove subpad margin
    canvas->DrawClonePad(); // Clone on its own subpad?

    // Delete all primitives but keep subpads
    TList* primitives = canvas->GetListOfPrimitives();
    if (primitives){
      TListIter next(primitives);
      TObject *object;
      while ((object=next())) {
        if (!object->InheritsFrom(TPad::Class())){
          // Not deleting primitives but removing them from the list - safer.
          // https://root.cern/root/roottalk/roottalk00/2082.html
          primitives->Remove(object);
        }
      }
    }

    // TODO: do we need this?
    canvas->Modified();
    canvas->Update();
  }

  // Resize child canvases to free space on to for title
  // If Canvas has child pads
  for (Int_t i=1; canvas->GetPad(i)!=NULL; i++){
    TVirtualPad* childPad = canvas->GetPad(i);
    Double_t yLow = childPad->GetAbsYlowNDC();
    Double_t xLow = childPad->GetAbsXlowNDC();
    Double_t height = childPad->GetAbsHNDC();
    Double_t width = childPad->GetAbsWNDC();
    childPad->SetPad(xLow, yLow*(1-titlePadHeightNDC), xLow+width, (yLow+height)*(1-titlePadHeightNDC));
    canvas->Modified();
    canvas->Update();
  }

  // Add extra pad for the title on top
  canvas->cd();

  // Add title text (fixed size in px)
  Double_t titleY = 1. - titleHeightPx/2./canvasHeightPx;
  TText *t = new TText(0.5, titleY, title);
  t->SetNDC(kTRUE);
  t->SetTextFont(63);
  t->SetTextSizePixels(21);
  t->SetTextAlign(ETextAlign::kHAlignCenter + ETextAlign::kVAlignCenter);
  t->Draw();

  // Add subtitle text
  if (subtitle == 0) return;
  Double_t subtitleY = 1. - (titleHeightPx + subtitleHeightPx/2.)/canvasHeightPx;
  TText *s = new TText(0.5, subtitleY, subtitle);
  s->SetNDC(kTRUE);
  s->SetTextFont(63);
  s->SetTextColor(kGray+2);
  s->SetTextSizePixels(16);
  s->SetTextAlign(ETextAlign::kHAlignCenter + ETextAlign::kVAlignCenter);
  s->Draw();

  canvas->Modified();
  canvas->Update();
}
