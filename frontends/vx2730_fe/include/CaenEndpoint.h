#ifndef CAEN_ENDPOINT_H
#define CAEN_ENDPOINT_H

#include <memory>
#include <string>

class CaenDigitizer;
class CaenData;

//base class
class CaenEndpoint {
   public:
      CaenEndpoint(const std::string& n, const std::string& f): name(n), format(f) {};
      virtual ~CaenEndpoint() noexcept {};

      //timeout setters
      void SetTimeout(int value) noexcept { timeout = value; }
      int GetTimeout() const noexcept { return timeout; }

      std::string GetNameString() const noexcept { return name; }
      const char* GetName() const noexcept { return name.c_str(); }
      const char* GetFormat() const noexcept { return format.c_str(); }

      bool HasData(); //checks wheter data is available
      virtual std::unique_ptr<CaenData> ReadData() ;

      friend class CaenDigitizer;

   protected:
      const std::string name;
      const std::string format;
      int timeout = 500; //timeout in ms
      std::weak_ptr<CaenDigitizer> dgtz;
      uint64_t ep_handle = 0;
      virtual void Configure() {}; //called by CaenDigitizer once equipment is installed
};

// raw implementation
class CaenRawEndpoint : public CaenEndpoint{
  public:
    CaenRawEndpoint(): CaenEndpoint("raw", "") {};
    virtual ~CaenRawEndpoint() noexcept {};
    std::unique_ptr<CaenData> ReadData() final;

  protected:
    uint64_t maxrawdatasize;
    void Configure();
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
    std::unique_ptr<CaenData> ReadData() final;

  protected:
    uint64_t numch;
    uint64_t recordlengths;
    void Configure();
};

#endif
