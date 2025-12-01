#include <TGraph.h>
#include <TH1D.h>
#include <TMultiGraph.h>
#include <TList.h>
#include <TFile.h>
#include <TF1.h>
#include <TLine.h>

double maxwellian(Double_t* x, Double_t* pars) {
	static double xV;
	xV = *x;
	return 4 * xV * xV * pars[1] * 
		sqrt(std::pow(pars[2]/2./pars[0], 3)/M_PI) *
		std::pow(M_E, - pars[2] * xV * xV/2./pars[0]);
}

void makeInputFile () {
	TFile* inputFile = new TFile("inputs/input.root", "RECREATE", "inputFile");
	TH1D* speedsHTemplate = new TH1D("speedsHTemplate", "Squared velocities distribution", 30, 0., 30.);
	speedsHTemplate->GetXaxis()->SetTitle("Speed norm (m/s)");
	speedsHTemplate->GetYaxis()->SetTitle("Count");
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
	pGraphs->GetXaxis()->SetTitle("Time (s)");
	pGraphs->GetYaxis()->SetTitle("Pressure (N/m^{2})");
	TGraph* g = new TGraph();
	g->SetLineColor(1);
	g->SetLineWidth(2);
	pGraphs->Add(g);

	TGraph* kBGraph = new TGraph();
	kBGraph->SetName("kBGraph");
	kBGraph->SetTitle("Measured kB");
	kBGraph->GetXaxis()->SetTitle("Time (s)");
	kBGraph->GetYaxis()->SetTitle("measured kB (expected 1) (J/T unit (#alpha))");
	TGraph* mfpGraph = new TGraph();
	mfpGraph->SetName("mfpGraph");
	mfpGraph->SetTitle("Mean free path");

	graphsList->Add(pGraphs);
	graphsList->Add(kBGraph);
	graphsList->Add(mfpGraph);

	TF1* pLineF {new TF1("pLineF", "[0]", 0., 1.)};
	pLineF->SetLineColor(kOrange + 4);
	pLineF->SetLineWidth(2);
	TF1* kBGraphF {new TF1("kBGraphF", "[0]", 0., 1.)};
	kBGraphF->SetLineColor(kPink + 5);
	kBGraphF->SetLineWidth(2);
	TF1* maxwellF {new TF1(
		"maxwellF", "4*pi*[1]*pow([2]/(2*pi*[0]),1.5)*x*x*exp(-[2]*x*x/(2*[0]))",
		speedsHTemplate->GetXaxis()->GetXmin(),
		speedsHTemplate->GetXaxis()->GetXmax()
		)
	};
	maxwellF->SetLineColor(kAzure + 5);
	maxwellF->SetLineWidth(2);

	TF1* mfpGraphF {new TF1("mfpGraphF", "[0] - [1]*pow(e, [2]*x)", 0., 1.)};
	mfpGraphF->SetLineColor(kSpring + 6);
	mfpGraphF->SetLineWidth(2);
	mfpGraph->GetXaxis()->SetTitle("Time (s)");
	mfpGraph->GetYaxis()->SetTitle("Mean free path (m)");
	TH1D* cumulatedSpeedsH {new TH1D(
		"cumulatedSpeedsH",
		"Cumulated speeds norms over whole simulation",
		speedsHTemplate->GetNbinsX(), speedsHTemplate->GetXaxis()->GetXmin(),
		speedsHTemplate->GetXaxis()->GetXmax())
	};
	cumulatedSpeedsH->GetXaxis()->SetTitle("Speed norm (m/s)");
	cumulatedSpeedsH->GetYaxis()->SetTitle("Count");
	std::cout << "Extracted graphsList." << std::endl;
	TLine* meanLine {new TLine(0, 0, 1, 1)};
	meanLine->SetLineColor(kOrange);
	meanLine->SetLineWidth(2);
	TLine* meanSqLine {new TLine(0, 0, 1, 1)};
	meanSqLine->SetLineColor(kRed);
	meanSqLine->SetLineWidth(2);
	TF1* expP {new TF1("expP", "[0]", 0., 1.)};
	TF1* expkB {new TF1("expkB", "[0]", 0., 1.)};
	TF1* expMFP {new TF1("expMFP", "[0]", 0., 1.)};


	speedsHTemplate->Write();
	graphsList->Write();
	pLineF->Write();
	kBGraphF->Write();
	maxwellF->Write();
	mfpGraphF->Write();
	cumulatedSpeedsH->Write();
	meanLine->Write("meanLine");
	meanSqLine->Write("meanSqLine");
	expP->Write();
	expkB->Write();
	expMFP->Write();

	inputFile->Close();
}
