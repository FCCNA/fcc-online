#include <CaenEndpoint.h>
#include <CaenData.h>
#include <CaenDigitizer.h>
#include <iostream>

bool CaenEndpoint::HasData(){
  if(ep_handle){
    int ret = CAEN_FELib_HasData(ep_handle, timeout);
    if (ret == CAEN_FELib_Success)
      return true;
    else if (ret == CAEN_FELib_Timeout)
      return false;
    else if (ret == CAEN_FELib_Stop){
      //stop_rreceived = true;
      return false;
    } else {
      throw CaenException(ret);
    }
  }

  return false;
}
std::unique_ptr<CaenData> CaenEndpoint::ReadData() {
  return std::unique_ptr<CaenData>{}; 
};


void CaenRawEndpoint::Configure() {
  if (auto digitizer = dgtz.lock()) {
    auto root = digitizer->GetRootParameter();
    maxrawdatasize = (uint64_t)root["/par/maxrawdatasize"];
    std::cout << "Max Raw Datasize: " << maxrawdatasize << std::endl;
  } else {
    std::cout << "Cannot lock CaenDigitizer pointer" << std::endl;
  }
}

std::unique_ptr<CaenData> CaenRawEndpoint::ReadData() {
  return std::make_unique<CaenRawData>(maxrawdatasize);
}

void CaenScopeEndpoint::Configure() {
  if (auto digitizer = dgtz.lock()) {
    auto root = digitizer->GetRootParameter();
    numch = (uint64_t)root["/par/numch"];
    recordlengths = (uint64_t)root["/par/recordlengths"];
    std::cout << "Numch: " << numch << " recordlengths: " << recordlengths << std::endl;
  } else {
    std::cout << "Cannot lock CaenDigitizer pointer" << std::endl;
  }
}

std::unique_ptr<CaenData> CaenScopeEndpoint::ReadData() {
  return std::make_unique<CaenScopeData>(numch, recordlengths);
}
