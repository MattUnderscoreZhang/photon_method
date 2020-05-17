#ifndef COMMON_SETTINGS
#define COMMON_SETTINGS

#include <cstdlib>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <boost/algorithm/string.hpp>
#include <string>
#include <iomanip> 
#include <algorithm>
#include <math.h>

#include "TFile.h"
#include "TF1.h"
#include "TH1.h"
#include "TTree.h"
#include "TString.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TStopwatch.h"
#include "TChain.h"
#include "TBranch.h"
#include "TLorentzVector.h"
#include "TGraphAsymmErrors.h"
#include "TSpectrum.h"
#include "TVirtualFFT.h"
#include "TGraphErrors.h"
#include "TObject.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TDirectory.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TMath.h"
#include "TPad.h"
#include "TCut.h"
#include "TProfile.h"
#include "THStack.h"
#include "TLatex.h"
#include "TGraphErrors.h"
#include "TStyle.h"
#include "TLine.h"
#include "TRandom.h"
#include <ROOT/RDataFrame.hxx>
//#include <ROOT/RDF/RInterface.hxx>
#include "TInterpreter.h"

//--- printing colors
#define RST  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define PRED(x) KRED x RST
#define PGRN(x) KGRN x RST
#define PYEL(x) KYEL x RST
#define PBLU(x) KBLU x RST
#define PMAG(x) KMAG x RST
#define PCYN(x) KCYN x RST
#define PWHT(x) KWHT x RST

#define BOLD(x) "\x1B[1m" x RST
#define UNDL(x) "\x1B[4m" x RST

namespace cuts {

    double leading_lep_pt_cut = 25.; // also used for smearing
    double second_lep_pt_cut = 25.; // also used for smearing

    TCut bkg_weight("totalWeight");
    TCut photon_weight("totalWeight");
    TCut photon_weight_rw("totalWeight*reweight_Ptll");

    TCut lep0 = "Lep_base==0";
    TCut lep2 = "nLep_signal==2 && nLep_base==2 && lepPt[0]>25 && lepPt[1]>25 && lepIsoFCTight[0] && lepIsoFCTight[1] && trigMatch_2LTrigOR";
    TCut jet1 = "nJet30>=1";
    TCut jet2 = "nJet30>=2";
    TCut jet4 = "nJet30>=4";
    TCut jet6 = "nJet30>=6";
    TCut bjet0 = "bjet_n==0";
    TCut is_OS = "is_OS";
    TCut is_SF = "abs(lepFlavor[0])==abs(lepFlavor[1])";
    TCut mll_12 = "mll>12";
    TCut mll_Zwindow = "mll>81 && mll<101";
    TCut mll_lt81 = "mll<81";
    TCut PPtll_25 = "PhotonPt>25";
    TCut gPtll_25 = "gamma_pt>25";
    TCut Ptll_25 = "Ptll>25";
    TCut Ptll_40 = "Ptll>40";
    TCut Ptll_lt100 = "Ptll<100";
    TCut Ptll_lt500 = "Ptll<500";
    TCut Ptll_lt800 = "Ptll<800";
    TCut MT2_75 = "mt2leplsp_0>75";
    TCut MT2_90 = "mt2leplsp_0>90";
    TCut MT2_100 = "mt2leplsp_0>100";
    TCut METSig_10 = "met_Sign>10";
    TCut HT_250 = "Ht30>250";
    TCut HT_500 = "Ht30>500";
    TCut HT_800 = "Ht30>800";
    TCut minDPhi2JetsMet_0p4 = "minDPhi2JetsMet>0.4";
    TCut minDPhi2JetsMet_anti0p4 = "minDPhi2JetsMet<0.4";
    TCut not_diboson_2L = "((lepFlavor[0] == lepFlavor[1] && DatasetNumber != 363356 && DatasetNumber != 363358) || (lepFlavor[0] != lepFlavor[1]))";

    TCut strong_preselection_noDPhi = lep2 + is_OS + jet2 + mll_12 + Ptll_40;
    TCut strong_preselection = strong_preselection_noDPhi + minDPhi2JetsMet_0p4;
    TCut strong_VRDPhi_preselection = strong_preselection + mll_Zwindow + minDPhi2JetsMet_anti0p4 + HT_250 + MT2_75;

    TCut CR_MET("met_Et<100");

    std::unordered_map<std::string, TCut> selections = {
        {"mm", "channel==0"},
        {"ee", "channel==1"},
        {"em", "channel==2"},
        {"me", "channel==3"},
        {"SF", "channel==0 || channel==1"},
        {"DF", "channel==2 || channel==3"},

        {"bkg_baseline", lep2 + jet1 + is_OS + Ptll_25 + not_diboson_2L},
        {"photon_baseline_ntuples", lep0 + jet1 + PPtll_25},
        {"photon_baseline", jet1 + gPtll_25},
        {"reweight", lep2 + jet2 + is_SF},
        //{"bkg_baseline", lep2 + is_OS + jet2 + Ptll_25 + not_diboson_2L},
        //{"photon_baseline_ntuples", lep0 + jet2 + PPtll_25},
        //{"photon_baseline", jet2 + gPtll_25},
        //{"reweight", lep2 + is_OS + is_SF + jet2 + PPtll_25},

        {"VRZjets", lep2 + jet2 + bjet0 + Ptll_40 + mll_Zwindow + is_SF},
        {"VRZjets_noZwindow", lep2 + jet2 + bjet0 + Ptll_40 + is_SF},
        {"VRZjets_noZwindow_MET100_200", lep2 + jet2 + bjet0 + Ptll_40 + is_SF},
        {"VRZjets_noZwindow_MET200_300", lep2 + jet2 + bjet0 + Ptll_40 + is_SF},
        {"VRZjets_noZwindow_MET300_400", lep2 + jet2 + bjet0 + Ptll_40 + is_SF},
        {"VRZjets_noZwindow_METgt400", lep2 + jet2 + bjet0 + Ptll_40 + is_SF},
        {"VRZjets_noZwindow_noBveto", lep2 + jet2 + Ptll_40 + is_SF},
        {"strong_preselection", strong_preselection},

        {"SRC", strong_preselection + MT2_90 + METSig_10 + Ptll_lt100},
        {"SRLow", strong_preselection + jet2 + HT_250 + MT2_100 + Ptll_lt500},
        {"SRMed", strong_preselection + jet2 + HT_500 + MT2_75 + Ptll_lt800},
        {"SRHigh", strong_preselection + jet2 + HT_800 + MT2_75},
        {"SRLowZ", strong_preselection + jet4 + HT_250 + MT2_100 + Ptll_lt500 + mll_Zwindow},
        {"SRMedZ", strong_preselection + jet4 + HT_500 + MT2_75 + Ptll_lt800 + mll_Zwindow},
        {"SRHighZ", strong_preselection + jet4 + HT_800 + MT2_75 + mll_Zwindow},

        {"VRC", strong_preselection + MT2_90 + METSig_10 + Ptll_lt100 + mll_lt81},
        {"VRLow", strong_preselection + jet2 + HT_250 + MT2_100 + Ptll_lt500},
        {"VRMed", strong_preselection + jet2 + HT_500 + MT2_75 + Ptll_lt800},
        {"VRHigh", strong_preselection + jet2 + HT_800 + MT2_75},
        {"VRLowZ", strong_preselection + jet4 + HT_250 + MT2_100 + Ptll_lt500 + mll_Zwindow},
        {"VRMedZ", strong_preselection + jet4 + HT_500 + MT2_75 + Ptll_lt800 + mll_Zwindow},
        {"VRHighZ", strong_preselection + jet4 + HT_800 + MT2_75 + mll_Zwindow},

        {"VRDPhi", strong_VRDPhi_preselection},
        {"VRDPhiLow2", strong_VRDPhi_preselection + jet2 + HT_250 + MT2_100 + Ptll_lt500},
        {"VRDPhiMed2", strong_VRDPhi_preselection + jet2 + HT_500 + MT2_75 + Ptll_lt800},
        {"VRDPhiHigh2", strong_VRDPhi_preselection + jet2 + HT_800 + MT2_75},
        {"VRDPhiLow6", strong_VRDPhi_preselection + jet6 + HT_250 + MT2_100 + Ptll_lt500},
        {"VRDPhiMed6", strong_VRDPhi_preselection + jet6 + HT_500 + MT2_75 + Ptll_lt800},
        {"VRDPhiHigh6", strong_VRDPhi_preselection + jet6 + HT_800 + MT2_75},
    };

    std::unordered_map<std::string, TCut> plot_region_met_portions = {
        {"VRZjets", "met_Et>100 && met_Et<200"},
        {"VRZjets_noZwindow_MET100_200", "met_Et>100 && met_Et<200"},
        {"VRZjets_noZwindow_MET200_300", "met_Et>200 && met_Et<300"},
        {"VRZjets_noZwindow_MET300_400", "met_Et>300 && met_Et<400"},
        {"VRZjets_noZwindow_METgt400", "met_Et>400"},
        {"VRZjets_noZwindow_noBveto", "met_Et>100 && met_Et<200"},

        {"SRC", "met_Et>250"},
        {"SRLow", "met_Et>250"},
        {"SRMed", "met_Et>300"},
        {"SRHigh", "met_Et>300"},
        {"SRLowZ", "met_Et>250"},
        {"SRMedZ", "met_Et>300"},
        {"SRHighZ", "met_Et>300"},

        {"VRC", "(met_Et>150 && met_Et<250)"},
        {"VRLow", "(met_Et>150 && met_Et<250)"},
        {"VRMed", "(met_Et>150 && met_Et<250)"},
        {"VRHigh", "(met_Et>150 && met_Et<250)"},
        {"VRLowZ", "(met_Et>150 && met_Et<250)"},
        {"VRMedZ", "(met_Et>150 && met_Et<250)"},
        {"VRHighZ", "(met_Et>150 && met_Et<250)"},

        {"VRDPhiLow2", "met_Et>150"},
        {"VRDPhiMed2", "met_Et>150"},
        {"VRDPhiHigh2", "met_Et>150"},
        {"VRDPhiLow6", "met_Et>150"},
        {"VRDPhiMed6", "met_Et>150"},
        {"VRDPhiHigh6", "met_Et>150"},
    };
}

enum branch_type {INT, FLOAT};
namespace bins {
    //--- smearing
    const int n_smearing_bins = 200;
    double smearing_low = -500;
    double smearing_high = 500;

    const int n_pt_bins = 20;
    double pt_bins[] = {25,26,27,28,30,32,35,40,45,50,55,60,70,80,100,140,180,250,300,500,800};
    double MET_bins[] = {20,30,40,50,60,70,80,90,100,120,140,160,180,200,230,260,300,350,400,500,600};

    const int n_METl_bins = 21;
    double METl_bins[] = {-700,-500,-400,-300,-250,-200,-150,-100,-60,-40,-20,20,40,60,100,150,200,250,300,400,500,700};

    const int n_mll_bins = 43;
    double mll_bin[] = {12,20,30,40,50,60,70,80,82,84,86,88,90,92,94,96,98,100,110,120,130,140,150,160,170,180,190,200,220,240,260,280,300,320,340,360,380,400,440,480,520,560,600,800};

    TH1D *hist_METl_bins, *hist_pt_bins, *hist_MET_bins;
    void init_binning_histograms() {
        hist_METl_bins = new TH1D("hist_METl_bins","",n_METl_bins,METl_bins);
        hist_pt_bins = new TH1D("hist_pt_bins","",n_pt_bins,pt_bins);
        hist_MET_bins = new TH1D("hist_MET_bins","",n_pt_bins,MET_bins); //hist_MET_bins->SetStats(0);
    }

    //--- reweighting (and plotting)
    const map<string, int> reweighting_type = {
        {"Ptll", FLOAT},
        {"nBJet20_MV2c10_FixedCutBEff_77", INT},
        {"nJet30", INT},
        {"Ht30", FLOAT},
    };
    const map<string, int> n_reweighting_bins = {
        {"Ptll", 22},
        {"nBJet20_MV2c10_FixedCutBEff_77", 10},
        {"nJet30", 10},
        {"Ht30", 22},
    };
    const map<string, vector<double>> reweighting_bins = {
        {"Ptll", {0,30,35,40,45,50,55,60,70,80,100,120,140,160,180,200,220,260,280,300,350,400,600}},
        {"nBJet20_MV2c10_FixedCutBEff_77", {0,1,2,3,4,5,6,7,8,9,10}},
        {"nJet30", {0,1,2,3,4,5,6,7,8,9,10}},
        {"Ht30", {0,30,35,40,45,50,55,60,70,80,100,120,140,160,180,200,220,260,280,300,350,400,600}},
    };
}

//----------
// PLOTTING
//----------

ROOT::RDF::TH1DModel getHistogramInfo(string plot_feature) {
    map<string, ROOT::RDF::TH1DModel> plot_settings;
    plot_settings["met_Et"] = ROOT::RDF::TH1DModel("", "E_{T}^{miss} [GeV]", 20, 0, 300);
    plot_settings["METl"] = ROOT::RDF::TH1DModel("", "E_{T,||}^{miss} [GeV]", 30, -150, 150);
    plot_settings["METt"] = ROOT::RDF::TH1DModel("", "E_{T,#perp}^{miss} [GeV]", 30, -150, 150);
    plot_settings["met_Sign"] = ROOT::RDF::TH1DModel("", "E_{T}^{miss} significance", 20, 0, 20);
    plot_settings["MET_loose"] = ROOT::RDF::TH1DModel("", "E_{T,loose}^{miss} [GeV]", 20, 0, 200);
    plot_settings["MET_tight"] = ROOT::RDF::TH1DModel("", "E_{T,tight}^{miss} [GeV]", 20, 0, 200);
    plot_settings["MET_tighter"] = ROOT::RDF::TH1DModel("", "E_{T,tighter}^{miss} [GeV]", 20, 0, 200);
    plot_settings["MET_tenacious"] = ROOT::RDF::TH1DModel("", "E_{T,tenacious}^{miss} [GeV]", 20, 0, 200);
    plot_settings["mt2leplsp_0"] = ROOT::RDF::TH1DModel("", "m_{T2}^{0} [GeV]", 20, 0, 500);
    plot_settings["Ptll"] = ROOT::RDF::TH1DModel("", "p_{T} [GeV]", 25, 0, 1000);
    //plot_settings["Ptll_reweight"] = ROOT::RDF::TH1DModel("", "p_{T} [GeV]", bins::n_reweighting_bins.at("Ptll"),
        //&bins::reweighting_bins.at("Ptll")[0]);
    //plot_settings["Ptll"] = ROOT::RDF::TH1DModel("", "p_{T} [GeV]", bins::n_reweighting_bins.at("Ptll"),
        //&bins::reweighting_bins.at("Ptll")[0]);
    plot_settings["Z_pt"] = ROOT::RDF::TH1DModel("", "p_{T} [GeV]", 20, 0, 100);
    plot_settings["nJet30"] = ROOT::RDF::TH1DModel("", "n_{jets}", 6, 2, 8);
    plot_settings["jet_n"] = ROOT::RDF::TH1DModel("", "n_{jets}", 6, 2, 8);
    plot_settings["jet_eta"] = ROOT::RDF::TH1DModel("", "jet_{#eta}", 30, -3, 3);
    plot_settings["jet_phi"] = ROOT::RDF::TH1DModel("", "jet_{#phi}", 20, 0, 3.14);
    plot_settings["jetPt"] = ROOT::RDF::TH1DModel("", "jet_{p_{T}} [GeV]", 20, 0, 300);
    plot_settings["jetPt[0]"] = ROOT::RDF::TH1DModel("", "jet_{p_{T},1} [GeV]", 20, 0, 300);
    plot_settings["jetPt[1]"] = ROOT::RDF::TH1DModel("", "jet_{p_{T},2} [GeV]", 20, 0, 300);
    plot_settings["bjet_n"] = ROOT::RDF::TH1DModel("", "n_{b-jets}", 4, 0, 4);
    plot_settings["Ht30"] = ROOT::RDF::TH1DModel("", "H_{T}", 15, 0, 1500);
    //plot_settings["Ht30_reweight"] = ROOT::RDF::TH1DModel("", "H_{T} [GeV]", bins::n_reweighting_bins.at("Ht30"),
        //&bins::reweighting_bins.at("Ht30")[0]);
    //plot_settings["Ht30"] = ROOT::RDF::TH1DModel("", "H_{T} [GeV]", bins::n_reweighting_bins.at("Ht30"),
        //&bins::reweighting_bins.at("Ht30")[0]);
    plot_settings["mll"] = ROOT::RDF::TH1DModel("", "m_{ll} [GeV]", 30, 0, 300);
    plot_settings["MT2"] = ROOT::RDF::TH1DModel("", "m_{T2} [GeV]", 20, 0, 200);
    plot_settings["MT2W"] = ROOT::RDF::TH1DModel("", "m_{T2}^{W} [GeV]", 20, 0, 200);
    plot_settings["lepEta"] = ROOT::RDF::TH1DModel("", "lep_{#eta}", 30, -3, 3);
    plot_settings["lepPhi"] = ROOT::RDF::TH1DModel("", "lep_{#phi}", 20, 0, 3.14);
    plot_settings["lepPt"] = ROOT::RDF::TH1DModel("", "lep_{p_{T}} [GeV]", 20, 0, 300);
    plot_settings["lepPt[0]"] = ROOT::RDF::TH1DModel("", "lep_{p_{T},1} [GeV]", 20, 0, 300);
    plot_settings["lepPt[1]"] = ROOT::RDF::TH1DModel("", "lep_{p_{T},2} [GeV]", 20, 0, 200);
    plot_settings["lepEta[0]"] = ROOT::RDF::TH1DModel("", "lep_{#eta,1}", 30, -3, 3);
    plot_settings["lepEta[1]"] = ROOT::RDF::TH1DModel("", "lep_{#eta,2}", 30, -3, 3);
    plot_settings["DPhi_METLepLeading"] = ROOT::RDF::TH1DModel("", "#Delta#phi(lep_{1},E_{T}^{miss})", 20, 0, 3.14);
    plot_settings["DPhi_METLepSecond"] = ROOT::RDF::TH1DModel("", "#Delta#phi(lep_{2},E_{T}^{miss})", 20, 0, 3.14);
    plot_settings["dPhiMetJet1"] = ROOT::RDF::TH1DModel("", "#Delta#phi(jet_{1},E_{T}^{miss})", 20, 0, 3.14);
    plot_settings["dPhiMetJet2"] = ROOT::RDF::TH1DModel("", "#Delta#phi(jet_{2},E_{T}^{miss})", 20, 0, 3.14);
    plot_settings["minDPhi2JetsMet"] = ROOT::RDF::TH1DModel("", "#Delta#phi(jet_{min(1,2)},E_{T}^{miss})", 20, 0, 3.14);
    plot_settings["dPhiPllMet"] = ROOT::RDF::TH1DModel("", "#Delta#phi(p_{T},E_{T}^{miss})", 20, 0, 3.14);

    ROOT::RDF::TH1DModel hist_model = plot_settings[plot_feature];
    return hist_model;
}

//-----------
// FUNCTIONS
//-----------

template<class variableType>
void SetInputBranch(TTree* inputTree, string branchName, variableType variablePointer) {
    // set variablePointer to reference value in branch if it exists
    for (auto branch : *(inputTree->GetListOfBranches())) {
        if (branch->GetName() == branchName) {
            inputTree->SetBranchStatus(branchName.c_str(), 1);
            inputTree->SetBranchAddress(branchName.c_str(), variablePointer);
        }
    }
}

template<class variableType>
void CopyBranch(TTree* inputTree, TTree* outputTree, string inputBranchName, string outputBranchName, variableType variablePointer, string varType) {
    // copy branch and set variablePointer to reference value in branch if it exists
    for (auto branch : *(inputTree->GetListOfBranches())) {
        if (branch->GetName() == inputBranchName) {
            inputTree->SetBranchStatus(inputBranchName.c_str(), 1);
            inputTree->SetBranchAddress(inputBranchName.c_str(), variablePointer);
            if (varType.compare(0, 6, "vector") == 0)
                outputTree->Branch(outputBranchName.c_str(), varType.c_str(), variablePointer);
            else
                outputTree->Branch(outputBranchName.c_str(), variablePointer, (outputBranchName+"/"+varType).c_str());
        }
    }
}

vector<int> int_copy_vars;
vector<double> double_copy_vars;
vector<float> float_copy_vars;

void CopyAllBranches(TTree* inputTree, TTree* outputTree, vector<string> branches) {
    int_copy_vars.clear(); int_copy_vars.reserve(branches.size());
    double_copy_vars.clear(); double_copy_vars.reserve(branches.size());
    float_copy_vars.clear(); float_copy_vars.reserve(branches.size());
    for (string branch : branches) {
        string branch_name = branch.substr(0,branch.length()-2);
        char branch_type = branch.substr(branch.length()-1)[0];
        switch (branch_type) {
            case 'I':
                int int_copy_var; int_copy_vars.push_back(int_copy_var);
                CopyBranch(inputTree, outputTree, branch_name, branch_name, &int_copy_vars.back(), "I");
                break;
            case 'D':
                double double_copy_var; double_copy_vars.push_back(double_copy_var);
                CopyBranch(inputTree, outputTree, branch_name, branch_name, &double_copy_vars.back(), "D");
                break;
            case 'F':
                float float_copy_var; float_copy_vars.push_back(float_copy_var);
                CopyBranch(inputTree, outputTree, branch_name, branch_name, &float_copy_vars.back(), "F");
                break;
            default:
                cout << "Unknown branch type" << endl;
        }
    }
}

//  period: data15-16 (input) -> ZMC16a (source file), data17 -> ZMC16cd, data18 -> ZMC16e
float GetLumi(TString period) {
    float lumi = 1.0;
    if (period.Contains("mc16e") || period.Contains("data18")) lumi = 59900;
    else if (period.Contains("mc16cd") || period.Contains("data17")) lumi = 44000;
    else if (period.Contains("mc16a") || period.Contains("data15-16")) lumi = 36100;
    return lumi;
}

string getMCPeriod(TString period) {
    string mc_period;
    if (period == "data15-16") mc_period = "mc16a";
    else if (period == "data17") mc_period = "mc16cd";
    else if (period == "data18") mc_period = "mc16e";
    else mc_period = period;
    return mc_period;
}

string getMCPeriod(string period) {
    return string(getMCPeriod(TString(period)));
}

string DataPeriod(TString period) {
    string data_period;
    if (period == "mc16a") data_period = "data15-16";
    else if (period == "mc16cd") data_period = "data17";
    else if (period == "mc16e") data_period = "data18";
    else data_period = period;
    return data_period;
}

template <typename T> constexpr string_view type_name() {
    // from https://stackoverflow.com/questions/81870/is-it-possible-to-print-a-variables-type-in-standard-c/56766138#56766138
    // e.g. cout << type_name<decltype(weighted_dataframe)>() << endl;
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "std::string_view type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr std::string_view type_name() [with T = ";
    suffix = "; std::string_view = std::basic_string_view<char>]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl type_name<";
    suffix = ">(void)";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}

void passTest(string msg) {
    cout << BOLD(PGRN("PASSED TEST: ")) << msg << endl;
}

void failTest(string msg) {
    cout << BOLD(PRED("ERROR: ")) << msg << endl;
    exit(0);
}

string padString(string unpadded_string) {
    int pad_len = 23;
    string padded_string = unpadded_string + string(pad_len - unpadded_string.length(), ' ');
    return (padded_string + ": ");
}

float convertThetaToEta(float theta) {
    return -log(tan(theta/2));
}

//-------------------------
// TREE MANIPULATION CLASS
//-------------------------

using BranchRenameOptions = vector<tuple<string, string>>;
using BranchAddOptions = vector<tuple<string, string>>;

class TreeCreator {
public:
    ROOT::RDataFrame *dataframe;
    string out_file_name;
    string out_tree_name;
    string cut;
    string final_cut;
    vector<string> branches_to_copy;
    BranchRenameOptions branches_to_rename;
    BranchAddOptions branches_to_add;

    TreeCreator() {
    }

    void read(string file_name, string tree_name) {
        cout << "Opening read file      : " << file_name << endl;
        cout << "Tree name              : " << tree_name << endl;

        this->dataframe = new ROOT::RDataFrame(tree_name, file_name);
    }

    void setBranchesToCopy(vector<string> branches_to_copy) {
        this->branches_to_copy = branches_to_copy;
    }

    void setBranchesToRename(BranchRenameOptions branches_to_rename) {
        this->branches_to_rename = branches_to_rename;
    }

    void setBranchesToAdd(BranchAddOptions branches_to_add) {
        this->branches_to_add = branches_to_add;
    }

    void setCut(string cut) {
        this->cut = cut;
    }

    void setFinalCut(string cut) {
        this->final_cut = cut;
    }

    void write(string file_name, string tree_name) {
        this->out_file_name = file_name;
        this->out_tree_name = tree_name;

        cout << "Opening write file     : " << file_name << endl;
        cout << "Tree name              : " << tree_name << endl;
        cout << endl;

        cout << "Processing" << endl;

        //--- apply cut
        auto reduced_dataframe = this->dataframe->Filter(this->cut.c_str());

        //--- get all branches to save
        vector<string> all_out_branches = this->branches_to_copy;

        //--- rename branches
        for (auto branch : this->branches_to_rename) {
            string old_name = get<0>(branch);
            string new_name = get<1>(branch);
            reduced_dataframe = reduced_dataframe.Define(new_name.c_str(), old_name.c_str());
            all_out_branches.push_back(new_name);
        }

        //--- add branches
        for (auto branch : this->branches_to_add) {
            string branch_name = get<0>(branch);
            //string expression = get<1>(branch);
            string call = get<1>(branch);
            //gInterpreter->Declare(expression.c_str());
            reduced_dataframe = reduced_dataframe.Define(branch_name.c_str(), call.c_str());
            all_out_branches.push_back(branch_name);
        }

        //--- apply cut
        if (!final_cut.empty())
            reduced_dataframe = reduced_dataframe.Filter(this->final_cut.c_str());

        if (all_out_branches.size() > 0)
            reduced_dataframe.Snapshot(this->out_tree_name.c_str(), out_file_name.c_str(), all_out_branches);
        else
            reduced_dataframe.Snapshot(this->out_tree_name.c_str(), out_file_name.c_str());
        cout << endl;
    }
};

//---------
// OPTIONS
//---------

struct Options {
    //--- filepaths
    string bkg_mc_path, bkg_data_path, photon_mc_path, photon_data_path;
    string my_samples_folder, sampling_method;
    string reduction_folder, smearing_folder, reweighting_folder, plots_folder, unit_test_folder;

    //--- run info
    string period, data_period, mc_period;

    string channel;
    string type;

    string tree_name;

    bool is_photon; // vs. bkg
    bool is_data; // vs. MC

    bool unit_testing;

    //--- reduction options
    string sampleID;
    string in_file_name;
    string in_tree_name;
    string out_file_name;

    vector<string> branches_to_copy;
    BranchRenameOptions branches_to_rename;
    BranchAddOptions branches_to_add;

    string cut;
    string final_cut;

    //--- smearing options
    TCut bkg_smearing_selection;
    TCut photon_smearing_selection;

    bool turn_off_shifting_and_smearing;
    bool make_diagnostic_plots;
    bool run_vgamma;

    //--- reweighting options
    string smearing_file_name;
    string reweighting_file_name;

    vector<string> reweight_vars;
    vector<string> processes;

    TCut reweight_region;

    //--- plotting options
    string reweight_branch;

    vector<string> plot_regions;
    vector<string> plot_features;
    vector<string> plot_channels;

    TCut additional_plot_cut = "1";

    map<string, int> process_colors;
    map<string, string> process_latex;

    bool blinded;
    bool print_photon_yield_only;
    vector<string> diagnostic_plots;
    bool do_vgamma_subtraction;

    bool plot_reweighted_photons;
    bool plot_unreweighted_photons;
    bool plot_zmc;

    bool scale_zmc;

    string scaling_method;
};

#endif
