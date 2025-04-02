#ifndef CAEN_DATA_H
#define CAEN_DATA_H

#include <vector>

//base class
class CaenData {
   public:
      CaenData() {};
      virtual ~CaenData() noexcept {};
};

// raw implementation
class CaenRawData : public CaenData{
  public:
    size_t size;
    uint32_t nevents; //number of events in blob 
    std::vector<uint8_t> data;
    CaenRawData(uint64_t maxsize): CaenData(), data(maxsize) {};
    virtual ~CaenRawData() noexcept {};
};

// scope impementation
class CaenScopeData : public CaenData{
  public:
    uint64_t timestamp;
    //uint64_t timestamp_ns;
    uint32_t trigger_id;
    std::vector<std::vector<uint16_t>> waveform;
    std::vector<size_t> waveform_size;
    //size_t event_size;
    uint16_t flags;
    //bool board_fail;
    //uint8_t samples_overlapped;

    CaenScopeData(uint64_t numch, uint64_t recordlengths): CaenData() {};
    virtual ~CaenScopeData() noexcept {};
};

#endif
