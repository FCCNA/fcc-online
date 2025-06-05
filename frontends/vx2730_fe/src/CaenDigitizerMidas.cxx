#include "CaenDigitizer.h"
#include "CaenDigitizerMidas.h"
#include "odbxx.h"


CaenDigitizerMidas::CaenDigitizerMidas(int index, EQUIPMENT* eq): fFrontendIndex(index), fMidasEquipment(eq), fOdbSettings({{"Hostname", "192.168.50.22"}, {"Protocol", "Dig2:"}}) {
  fOdbSettings.connect("/Equipment/"+std::string(fMidasEquipment->name)+"/Settings");
  fOdbVariables.connect("/Equipment/"+std::string(fMidasEquipment->name)+"/Variables");
  fOdbStatus.connect("/Equipment/"+std::string(fMidasEquipment->name)+"/Status");

  digitizer = CaenDigitizer::MakeNewDigitizer();
}

void CaenDigitizerMidas::parameterToOdb(midas::odb& odb, CaenParameter& param){
  if(param.GetName()=="itlamask" || param.GetName()=="itlbmask"){
    odb.set_trigger_hotlink(false);
    auto value = (uint64_t)param;
    odb[param.GetName()] = value;
    odb.set_trigger_hotlink(true);
  }

  odb.set_trigger_hotlink(false);
  switch(param.GetDataType()){
  case CaenParameter::DataType::Boolean:
    {
      auto value = (bool)param;
      odb[param.GetName()] = value;
    }
    break;
  case CaenParameter::DataType::Integer:
    {
      auto value = (int64_t)param;
      odb[param.GetName()] = value;
    }
    break;
  case CaenParameter::DataType::Positive:
    {
      auto value = (uint64_t)param;
      odb[param.GetName()] = value;
    }
    break;
  case CaenParameter::DataType::Floating:
    {
      auto value = (double)param;
      odb[param.GetName()] = value;
    }
    break;
  case CaenParameter::DataType::String:
    {
      auto value = (std::string)param;
      odb[param.GetName()] = value;
    }
    break;
  default:
    break;
  }
  odb.set_trigger_hotlink(true);
}

void CaenDigitizerMidas::odbToParameter(CaenParameter& param, midas::odb& odb){
  switch(param.GetDataType()){
  case CaenParameter::DataType::Boolean:
    {
      param = (bool)odb;
    }
    break;
  case CaenParameter::DataType::Integer:
    {
      param = (int)odb;
    }
    break;
  case CaenParameter::DataType::Positive:
    {
      param = (uint64_t)odb;
    }
    break;
  case CaenParameter::DataType::Floating:
    {
      param = (double)odb;
    }
    break;
  case CaenParameter::DataType::String:
    {
      param = (std::string)odb;
    }
    break;
  default:
    break;
  }
}

INT CaenDigitizerMidas::Initialize() {
  if(state != DaqState::Uninitialized)
    return FE_ERR_DRIVER;

  std::string hostname = fOdbSettings["Hostname"];
  std::string protocol = fOdbSettings["Protocol"];
  try{
    digitizer->Connect(hostname, protocol);
  } catch(CaenException &ex){
    std::cout << "Error opening the digitizer" <<std::endl;
    std::cout << "Error " << ex.GetName() << ": " << ex.GetDescription() << std::endl;
    state = DaqState::Error;
    return FE_ERR_HW;
  }

  try{
    Sync();
    std::function<void(midas::odb&)> f = std::bind(&CaenDigitizerMidas::SettingsCallback, this, std::placeholders::_1);
    fOdbDigitizerSettings.watch(f);
    int i=0;
    for(auto& ch: fOdbChannelSettings){
      std::function<void(midas::odb&)> fCh = std::bind(&CaenDigitizerMidas::ChannelCallback, this, i, std::placeholders::_1);
      ch.watch(fCh);
      i++;
    }

  } catch(CaenException &ex){
    std::cout << "Error popularing ODB" <<std::endl;
    std::cout << "Error " << ex.GetName() << ": " << ex.GetDescription() << std::endl;
    state = DaqState::Error;
    return FE_ERR_ODB;
  }

  //start Sync thread
  runSyncThread = true;
  syncThread = std::thread(std::bind(&CaenDigitizerMidas::SyncThreadFunction, this));

  state = DaqState::Unconfigured;
  return SUCCESS;
}

INT CaenDigitizerMidas::Terminate() {
  runSyncThread = false;
  syncThread.join();
  digitizer->Disconnect();
  state = DaqState::Uninitialized;
  return SUCCESS;
}

void CaenDigitizerMidas::SettingsCallback(midas::odb &o) {
  std::cout << "ODB State changed: " << o.get_name() << " -> " << o << std::endl;
  auto rootParameter = digitizer->GetRootParameter();
  auto myParameter = rootParameter["/par/"+ o.get_name()];

  odbToParameter(myParameter, o);
}

void CaenDigitizerMidas::ChannelCallback(int channel, midas::odb &o) {
  std::cout << "ODB State changed for channel "<< channel <<": " << o.get_name() << " -> " << o << std::endl;
  auto rootParameter = digitizer->GetRootParameter();
  auto myParameter = rootParameter["/ch/"+std::to_string(channel) + "/par/"+ o.get_name()];

  odbToParameter(myParameter, o);
}

INT CaenDigitizerMidas::Configure() {
  if(state != DaqState::Unconfigured && state != DaqState::Configured)
    return FE_ERR_DRIVER;

  //setup endpoints
  try{
    //digitizer->ConfigureEndpoint(std::make_unique<CaenRawEndpoint>());
    digitizer->ConfigureEndpoint(std::make_unique<CaenScopeEndpoint>());
    digitizer->RunCmd("armacquisition");
  } catch(CaenException &ex){
    std::cout << "Error configuring endpoint" <<std::endl;
    std::cout << "Error " << ex.GetName() << ": " << ex.GetDescription() << std::endl;
    state = DaqState::Error;
    return FE_ERR_HW;
  }

  state = DaqState::Configured;
  return SUCCESS;
}

INT CaenDigitizerMidas::StartRun() {
  if(state != DaqState::Configured)
    return FE_ERR_DRIVER;

  try{
    Sync();
    digitizer->RunCmd("swstartacquisition");
    digitizer->Start();
  } catch(CaenException &ex){
    std::cout << "Error starting run" <<std::endl;
    std::cout << "Error " << ex.GetName() << ": " << ex.GetDescription() << std::endl;
    state = DaqState::Error;
    return FE_ERR_HW;
  }

  state = DaqState::Running;
  return SUCCESS;
}

INT CaenDigitizerMidas::StopRun() {
  try{
    digitizer->RunCmd("swstopacquisition");
    digitizer->RunCmd("disarmacquisition");
  } catch(CaenException &ex){
    std::cout << "Error stopping run" <<std::endl;
    std::cout << "Error " << ex.GetName() << ": " << ex.GetDescription() << std::endl;
    state = DaqState::Error;
    return FE_ERR_HW;
  }

  state = DaqState::Configured;
  return SUCCESS;
}

INT CaenDigitizerMidas::HasData() {
  if(state == DaqState::Running){
    if(digitizer->HasData())
      return 1;
  }

  return 0;
}

INT CaenDigitizerMidas::ReadData(char* pevent) {
  if(state != DaqState::Running){
      return 0;
  }

  auto data = digitizer->ReadData();
  auto rawdata = dynamic_cast<CaenRawData*>(data.get());
  auto scopedata = dynamic_cast<CaenScopeData*>(data.get());

  /* init bank structure */
  bk_init32(pevent);

  if(rawdata){
    /* create a bank called RAWx, x = frontend index */
    UINT8 *pdata;
    char bkname[5] = "RAW0";
    bkname[3] += (fFrontendIndex>=0)?fFrontendIndex%10:0;
    bk_create(pevent, bkname, TID_UINT8, (void **)&pdata);

    pdata += data->Serialize((uint8_t*)pdata);

    bk_close(pevent, pdata);
  } else if (scopedata){
    /* create a bank called WxEV, x= frontend index and yy=channel */
    UINT64 *pev;
    char bkevname[5] = "W0EV";
    bkevname[1] += (fFrontendIndex>=0)?fFrontendIndex%10:0;
    bk_create(pevent, bkevname, TID_UINT64, (void **)&pev);
    *(pev++) = (fFrontendIndex & 0xFFFF) | 0xA000000000000000;
    *(pev++) = scopedata->timestamp;
    *(pev++) = scopedata->trigger_id;
    *(pev++) = scopedata->flags;
    bk_close(pevent, pev);

    /* create a bank called Wxyy, x= frontend index and yy=channel */
    for(int i=0; i<scopedata->waveform_size.size(); i++){
      //only write channels with samples
      if(scopedata->waveform_size[i]){
        UINT16 *pdata;
        char bkname[5] = "W000";
        bkname[1] += (fFrontendIndex>=0)?fFrontendIndex%10:0;
        bkname[2] += (i/10)%10;
        bkname[3] += i%10;

        bk_create(pevent, bkname, TID_UINT16, (void **)&pdata);

        memcpy((uint8_t*) pdata, scopedata->waveform[i], scopedata->waveform_size[i]*sizeof(uint16_t));
        pdata +=scopedata->waveform_size[i];

        bk_close(pevent, pdata);
      }
    }
  }

  return bk_size(pevent);
}

void CaenDigitizerMidas::Sync(bool all) {
  if(fOdbDigitizerSettings.get_name().length()==0){
    std::cout << "connecting digitizer settings" << std::endl;
    fOdbDigitizerSettings.connect(fOdbSettings["Digitizer"].get_full_path());
  }
  if(fOdbDigitizerStatus.get_name().length()==0){
    std::cout << "connecting digitizer status" << std::endl;
    fOdbDigitizerStatus.connect(fOdbStatus["Digitizer"].get_full_path());
  }

  auto rootParameter = digitizer->GetRootParameter();
  std::vector<CaenParameter> chVector = rootParameter["/ch"].GetChilds();
  if(fOdbChannelSettings.size()!= chVector.size()){
    std::cout << "connecting digitizer channels" << std::endl;
    for(auto ch: chVector){
      fOdbChannelSettings.emplace_back(fOdbSettings["Channel"+ch.GetName()]);
    }
  }

  if(all){
    //sync all keys
    //fOdbDigitizerSettings.set_trigger_hotlink(false);
    for(auto& par: rootParameter["/par"].GetChilds()){
      if(par.GetName()=="freqsenscore") continue; //Problematic parameter
      if(par.GetName()=="dutycyclesensdcdc") continue; //Problematic parameter

      if(par.GetAccessMode() == CaenParameter::AccessMode::ReadWrite){
        parameterToOdb(fOdbDigitizerSettings, par);
      } else if ( par.GetAccessMode() == CaenParameter::AccessMode::ReadOnly) {
        parameterToOdb(fOdbDigitizerStatus, par);
      }
    }
    //fOdbDigitizerSettings.set_trigger_hotlink(true);

    for(int i=0; i<fOdbChannelSettings.size() && i<chVector.size(); i++){
      //fOdbChannelSettings[i].set_trigger_hotlink(false);
      for(auto& par: chVector[i]["/par"].GetChilds()){
        parameterToOdb(fOdbChannelSettings[i], par);
      }
      //fOdbChannelSettings[i].set_trigger_hotlink(true);
    }
  } else {
    //sync only relevant keys
    //fOdbDigitizerSettings.set_trigger_hotlink(false);
    for(const auto& parName: parametersToSync){
      auto par = rootParameter[parName];

      if(par.GetAccessMode() == CaenParameter::AccessMode::ReadWrite){
        parameterToOdb(fOdbDigitizerSettings, par);
      } else if ( par.GetAccessMode() == CaenParameter::AccessMode::ReadOnly) {
        parameterToOdb(fOdbDigitizerStatus, par);
      }
    }
    //fOdbDigitizerSettings.set_trigger_hotlink(true);

    for(int i=0; i<fOdbChannelSettings.size() && i<chVector.size(); i++){
      //fOdbChannelSettings[i].set_trigger_hotlink(false);
      for(const auto& parName: channelParametersToSync){
        auto par = chVector[i][parName];
        parameterToOdb(fOdbChannelSettings[i], par);
      }
      //fOdbChannelSettings[i].set_trigger_hotlink(true);
    }
  }
}

void CaenDigitizerMidas::SyncThreadFunction() {
  std::cout << "sync thread running"<< std::endl;
  while(runSyncThread){
    Sync(false);
    std::this_thread::sleep_for(10s);
  }
  std::cout << "stop sync thread"<< std::endl;
}
