#include <string>

#ifndef TEK_H
#define TEK_H

#define TEK_NCHANNEL 6

#ifdef VERBOSE_LOGGING
#include <ctime>
inline std::string current_time() {
  std::time_t t = std::time(nullptr);
  std::tm tm_buf;
  char buf[26];
  localtime_r(&t, &tm_buf);
  asctime_r(&tm_buf, buf);
  buf[24]=' '; // remove newline
  return buf;
}

#define LOG std::cout << current_time()
#else
#define LOG if (false) std::cout
#endif

class tek {
protected:
   int sockfd;
   volatile int state;
   volatile bool receivingData;
   bool fChannelEnabled[TEK_NCHANNEL];
   float fChannelScale[TEK_NCHANNEL];
   float fChannelPosition[TEK_NCHANNEL];
   float fChannelOffset[TEK_NCHANNEL];
   float fChannelBandwidth[TEK_NCHANNEL];
   float fHorizontalScale;
   float fHorizontalPosition;
   float fHorizontalSampleRate;
   std::string fAcquisitionMode;
   int fEventNumber = 0;
   const bool fPushMode;

   std::string ReadCmd(const std::string &cmd);
   int CharArrayToInt(char* array, int n);
   void WriteCmd(const std::string &cmd);
   void SendClear();
   void WaitOperationComplete();
   void QueryState();
   void EmptySocket();
   int ReadFromSocket(void* buffer, int n);
   virtual bool ConsumeChannel(int npt, int id);
   virtual void BeginOfRun(){};
   virtual void EndOfRun(){};

   class ReceivingDataGuard {
   public:
       explicit ReceivingDataGuard(tek *instrument);
       ~ReceivingDataGuard();
   protected:
       tek* fInstrument = nullptr;
   };

public:

   tek(bool pushMode = false);
   void Connect(const std::string &ip, int port);
   bool IsStreaming();
   bool IsReceivingData();
   bool IsBusy();
   void Start();
   void Stop();
   virtual void Configure();
   bool HasEvent();
   bool ReadData(); 
   bool IsPushMode() { return fPushMode; };
};

#endif
