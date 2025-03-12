#ifndef CAEN_PARAMETER_H
#define CAEN_PARAMETER_H

#include <stdint.h>
#include <string>
#include <vector>
#include "CAEN_FELib.h"

class CaenParameter {
   public:
      CaenParameter() {};
      CaenParameter(const CaenParameter &obj): connected(obj.connected),
                                               handle(obj.handle),
                                               type(obj.type),
                                               name(obj.name),
                                               accessmod(obj.accessmod),
                                               dtype(obj.dtype)
                                               {}
      CaenParameter(CaenParameter &&obj) noexcept : connected(std::move(obj.connected)),
                                          handle(std::move(obj.handle)),
                                          type(std::move(obj.type)), 
                                          name(std::move(obj.name)),
                                          accessmod(std::move(obj.accessmod)),
                                          dtype(std::move(obj.dtype))
                                          { obj.connected = false; } //make sure original object is unusable
      virtual ~CaenParameter() noexcept {};

      CaenParameter &operator=(CaenParameter &&obj) {
        connected = std::move(obj.connected);
        handle = std::move(obj.handle);
        type = std::move(obj.type);
        name = std::move(obj.name);
        accessmod = std::move(obj.accessmod);
        dtype = std::move(obj.dtype);
        return *this;
      }

      //enum class for parameter attributes
      enum class AccessMode {Unknown, ReadOnly, ReadWrite};
      enum class DataType {Unknown, String, Boolean, Integer, Positive, Floating};

      void SetHandle(uint64_t hndl);
      std::string GetPath() const;
      std::string GetName() const noexcept {
        return name;
      }

      //navigation
      CaenParameter GetParent() const;
      CaenParameter GetChildAt(const std::string &path) const;
      std::vector<CaenParameter> GetChilds() const;

      //basic get-set
      std::string Get();
      void Set(const std::string &value);

      //getters for attributes
      AccessMode GetAccessMode() const noexcept {
        return accessmod;
      }
      DataType GetDataType() const noexcept {
        return dtype;
      }

      // conversion operators
      explicit operator std::string(); //value get for string parameters
      explicit operator bool(); //value get for boolean parameters
      explicit operator int64_t(); //value get for integer parameters
      explicit operator uint64_t(); //value get for positive number parameters
      explicit operator double(); //value get for floating number parameters

      // map GetChildAt to the [] operator
      CaenParameter operator[](const std::string &path) const { return GetChildAt(path); }

      // assignment operators
      void operator=(const std::string& val); //value set for string parameters
      void operator=(const bool& val); //value set for boolean parameters
      void operator=(const int64_t& val); //value set for integer parameters
      void operator=(const uint64_t& val); //value set for positive numeber parameters
      void operator=(const double& val); //value set for floating number parameters

      //Print node and subtree
      void Print(bool recurse = false);
      static std::string to_string(CAEN_FELib_NodeType_t type);
      static std::string to_string(AccessMode mode);
      static std::string to_string(DataType type);

   protected:
      bool connected = false;
      uint64_t handle;
      CAEN_FELib_NodeType_t type;
      std::string name = "not connected";
      AccessMode accessmod = AccessMode::Unknown;
      DataType dtype = DataType::Unknown;

   private:
      //private implementation for recursion
      void Print(std::string prefix, bool recurse = false);
};

#endif
