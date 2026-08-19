#ifndef APIX9DATA_TBCERN_H
#define APIX9DATA_TBCERN_H

#include <cstddef>
#include <cstdint>
#include <string>

// Decoded representation used by decode_cern.py.  Signed integer fields are
// intentional: a frame cut off at a readout boundary is recorded as -1.
struct APIXData {
    int32_t readout;
    int32_t layer;
    int32_t chipID;
    int32_t payload;
    int32_t location;
    bool isCol;
    int32_t timestamp;
    int32_t tot_msb;
    int32_t tot_lsb;
    int32_t tot_total;
    double tot_us;
    int64_t fpga_ts;
};

inline void SetInvalidAPIXData(APIXData& out, int32_t readout) {
    out.readout = readout;
    out.layer = -1;
    out.chipID = -1;
    out.payload = -1;
    out.location = -1;
    // decode_cern.py first assigns col=-1, then bool-converts it when creating
    // the dictionary.  bool(-1) is True.
    out.isCol = true;
    out.timestamp = -1;
    out.tot_msb = -1;
    out.tot_lsb = -1;
    out.tot_total = -1;
    out.tot_us = -1.0;
    out.fpga_ts = -1;
}

// 'data' includes the packet-length byte at data[0].
inline bool ConvertAPIXData(const uint8_t* data, std::size_t size,
                            int32_t readout, APIXData& out) {
    // Python scalar indexing succeeds once bytes 0..6 exist.  Its hit[7:11]
    // slice may be short (or empty) without raising IndexError.
    if (size < 7) {
        SetInvalidAPIXData(out, readout);
        return false;
    }

    out.readout = readout;
    out.layer = data[1];
    out.chipID = data[2] >> 3;
    out.payload = data[2] & 0x07;
    out.location = data[3] & 0x3f;
    out.isCol = ((data[3] >> 7) & 0x01) != 0;
    out.timestamp = data[4];
    out.tot_msb = data[5] & 0x0f;
    out.tot_lsb = data[6];
    out.tot_total = (out.tot_msb << 8) | out.tot_lsb;
    out.tot_us = out.tot_total * 5.0 / 1000.0;

    // CERN firmware: FPGA timestamp is stored big-endian.
    out.fpga_ts = 0;
    for (std::size_t i = 7; i < size && i < 11; ++i) {
        out.fpga_ts = (out.fpga_ts << 8) | data[i];
    }
    return true;
}

inline int MakeBranchOfOutputTree(TTree* tree, const char* prefix, APIXData& d) {
    const std::string p(prefix);
    tree->Branch((p + "_readout").c_str(), &d.readout, (p + "_readout/I").c_str());
    tree->Branch((p + "_layer").c_str(), &d.layer, (p + "_layer/I").c_str());
    tree->Branch((p + "_chipID").c_str(), &d.chipID, (p + "_chipID/I").c_str());
    tree->Branch((p + "_payload").c_str(), &d.payload, (p + "_payload/I").c_str());
    tree->Branch((p + "_location").c_str(), &d.location, (p + "_location/I").c_str());
    tree->Branch((p + "_isCol").c_str(), &d.isCol, (p + "_isCol/O").c_str());
    tree->Branch((p + "_timestamp").c_str(), &d.timestamp, (p + "_timestamp/I").c_str());
    tree->Branch((p + "_tot_msb").c_str(), &d.tot_msb, (p + "_tot_msb/I").c_str());
    tree->Branch((p + "_tot_lsb").c_str(), &d.tot_lsb, (p + "_tot_lsb/I").c_str());
    tree->Branch((p + "_tot_total").c_str(), &d.tot_total, (p + "_tot_total/I").c_str());
    tree->Branch((p + "_tot_us").c_str(), &d.tot_us, (p + "_tot_us/D").c_str());
    tree->Branch((p + "_fpga_ts").c_str(), &d.fpga_ts, (p + "_fpga_ts/L").c_str());
    return 0;
}

inline int SetBranchAddressOfInputTree(TTree* tree, const char* prefix, APIXData& d) {
    const std::string p(prefix);
    tree->SetBranchAddress((p + "_readout").c_str(), &d.readout);
    tree->SetBranchAddress((p + "_layer").c_str(), &d.layer);
    tree->SetBranchAddress((p + "_chipID").c_str(), &d.chipID);
    tree->SetBranchAddress((p + "_payload").c_str(), &d.payload);
    tree->SetBranchAddress((p + "_location").c_str(), &d.location);
    tree->SetBranchAddress((p + "_isCol").c_str(), &d.isCol);
    tree->SetBranchAddress((p + "_timestamp").c_str(), &d.timestamp);
    tree->SetBranchAddress((p + "_tot_msb").c_str(), &d.tot_msb);
    tree->SetBranchAddress((p + "_tot_lsb").c_str(), &d.tot_lsb);
    tree->SetBranchAddress((p + "_tot_total").c_str(), &d.tot_total);
    tree->SetBranchAddress((p + "_tot_us").c_str(), &d.tot_us);
    tree->SetBranchAddress((p + "_fpga_ts").c_str(), &d.fpga_ts);
    return 0;
}

#endif
