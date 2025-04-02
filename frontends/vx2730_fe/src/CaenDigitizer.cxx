#include "CaenDigitizer.h"
#include "CaenException.h"
#include <stdexcept>

std::shared_ptr<CaenDigitizer> CaenDigitizer::MakeNewDigitizer() {
  //return std::make_shared<CaenDigitizer>();
  return std::shared_ptr<CaenDigitizer>(new CaenDigitizer());
}

CaenDigitizer::CaenDigitizer() {}


CaenDigitizer::~CaenDigitizer() {
    if (connected) {
        Disconnect();
    }
}

void CaenDigitizer::Connect(std::string hostname, std::string protocol) {
    std::string connection;
    if ((hostname.rfind("Dig1:", 0) == 0) || (hostname.rfind("Dig2:", 0) == 0))
        connection = hostname;
    else
        connection = protocol + hostname;

    dev_path = hostname;

    int result = CAEN_FELib_Open(connection.c_str(), &dev_handle);

    if (result != CAEN_FELib_Success) {
        throw CaenException(result);
    }

    connected = true;
    root.SetHandle(dev_handle);
}

void CaenDigitizer::Disconnect() {
    connected = false;
    int result = CAEN_FELib_Close(dev_handle);

    if (result != CAEN_FELib_Success) {
        throw CaenException(result);
    }
}

bool CaenDigitizer::IsConnected() {
    return connected;
}

void CaenDigitizer::ConfigureEndpoint(std::unique_ptr<CaenEndpoint> endpoint){
  if(!connected) //Digitizer have to be connected otherwise just drop the endpoint
    return;

  endpt = std::move(endpoint);
  //std::cout << "Endpoint Moved" << std::endl;

  uint64_t ep_handle;
  std::string full_endpoint_path = "/endpoint/" + endpt->GetNameString();
  int ret = CAEN_FELib_GetHandle(dev_handle, full_endpoint_path.c_str(), &ep_handle);
  if (ret != CAEN_FELib_Success) {
    throw CaenException(ret);
  }
  //std::cout << "Got endpoint handle" << std::endl;

	ret = CAEN_FELib_SetValue(dev_handle, "/endpoint/par/activeendpoint", endpt->GetName());
  if (ret != CAEN_FELib_Success) {
    throw CaenException(ret);
  }
  //std::cout << "ActiveEndpoint Value set" << std::endl;

	ret = CAEN_FELib_SetReadDataFormat(ep_handle, endpt->GetFormat());
  if (ret != CAEN_FELib_Success) {
    throw CaenException(ret);
  }
  //std::cout << "ReadDataFormat set" << std::endl;

  auto test = weak_from_this();
  endpt->dgtz = weak_from_this();
  endpt->ep_handle = ep_handle;

  endpt->Configure();
}

void CaenDigitizer::RunCmd(std::string cmd) {
    std::string full_cmd_path = "/cmd/" + cmd;
    int result = CAEN_FELib_SendCommand(dev_handle, full_cmd_path.c_str());

    if (result != CAEN_FELib_Success) {
        throw CaenException(result);
    }
}
