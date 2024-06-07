#include "tek.h"
#include <stdexcept>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <strings.h> // bzero()
#include <unistd.h> // read(), write(), close()
#include <iostream>
#include <sstream>

tek::tek(bool pushMode): fPushMode(pushMode){
   state = 0;
}

void tek::WriteCmd(const std::string &cmd){
   //std::cout << cmd << std::endl;
   int n = write(sockfd, cmd.c_str(), cmd.length());
   //std::cout << n << std::endl;
   if(cmd.back() != '\n'){
      write(sockfd, "\n", 1);
   }
}

std::string tek::ReadCmd(const std::string &cmd){
   char buff[1000];
   
   //wait max 1 s
   fd_set fds;
   struct timeval tv;
   tv.tv_sec=1;
   tv.tv_usec=0;

   WriteCmd(cmd);

   //command is not a query
   if (cmd.find('?') == std::string::npos)
      return "";

   FD_ZERO(&fds);
   FD_SET(sockfd, &fds);
   int ret = select(sockfd+1, &fds, 0, 0, &tv);
   if(ret < 0){
      return "";
   } else if (ret > 0){
      int n = read(sockfd, buff, sizeof(buff));
      while(buff[n-1]!='\n') n += read(sockfd, buff+n, sizeof(buff)-n);

      if (n>0) 
         return std::string(buff, n);
      else
         return "";
    } else {
      return "";
   } 
}

void tek::SendClear(){
   WriteCmd("!d\n");

   //Consume All buffer
   while (HasEvent()){
      ReadData();
   }
}

void tek::WaitOperationComplete(){
   std::string ret = ReadCmd("*OPC?\n");
   while(!ret.empty() && ret.front() == '1')
      ret = ReadCmd("*OPC?\n");
}

void tek::QueryState(){
   //check enabled channels
   /*std::string enabledChannels = ReadCmd("DAT:SOU?\n");
   for(int i=0; i< TEK_NCHANNEL; i++) fChannelEnabled[i] = false;

   std::istringstream streamEnabledChannels(enabledChannels); 
   std::string chn; 
   while (getline(streamEnabledChannels, chn, ',')) {
      if(chn.rfind("CH", 0) == 0){
         fChannelEnabled[chn[2]-'1'] = true;
      }
   }*/

   //get vertical 
   for(int i=0; i< TEK_NCHANNEL; i++){
      std::string enabled = ReadCmd("DIS:GLO:CH"+std::to_string(i+1)+":STATE?\n");
      if(enabled.front()=='1'){
         fChannelEnabled[i] = true;
      } else {
         fChannelEnabled[i] = false;
      }
      fChannelPosition[i] = std::stof(ReadCmd("CH"+std::to_string(i+1)+":POS?\n"));
      fChannelOffset[i] = std::stof(ReadCmd("CH"+std::to_string(i+1)+":OFFS?\n"));
      fChannelScale[i] = std::stof(ReadCmd("CH"+std::to_string(i+1)+":SCA?\n"));
   }
   fHorizontalPosition = std::stof(ReadCmd("HOR:POS?\n"));
   fHorizontalScale = std::stof(ReadCmd("HOR:SCA?\n"));
   fHorizontalSampleRate = std::stof(ReadCmd("HOR:SAMPLER?\n"));
}

void tek::Connect(const std::string &ip, int port){
   struct sockaddr_in servaddr, cli;

   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd == -1) {
      throw std::runtime_error("socket creation failed...");
   }

   bzero(&servaddr, sizeof(servaddr));

   // assign IP, PORT
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = inet_addr(ip.c_str());
   servaddr.sin_port = htons(port);

   if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
      throw std::runtime_error("connection failed...");
   }

   std::cout << ReadCmd("*IDN?");
   WriteCmd("*CLS\n");
   QueryState();
}

bool tek::IsStreaming(){
   return state == 1;
}

bool tek::IsBusy(){
   char buff[2];
   buff[0]=0;
   buff[1]=0;
   
   WriteCmd("BUSY?\n");

   int n = read(sockfd, buff, sizeof(buff));
   while(n <= 0) read(sockfd, buff, sizeof(buff));

   if(buff[0]== '0')
      return false;
   else
      return true;
}

void tek::Start(){
   Configure();
   QueryState();
   BeginOfRun();

   std::cout << "Starting... ";
   WriteCmd("DATA:ENC RPB\n");
   WriteCmd("DIS:WAVE OFF\n");
   if(fPushMode){
      WriteCmd("ACQ:STATE RUN\n");
      WriteCmd("CURVES?\n");
   } else {
      WriteCmd("ACQ:STATE STOP\n");
      WriteCmd("ACQ:STOPA SEQUENCE\n");
      WriteCmd("ACQ:STATE RUN\n");

   }
   std::cout << "DONE!"<< std::endl;
   state = 1;
}

void tek::Stop(){
   std::cout << "Stopping... ";
   if(fPushMode){
      SendClear();
      state = 0;
      WriteCmd("ACQ:STATE STOP\n");
   } else {
      state = 0;
      WriteCmd("ACQ:STATE STOP\n");
   }
   WriteCmd("DIS:WAVE ON\n");
   std::cout << "DONE!"<< std::endl;

   EndOfRun();
}

void tek::Configure(){
}

bool tek::HasEvent(){

   if(fPushMode){
      //wait max 0.01 s
      fd_set fds;
      struct timeval tv;
      tv.tv_sec=0;
      tv.tv_usec=10000;
      FD_ZERO(&fds);
      FD_SET(sockfd, &fds);
      int ret = select(sockfd+1, &fds, 0, 0, &tv);
      if(ret > 0){
         return true;
      } else {
         return false;
      }
   } else {
      if(IsBusy()){
         return false;
      } else {
    return true;
      }
   }
}

int tek::CharArrayToInt(char* array, int n){
   int base = 1;
   int ret = 0;
   for (int i=n-1; i>=0; i--){
      ret += (array[i]-'0')*base;
      base *= 10;
   }
   return ret;
}

void tek::ConsumeChannel(int npt, int id){
   //dummy implementation to be reimplemented by derived classes
   char buff[80];

   int nbyte = 0;
   while(nbyte < npt){
      int size = (sizeof(buff)>(npt-nbyte))?(npt-nbyte):sizeof(buff);
      nbyte += read(sockfd, buff, size);
   }
}

bool tek::ReadData(){
   char buff[80];
   int iChannelBlk=0;
   bool gotFooter=false;

   //if pull mode, fetch data
   if(!fPushMode){
      /*std::string channels = ReadCmd("DAT:SOU:AVAIL?\n");
      std::cout << "Available: "<< channels;
      while(channels.front() != 'C'){
         //std::string busy = ReadCmd("BUSY?\n");
         //std::cout << "Busy State: "<< busy;
         usleep(1000);
         channels = ReadCmd("DAT:SOU:AVAIL?\n");
         std::cout << "Available: "<< channels;
      }*/
      WriteCmd("CURV?\n");
      fd_set fds;
      struct timeval tv;
      tv.tv_sec=0;
      tv.tv_usec=100000;
      FD_ZERO(&fds);
      FD_SET(sockfd, &fds);
      int ret = select(sockfd+1, &fds, 0, 0, &tv);
      if(ret <= 0){
         std::cout << "No data received" << std::endl;
         std::cout << ReadCmd("*ESR?\n");
         std::cout << ReadCmd("ALLEV?\n");
         return false;
      }
   }

   while(!gotFooter){
      int n = read(sockfd, buff, 1);

      //consume until header
      while(buff[0] != '#') n = read(sockfd, buff, 1);
      //std::cout << "Header found" << std::endl;

      n = read(sockfd, buff, 1);
      n = read(sockfd, buff, buff[0]-'0');

      int npt = CharArrayToInt(buff, n);
      //printf("got event with %d bytes\n", npt);

      ConsumeChannel(npt, iChannelBlk);
      //std::cout << "Read channel " << iChannelBlk << std::endl;

      //read footer
      n = read(sockfd, buff, 1);
      if(n == 1 && buff[0]==10){
         //std::cout << "Got Last Channel "<< iChannelBlk << std::endl;
         gotFooter = true;
      } else if (n==1 && buff[0] == ';'){
         //std::cout << "Got Channel "<< iChannelBlk << std::endl;
         iChannelBlk ++; 
      } else {
         printf("bad footer, got %d with val %d\n", n, buff[0]);
         return false;
      }
   }

   //std::cout << "Event done" << std::endl;
   fEventNumber++;

   //if pull mode, arm trigger
   if(!fPushMode)
      WriteCmd("ACQ:STATE RUN\n");

   return true;
}

