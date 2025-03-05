#include <string>

#ifndef CANE_H
#define CANE_H

#define DOG_NCHANNEL 32

class cane
{
protected:
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


    void WriteCmd(const std::string &cmd);
    std::string ReadCmd(const std::string &cmd);
    void QueryState();

public:
    cane();
    void Start();
    void Stop();
    void Connect(const std::string &ip, int port);
    bool HasEvent();
    bool ReadData();
};

#endif
