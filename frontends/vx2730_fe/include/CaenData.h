#ifndef CAEN_DATA_H
#define CAEN_DATA_H

#include <vector>
#include <stdint.h>

//base class
class CaenData {
   public:
      CaenData() {};
      virtual ~CaenData() noexcept {};
      virtual void Print() noexcept {};
      virtual uint64_t Serialize(uint8_t* ptr, uint64_t maxsize=0) { return 0; };
};

// raw implementation
class CaenRawData : public CaenData{
  public:
    uint32_t size;
    uint32_t nevents; //number of events in blob 
    std::vector<uint8_t> data;
    CaenRawData(uint64_t maxsize): CaenData(), data(maxsize) {};
    virtual ~CaenRawData() noexcept {};

    void Print() noexcept final;
    uint64_t Serialize(uint8_t* ptr, uint64_t maxsize=0) final;
};

// scope impementation
class CaenScopeData : public CaenData{
  public:
    uint64_t timestamp;
    //uint64_t timestamp_ns;
    uint32_t trigger_id;
    std::vector<uint16_t*> waveform;
    std::vector<uint32_t> waveform_size;
    //uin32_t event_size;
    uint16_t flags;
    //bool board_fail;
    //uint8_t samples_overlapped;

    CaenScopeData(uint64_t numch, uint64_t recordlengths): CaenData(), waveform(numch), waveform_size(numch, 0) {
      for(auto& wf: waveform){
        wf = new uint16_t[recordlengths];
      }
    };
    virtual ~CaenScopeData() {
      for(auto& wf: waveform){
        delete[] wf;
        wf = nullptr;
      }
    };

    void Print() noexcept final;
    uint64_t Serialize(uint8_t* ptr, uint64_t maxsize=0) final;
};

#endif
