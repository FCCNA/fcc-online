#include "CaenException.h"

char CaenException::name[32];
char CaenException::description[256];

std::string CaenException::GetName(){
  CAEN_FELib_GetErrorName(value, name);
  return std::string(name);
}

std::string CaenException::GetDescription(){
  CAEN_FELib_GetErrorDescription(value, description);
  return std::string(description);
}
