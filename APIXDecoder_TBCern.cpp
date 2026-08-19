#include "APIX9Data_TBCern.h"
#include "APIX9Data_TBCern_Matching.h"

void APIXDecoder_TBCern(const std::string& binfile, 
                        int32_t diffTimestamp = 2, 
                        double diffTotus = 10) {

  // Check if input file exists & can be accessed without errors
  // ===========================================================
  if (!std::filesystem::exists(binfile)) {
      std::cerr << "Input file not found: " << binfile << '\n';
      return;
  }

  std::ifstream input(binfile, std::ios::binary);
  if (!input) {
      std::cerr << "Failed to open input file: " << binfile << '\n';
      return;
  }

  // Make output file
  // ================
  std::filesystem::path fullPath = binfile;
  std::string pathOfInfile = fullPath.parent_path().string();
  std::string stemOfInfile = fullPath.stem().string();

  TFile* output = new TFile(Form("%s/%s.root", pathOfInfile.c_str(), stemOfInfile.c_str()), "recreate");
  if (output->IsZombie()) {
      std::cerr << "Failed to create output file: " << std::endl;
      return;
  }
  
  TTree* tree = new TTree("apix_tree", "Decoded APIX9 CERN-firmware data");
  TTree* tHit = new TTree("apix_tree_matched", "Mathced Hits");
  APIXData hit{};
  APIXMatched hitMatched{};
  MakeBranchOfOutputTree(tree, "apix", hit);
  MakeBranchOfOutTreeMatched(tHit, "apix", hitMatched);

  // The acquisition program passed 4096-byte readouts separately to
  // decode_cern.py.  Resetting the packet scan at each boundary is required
  // to reproduce its CSV exactly.
  constexpr std::size_t kReadoutSize = 4096;
  std::array<uint8_t, kReadoutSize> readoutBuffer{};
  int32_t readoutIndex = 0;

  while (input) {
      input.read(reinterpret_cast<char*>(readoutBuffer.data()),
                 readoutBuffer.size());
      const std::size_t bytesRead = static_cast<std::size_t>(input.gcount());
      if (bytesRead == 0) break;

      std::size_t pos = 0;
      while (pos < bytesRead) {
          const std::size_t packetLength = readoutBuffer[pos];
          if (packetLength > 16) {
              ++pos;
              continue;
          }

          const std::size_t available = bytesRead - pos;
          const std::size_t frameSize =
              (packetLength + 1 < available) ? packetLength + 1 : available;
          ConvertAPIXData(readoutBuffer.data() + pos, frameSize,
                          readoutIndex, hit);
          tree->Fill();
          pos += packetLength + 1;
      }
      ++readoutIndex;
  }

  output->cd();
  tree->Write();

  std::cout << "Decoding is done...Proceed to hit matching" << std::endl;
  MatchArgs args{diffTimestamp, diffTotus};
  MatchHits(tree, tHit, hitMatched, "apix", args);

  output -> cd();
  tHit -> Write();
  output -> Close();
  
  std::cout << "Hit mathcing is done" << std::endl;
  delete output;

}

