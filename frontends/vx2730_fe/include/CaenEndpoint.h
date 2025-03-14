#ifndef CAEN_ENDPOINT_H
#define CAEN_ENDPOINT_H

#include <memory>

class CaenDigitizer;

//base class
class CaenEndpoint {
   public:
      CaenEndpoint(const std::string& n, const std::string& f): name(n), format(f) {};
      virtual ~CaenEndpoint() noexcept {};

      //timeout setters
      void SetTimeout(int value) noexcept { timeout = value; }
      int GetTimeout() noexcept const { return timeout; }

      std::string GetNameString() const noexcept { return name; }
      const char* GetName() const noexcept { return name.c_str(); }
      const char* GetFormat() const noexcept { return format.c_str(); }

      //bool HasData() const { return true; }
      //std::unique_ptr<CaenData> ReadData() {}

      friend class CaenDigitizer;

   protected:
      const std::string name;
      const std::string format;
      int timeout = 500; //timeout in ms
      std::weak_ptr<CaenDigitizer> dgtz;
      uint64_t ep_handle;
      virtual void Configure() {}; //called by CaenDigitizer once equipment is installed
};

// raw implementation
class CaenRawEndpoint : public CaenEndpoint{
  public:
    CaenRawEndpoint(): CaenEndpoint("raw", "") {};
    virtual ~CaenRawEndpoint() noexcept {};

  protected:
    void Configure() {}; //this should fetch max_raw_bytes_per_read 
};

// scope impementation
class CaenScopeEndpoint : public CaenEndpoint{
  public:
    CaenScopeEndpoint(): CaenEndpoint("scope", "[ { \"name\" : \"TIMESTAMP\", \"type\" : \"U64\" }, \
                                                  { \"name\" : \"TRIGGER_ID\", \"type\" : \"U32\" }, \
                                                  { \"name\" : \"WAVEFORM\", \"type\" : \"U16\", \"dim\" : 2 }, \
                                                  { \"name\" : \"WAVEFORM_SIZE\", \"type\" : \"U32\", \"dim\" : 1 }, \
                                                  { \"name\" : \"FLAGS\", \"type\" : \"U16\" } \
                                                ]") {};
    virtual ~CaenScopeEndpoint() noexcept {};

  protected:
    void Configure() {}; //this should fetch nchannel e nsample per channel
};

#endif
