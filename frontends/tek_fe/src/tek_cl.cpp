#include "tek.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <unistd.h> // read(), write(), close()
#include <sys/poll.h> //poll()

class tek_file: public tek {
   std::ofstream fOutputStream;

public:
   tek_file(std::string output = "test.txt", bool pushMode = false): tek(pushMode){
      fOutputStream.open(output, std::ofstream::out);
      std::cout << "opened file "<< output <<std::endl;
      if(IsPushMode())
	  std::cout << "Push Mode" << std::endl;
      else
	  std::cout << "Pull Mode" << std::endl;
   }

   ~tek_file(){
      fOutputStream.close();
   }

   void Configure(){
      //set transmit window to full recordlenght
      WriteCmd("DAT:STAR 1\n");
      std::string points = ReadCmd("HOR:MOD:RECO?\n");
      WriteCmd("DAT:STOP "+ points + "\n");

      //set 8 bit
      //WriteCmd("DAT:WID 1\n");

      //send all enabled channels
      /*std::string channels = ReadCmd("DAT:SOU:AVAIL?\n");*/
      std::string channels = "";
      for(int i=0; i< TEK_NCHANNEL; i++){
	 std::string enabled = ReadCmd("DIS:GLO:CH"+std::to_string(i+1)+":STATE?\n");
	 if(enabled.front()=='1'){
		if(channels.length())
			channels += ",";
		 channels += "CH"+std::to_string(i+1);
	 }
      }
      channels += '\n';
      LOG << "Enabled Channels: " << channels;
      WriteCmd("DAT:SOU " + channels + "\n");
      
      if(IsPushMode()){
          WriteCmd("!t 300000\n");
      } else {
          WriteCmd("!t 10000\n");
      }
   }

   void BeginOfRun(){
      for(int i=0; i<TEK_NCHANNEL; i++){
         if(fChannelEnabled[i]){
            fOutputStream << "# Channel " << i << ": Position=" << fChannelPosition[i] << " Offset=" << fChannelOffset[i] << " Scale=" << fChannelScale[i] << std::endl; 
         }
      }

      fOutputStream << "# Horizontal: Position=" << fHorizontalPosition << " Scale=" << fHorizontalScale << " SampleRate="<< fHorizontalSampleRate << std::endl; 

   }

   bool ConsumeChannel(int npt, int id){
      LOG << "Consuming channel " << id << ", " << npt << " points" << std::endl;
      unsigned short buff[10000];
      if (npt > 10000*sizeof(unsigned short)){
         std::cout << "Channel cannot fit in buffer!" << std::endl;
	 return false;
      }

      int nbyte = 0;
      fOutputStream << fEventNumber << ", " << id;
      while(nbyte < npt){
         int size = (sizeof(buff)>(npt-nbyte))?(npt-nbyte):sizeof(buff);
         int n = ReadFromSocket((char*)buff + nbyte, size);
	 if (n < 0) return false;
         nbyte += n;
      }
      for(int i=0; i<npt/sizeof(unsigned short); i++){
	      fOutputStream << ", " << buff[i];
      }

      fOutputStream << std::endl;
      return true;

   };

};

int main(int argc, char** argv){
   bool pushMode = true;
   std::string outfile = "test.txt";

   if(argc == 2)
      outfile = argv[1];
   else if (argc == 3) {
      outfile = argv[1];
      if(argv[2][0] == '0'){
	      pushMode = false;
      } else {
	      pushMode = true;
      }
   } else if(argc > 1){
	std::cout << "Usage: tek_cl [filename.txt] [pushMode]" <<std::endl;
	return -1;
   }

   tek* instrument = new tek_file(outfile, pushMode);
   instrument->Connect("192.168.50.25", 4000);

   instrument->Start();
   std::chrono::high_resolution_clock::time_point tstart = std::chrono::high_resolution_clock::now();
   int nevents = 0;
   bool run = true;
   while(run){
      struct pollfd fds;
      int ret;
      fds.fd = 0; /* this is STDIN */
      fds.events = POLLIN;
      ret = poll(&fds, 1, 0);
      if(ret == 1){
         std::cout << "Closing\n" << std::endl;
         run = false;
      } else if(ret<0){
         std::cout << "Error in poll stopping\n" << std::endl;
         run=false;
      }

      if(instrument->HasEvent())
         if(instrument->ReadData()){
            std::cout << "Success! " << nevents << std::endl;
            nevents++;
         }
      else if(!pushMode)
	 usleep(10000);
   }
   std::chrono::high_resolution_clock::time_point tstop = std::chrono::high_resolution_clock::now();
   instrument->Stop();

   std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(tstop - tstart);
   std::cout << "Collected " << nevents << " Events in " << time_span.count() <<" seconds. Rate : " << nevents/time_span.count() << std::endl;
   return 0;
}
