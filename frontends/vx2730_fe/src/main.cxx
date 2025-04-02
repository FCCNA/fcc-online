#include "CaenDigitizer.h"
#include <iostream>

int main(int argc, char** argv){
  //CaenDigitizer dig;
  auto dig = CaenDigitizer::MakeNewDigitizer();
  if(argc != 2){
    std::cout << "usage '" << argv[0] << " path'"<< std::endl;
  }

  std::string path(argv[1]);

  try{
    dig->Connect(path);
  } catch(CaenException &ex){
    std::cout << "Error opening the digitizer" <<std::endl;
    std::cout << "Error " << ex.GetName() << ": " << ex.GetDescription() << std::endl;
    return -1;
  }

  if(dig->IsConnected())
    std::cout << "connected: yuppi!" << std::endl;

  try{
    dig->RunCmd("Reset");
  } catch(CaenException &ex){
    std::cout << "Error running the command" <<std::endl;
    std::cout << "Error " << ex.GetName() << ": " << ex.GetDescription() << std::endl;
    return -1;
  }
  std::cout << "command successful!" << std::endl;

  CaenParameter root = dig->GetRootParameter();
  auto numchPar = root.GetChildAt("/par/numch");
  std::cout << "number of channels: "<< static_cast<int64_t>(numchPar) << std::endl;
  std::cout << "number of bits: "<< root["/par/adc_nbit"].Get() << std::endl;

  //numchPar = (int64_t)10; // will fail because is readonly
  //numchPar = "abc"; // will fail because is readonly

  //root.Print(true);

  //configure
  uint64_t nCh = (uint64_t)numchPar;
  for(uint64_t iCh=0; iCh< nCh; iCh++){
    std::cout << "Configuring ch " << iCh << std::endl;
    root["/ch/"+std::to_string(iCh)+"/par/ChEnable"] = true;
    root["/ch/"+std::to_string(iCh)+"/par/DCOffset"] = 50.;
  }
  std::cout << "Configuring record len" << std::endl;
  root["/par/RecordLengthS"]=1024;
  root["/par/PreTriggerS"]=100;
  std::cout << "Configuring testpulse" << std::endl;
  root["/par/TestPulsePeriod"]=100e6;
  root["/par/TestPulseWidth"]=1000;
  std::cout << "Configuring acqsource" << std::endl;
  root["/par/acqtriggersource"]="SWTrg|TestPulse";
  std::cout << "Configuration DONE!" << std::endl;

  //dig->ConfigureEndpoint(std::make_unique<CaenRawEndpoint>());
  dig->ConfigureEndpoint(std::make_unique<CaenScopeEndpoint>());
  std::cout << "Endpoint configured" << std::endl;

  dig->RunCmd("armacquisition");
  dig->RunCmd("swstartacquisition");

  return 0;
}
