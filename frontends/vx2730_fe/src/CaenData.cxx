#include "CaenData.h"
#include <iostream>
#include <exception>
#include <cstring>


void CaenRawData::Print() noexcept{
  std::cout << "Raw Event with " << nevents << " subevents, size " << size << std::endl;
}
uint64_t CaenRawData::Serialize(uint8_t* ptr, const uint64_t maxsize) {
  if(maxsize)
    if(size>maxsize){
      throw std::runtime_error("data exceed buffer space");
    }


  memcpy(ptr, data.data(), size);

  return size;
}

void CaenScopeData::Print() noexcept { 
  std::cout << "Scope Event "<< trigger_id <<" (flags=0x"<<std::hex <<flags << std::dec << ")with sizes ";
  for(auto size : waveform_size)
    std::cout << size << " ";
  std::cout << std::endl; 
}
uint64_t CaenScopeData::Serialize(uint8_t* ptr, const uint64_t maxsize) {
  uint64_t *ptr64 = (uint64_t*) ptr;
  *(ptr64++) = timestamp;
  *(ptr64++) = trigger_id;
  *(ptr64++) = flags;
  for(int i=0; i<waveform_size.size(); i++){
    uint64_t wf_size = waveform_size[i]*sizeof(uint16_t);
    uint64_t block_size = sizeof(uint64_t)*((wf_size/sizeof(uint64_t))+(wf_size%sizeof(uint64_t)?1:0));


    *(ptr64++) = block_size;
    
    memcpy((uint8_t*) ptr64, waveform[i], wf_size);
    ptr64 += block_size/sizeof(uint64_t);
  }

  return (uint8_t*)ptr64 - ptr;

}
