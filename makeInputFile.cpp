#include <TGraph.h>
#include <TH1D.h>
#include <TMultiGraph.h>
#include <TList.h>
#include <TFile.h>
void makeInputFile () {
	TFile* inputFile = new TFile("inputs/input.root", "RECREATE", "inputFile");
	TH1D* speedsHTemplate = new TH1D("speedsHTemplate", "Squared velocities distribution", 33, 0., 11.);
	speedsHTemplate->SetFillColor(kViolet - 6);
	speedsHTemplate->SetLineColor(kGreen + 3);
	TList* graphsList = new TList(); 
	graphsList->SetName("graphsList");
	TMultiGraph* pGraphs = new TMultiGraph("pGraphs", "Pressure graphs");
	for(int i {1}; i < 7; ++i) {
		TGraph* g = new TGraph();
		g->SetLineColor(i + 1);
		pGraphs->Add(g);
	}	
	TGraph* g = new TGraph();
	g->SetLineColor(1);
	g->SetLineWidth(2);
	pGraphs->Add(g);

	TGraph* kBGraph = new TGraph();
	kBGraph->SetName("kBGraph");
	kBGraph->SetTitle("Measured kB");
	TGraph* mfpGraph = new TGraph();
	mfpGraph->SetName("mfpGraph");
	mfpGraph->SetTitle("Mean free path");

	graphsList->Add(pGraphs);
	graphsList->Add(kBGraph);
	graphsList->Add(mfpGraph);

	speedsHTemplate->Write();
	graphsList->Write();

	inputFile->Close();
}
