#ifndef CAEN_DIGITIZER_H
#define CAEN_DIGITIZER_H

#include "CaenException.h"
#include "CaenParameter.h"
#include "CaenEndpoint.h"
#include <string>
#include <memory>

class CaenDigitizer: public std::enable_shared_from_this<CaenDigitizer>{
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

    void ConfigureEndpoint(std::unique_ptr<CaenEndpoint> endpoint);

    void RunCmd(std::string cmd);
    void WriteCmd(const std::string &cmd);

private:
    bool connected = false;
    uint64_t dev_handle = 0;
    std::string dev_path;
    CaenParameter root;
    std::unique_ptr<CaenEndpoint> endpt = nullptr;
};

#endif // CAEN_DIGITIZER_H
