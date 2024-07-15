#include <string>

#ifndef TEK_H
#define TEK_H

#define TEK_NCHANNEL 6

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
