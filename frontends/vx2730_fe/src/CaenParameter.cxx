#include "CaenParameter.h"
#include "CaenException.h"
#include <array>
#include <iostream>
#include <algorithm>

void CaenParameter::SetHandle(uint64_t hndl){
  //check handle already set
  if(connected)
    return;

  handle = hndl;

  char name_str[32];
  int ret = CAEN_FELib_GetNodeProperties(handle, "", name_str, &type);
  if(ret != CAEN_FELib_Success){
    throw CaenException(ret);
  }

  name = std::string(name_str);
  connected = true;

  //get access_mode and datatype
  if(type == CAEN_FELib_PARAMETER){
    CaenParameter accessmode_par = GetChildAt("/accessmode");
    CaenParameter datatype_par = GetChildAt("/datatype");

    std::string accessmode = accessmode_par.Get();
    std::string datatype = datatype_par.Get();

    dtype = DataType::String;
    if(accessmode == "READ_WRITE"){
      accessmod = AccessMode::ReadWrite;
    } else if(accessmode == "READ_ONLY"){
      accessmod = AccessMode::ReadOnly;
    } else {
      accessmod = AccessMode::Unknown;
    }

    //guess datatype
    if(datatype == "STRING"){
      std::string val = Get();
      if(val == "True" || val == "False")
        dtype = DataType::Boolean;
      else
        dtype = DataType::String;
    } else if (datatype == "NUMBER"){
      if(accessmod == AccessMode::ReadWrite){
        CaenParameter increment_par = GetChildAt("/increment");
        CaenParameter minvalue_par = GetChildAt("/minvalue");

        std::string increment = increment_par.Get();
        if(!increment.empty() && std::all_of(increment.begin(), increment.end(), ::isdigit)){
          std::string minvalue = minvalue_par.Get();
          if(stoi(minvalue)>=0){
            dtype = DataType::Positive;
          } else {
            dtype = DataType::Integer;
          }
        } else {
          dtype = DataType::Floating;
        }
      } else {
        dtype = DataType::Floating;
      }
    }
  }
}

std::string CaenParameter::GetPath() const {
  if(!connected)
    return std::string("not connected");

  char path_str[256];
  int ret = CAEN_FELib_GetPath(handle, path_str);
  if(ret != CAEN_FELib_Success){
    throw CaenException(ret);
  }

  return std::string(path_str);
}

CaenParameter CaenParameter::GetParent() const {
  CaenParameter parent;

  if(connected){
    uint64_t parent_handle;
    int ret = CAEN_FELib_GetParentHandle(handle, nullptr, &parent_handle);
    if(ret != CAEN_FELib_Success){
      throw CaenException(ret);
    }

    parent.SetHandle(parent_handle);
  }

  return parent;
}

CaenParameter CaenParameter::GetChildAt(const std::string &path) const {
  CaenParameter child;

  if(connected){
    uint64_t child_handle;
    int ret = CAEN_FELib_GetHandle(handle, path.c_str(), &child_handle);
    if(ret != CAEN_FELib_Success){
      throw CaenException(ret);
    }

    child.SetHandle(child_handle);
  }

  return child;
}

std::vector<CaenParameter> CaenParameter::GetChilds() const {
  std::vector<CaenParameter> childs;

  if(connected){
    size_t child_size = 128;
    std::array<uint64_t, 128> child_handles;
    int ret =  CAEN_FELib_GetChildHandles(handle, nullptr, child_handles.data(), child_size);
    if(ret < 0){
      throw CaenException(ret);
    }
    if (ret >= child_size){
      std::cout << "missing keys in GetChilds" << std::endl;
      ret = child_size;
    }

    for(int iHandle =0; iHandle<ret; iHandle++){
      auto &child = childs.emplace_back();
      child.SetHandle(child_handles[iHandle]);
    }
  }

  return childs;
}

//basic get-set
std::string CaenParameter::Get(){
  char value[256];

  int ret = CAEN_FELib_GetValue(handle, nullptr, value);
  if(ret == CAEN_FELib_CommandError){
    return "ERROR";//TODO: remove when read is fixed on all parameters
  }
  if(ret != CAEN_FELib_Success){
    throw CaenException(ret);
  }

  return std::string(value);
}
void CaenParameter::Set(const std::string &value){
  //std::cout << "Setting " << name << " to '" << value << "'" << std::endl;
  int ret = CAEN_FELib_SetValue(handle, nullptr, value.data());
  if(ret != CAEN_FELib_Success){
    throw CaenException(ret);
  }
}

CaenParameter::operator std::string(){
  //should check type is String
  return Get();
}

CaenParameter::operator bool(){
  //should check type is Boolean
  return (Get()=="True")? true : false;  
}

CaenParameter::operator int64_t(){
  //should check type is Integer
  return stoi(Get());
}

CaenParameter::operator uint64_t(){
  //should check type is Positive
  return stoul(Get());
}

CaenParameter::operator double(){
  //should check type is Floting
  return stod(Get());
}

void CaenParameter::operator=(const char* val){
  //should check accessmode is ReadWrite
  //should check type is String
  Set(std::string(val));
}

void CaenParameter::operator=(const std::string& val){
  //should check accessmode is ReadWrite
  //should check type is String
  Set(val);
}

void CaenParameter::operator=(const bool& val){
  //should check accessmode is ReadWrite
  //should check type is Boolean
  if(val){
    Set("True");
  } else {
    Set("False");
  }
}

void CaenParameter::operator=(const int& val){
  //should check accessmode is ReadWrite
  //should check type is Integer
  Set(std::to_string(val));
}

void CaenParameter::operator=(const int64_t& val){
  //should check accessmode is ReadWrite
  //should check type is Integer
  Set(std::to_string(val));
}

void CaenParameter::operator=(const uint64_t& val){
  //should check accessmode is ReadWrite
  //should check type is Positive
  Set(std::to_string(val));
}

void CaenParameter::operator=(const double& val){
  //should check accessmode is ReadWrite
  //should check type is Floating
  Set(std::to_string(val));
}

//print
std::string CaenParameter::to_string(CAEN_FELib_NodeType_t type){
  switch(type){
    case CAEN_FELib_PARAMETER:
      return "para";
    case CAEN_FELib_COMMAND:
      return "cmd ";
    case CAEN_FELib_FEATURE:
      return "feat";
    case CAEN_FELib_ATTRIBUTE:
      return "attr";
    case CAEN_FELib_ENDPOINT:
      return "endp";
    case CAEN_FELib_CHANNEL:
      return "chn ";
    case CAEN_FELib_DIGITIZER:
      return "digi";
    case CAEN_FELib_FOLDER:
      return "fold";
    case CAEN_FELib_LVDS:
      return "lvds";
    case CAEN_FELib_VGA:
      return "vga ";
    case CAEN_FELib_HV_CHANNEL:
      return "hvch";
    case CAEN_FELib_MONOUT:
      return "mon ";
    case CAEN_FELib_VTRACE:
      return "vtr ";
    case CAEN_FELib_GROUP:
      return "grp ";
    case CAEN_FELib_HV_RANGE:
      return "hvrg";
    default:
      return "unkn";
  }
}

std::string CaenParameter::to_string(CaenParameter::AccessMode mode){
  switch(mode){
  case CaenParameter::AccessMode::Unknown:
  default:
      return "UNKNOWN";
    case CaenParameter::AccessMode::ReadOnly:
      return "R";
    case CaenParameter::AccessMode::ReadWrite:
      return "RW";
  }
}

std::string CaenParameter::to_string(CaenParameter::DataType type){
  switch(type){
  case CaenParameter::DataType::Unknown:
  default:
      return "UNKNOWN";
    case CaenParameter::DataType::String:
      return "string";
    case CaenParameter::DataType::Boolean:
      return "bool";
    case CaenParameter::DataType::Integer:
      return "int";
    case CaenParameter::DataType::Positive:
      return "uint";
    case CaenParameter::DataType::Floating:
      return "float";
  }
}

void CaenParameter::Print(bool recurse){
  Print("", recurse);
}
void CaenParameter::Print(std::string prefix, bool recurse){
  if(type == CAEN_FELib_ATTRIBUTE) return;

  std::cout <<prefix << "->" << name << " (" << to_string(type) << ")";
  if(type == CAEN_FELib_ATTRIBUTE || type == CAEN_FELib_PARAMETER){
    std::cout << ": \'"<<  Get() << "\'";
  }

  if(type == CAEN_FELib_PARAMETER){
    std::cout << ", "<<  to_string(accessmod) << " " << to_string(dtype);
  }
 
  std::cout << std::endl;
  if(recurse){
    std::vector<CaenParameter> childs = GetChilds();
    for(auto&child : childs){
      child.Print(prefix+"|", recurse);
    }
  }
}
