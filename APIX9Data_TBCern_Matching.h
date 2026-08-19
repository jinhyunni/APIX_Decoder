#ifndef APIX9DATA_TBCERN_MATCHING_H
#define APIX9DATA_TBCERN_MATCHING_H

#include "APIX9Data_TBCern.h"

// Decoded representation used by decode_cern.py.  Signed integer fields are
// intentional: a frame cut off at a readout boundary is recorded as -1.

struct APIXMatched {
  int32_t readout       = -99;
  int32_t layer         = -99;
  int32_t chipID        = -99;
  int32_t location_col  = -99;
  int32_t location_row  = -99;
  int32_t timestamp_col = -99;
  int32_t timestamp_row = -99;
  double tot_us_col     = -99;
  double tot_us_row     = -99;
  int64_t fpga_ts_col   = -99;
  int64_t fpga_ts_row   = -99;
};

struct MatchArgs {
  // Default values are taken from generated~ python code
  int32_t absDiffTimestamp = 2;
  double  relDiffTotus     = 10;
};

inline int MakeBranchOfOutTreeMatched(TTree* tree, const char* prefix, APIXMatched& m) {
  const std::string p(prefix);
  tree->Branch((p + "_readout").c_str(), &m.readout, (p + "_readout/I").c_str());
  tree->Branch((p + "_layer").c_str(), &m.layer, (p + "_layer/I").c_str());
  tree->Branch((p + "_chipID").c_str(), &m.chipID, (p + "_chipID/I").c_str());
  tree->Branch((p + "_location_col").c_str(), &m.location_col, (p + "_location_col/I").c_str());
  tree->Branch((p + "_location_row").c_str(), &m.location_row, (p + "_location_row/I").c_str());
  tree->Branch((p + "_timestamp_col").c_str(), &m.timestamp_col, (p + "_timestamp_col/I").c_str());
  tree->Branch((p + "_timestamp_row").c_str(), &m.timestamp_row, (p + "_timestamp_row/I").c_str());
  tree->Branch((p + "_tot_us_col").c_str(), &m.tot_us_col, (p + "_tot_us_col/D").c_str());
  tree->Branch((p + "_tot_us_row").c_str(), &m.tot_us_row, (p + "_tot_us_row/D").c_str());
  tree->Branch((p + "_fpga_ts_col").c_str(), &m.fpga_ts_col, (p + "_fpga_ts_col/L").c_str());
  tree->Branch((p + "_fpga_ts_row").c_str(), &m.fpga_ts_row, (p + "_fpga_ts_row/L").c_str());
  return 0;
}

inline int SetBranchAddressOfMatched(TTree* tree, const char* prefix, APIXMatched& m) {
  const std::string p(prefix);
  tree->SetBranchAddress((p + "_readout").c_str(), &m.readout);
  tree->SetBranchAddress((p + "_layer").c_str(), &m.layer);
  tree->SetBranchAddress((p + "_chipID").c_str(), &m.chipID);
  tree->SetBranchAddress((p + "_location_col").c_str(), &m.location_col);
  tree->SetBranchAddress((p + "_location_row").c_str(), &m.location_row);
  tree->SetBranchAddress((p + "_timestamp_col").c_str(), &m.timestamp_col);
  tree->SetBranchAddress((p + "_timestamp_row").c_str(), &m.timestamp_row);
  tree->SetBranchAddress((p + "_tot_us_col").c_str(), &m.tot_us_col);
  tree->SetBranchAddress((p + "_tot_us_row").c_str(), &m.tot_us_row);
  tree->SetBranchAddress((p + "_fpga_ts_col").c_str(), &m.fpga_ts_col);
  tree->SetBranchAddress((p + "_fpga_ts_row").c_str(), &m.fpga_ts_row);
  return 0;
}

inline bool RejectInvalidData(APIXData& rawdata) {

  if (rawdata.readout == -1)
    return false;
  if (rawdata.layer == -1)
    return false;
  if (rawdata.chipID == -1)
    return false;
  if (rawdata.payload == -1)
    return false;
  if (rawdata.location == -1)
    return false;
  if (rawdata.timestamp == -1)
    return false;
  if (rawdata.tot_msb == -1)
    return false;
  if (rawdata.tot_lsb == -1)
    return false;
  if (rawdata.tot_us == -1.0)
    return false;
  if (rawdata.fpga_ts == -1)
    return false;

  // Reaching here means all the data formats are not destroyed
  return true;
}

inline void MatchHits(TTree* iTree, TTree* oTree, APIXMatched& matchedData, const char* suffix_iTree, MatchArgs& args) {
  
  int nMatched{0};

  APIXData hits{};
  SetBranchAddressOfInputTree(iTree, "apix", hits);

  //APIXMatched matchedData{};
  //SetBranchAddressOfMatched(oTree, "apix_matched", matchedData);

  // Loop over readouts
  int32_t nReadouts = iTree -> GetMaximum(Form("%s_readout", suffix_iTree));

  for (int32_t iReadout=0; iReadout < nReadouts+1; iReadout++) {

    // Divide inTree with this Readout & col & row
    TTree* treeCurrentReadout = (TTree*)iTree -> CopyTree(Form("%s_readout == %d && %s_payload == 4", suffix_iTree, iReadout, suffix_iTree));
    TTree* treeCurrentReadout_col = (TTree*)treeCurrentReadout -> CopyTree(Form("%s_isCol == 1", suffix_iTree));  
    TTree* treeCurrentReadout_row = (TTree*)treeCurrentReadout -> CopyTree(Form("%s_isCol == 0", suffix_iTree));
    std::vector<bool> rowUsed(treeCurrentReadout_row->GetEntries(), false);

    std::cout << Form("%d-Readout, Total entries: %lld (Col %lld/ Row %lld/ Sum %lld)", 
                      iReadout, treeCurrentReadout->GetEntries(), 
                      treeCurrentReadout_col->GetEntries(), treeCurrentReadout_row->GetEntries(),
                      treeCurrentReadout_col->GetEntries()+treeCurrentReadout_row->GetEntries()) << std::endl;

    APIXData hitsCol{}, hitsRow{};
    SetBranchAddressOfInputTree(treeCurrentReadout_col, suffix_iTree, hitsCol);
    SetBranchAddressOfInputTree(treeCurrentReadout_row, suffix_iTree, hitsRow);

    for (int iCol=0; iCol<treeCurrentReadout_col->GetEntries(); iCol++) {
      treeCurrentReadout_col -> GetEntry(iCol);
      // Reject invalide data
      if (hitsCol.location > 34) continue;
      if (hitsCol.location < 3) continue; // Reject hits from Col0-2
      if (hitsCol.layer == 0 || hitsCol.layer > 4) continue;
      if (hitsCol.chipID > 8) continue;
      if (hitsCol.tot_us == 0) continue; // -> Test: Turn off tot_us rejection

      for (int iRow=0; iRow<treeCurrentReadout_row->GetEntries(); iRow++) {
        treeCurrentReadout_row -> GetEntry(iRow);
        // Reject invalide data
        if (hitsRow.location > 34) continue;
        if (hitsRow.layer == 0 || hitsRow.layer > 4) continue;
        if (hitsRow.chipID > 8) continue;
        if (hitsRow.tot_us == 0) continue; // -> Test: Turn off tot_us rejection

        // Reject hits & column in differnet layers
        if (hitsCol.layer != hitsRow.layer) continue;

        // Reject hit & column in different ChipID
        if (hitsCol.chipID != hitsRow.chipID) continue;

        // Calculate properties for hit-matching
        int32_t abs_diff_timestamp = std::abs(hitsCol.timestamp - hitsRow.timestamp);
        double abs_diff_tot_us = std::abs(hitsCol.tot_us - hitsRow.tot_us);
        double rel_diff_tot_us = abs_diff_tot_us / hitsCol.tot_us * 100;

        if ((abs_diff_timestamp >= args.absDiffTimestamp) || (rel_diff_tot_us >= args.relDiffTotus)) continue;
        // Row - Column pair reached here are mathced!
        if (rowUsed[iRow]) continue; // -> Reject if this row entry was used to match hits in different column
        rowUsed[iRow] = true; // Set flag to indicate this row was used to match hit
      
        // Fill matched ttree
        matchedData.readout       = hitsCol.readout;
        matchedData.layer         = hitsCol.layer;
        matchedData.chipID        = hitsCol.chipID;
        matchedData.location_col  = hitsCol.location;
        matchedData.location_row  = hitsRow.location;
        matchedData.timestamp_col = hitsCol.timestamp;
        matchedData.timestamp_row = hitsRow.timestamp;
        matchedData.tot_us_col    = hitsCol.tot_us;
        matchedData.tot_us_row    = hitsRow.tot_us;
        matchedData.fpga_ts_col   = hitsCol.fpga_ts;
        matchedData.fpga_ts_row   = hitsRow.fpga_ts;

        oTree -> Fill();
        nMatched++;

        break;
      }// loop Row
    }// loop Column
  }// loop readout

  std::cout << "Total entries of matching: " << nMatched << std::endl;
}

#endif
