#include "APIX9Data_TBCern.h"
#include "APIX9Data_TBCern_Matching.h"

void Draw_RunQA(std::string inputData, double dataTakenTime = 0, bool savePlots = true,  bool saveRoot = true)
{
  // Making input
  // ==========================
  if (!std::filesystem::exists(inputData)) {
    std::cout << "Required inputfile not found. Check if decoded .root file exists" << std::endl;
    return;
  }

  std::filesystem::path fullPath = inputData;
  std::string pathOfInfile = fullPath.parent_path().string();
  std::string baseOfInfile = fullPath.filename().string();
  std::string stemOfInfile = fullPath.stem().string();

  TFile* infile = new TFile(fullPath.c_str(), "read");
  if (infile == nullptr || infile -> IsZombie()) {
    std::cout << "Required .root file exists but cannot be opened" << std::endl;
    return;
  }

  TTree* inTree_raw = (TTree*)infile -> Get("apix_tree");
  TTree* inTree_hit = (TTree*)infile -> Get("apix_tree_matched");

  if (!inTree_raw || !inTree_hit) {
    std::cout << "Required TTree not found: " << std::endl;
    return;
  } else {
    std::cout << "Infile and TTree found" << std::endl;
  }

  APIXData raw;
  APIXMatched matched;
  SetBranchAddressOfInputTree(inTree_raw, "apix", raw);
  SetBranchAddressOfMatched(inTree_hit, "apix", matched);

  // Perform analysis
  // ==========================

  // Prepare QA objects
  TObjArray* hList = new TObjArray(0);

  std::vector<TH2D*> hitmaps;
  std::vector<TH1D*> tots;

  for (int ichip=0; ichip<9; ichip++) {
    std::string name_hitmap = Form("hitmap_chip%d", ichip);
    std::string name_tot = Form("tot_us_chip%d", ichip);

    hitmaps.push_back(new TH2D(name_hitmap.c_str(), name_hitmap.c_str(), 35, 0, 35, 35, 0, 35));
    tots.push_back(new TH1D(name_tot.c_str(), name_tot.c_str(), 40, 0, 20));

    hitmaps[ichip] -> Sumw2();
    tots[ichip] -> Sumw2();

    hList -> Add(hitmaps[ichip]);
    hList -> Add(tots[ichip]);
  }

  //TH1D* effPerChip = new TH1D("EffPerChip", "x: ChipID y: Eff", 9, 0, 9);
  TH1D* entriesPerChip = new TH1D("EntriesPerChip", "x: ChipID y: Entries", 9, 0, 9);
  TH1D* hitsPerChip = new TH1D("HitsPerChip", "x: ChipID y: #Hits", 9, 0, 9);

  for (auto& hist : {entriesPerChip, hitsPerChip}) {
    hist -> Sumw2();
    hList -> Add(hist);
  }

  // Fill QA objects
  for (int iraw=0; iraw<inTree_raw -> GetEntries(); iraw++) {
    inTree_raw -> GetEntry(iraw);
    entriesPerChip -> Fill(raw.chipID);
  }

  for (int ihit=0; ihit<inTree_hit -> GetEntries(); ihit++) {
    inTree_hit -> GetEntry(ihit);

    hitmaps[matched.chipID] -> Fill(matched.location_col, matched.location_row);
    tots[matched.chipID]    -> Fill((matched.tot_us_col + matched.tot_us_row)/2.);
    hitsPerChip             -> Fill(matched.chipID);
  }

  TH1D* entriesPerChipPerSec = (TH1D*)entriesPerChip -> Clone("EntriesPerChipPerSec");
  TH1D* hitsPerChipPerSec    = (TH1D*)hitsPerChip -> Clone("HitsPerChipPerSec");
  if (!entriesPerChipPerSec || !hitsPerChipPerSec) {
    std::cout << "Histograms not safely cloned" << std::endl;
    return;
  } else {
    std::cout << "Cloned" << std::endl;
    hList -> Add(entriesPerChipPerSec);
    hList -> Add(hitsPerChipPerSec);
  }

  if (dataTakenTime != 0) {
    entriesPerChipPerSec -> Scale(1./dataTakenTime);
    hitsPerChipPerSec -> Scale(1./dataTakenTime);
    std::cout << "Scaled time" << std::endl;
  }

  TH1D* effPerChip = (TH1D*)hitsPerChip -> Clone();
  effPerChip -> SetName("EffPerChip");
  effPerChip -> SetTitle("x: ChipID, y: Eff");
  effPerChip -> Scale(2.0); // Since 1 hit outputs 2 raw info(col/row)
  effPerChip -> Divide(entriesPerChip);
  hList -> Add(effPerChip);

  // Draw Plots
  // ==========================
  // Modified: arrange nine chip pads and one information pad in a 2x5 grid.
  const int nCols = 5;              // Modified: use five columns instead of one horizontal row.
  const int nRows = 2;              // Modified: use two rows.
  const int nPads = nCols*nRows;    // Modified: total number of chip/information pads.
  const int padWidth = 350;
  const double leftMargin = 0.14;
  const double rightMargin = 0.22;
  const double topMargin = 0.05;
  const double bottomMargin = 0.16;

  // Modified: calculate the height of one pad, then scale the canvas to the 2x5 layout.
  const int padHeight = static_cast<int>(padWidth*(1.0-leftMargin-rightMargin)/(1.0-topMargin-bottomMargin));
  const int canvasWidth = nCols*padWidth;     // Modified: canvas width for five columns.
  const int canvasHeight = nRows*padHeight;   // Modified: canvas height for two rows.

  TCanvas* c0 = new TCanvas("c0", "c0", canvasWidth, canvasHeight);
  TCanvas* c1 = new TCanvas("c1", "c1", canvasWidth, canvasHeight);
  TCanvas* c2 = new TCanvas("c2", "c2", 2*1.2*500, 500);
  TCanvas* c3 = new TCanvas("c3", "c3", 1.2*1000, 1000);

  // Draw Hitmap & ToT distribution of each chip
  // Modified: create pads according to the total number of cells in the 2x5 grid.
  for (int ipad=0; ipad<nPads; ipad++) {

    // Modified: convert the one-dimensional pad index into 2D row and column indices.
    const int irow = ipad/nCols;
    const int icol = ipad%nCols;

    // Modified: calculate the normalized pad boundaries for the 2x5 layout.
    const double xMin = static_cast<double>(icol)/nCols;
    const double xMax = static_cast<double>(icol+1)/nCols;
    const double yMax = 1.0 - static_cast<double>(irow)/nRows;
    const double yMin = 1.0 - static_cast<double>(irow+1)/nRows;

    // Modified: place both hitmap and ToT pads at the calculated 2D position.
    TPad* p0 = new TPad(Form("p%d_c0", ipad), Form("p%d_c0", ipad), xMin, yMin, xMax, yMax);
    TPad* p1 = new TPad(Form("p%d_c1", ipad), Form("p%d_c1", ipad), xMin, yMin, xMax, yMax);

    p0 -> SetTopMargin(topMargin);
    p0 -> SetBottomMargin(bottomMargin);
    p0 -> SetLeftMargin(leftMargin);
    p0 -> SetRightMargin(rightMargin);

    p1 -> SetTopMargin(topMargin);
    p1 -> SetBottomMargin(bottomMargin);
    p1 -> SetLeftMargin(leftMargin);
    p1 -> SetRightMargin(rightMargin);

    // Write general information of the run
    // Modified: reserve the last cell of the 2x5 grid for run information.
    if (ipad == nPads-1) {

      std::vector<std::string> infos {
        Form("Used file:"),
        Form("%s", stemOfInfile.c_str()),
      };

      TPaveText* text = new TPaveText(0.0, 0.9-0.05*infos.size(), 0.9, 0.9, "NDC");
      text -> SetFillStyle(0);
      text -> SetBorderSize(0);
      text -> SetTextSize(0.04);
      text -> SetTextAlign(12);
      for (const auto& info : infos) {
        text -> AddText(info.c_str());
      }

      c0 -> cd();
      p0 -> Draw();
      p0 -> cd();
      text -> Draw("same");

      c1 -> cd();
      p1 -> Draw();
      p1 -> cd();
      text -> Draw("same");

    } else {

      // Draw Hitmap
      c0 -> cd();
      p0 -> Draw();
      p0 -> cd();
      p0 -> SetLogz();
      TH1D* htmp0 = (TH1D*)gPad -> DrawFrame(0.0, 0.0, 35, 35);
      htmp0 -> GetXaxis() -> SetTitleFont(43);
      htmp0 -> GetXaxis() -> SetTitleSize(10);
      htmp0 -> GetXaxis() -> SetLabelFont(43);
      htmp0 -> GetXaxis() -> SetLabelSize(10);
      htmp0 -> GetXaxis() -> SetTitle("Col ID");

      htmp0 -> GetYaxis() -> SetTitleFont(43);
      htmp0 -> GetYaxis() -> SetTitleSize(10);
      htmp0 -> GetYaxis() -> SetLabelFont(43);
      htmp0 -> GetYaxis() -> SetLabelSize(10);
      htmp0 -> GetYaxis() -> SetTitle("Row ID");

      htmp0 -> Draw("same");
      hitmaps[ipad] -> Draw("same COLZ");

      TLegend* l0 = new TLegend(0.1, 0.0, 0.15, 0.15);
      l0 -> SetFillStyle(0);
      l0 -> SetBorderSize(0);
      l0 -> SetMargin(0.3);
      l0 -> SetTextFont(43);
      l0 -> SetTextSize(20);
      l0 -> AddEntry((TObject*)0, Form("ChipID %d", ipad), "");
      l0 -> Draw("same");

      // Draw ToT distribution
      c1 -> cd();
      p1 -> Draw();
      p1 -> cd();
      //float xmax = (ipad == 2) ? 0.1 : 15;
      //TH1D* htmp1 = (TH1D*)gPad -> DrawFrame(0.0, 0.0, xmax, tots[ipad] -> GetMaximum()*1.5);
      TH1D* htmp1 = (TH1D*)gPad -> DrawFrame(0.0, 0.0, 15, tots[ipad] -> GetMaximum()*1.5);
      htmp1 -> GetXaxis() -> SetTitleFont(43);
      htmp1 -> GetXaxis() -> SetTitleSize(10);
      htmp1 -> GetXaxis() -> SetLabelFont(43);
      htmp1 -> GetXaxis() -> SetLabelSize(10);
      htmp1 -> GetXaxis() -> SetTitle("Avg ToT");

      htmp1 -> GetYaxis() -> SetTitleFont(43);
      htmp1 -> GetYaxis() -> SetTitleSize(10);
      htmp1 -> GetYaxis() -> SetLabelFont(43);
      htmp1 -> GetYaxis() -> SetLabelSize(10);
      htmp1 -> GetYaxis() -> SetTitle("#Entries");

      tots[ipad] -> SetFillColor(kBlue);
      tots[ipad] -> Draw("same");

      TLegend* l1 = new TLegend(0.1, 0.0, 0.15, 0.15);
      l1 -> SetFillStyle(0);
      l1 -> SetBorderSize(0);
      l1 -> SetMargin(0.3);
      l1 -> SetTextFont(43);
      l1 -> SetTextSize(20);
      l1 -> AddEntry((TObject*)0, Form("ChipID %d", ipad), "");
      l1 -> Draw("same");

      TLegend* l2 = new TLegend(0.05, 0.8, 0.5, 0.9);
      l2 -> SetFillStyle(0);
      l2 -> SetBorderSize(0);
      l2 -> SetMargin(0.3);
      l2 -> SetTextFont(43);
      l2 -> SetTextSize(9);
      l2 -> AddEntry((TObject*)0, Form("Avg ToT = (ToT_{col}+ToT_{row})/2"), "");
      l2 -> AddEntry((TObject*)0, Form("Mean: %.2f", tots[ipad] -> GetMean()), "");
      l2 -> AddEntry((TObject*)0, Form("RMS: %.2f", tots[ipad] -> GetRMS()), "");
      l2 -> Draw("same");

    }
  }

  // Draw eff per chip
  c2 -> cd();
  TPad* p2 = new TPad("p2", "p2", 0.0, 0.0, 0.5, 1.0);
  TPad* p3 = new TPad("p3", "p3", 0.5, 0.0, 1.0, 1.0);

  p2 -> SetTopMargin(0.05);
  p2 -> SetBottomMargin(0.1);
  p2 -> SetLeftMargin(0.1);
  p2 -> SetRightMargin(0.05);
  p2 -> SetLogy();
  p2 -> SetGrid();
  p2 -> Draw();

  p3 -> SetTopMargin(0.05);
  p3 -> SetBottomMargin(0.1);
  p3 -> SetLeftMargin(0.05);
  p3 -> SetRightMargin(0.1);
  p3 -> SetGrid();
  p3 -> Draw();

  p2 -> cd();
  TH1D* htmp2 = (TH1D*)gPad -> DrawFrame(0.0, 1e-1, 9.0, entriesPerChip-> GetMaximum()*1e3);
  htmp2 -> GetXaxis() -> SetTitleFont(43);
  htmp2 -> GetXaxis() -> SetTitleSize(10);
  htmp2 -> GetXaxis() -> SetLabelFont(43);
  htmp2 -> GetXaxis() -> SetLabelSize(10);
  htmp2 -> GetXaxis() -> SetTitle("Chip ID");

  htmp2 -> GetYaxis() -> SetTitleFont(43);
  htmp2 -> GetYaxis() -> SetTitleSize(10);
  htmp2 -> GetYaxis() -> SetLabelFont(43);
  htmp2 -> GetYaxis() -> SetLabelSize(10);
  htmp2 -> GetYaxis() -> SetTitle("#Entries");

  entriesPerChip -> SetLineColor(kBlue+1);
  entriesPerChip -> SetMarkerColor(kBlue+1);
  entriesPerChip -> SetMarkerStyle(20);
  entriesPerChip -> SetFillColorAlpha(kBlue, 0.25);
  entriesPerChip -> SetFillStyle(1001);
  entriesPerChip -> Draw("same E1 BAR");

  hitsPerChip -> SetLineColor(kBlue+3);
  hitsPerChip -> SetMarkerColor(kBlue+3);
  hitsPerChip -> SetMarkerStyle(20);
  hitsPerChip -> SetFillColorAlpha(kBlue+3, 0.25);
  hitsPerChip -> SetFillStyle(1001);
  hitsPerChip -> Draw("same E1 BAR");

  TLegend* l2 = new TLegend(0.1, 0.7, 0.9, 0.9);
  l2 -> SetFillStyle(0);
  l2 -> SetBorderSize(0);
  l2 -> SetMargin(0.3);
  l2 -> SetTextFont(43);
  l2 -> SetTextSize(15);
  l2 -> AddEntry((TObject*)0, Form("Used File: %s", stemOfInfile.c_str()), "h");
  l2 -> AddEntry(entriesPerChip, Form("Total # of raw data: %.0f", entriesPerChip -> GetEntries()), "F");
  l2 -> AddEntry(hitsPerChip, Form("Total # of matched hit: %.0f", hitsPerChip -> GetEntries()), "F");
  l2 -> Draw("same");

  p3 -> cd();
  //TH1D* htmp3 = (TH1D*)gPad -> DrawFrame(0.0, 0.0, 9.0, effPerChip -> GetMaximum()*1.5);
  TH1D* htmp3 = (TH1D*)gPad -> DrawFrame(0.0, 0.0, 9.0, 1.5);
  htmp3 -> GetXaxis() -> SetTitleFont(43);
  htmp3 -> GetXaxis() -> SetTitleSize(10);
  htmp3 -> GetXaxis() -> SetLabelFont(43);
  htmp3 -> GetXaxis() -> SetLabelSize(10);
  htmp3 -> GetXaxis() -> SetTitle("Chip ID");

  htmp3 -> GetYaxis() -> SetTitleFont(43);
  htmp3 -> GetYaxis() -> SetTitleSize(10);
  htmp3 -> GetYaxis() -> SetLabelFont(43);
  htmp3 -> GetYaxis() -> SetLabelSize(10);
  htmp3 -> GetYaxis() -> SetTitle("Eff");

  effPerChip -> SetLineColor(kGreen+3);
  effPerChip -> SetMarkerColor(kGreen+3);
  effPerChip -> SetMarkerStyle(20);
  effPerChip -> SetFillColorAlpha(kGreen+3, 0.25);
  effPerChip -> SetFillStyle(1001);
  effPerChip -> Draw("same E1 TEXT BAR");

  TLegend* l3 = new TLegend(0.1, 0.8, 0.9, 0.9);
  l3 -> SetFillStyle(0);
  l3 -> SetBorderSize(0);
  l3 -> SetMargin(0.3);
  l3 -> SetTextFont(43);
  l3 -> SetTextSize(15);
  l3 -> AddEntry((TObject*)0, Form("Matching Eff_{chip i} = 2#timesN^{hit}_{chip i} / N^{raw}_{chip i}"), "h");
  l3 -> AddEntry(effPerChip, Form("Matching Eff per chip"), "F");
  l3 -> Draw("same");

  // Draw hits per sec if requested
  if (dataTakenTime != 0) {
    c3 -> cd();

    TPad* p4 = new TPad("p4", "p4", 0.0, 0.0, 1.0, 1.0);
    p4 -> SetTopMargin(0.1);
    p4 -> SetRightMargin(0.1);
    p4 -> SetBottomMargin(0.1);
    p4 -> SetLeftMargin(0.1);
    p4 -> SetLogy();
    p4 -> SetGrid();
    p4 -> Draw();

    p4 -> cd();
    TH1D* htmp = (TH1D*)gPad -> DrawFrame(0.0, 1e-3, 9.0, entriesPerChipPerSec -> GetMaximum()*1.5e2);
    htmp -> GetXaxis() -> SetLabelFont(43);
    htmp -> GetXaxis() -> SetLabelSize(20);
    htmp -> GetXaxis() -> SetTitleFont(43);
    htmp -> GetXaxis() -> SetTitleSize(20);
    htmp -> GetXaxis() -> SetTitle("Chip ID");
    htmp -> GetYaxis() -> SetLabelFont(43);
    htmp -> GetYaxis() -> SetLabelSize(20);
    htmp -> GetYaxis() -> SetTitleFont(43);
    htmp -> GetYaxis() -> SetTitleSize(20);
    htmp -> GetYaxis() -> SetTitle("Hit(Matching) rate(s^{-1})");

    entriesPerChipPerSec -> SetLineColor(kBlue+1);
    entriesPerChipPerSec -> SetMarkerColor(kBlue+1);
    entriesPerChipPerSec -> SetMarkerStyle(20);
    entriesPerChipPerSec -> SetFillColorAlpha(kBlue, 0.25);
    entriesPerChipPerSec -> SetFillStyle(1001);
    entriesPerChipPerSec -> Draw("same E1 BAR");

    hitsPerChipPerSec -> SetLineColor(kBlue+3);
    hitsPerChipPerSec -> SetMarkerColor(kBlue+3);
    hitsPerChipPerSec -> SetMarkerStyle(20);
    hitsPerChipPerSec -> SetFillColorAlpha(kBlue+3, 0.25);
    hitsPerChipPerSec -> SetFillStyle(1001);
    hitsPerChipPerSec -> Draw("same E1 BAR");

    TLegend* l4 = new TLegend(0.1, 0.7, 0.9, 0.9);
    l4 -> SetFillStyle(0);
    l4 -> SetBorderSize(0);
    l4 -> SetMargin(0.3);
    l4 -> SetTextFont(43);
    l4 -> SetTextSize(15);
    l4 -> AddEntry((TObject*)0, Form("Used File: %s", stemOfInfile.c_str()), "h");
    l4 -> AddEntry((TObject*)0, Form("Data taken time: %.f second", dataTakenTime), "h");
    l4 -> AddEntry(entriesPerChip, Form("Total # of raw data: %.0f", entriesPerChip -> GetEntries()), "F");
    l4 -> AddEntry(hitsPerChip, Form("Total # of matched hit: %.0f", hitsPerChip -> GetEntries()), "F");
    l4 -> Draw("same");
  }

  // save plots as .pdf if requested
  if (savePlots) {
    std::string saveDirName = Form("%s/QaPlots", pathOfInfile.c_str());
    if (!std::filesystem::exists(saveDirName)) {
      std::filesystem::create_directories(saveDirName);
    }

    std::string saveFileName_QA0 = Form("%s/RunQA_Plot0_Hitmap_%s.pdf", saveDirName.c_str(), stemOfInfile.c_str());
    std::string saveFileName_QA1 = Form("%s/RunQA_Plot1_Tot_%s.pdf", saveDirName.c_str(), stemOfInfile.c_str());
    std::string saveFileName_QA2 = Form("%s/RunQA_Plot2_MatchEffPerChip_%s.pdf", saveDirName.c_str(), stemOfInfile.c_str());
    std::string saveFileName_QA3 = Form("%s/RunQA_Plot3_EntriesAndHitsPerSec_%s.pdf", saveDirName.c_str(), stemOfInfile.c_str());

    // std::format supported after c++20
    //std::string saveFileName_QA0 = std::format("{}/RunQA_Plot0_Hitmap_{}.pdf", saveDirName, stemOfInfile);
    //std::string saveFileName_QA1 = std::format("{}/RunQA_Plot1_Tot_{}.pdf", saveDirName, stemOfInfile);
    //std::string saveFileName_QA2 = std::format("{}/RunQA_Plot2_EffPerChip_{}.pdf", saveDirName, stemOfInfile);

    c0 -> SaveAs(saveFileName_QA0.c_str());
    c1 -> SaveAs(saveFileName_QA1.c_str());
    c2 -> SaveAs(saveFileName_QA2.c_str());
    if (dataTakenTime != 0) {
      c3 -> SaveAs(saveFileName_QA3.c_str());
    }
  }

  // save output histogarm as .root if requested
  if (saveRoot) {
    std::string saveFileName = Form("%s/RunQA_%s.root", pathOfInfile.c_str(), stemOfInfile.c_str());
    TFile* ofile = new TFile(saveFileName.c_str(), "recreate");

    std::cout << "QA histograms will be saved in the following .root file as requested: " << ofile -> GetName() << std::endl;

    ofile -> cd();
    hList -> Write();
    ofile -> Close();
    delete ofile;
  }

}
