#ifndef CAEN_EXCEPTION_H
#define CAEN_EXCEPTION_H

#include <exception>
#include <string>
#include "CAEN_FELib.h"

class CaenException : public std::exception {
  public:
    CaenException(int val): value((CAEN_FELib_ErrorCode) val) {};
    CaenException(CAEN_FELib_ErrorCode val): value(val) {};
    virtual ~CaenException() noexcept {}

    std::string GetName();
    std::string GetDescription();

    //static std::string GetLastError();

    virtual const char* what() const noexcept {
      CAEN_FELib_GetErrorName(value, name);

      return name;
    }

  private:
    CAEN_FELib_ErrorCode value;
    static char name[32];
    static char description[256];
};

#endif
