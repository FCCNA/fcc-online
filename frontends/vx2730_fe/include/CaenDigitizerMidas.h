#ifndef CAEN_DIGITIZER_MIDAS_H
#define CAEN_DIGITIZER_MIDAS_H

#include <iostream>
#include "midas.h"
#include "msystem.h"
#include "odbxx.h"
#include "CaenDigitizer.h"

#define DOG_NCHANNEL 32

class CaenDigitizerMidas {
private:
    std::shared_ptr<CaenDigitizer> digitizer;
    midas::odb fOdbSettings{};
    midas::odb fOdbVariables{};

    bool fChannelEnabled[DOG_NCHANNEL];
    float fChannelScale[DOG_NCHANNEL];
    float fChannelPosition[DOG_NCHANNEL];
    float fChannelOffset[DOG_NCHANNEL];
    float fChannelBandwidth[DOG_NCHANNEL];
    std::string fChannelStatus[DOG_NCHANNEL];
    std::string fChannelInfo[DOG_NCHANNEL];

    float fHorizontalScale;
    float fHorizontalPosition;
    float fHorizontalSampleRate;
    std::string fAcquisitionMode;

public:
    CaenDigitizerMidas();

    void Connect(std::string hostname, std::string protocol);
    void Disconnect();
    bool IsConnected();
    void RunCmd(std::string cmd);

    void QueryState();
    void Start();
    void Stop();
    bool HasEvent();
    bool ReadData();
    void AlignODB();

    void StateCallback(midas::odb &o);
    void SetupCallback();
};

#endif // CAEN_DIGITIZER_MIDAS_H
