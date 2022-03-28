#include "HistUtils.h"

Double_t HistUtils::getMeanY(TH1* hist){
	Double_t sum = 0;
	for (int i=1; i <= hist->GetNbinsX(); i++){
		sum += hist->GetBinContent(i);
	}
	return sum/hist->GetNbinsX();
}
