#include <CaenEndpoint.h>
#include <CaenData.h>
#include <CaenDigitizer.h>
#include <iostream>

bool CaenEndpoint::ParseReturnCode(int code){
  if (code == CAEN_FELib_Success)
    return true;
  else if (code == CAEN_FELib_Timeout)
    return false;
  else if (code == CAEN_FELib_Stop){
    status = ReadoutStatus::Stopped;
    return false;
  } else {
    throw CaenException(code);
  }

}

void CaenEndpoint::Start() noexcept {
    status = ReadoutStatus::Running;
}

bool CaenEndpoint::HasData(){
  if(ep_handle){
    int ret = CAEN_FELib_HasData(ep_handle, timeout);
    return ParseReturnCode(ret);
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
  auto event = std::make_unique<CaenRawData>(maxrawdatasize);

  auto ret = CAEN_FELib_ReadData(ep_handle, timeout,
                                 event->data.data(),
                                 &event->size,
                                 &event->nevents
                                );


  // Raw endpoint does not handle start and stop events
  if(ret == CAEN_FELib_Success){
    if(event->data[0]== 0x32){
      //stop received
      status = ReadoutStatus::Stopped;
    }
  }

  if(ParseReturnCode(ret))
    return event;
  else
    return std::make_unique<CaenRawData>(1);
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
  auto event = std::make_unique<CaenScopeData>(numch, recordlengths);
  auto ret = CAEN_FELib_ReadData(ep_handle, timeout,
                                 &event->timestamp,
                                 &event->trigger_id,
                                 event->waveform.data(),
                                 event->waveform_size.data(),
                                 &event->flags
                                );


  if(ParseReturnCode(ret))
    return event;
  else
    return std::make_unique<CaenScopeData>(numch, 0);
}
