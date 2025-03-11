#include "CaenDigitizer.h"
#include <iostream>

int main(int argc, char** argv){
  CaenDigitizer dig;
  if(argc != 2){
    std::cout << "usage '" << argv[0] << " path'"<< std::endl;
  }

  std::string path(argv[1]);

  try{
    dig.Connect(path);
  } catch(CaenException &ex){
    std::cout << "Error opening the digitizer" <<std::endl;
    std::cout << "Error " << ex.GetName() << ": " << ex.GetDescription() << std::endl;
    return -1;
  }

  if(dig.IsConnected())
    std::cout << "connected: yuppi!" << std::endl;

  try{
    dig.RunCmd("Reset");
  } catch(CaenException &ex){
    std::cout << "Error running the command" <<std::endl;
    std::cout << "Error " << ex.GetName() << ": " << ex.GetDescription() << std::endl;
    return -1;
  }
  std::cout << "command successful!" << std::endl;

  CaenParameter root = dig.GetRootParameter();
  auto numchPar = root.GetChildAt("/par/numch");
  std::cout << "number of channels: "<< numchPar.Get() << std::endl;
  std::cout << "number of bits: "<< root.GetChildAt("/par/adc_nbit").Get() << std::endl;

  root.Print(true);

  /*std::vector<CaenParameter> v = root.GetChilds();
  for(auto & p :v){
    std::cout << "hei midas, creami " << p.GetName() <<std::endl;
    std::vector<CaenParameter> v2 = p.GetChilds();


    for(auto& p2: v2){
      std::cout << "hei midas, cream dentro " << p2.GetName() <<std::endl;
      
    }
  }*/


  return 0;
}
