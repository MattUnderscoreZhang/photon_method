#include "Settings.cpp"
#include "MT2.cpp"

using namespace std;

//-----------------
// CONVERTOR CLASS
//-----------------

class PhotonToZConverter {
public:
    SmearingOptions options;
    std::default_random_engine *random_generator;
    TRandom1 myRandom;
    TChain *inputTree;
    TFile* outputFile;
    TTree *outputTree;

    TH1F* h_lep_cm_theta;
    std::vector<float> lep_cm_theta_bin_bounds;
    std::discrete_distribution<int> *lep_cm_theta_distribution;

    TH1D* hist_z_metl_bin_pt[bins::n_pt_bins+2];  // 0 = underflow, n_pt_bins + 1 = overflow
    TH1D* hist_g_metl_bin_pt[bins::n_pt_bins+2];
    TH1D* hist_g_smeared_metl_bin_pt[bins::n_pt_bins+2];
    vector<vector<TH1D*>> hist_z_mll_bin_pt_metl;

    map<string, map<int, TH1F*>> unit_test_plots;

    map<int, pair<float, float>> smearing_gaussians;

    PhotonToZConverter(SmearingOptions options) {
        this->options = options;

        //--- open files
        cout << "channel                : " << options.channel         << endl;
        cout << "period                 : " << options.period          << endl;
        cout << "is data?               : " << options.is_data          << endl;
        cout << "smearing output        : " << options.out_file_name   << endl;

        TH1::SetDefaultSumw2();

        string photon_file_name;
        if (options.is_data) photon_file_name = options.in_file_path + options.data_period + "_data_photon.root";
        else photon_file_name = options.in_file_path + options.mc_period + "_SinglePhoton222.root";

        this->inputTree = new TChain("BaselineTree");
        this->inputTree->Add(photon_file_name.c_str());

        cout << endl;
        cout << "Opening read file      : " << photon_file_name        << endl;
        cout << "Events in ntuple       : " << inputTree->GetEntries() << endl;

        TH1::SetDefaultSumw2();

        string outfilename = options.out_file_name;

        this->outputFile = new TFile(outfilename.c_str(), "recreate");          
        this->outputTree = new TTree("BaselineTree", "baseline tree");
        this->outputTree->SetDirectory(outputFile);

        cout << "Opening write file     : " << outfilename << endl;
        cout << endl;

        //--- set up random generators
        unsigned random_seed = std::chrono::system_clock::now().time_since_epoch().count();
        this->random_generator = new default_random_engine(random_seed);
        myRandom.SetSeed(0);

        //--- set up histograms
        for (int bin=0; bin<bins::n_pt_bins+2; bin++)
            this->hist_g_smeared_metl_bin_pt[bin] = new TH1D(TString("hist_g_smeared_metl_")+TString::Itoa(bin,10),"",bins::n_smearing_bins,bins::smearing_low,bins::smearing_high);

        for (int bin=0; bin<bins::n_pt_bins+2; bin++) {
            this->hist_z_metl_bin_pt[bin] = new TH1D(TString("hist_z_metl_")+TString::Itoa(bin,10),"",bins::n_smearing_bins,bins::smearing_low,bins::smearing_high);
            this->hist_g_metl_bin_pt[bin] = new TH1D(TString("hist_g_metl_")+TString::Itoa(bin,10),"",bins::n_smearing_bins,bins::smearing_low,bins::smearing_high);

            vector<TH1D*> hist_z_mll_bin_pt_metl_pt_bin;
            for (int bin1=0; bin1<bins::n_METl_bins+2; bin1++) {
                hist_z_mll_bin_pt_metl_pt_bin.push_back(new TH1D(TString("hist_z_Mll_dPt_")+TString::Itoa(bin,10)+TString("_")+TString::Itoa(bin1,10),"",bins::n_mll_bins,bins::mll_bin));
            }
            this->hist_z_mll_bin_pt_metl.push_back(hist_z_mll_bin_pt_metl_pt_bin);
        }

        this->getLepThetaHistograms();
        this->getSmearingGaussians();
    }

    //------------
    // UNIT TESTS
    //------------

    void fillUnitTestPlots(TTree* ttree_data, TTree* ttree_tt, TTree* ttree_vv, TTree* ttree_zjets, TCut bkg_baseline_with_channel) {
        //--- initialize histograms
        vector<string> features{"lepEta", "METl", "mll"};
        vector<string> processes = options.is_data ? vector<string>{"data", "tt", "vv"} : vector<string>{"z"};

        map<string, map<string, map<int, TH1F*>>> h_features;
        for (auto feature : features) {
            for (auto process : processes) {
                for (int pt_bin=0; pt_bin<bins::n_pt_bins+2; pt_bin++) {
                    TString hist_name = "h" + process + "_" + feature + "_pt_bin_" + pt_bin;
                    TH1F* h_temp;
                    if (feature == "lepEta") h_temp = new TH1F(hist_name, "", 100, -3, 3);
                    else if (feature == "METl") h_temp = new TH1F(hist_name, "", 100, -100, 100);
                    else if (feature == "mll") h_temp = new TH1F(hist_name, "", 100, 0, 150);
                    h_features[feature][process][pt_bin] = h_temp;
                }
            }
        }

        //--- fill histograms
        map<string, TTree*> ttrees;
        ttrees["data"] = ttree_data; ttrees["tt"] = ttree_tt; ttrees["vv"] = ttree_vv; ttrees["zjets"] = ttree_zjets;
        for (auto process : processes) {
            ttrees[process]->Draw(">>event_list", bkg_baseline_with_channel);
            auto event_list = (TEventList*) gDirectory->Get("event_list");

            ttrees[process]->SetBranchStatus("*", 0);
            vector<float> *lepEta = new vector<float>(10); SetInputBranch(ttrees[process], "lepEta", &lepEta);
            float METl; SetInputBranch(ttrees[process], "METl", &METl);
            float mll; SetInputBranch(ttrees[process], "mll", &mll);
            float Ptll; SetInputBranch(ttrees[process], "Ptll", &Ptll);
            double totalWeight; SetInputBranch(ttrees[process], "totalWeight", &totalWeight);

            for (int i=0; i<event_list->GetN(); i++) {
                ttrees[process]->GetEntry(i);
                int pt_bin = bins::hist_pt_bins->FindBin(Ptll);

                h_features["lepEta"][process][pt_bin]->Fill(lepEta->at(0), totalWeight);
                h_features["lepEta"][process][pt_bin]->Fill(lepEta->at(1), totalWeight);
                h_features["METl"][process][pt_bin]->Fill(METl, totalWeight);
                h_features["mll"][process][pt_bin]->Fill(mll, totalWeight);
            }
        }

        //--- add histograms for data
        for (int pt_bin=0; pt_bin<bins::n_pt_bins+2; pt_bin++)
            this->unit_test_plots["lep_cm_theta"][pt_bin] = this->h_lep_cm_theta;

        for (auto feature : features) {
            for (int pt_bin=0; pt_bin<bins::n_pt_bins+2; pt_bin++) {
                if (options.is_data) {
                    h_features[feature]["data"][pt_bin]->Add(h_features[feature]["tt"][pt_bin], -1.0);
                    h_features[feature]["data"][pt_bin]->Add(h_features[feature]["vv"][pt_bin], -1.0);
                    this->unit_test_plots[feature][pt_bin] = h_features[feature]["data"][pt_bin];
                }
                else {
                    this->unit_test_plots[feature][pt_bin] = h_features[feature]["z"][pt_bin];
                }
            }
        }
    }

    map<string, map<int, TH1F*>> getUnitTestPlots() {
        return this->unit_test_plots;
    };

    //------------------
    // HELPER FUNCTIONS
    //------------------

    int rebinHistogram(TH1D* hist, int rebin) {

        /**
         * If rebin=0, rebin histogram by factors of 2, up to 32, until it looks reasonable or there are an odd number of bins.
         * This means having abs(negative_yield/positive_yield)<0.005 and core_yield/positive_yield>0.4.
         * Else, rebin by #(rebin).
         * Finally, set all negative bins values in the histogram to 0.
         * Return the final rebinning factor for the histogram.
         */

        float negative_yield = 0.; // sum of bins with negative values
        float positive_yield = 0.001; // sum of bins with positive values
        float core_yield = 0.; // sum of bins within one sigma of axis center

        if (rebin==0) {
            rebin = 1;
            for (int bin=1;bin<=hist->GetNbinsX();bin++) {
                if (hist->GetBinContent(bin)>=0) positive_yield += hist->GetBinContent(bin);
                else negative_yield += hist->GetBinContent(bin);
            }
            int has_odd_nbins = hist->GetNbinsX() % 2;
            while ((abs(negative_yield/positive_yield)>0.005 || core_yield/positive_yield<0.4) && has_odd_nbins==0 && rebin<=32) {
                hist->Rebin(2);
                rebin = rebin*2;
                has_odd_nbins = hist->GetNbinsX() % 2;
                negative_yield = 0.;
                positive_yield = 0.;
                core_yield = 0.;
                for (int bin=1;bin<=hist->GetNbinsX();bin++) {
                    if (hist->GetBinContent(bin)>=0) positive_yield += hist->GetBinContent(bin);
                    else negative_yield += hist->GetBinContent(bin);
                    if (abs(hist->GetBinCenter(bin)-hist->GetMean())<hist->GetRMS()) { // if this bin is within a sigma of the center of the axis
                        core_yield += hist->GetBinContent(bin); // core_yield = 68% for a perfect Guassian
                    }
                }
            }
        }
        else {
            // merge every #rebin bins into a single bin
            hist->Rebin(rebin);
        }

        for (int bin=1;bin<=hist->GetNbinsX();bin++) {
            hist->SetBinContent(bin,max(hist->GetBinContent(bin),0.));
        }

        return rebin;
    };

    //------------------
    // LEPTON SPLITTING
    //------------------

    void getLepThetaHistograms() {
        cout << PBLU("Getting Z lepton CM theta distribution histogram") << endl;
        cout << endl;
        gStyle->SetOptStat(0);

        //--- open files and create TChains
        TTree *ttree_data, *ttree_tt, *ttree_vv, *ttree_zjets;

        string data_filename = options.in_file_path + options.data_period + "_data_bkg.root";
        string tt_filename = options.in_file_path + options.mc_period + "_ttbar.root";
        string vv_filename = options.in_file_path + options.mc_period + "_diboson.root";
        string zjets_filename = options.in_file_path + options.mc_period + "_Zjets.root";

        cout << "Opening data file      : " << data_filename << endl;
        ttree_data = (TTree*)(new TFile(data_filename.c_str()))->Get("BaselineTree");
        cout << "data entries           : " << ttree_data->GetEntries() << endl;
        cout << "Opening ttbar file     : " << tt_filename << endl;
        ttree_tt = (TTree*)(new TFile(tt_filename.c_str()))->Get("BaselineTree");
        cout << "ttbar entries          : " << ttree_tt->GetEntries() << endl;
        cout << "Opening diboson file   : " << vv_filename << endl;
        ttree_vv = (TTree*)(new TFile(vv_filename.c_str()))->Get("BaselineTree");
        cout << "diboson entries        : " << ttree_vv->GetEntries() << endl;
        cout << "Opening Z+jets file    : " << zjets_filename << endl;
        ttree_zjets = (TTree*)(new TFile(zjets_filename.c_str()))->Get("BaselineTree");
        cout << "Z+jets entries         : " << ttree_zjets->GetEntries() << endl;
        cout << endl;

        //--- modify event selections and weights
        TCut bkg_baseline_with_channel;
        if (TString(options.channel).EqualTo("ee")) bkg_baseline_with_channel = cuts::bkg_baseline + cuts::ee;
        else if (TString(options.channel).EqualTo("mm")) bkg_baseline_with_channel = cuts::bkg_baseline + cuts::mm;
        else {
            cout << "Unrecognized channel! quitting   " << options.channel << endl;
            exit(0);
        }

        cout << "bkg selection          : " << bkg_baseline_with_channel.GetTitle() << endl;
        cout << "bkg weight             : " << cuts::bkg_weight.GetTitle() << endl;

        //--- fill lep theta histogram
        TH1F* hz = new TH1F("hz", "", 100, 0, 3);

        ttree_zjets->Draw("Z_cm_lep_theta>>hz", bkg_baseline_with_channel*cuts::bkg_weight, "goff");
        cout << "Z+jets integral        : " << hz->Integral() << endl;

        this->h_lep_cm_theta = (TH1F*) hz->Clone("h_lep_cm_theta");

        if (options.diagnostic_plots || options.unit_testing) {
            this->fillUnitTestPlots(ttree_data, ttree_tt, ttree_vv, ttree_zjets, bkg_baseline_with_channel);
        }
        cout << endl;

        //--- get lep theta histogram bin boundaries
        std::vector<int> h_lep_cm_bin_counts;
        for (int i=0; i<this->h_lep_cm_theta->GetNbinsX(); i++) {
            h_lep_cm_bin_counts.push_back(this->h_lep_cm_theta->GetBinContent(i));
            this->lep_cm_theta_bin_bounds.push_back(this->h_lep_cm_theta->GetBinLowEdge(i));
        }
        this->lep_cm_theta_bin_bounds.push_back(this->h_lep_cm_theta->GetBinLowEdge(this->h_lep_cm_theta->GetNbinsX()) +
            this->h_lep_cm_theta->GetBinWidth(this->h_lep_cm_theta->GetNbinsX()));
        this->lep_cm_theta_distribution = new std::discrete_distribution<int>(h_lep_cm_bin_counts.begin(),h_lep_cm_bin_counts.end());
    }

    float getRandomLepTheta() {
        int lep_theta_bin = (*this->lep_cm_theta_distribution)(*this->random_generator);
        float low_lep_theta = this->lep_cm_theta_bin_bounds[lep_theta_bin];
        float high_lep_theta = this->lep_cm_theta_bin_bounds[lep_theta_bin+1];
        float lep_theta_cm = this->myRandom.Rndm()*(high_lep_theta-low_lep_theta) + low_lep_theta;
        return lep_theta_cm;
    }

    tuple<bool, TLorentzVector, TLorentzVector> splitLeptons(TLorentzVector z_4vec) {
        // boost along z axis (since we measure angles in CM relative to boost direction)
        TVector3 boost_vec(0, 0, z_4vec.BoostVector().Mag());

        TRandom1 myRandom;
        myRandom.SetSeed(0);

        TLorentzVector l0_lab_4vec, l1_lab_4vec;
        //int n_tries = 0;
        //while (n_tries++ < 10) {

            double lep_phi_cm = myRandom.Rndm()*2.*TMath::Pi();
            float lep_theta_cm = this->getRandomLepTheta(); // Histogram sampling

            // Split leptons in Z rest frame
            TLorentzVector l0_cm_4vec, l1_cm_4vec;
            double lep_E_cm = z_4vec.M()/2.;
            double lep_px_cm = lep_E_cm*TMath::Sin(lep_theta_cm)*TMath::Cos(lep_phi_cm);
            double lep_py_cm = lep_E_cm*TMath::Sin(lep_theta_cm)*TMath::Sin(lep_phi_cm);
            double lep_pz_cm = lep_E_cm*TMath::Cos(lep_theta_cm);
            l0_cm_4vec.SetPxPyPzE(lep_px_cm, lep_py_cm, lep_pz_cm, lep_E_cm);
            l1_cm_4vec.SetPxPyPzE(-lep_px_cm, -lep_py_cm, -lep_pz_cm, lep_E_cm);

            // Boost to lab frame using smeared photon pT, eta, and phi
            l0_lab_4vec = l0_cm_4vec;
            l1_lab_4vec = l1_cm_4vec;
            l0_lab_4vec.Boost(boost_vec);
            l1_lab_4vec.Boost(boost_vec);
            if (l0_lab_4vec.Pt() < l1_lab_4vec.Pt()) {
                TLorentzVector lep_placeholder = l1_lab_4vec;
                l1_lab_4vec = l0_lab_4vec;
                l0_lab_4vec = lep_placeholder;
            }

            // Rotate to lab coordinates
            l0_lab_4vec.RotateY(z_4vec.Theta());
            l0_lab_4vec.RotateZ(z_4vec.Phi());
            l1_lab_4vec.RotateY(z_4vec.Theta());
            l1_lab_4vec.RotateZ(z_4vec.Phi());

            bool good_event = abs(l0_lab_4vec.Eta()) < 2.5 && abs(l1_lab_4vec.Eta()) < 2.5 &&
                              l0_lab_4vec.Pt() > cuts::leading_lep_pt_cut && l1_lab_4vec.Pt() > cuts::second_lep_pt_cut;
        //}
        //if (n_tries==11) continue;

        return make_tuple(good_event, l0_lab_4vec, l1_lab_4vec);
    }

    //--------------
    // MET SMEARING
    //--------------

    void getMETlAndMllHistograms() {
        /**
         * Fills histogram arrays with Z/photon METl distributions (and Z mll distribution) in binned regions.
         */

        cout << PBLU("Getting METl and mll histograms") << endl;
        cout << endl;

        //--- set up histograms and input files
        TFile* data_file = new TFile((this->options.in_file_path + this->options.data_period + "_data_bkg.root").c_str());
        TFile* ttbar_mc_file = new TFile((this->options.in_file_path + this->options.mc_period + "_ttbar.root").c_str());
        TFile* diboson_mc_file = new TFile((this->options.in_file_path + this->options.mc_period + "_diboson.root").c_str());
        TFile* Z_mc_file = new TFile((this->options.in_file_path + this->options.mc_period + "_Zjets.root").c_str());
        TFile* photon_file;
        if (this->options.is_data)
            photon_file = new TFile((this->options.in_file_path + this->options.data_period + "_data_photon.root").c_str());
        else
            photon_file = new TFile((this->options.in_file_path + this->options.mc_period + "_SinglePhoton222.root").c_str());

        vector<TFile*> z_files;
        vector<int> z_file_weights;

        if (this->options.is_data) {
            z_files = {data_file, ttbar_mc_file, diboson_mc_file};
            z_file_weights = {1, -1, -1};
        }
        else {
            z_files = {Z_mc_file};
            z_file_weights = {1};
        }

        //--- z samples
        for (int i=0; i<z_files.size(); i++) {
            TTree* tree = (TTree*)z_files[i]->Get("BaselineTree");
            tree->SetBranchStatus("*", 0);
            double totalWeight; SetInputBranch(tree, "totalWeight", &totalWeight);
            int jet_n; SetInputBranch(tree, "nJet30", &jet_n);
            int bjet_n; SetInputBranch(tree, "bjet_n", &bjet_n);
            float ptll; SetInputBranch(tree, "Ptll", &ptll);
            float mll; SetInputBranch(tree, "mll", &mll);
            float METl; SetInputBranch(tree, "METl", &METl);
            int channel; SetInputBranch(tree, "channel", &channel);
            int nLep_signal; SetInputBranch(tree, "nLep_signal", &nLep_signal);
            vector<float>* lep_pT = new vector<float>(10); SetInputBranch(tree, "lepPt", &lep_pT);

            int fileWeight = z_file_weights[i];

            //tree->Draw(">>event_list", cuts::reweight_region);
            tree->Draw(">>event_list", "nJet30>=2 && lepPt[0]>25 && lepPt[1]>25");
            //tree->Draw(">>event_list", "nJet30>=2");
            auto event_list = (TEventList*) gDirectory->Get("event_list");
            for (int entry=0; entry<event_list->GetN(); entry++) {
                tree->GetEntry(event_list->GetEntry(entry));
                if (TString(this->options.channel).EqualTo("ee") && channel != 1) continue;
                if (TString(this->options.channel).EqualTo("mm") && channel != 0) continue;
                int pt_bin = bins::hist_pt_bins->FindBin(ptll);
                int METl_bin = bins::hist_METl_bins->FindBin(METl);
                this->hist_z_mll_bin_pt_metl[pt_bin][METl_bin]->Fill(mll, fileWeight*totalWeight);
                //if (ptll<50. || jet_n<2 || lep_pT->at(0)<cuts::leading_lep_pt_cut || lep_pT->at(1)<cuts::second_lep_pt_cut) continue;
                //if (jet_n<2 || lep_pT->at(0)<cuts::leading_lep_pt_cut || lep_pT->at(1)<cuts::second_lep_pt_cut) continue;
                this->hist_z_metl_bin_pt[pt_bin]->Fill(METl, fileWeight*totalWeight);
            }
        }

        //--- photon sample
        TTree* tree = (TTree*)photon_file->Get("BaselineTree");
        tree->SetBranchStatus("*", 0);
        double totalWeight; SetInputBranch(tree, "totalWeight", &totalWeight);
        int jet_n; SetInputBranch(tree, "nJet30", &jet_n);
        int bjet_n; SetInputBranch(tree, "bjet_n", &bjet_n);
        float ptll; SetInputBranch(tree, "gamma_pt", &ptll);
        int nLep_signal; SetInputBranch(tree, "nLep_signal", &nLep_signal);
        float METl; SetInputBranch(tree, "METl_unsmeared", &METl);

        //tree->Draw(">>event_list", cuts::reweight_region);
        tree->Draw(">>event_list", "nJet30>=2");
        auto event_list = (TEventList*) gDirectory->Get("event_list");
        for (int entry=0; entry<event_list->GetN(); entry++) {
            tree->GetEntry(event_list->GetEntry(entry));
            //if (ptll<50. || jet_n!=1 || bjet_n!=0) continue;
            int pt_bin = bins::hist_pt_bins->FindBin(ptll);
            this->hist_g_metl_bin_pt[pt_bin]->Fill(METl, totalWeight);
        }

        //--- close files
        data_file->Close();
        ttbar_mc_file->Close();
        diboson_mc_file->Close();
        Z_mc_file->Close();
        photon_file->Close();
    };

    void makeSmearingDiagnosticPlots(map<int, pair<float, float>> smearing_gaussians) {
        for (auto const& [pt_bin, gaussian] : smearing_gaussians) {
            //cout << "Pt bin " << pt_bin << " has a smearing mean, std of " << gaussian.first << ", " << gaussian.second << endl;

            TCanvas *canvas = new TCanvas("canvas","canvas",600,600);
            canvas->cd();
            canvas->SetLogy();

            TH1D* z_hist = this->hist_z_metl_bin_pt[pt_bin];
            TString z_plot_name = Form("Diagnostics/Smearing/z_ptbin_%d.eps", pt_bin);
            z_hist->SetLineColor(1); z_hist->SetFillColor(42); z_hist->SetLineStyle(1);
            z_hist->GetXaxis()->SetTitle("METl");
            z_hist->GetYaxis()->SetTitle("entries / bin");
            z_hist->Draw("hist");
            canvas->Print(z_plot_name);

            TH1D* g_hist = this->hist_g_metl_bin_pt[pt_bin];
            TString g_plot_name = Form("Diagnostics/Smearing/g_ptbin_%d.eps", pt_bin);
            g_hist->SetLineColor(1); g_hist->SetFillColor(42); g_hist->SetLineStyle(1);
            g_hist->GetXaxis()->SetTitle("METl");
            g_hist->GetYaxis()->SetTitle("entries / bin");
            g_hist->Draw("hist");
            canvas->Print(g_plot_name);

            for (int metl_bin=0; metl_bin<bins::n_METl_bins+2; metl_bin++) {
                TH1D* z_mll_hist = this->hist_z_mll_bin_pt_metl[pt_bin][metl_bin];
                TString z_mll_plot_name = Form("Diagnostics/Smearing/z_mll_ptbin_%d_%d.eps", pt_bin, metl_bin);
                z_mll_hist->SetLineColor(1); z_mll_hist->SetFillColor(42); z_mll_hist->SetLineStyle(1);
                z_mll_hist->GetXaxis()->SetTitle("mll");
                z_mll_hist->GetYaxis()->SetTitle("entries / bin");
                z_mll_hist->Draw("hist");
                canvas->Print(z_mll_plot_name);
            }

            delete canvas;
        }
    }

    void getSmearingGaussians() {
        /**
         * Returns map with key = photon pt bin, value = (mean, std).
         * For a photon in a given pt bin, smear the event's MET using the given Gaussian numbers.
         */

        //--- METl histograms
        this->getMETlAndMllHistograms();

        //--- perform smearing
        cout << PBLU("Calculating smearing Gaussians") << endl;

        map<int, pair<float, float>> smearing_gaussians;
        for (int pt_bin=0; pt_bin<bins::n_pt_bins+2; pt_bin++) {
            //--- Rebin METl histograms to look reasonable
            int rebin_factor = rebinHistogram(this->hist_z_metl_bin_pt[pt_bin], 0);
            rebinHistogram(this->hist_g_metl_bin_pt[pt_bin], rebin_factor);
            for (int met_bin=0; met_bin<bins::n_METl_bins+2; met_bin++)
                rebinHistogram(this->hist_z_mll_bin_pt_metl[pt_bin][met_bin], 0);
            int n_hist_bins = bins::n_smearing_bins/rebin_factor;

            //--- Save smearing value
            float smear_mean = this->hist_z_metl_bin_pt[pt_bin]->GetMean() -
                               this->hist_g_metl_bin_pt[pt_bin]->GetMean();
            float smear_rms = sqrt(pow(this->hist_z_metl_bin_pt[pt_bin]->GetRMS(), 2) -
                              pow(this->hist_g_metl_bin_pt[pt_bin]->GetRMS(), 2));
            if (isnan(smear_rms)) smear_rms = 0.0;
            
            pair<float, float> gaussian = make_pair(0.0, 0.0);
            if (this->options.channel=="ee") gaussian = make_pair(smear_mean, 0.0);
            else gaussian = make_pair(smear_mean, smear_rms);
            smearing_gaussians.insert(pair<int, pair<float, float>>(pt_bin, gaussian));
        }

        if (this->options.diagnostic_plots) {
            cout << PBLU("Saving diagnostic smearing plots") << endl;
            this->makeSmearingDiagnosticPlots(smearing_gaussians);
        }
        cout << endl;

        return smearing_gaussians;
    }

    tuple<float, float> smearMETlAndMll(float METl, float gamma_pt) {
        int pt_bin = bins::hist_pt_bins->FindBin(gamma_pt);
        auto gaussian_vals = this->smearing_gaussians.find(pt_bin);
            
        float METl_smeared;
        if (this->options.turn_off_shifting_and_smearing)
            METl_smeared = METl;
        else {
            normal_distribution<float> smearing_gaussian(gaussian_vals->second.first, gaussian_vals->second.second);
            METl_smeared = METl + smearing_gaussian(*random_generator);
        }
        this->hist_g_smeared_metl_bin_pt[pt_bin]->Fill(METl_smeared);

        int METl_bin = bins::hist_METl_bins->FindBin(METl_smeared);
        float mll = 0;
        if (this->hist_z_mll_bin_pt_metl[pt_bin][METl_bin]->Integral()>0)
            mll = this->hist_z_mll_bin_pt_metl[pt_bin][METl_bin]->GetRandom();

        return make_tuple(METl_smeared, mll);
    }

    //------------------
    // EVENT CONVERSION
    //------------------

    void drawMETlDistributions() {
        for (int pt_bin=0; pt_bin<bins::n_pt_bins+2; pt_bin++) {
            TCanvas *canvas = new TCanvas("canvas","canvas",600,600);
            canvas->cd();
            canvas->SetLogy();

            TH1D* g_smeared_hist = this->hist_g_smeared_metl_bin_pt[pt_bin];
            TString g_smeared_plot_name = Form("Diagnostics/Smearing/g_smeared_ptbin_%d.eps", pt_bin);
            g_smeared_hist->SetLineColor(1); g_smeared_hist->SetFillColor(42); g_smeared_hist->SetLineStyle(1);
            g_smeared_hist->GetXaxis()->SetTitle("METl");
            g_smeared_hist->GetYaxis()->SetTitle("entries / bin");
            g_smeared_hist->Draw("hist");
            canvas->Print(g_smeared_plot_name);

            delete canvas, g_smeared_hist;
        }
    }

    void convertEvents() {
        TTree *inputTree = this->inputTree;
        TTree *outputTree = this->outputTree;

        float gamma_phi; inputTree->SetBranchAddress("gamma_phi", &gamma_phi);

        double totalWeight; CopyBranch(inputTree, outputTree, "totalWeight", "totalWeight", &totalWeight, "D");
        int bjet_n; CopyBranch(inputTree, outputTree, "bjet_n", "bjet_n", &bjet_n, "I");
        float gamma_pt; CopyBranch(inputTree, outputTree, "gamma_pt", "gamma_pt",  &gamma_pt, "F");
        float gamma_eta; CopyBranch(inputTree, outputTree, "gamma_eta", "Z_eta",  &gamma_eta, "F");
        float METl; CopyBranch(inputTree, outputTree, "METl_unsmeared", "METl_unsmeared", &METl, "F");
        float METt; CopyBranch(inputTree, outputTree, "METt_unsmeared", "METt_unsmeared", &METt, "F");
        float METl_smeared; outputTree->Branch("METl", &METl_smeared, "METl/F");
        float METt_smeared; outputTree->Branch("METt", &METt_smeared, "METt/F");
        float HT; CopyBranch(inputTree, outputTree, "Ht30", "Ht30", &HT, "F");
        float MET_raw; CopyBranch(inputTree, outputTree, "met_Et_unsmeared", "met_Et_unsmeared", &MET_raw, "F");
        float MET_phi; CopyBranch(inputTree, outputTree, "met_Phi", "met_Phi", &MET_phi, "F");

        vector<float>* jet_pT = new vector<float>(10); CopyBranch(inputTree, outputTree, "jetPt", "jetPt", &jet_pT, "vector<float>");
        vector<float>* jet_eta = new vector<float>(10); CopyBranch(inputTree, outputTree, "jet_eta", "jet_eta", &jet_eta, "vector<float>");
        vector<float>* jet_phi = new vector<float>(10); CopyBranch(inputTree, outputTree, "jet_phi", "jet_phi", &jet_phi, "vector<float>");

        float gamma_pt_smeared; outputTree->Branch("Ptll", &gamma_pt_smeared, "Ptll/F");
        float gamma_phi_smeared; outputTree->Branch("Z_phi", &gamma_phi_smeared, "Z_phi/F");
        int lep_n; outputTree->Branch("lep_n", &lep_n, "lep_n/I"); outputTree->Branch("nLep_signal", &lep_n, "nLep_signal/I");
            outputTree->Branch("nLep_base", &lep_n, "nLep_base/I");
        float MET_smeared; outputTree->Branch("met_Et", &MET_smeared, "met_Et/F");
        float DPhi_METLepLeading_smeared; outputTree->Branch("DPhi_METLepLeading", &DPhi_METLepLeading_smeared, "DPhi_METLepLeading/F");
        float DPhi_METLepSecond_smeared; outputTree->Branch("DPhi_METLepSecond", &DPhi_METLepSecond_smeared, "DPhi_METLepSecond/F");
        float DPhi_METZPhoton_smear; outputTree->Branch("DPhi_METZPhoton", &DPhi_METZPhoton_smear, "DPhi_METZPhoton/F");
        float MT2; outputTree->Branch("mt2leplsp_0", &MT2, "mt2leplsp_0/F");
        float DR_2Lep; outputTree->Branch("DR_2Lep", &DR_2Lep, "DR_2Lep/F");
        int photon_conversion_type; CopyBranch(inputTree, outputTree, "PhotonConversionType", "PhotonConversionType", &photon_conversion_type, "I");
        float lep_theta_cm; outputTree->Branch("Z_cm_lep_theta", &lep_theta_cm, "Z_cm_lep_theta/F");

        //--- HistFitter branches
        vector<string> histFitterBranches {"DatasetNumber/I", "H2PP/D", "H5PP/D", "H5PP_VR/D",
            "METOverPtISR/F", "METOverPtW/F", "METOverPtZ/F", "MJ/D", "MJ_VR/D", "MZ/D", "MZ_VR/D", "NjISR/D",
            "NjS/D", "PTCM/D", "PTCM_VR/D", "PTI/D", "PTISR/D", "PTISR_VR/D", "PTI_VR/D", "RISR/D", "RISR_VR/D",
            "RPT_HT5PP/D", "RPT_HT5PP_VR/D", "R_minH2P_minH3P/D", "R_minH2P_minH3P_VR/D", "Rjj/F", "Rll/F",
            "dPhiMetISR/F", "dPhiMetJet1/F", "dPhiMetJet2/F", "dPhiMetJet12Min/F", "dPhiPjjMet/F", "dPhiPllMet/F",
            "dphiISRI/D", "dphiISRI_VR/D", "dphiVP/D", "dphiVP_VR/D", "lept1Pt_VR/D", "lept2Pt_VR/D", "mTl3/D",
            "met_Sign/F", "minDphi/D", "minDPhi2JetsMet/F", "mll_RJ/D", "mll_RJ_VR/D", "nJet30/I", "nJet20/I", "mjj/F",
            "nBJet20_MV2c10_FixedCutBEff_77/I", "trigMatch_2LTrigOR/I", "genWeight/D", "eventWeight/D", "leptonWeight/D",
            "jvtWeight/D", "bTagWeight/D", "pileupWeight/D", "globalDiLepTrigSF/D", "RunNumber/I", "RandomRunNumber/I",
            "trigMatch_2LTrig/I", "lumi/D", "mjj_minDPhiZMET/F", "mbb/F", "PtISR/F"};
        CopyAllBranches(inputTree, outputTree, histFitterBranches);

        vector<float>* dPhiMetJet = new vector<float>(10); CopyBranch(inputTree, outputTree, "dPhiMetJet", "dPhiMetJet", &dPhiMetJet, "vector<float>");
        vector<float>* jetM = new vector<float>(10); CopyBranch(inputTree, outputTree, "jetM", "jetM", &jetM, "vector<float>");
        float mll; outputTree->Branch("mll", &mll, "mll/F");
        int is_OS = 1; outputTree->Branch("is_OS", &is_OS, "is_OS/I");
        vector<float>* lep_pT = new vector<float>(10); outputTree->Branch("lepPt", "vector<float>", &lep_pT);
        vector<float>* lep_eta = new vector<float>(10); outputTree->Branch("lepEta", "vector<float>", &lep_eta);
        vector<float>* lep_phi = new vector<float>(10); outputTree->Branch("lepPhi", "vector<float>", &lep_phi);
        vector<int>* lep_charge = new vector<int>(10); outputTree->Branch("lepCharge", "vector<int>", &lep_charge);
        vector<float>* lep_m = new vector<float>(10); outputTree->Branch("lepM", "vector<float>", &lep_m);
        vector<int>* lepIsoFCTight = new vector<int>{1,1}; outputTree->Branch("lepIsoFCTight", "vector<int>", &lepIsoFCTight);
        vector<int>* lepIsPR = new vector<int>{1,1}; outputTree->Branch("lepIsPR", "vector<int>", &lepIsPR);
        Int_t lepChannel; outputTree->Branch("channel", &lepChannel, "channel/I");
        vector<int>* lep_flavor = new vector<int>(10); outputTree->Branch("lepFlavor", "vector<int>", &lep_flavor);

        //-----------------------------
        // set global lepton info
        //-----------------------------

        int flavor;
        if (TString(options.channel).EqualTo("ee")) {
            flavor = 1;
            lepChannel = 1;
        }
        else if (TString(options.channel).EqualTo("mm")) {
            flavor = 2;
            lepChannel = 0;
        }

        lep_flavor->clear();
        lep_flavor->push_back(flavor);
        lep_flavor->push_back(flavor);

        lep_m->clear();
        if (lepChannel == 0) {
            lep_m->push_back(0.1056583);
            lep_m->push_back(0.1056583);
        }
        else if (lepChannel == 1) {
            lep_m->push_back(0.0005109);
            lep_m->push_back(0.0005109);
        }

        //-----------------------------
        // loop over events
        //-----------------------------

        Long64_t nentries = inputTree->GetEntries();
        for (Long64_t i=0; i<nentries; i++) {
            if (fmod(i,1e5)==0) cout << i << " events processed.\r" << flush;
            inputTree->GetEntry(i);

            //--- smearing
            gamma_pt_smeared = gamma_pt;
            gamma_phi_smeared = gamma_phi;

            METt_smeared = METt;
            try {
                auto [METl_smeared_return, mll_return] = this->smearMETlAndMll(METl, gamma_pt);
                METl_smeared = METl_smeared_return;
                mll = mll_return;
            }
            catch(...) {
                cout << PRED("Problem in event ") << i << endl;
                continue;
            }
            if (mll==0) continue;

            MET_smeared = sqrt(pow(METl_smeared, 2) + pow(METt_smeared, 2));
            TLorentzVector MET_smeared_4vec;
            MET_smeared_4vec.SetPtEtaPhiM(MET_smeared, 0, MET_phi, 0);
            DPhi_METZPhoton_smear = gamma_phi_smeared - MET_smeared_4vec.Phi();

            //--- lepton splitting
            TLorentzVector z_4vec;
            z_4vec.SetPtEtaPhiM(gamma_pt_smeared, gamma_eta, gamma_phi_smeared, mll);

            auto [good_event, l0_lab_4vec, l1_lab_4vec] = splitLeptons(z_4vec);
            if (!good_event) continue;

            lep_pT->clear();
            lep_pT->push_back(l0_lab_4vec.Pt());
            lep_pT->push_back(l1_lab_4vec.Pt());
            lep_eta->clear();
            lep_eta->push_back(l0_lab_4vec.Eta());
            lep_eta->push_back(l1_lab_4vec.Eta());
            lep_phi->clear();
            lep_phi->push_back(l0_lab_4vec.Phi());
            lep_phi->push_back(l1_lab_4vec.Phi());
            int charge = myRandom.Integer(2)*2-1; // random leading lepton charge
            lep_charge->clear();
            lep_charge->push_back(charge);
            lep_charge->push_back(-charge);

            lep_n = 2;
            MT2 = ComputeMT2(l0_lab_4vec, l1_lab_4vec, MET_smeared_4vec, 0, 0).Compute();
            DPhi_METLepLeading_smeared = fabs(MET_smeared_4vec.DeltaPhi(l0_lab_4vec));
            DPhi_METLepSecond_smeared = fabs(MET_smeared_4vec.DeltaPhi(l1_lab_4vec));
            DR_2Lep = l0_lab_4vec.DeltaR(l1_lab_4vec);

            outputTree->Fill();
        }
        cout << endl;

        //-----------------------------
        // write
        //-----------------------------

        this->outputFile->cd();
        this->outputTree->Write();

        cout << PBLU("Done with smearing") << endl;
        cout << endl;
        delete this->outputFile;
    }
};

//------------
// UNIT TESTS
//------------

SmearingOptions setUnitTestOptions(SmearingOptions options) {
    options.in_file_path = options.unit_test_folder + "ReducedNtuples/";
    options.out_file_name = "unit_test.root";
    options.in_tree_name = "BaselineTree";
    options.out_tree_name = "BaselineTree";

    options.period = "data15-16";
    options.mc_period = getMCPeriod(options.period);
    options.data_period = DataPeriod(options.period);
    options.is_data = true;
    //options.is_data = false;
    options.channel = "ee";

    options.turn_off_shifting_and_smearing = false;
    options.diagnostic_plots = false;

    return options;
}

void performUnitTests(SmearingOptions options) {
    cout << BOLD(PBLU("Performing unit testing on smearing step")) << endl;
    cout << endl;

    //--- get bkg feature plots and set up photon feature plots for comparison
    PhotonToZConverter converter(options);
    map<string, map<int, TH1F*>> bkg_plots = converter.getUnitTestPlots();

    TCanvas *can = new TCanvas("can","can",600,600);
    map<string, map<int, TH1F*>> photon_plots;

    for (int i=0; i<bins::n_pt_bins+2; i++) {
        photon_plots["lep_cm_theta"][i] = new TH1F("", "", 100, 0, 3);
        photon_plots["lepEta"][i] = new TH1F("", "", 100, -3, 3);
        photon_plots["METl"][i] = new TH1F("", "", 100, -100, 100);
        photon_plots["mll"][i] = new TH1F("", "", 100, 0, 150);
    }

    //--- open photon files
    string photon_file_name;
    if (options.is_data) photon_file_name = options.in_file_path + options.data_period + "_data_photon.root";
    else photon_file_name = options.in_file_path + options.mc_period + "_SinglePhoton222.root";
    TFile *photon_file = new TFile(photon_file_name.c_str());
    TTree *photon_tree = (TTree*)photon_file->Get("BaselineTree");

    float gamma_pt; photon_tree->SetBranchAddress("gamma_pt", &gamma_pt);
    float gamma_eta; photon_tree->SetBranchAddress("gamma_eta", &gamma_eta);
    float gamma_phi; photon_tree->SetBranchAddress("gamma_phi", &gamma_phi);
    double totalWeight; photon_tree->SetBranchAddress("totalWeight", &totalWeight);
    float METl_unsmeared; photon_tree->SetBranchAddress("METl_unsmeared", &METl_unsmeared);

    //--- fill photon histograms
    float Z_m = 91;
    for (int i=0; i<1000000; i++) {
        photon_tree->GetEntry(i);
        int pt_bin = bins::hist_pt_bins->FindBin(gamma_pt);

        photon_plots["lep_cm_theta"][pt_bin]->Fill(converter.getRandomLepTheta());

        TLorentzVector z_4vec;
        z_4vec.SetPtEtaPhiM(gamma_pt, gamma_eta, gamma_phi, Z_m);
        auto [good_event, l0_lab_4vec, l1_lab_4vec] = converter.splitLeptons(z_4vec);
        if (good_event) {
            photon_plots["lepEta"][pt_bin]->Fill(l0_lab_4vec.Eta(), totalWeight);
            photon_plots["lepEta"][pt_bin]->Fill(l0_lab_4vec.Eta(), totalWeight);
        }

        auto [METl, mll] = converter.smearMETlAndMll(METl_unsmeared, gamma_pt);
        //cout << METl << " " << mll << " " << pt_bin << endl;
        photon_plots["METl"][pt_bin]->Fill(METl, totalWeight);
        photon_plots["mll"][pt_bin]->Fill(mll, totalWeight);
    }

    //--- save comparison plots
    for (auto& [key, hist] : bkg_plots) {
        for (int i=0; i<bins::n_pt_bins+2; i++) {
            bkg_plots[key][i]->Draw("hist");
            photon_plots[key][i]->Scale(bkg_plots[key][i]->Integral()/photon_plots[key][i]->Integral());
            photon_plots[key][i]->SetLineColor(kRed);
            photon_plots[key][i]->Draw("samehist");
            can->Print("Diagnostics/Smearing/" + key + "_pt_bin_" + i + ".eps");
        }
        passTest("Comparison plots for " + key + " produced");
    }

    remove(options.out_file_name.c_str());
    delete can;

    passTest("Passed all unit tests");
    cout << endl;
}

//---------------
// MAIN FUNCTION
//---------------

void SmearPhotons(SmearingOptions options) {
    options.turn_off_shifting_and_smearing = false;
    options.diagnostic_plots = false;

    if (options.unit_testing) {
        options = setUnitTestOptions(options);
        performUnitTests(options);
    }
    else {
        cout << BOLD(PBLU("Performing smearing")) << endl;
        cout << endl;
        PhotonToZConverter converter(options);
        converter.convertEvents();
        if (options.diagnostic_plots) converter.drawMETlDistributions();
    }
}