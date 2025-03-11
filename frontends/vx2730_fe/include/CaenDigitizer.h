#ifndef CAEN_DIGITIZER_H
#define CAEN_DIGITIZER_H

#include "CaenException.h"
#include "CaenParameter.h"
#include <string>

class CaenDigitizer {
public:
    CaenDigitizer();
    ~CaenDigitizer();

    void Connect(std::string hostname, std::string protocol = "Dig2:");
    void Disconnect();
    bool IsConnected();

    CaenParameter GetRootParameter() const { return root; }

    std::string GetName() {
        return dev_path;
    }

    void RunCmd(std::string cmd);
    void WriteCmd(const std::string &cmd);

private:
    bool connected = false;
    uint64_t dev_handle = 0;
    std::string dev_path;
    CaenParameter root;
};

#endif // CAEN_DIGITIZER_H
