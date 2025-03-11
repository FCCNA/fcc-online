#include "CaenDigitizer.h"
#include "CaenDigitizerMidas.h"
#include "odbxx.h"


CaenDigitizerMidas::CaenDigitizerMidas() {
    midas::odb settings = {
        {"IP Address", "192.168.50.22"},
        {"IP Port", 4000},
        {"Channel Enable", false},
        {"Channel Position", 1.},
        {"Channel Offset", 1.},
        {"Channel Scale", 1.},
        {"Channel Bandwidth", 1.},
        {"Horizontal Position", 1.},
        {"Horizontal Scale", 1.},
        {"Sample Rate", 1.},
        {"Acquisition Mode", "SAMPLE"}};
    settings.connect("/Equipment/EquipmentGaudino/Settings");

    fOdbSettings.connect("/Equipment/EquipmentGaudino/Settings");
    fOdbSettings["Channel Enable"].resize(DOG_NCHANNEL);
    fOdbSettings["Channel Position"].resize(DOG_NCHANNEL);
    fOdbSettings["Channel Offset"].resize(DOG_NCHANNEL);
    fOdbSettings["Channel Scale"].resize(DOG_NCHANNEL);
    fOdbSettings["Channel Bandwidth"].resize(DOG_NCHANNEL);

    midas::odb variables = {
        {"Channel Status", "Qualcosa"},
        {"Channel Info", "Qualcosa"}};
    variables.connect("/Equipment/EquipmentGaudino/Variables");
    fOdbVariables.connect("/Equipment/EquipmentGaudino/Variables");
    fOdbVariables["Channel Status"].resize(DOG_NCHANNEL);
    fOdbVariables["Channel Info"].resize(DOG_NCHANNEL);
}

void CaenDigitizerMidas::Connect(std::string hostname, std::string protocol) {
    digitizer.Connect(hostname, protocol);
}

void CaenDigitizerMidas::Disconnect() {
    digitizer.Disconnect();
}

bool CaenDigitizerMidas::IsConnected() {
    return digitizer.IsConnected();
}

void CaenDigitizerMidas::RunCmd(std::string cmd) {
    digitizer.RunCmd(cmd);
}

void CaenDigitizerMidas::StateCallback(midas::odb &o) {
    std::cout << "ODB State changed: " << o.get_full_path() << " -> " << o << std::endl;
}

void CaenDigitizerMidas::SetupCallback() {
    fOdbSettings.watch([this](midas::odb &arg) { this->StateCallback(arg); });
}

void CaenDigitizerMidas::QueryState() {
    for (int i = 0; i < DOG_NCHANNEL; i++) {
        fChannelEnabled[i] = true;
        fChannelPosition[i] = std::stof("1.0");
        fChannelOffset[i] = std::stof("1.0");
        fChannelScale[i] = std::stof("1.0");
        fChannelBandwidth[i] = std::stof("1.0");
        fChannelInfo[i] = "info";
        fChannelStatus[i] = "status";
    }
    fHorizontalPosition = std::stof("1.0");
    fHorizontalScale = std::stof("1.0");
    fHorizontalSampleRate = std::stof("1.0");
    fAcquisitionMode = "SAMPLE";
}

void CaenDigitizerMidas::Start() {
    std::cout << "Cane Start()" << std::endl;
    AlignODB();
    fOdbSettings.set_trigger_hotlink(false);
    SetupCallback();
}

void CaenDigitizerMidas::Stop() {
    std::cout << "Cane Stop()" << std::endl;
}

void CaenDigitizerMidas::AlignODB() {
    QueryState();
    for (int i = 0; i < DOG_NCHANNEL; i++) {
        fOdbSettings["Channel Enable"][i] = fChannelEnabled[i];
        if (fChannelEnabled[i]) {
            fOdbSettings["Channel Position"][i] = fChannelPosition[i];
            fOdbSettings["Channel Offset"][i] = fChannelOffset[i];
            fOdbSettings["Channel Scale"][i] = fChannelScale[i];
            fOdbSettings["Channel Bandwidth"][i] = fChannelBandwidth[i];
            fOdbVariables["Channel Info"][i] = fChannelInfo[i];
            fOdbVariables["Channel Status"][i] = fChannelStatus[i];
        }
    }
    fOdbSettings["Horizontal Position"] = fHorizontalPosition;
    fOdbSettings["Horizontal Scale"] = fHorizontalScale;
    fOdbSettings["Sample Rate"] = fHorizontalSampleRate;
    fOdbSettings["Acquisition Mode"] = fAcquisitionMode.substr(0, 5);
}
