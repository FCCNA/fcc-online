#ifndef CAEN_DIGITIZER_MIDAS_H
#define CAEN_DIGITIZER_MIDAS_H

#include <iostream>
#include <thread>
#include <chrono>
#include "midas.h"
#include "msystem.h"
#include "odbxx.h"
#include "CaenDigitizer.h"

using namespace std::chrono_literals;

class CaenDigitizerMidas {
private:
    std::shared_ptr<CaenDigitizer> digitizer;

    midas::odb fOdbSettings{};
    midas::odb fOdbVariables{};
    midas::odb fOdbStatus{};

    //odb keys for watch
    midas::odb fOdbDigitizerSettings;
    midas::odb fOdbDigitizerStatus;
    std::vector<midas::odb> fOdbChannelSettings;

    //thread to call Sync() periodically
    std::atomic<bool> runSyncThread;
    std::thread syncThread;
    void SyncThreadFunction();

    const int fFrontendIndex;
    const EQUIPMENT* fMidasEquipment;

    void parameterToOdb(midas::odb& odb, CaenParameter& param);
    void odbToParameter(CaenParameter& param, midas::odb& odb);

public:
    CaenDigitizerMidas(int index, EQUIPMENT* eq);
    void Sync(); //populate ODB with parameters

    //EUDAQ-style transitions
    enum class DaqState {Uninitialized, Unconfigured, Configured, Running, Error};
    DaqState state = DaqState::Uninitialized;

    INT Initialize();
    INT Terminate();
    INT Configure();
    INT StartRun();
    INT StopRun();

    INT HasData();
    INT ReadData(char* pevent);

    void SettingsCallback(midas::odb &o);
    void ChannelCallback(int channel, midas::odb &o);
};

#endif // CAEN_DIGITIZER_MIDAS_H
