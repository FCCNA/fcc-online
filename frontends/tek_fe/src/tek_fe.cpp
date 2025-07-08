/********************************************************************\

  Name:         tek_fe.cpp
  Created by:   Marco Francesconi

  Contents:     Textronix frontend

  $Id: frontend.c 4089 2007-11-27 07:28:17Z ritt@PSI.CH $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "midas.h"
#include "msystem.h"

#include "mfe.h"
#include "tek.h"
#include "odbxx.h"

using namespace std::placeholders;

class tek_midas: public tek {
   char* fPointer = nullptr;
   midas::odb   fOdbSettings{};

   public:
   void stateCallback(midas::odb &o) {
	   printf("callback\n");
   }
   tek_midas(bool pushMode = false): tek(pushMode){
      midas::odb settings = {
         { "IP Address", "192.168.50.25"},
         { "IP Port", 4000},
         { "Channel Enable", false},
         { "Channel Position", 1.},
         { "Channel Offset", 1.},
         { "Channel Scale", 1.},
         { "Channel Bandwidth", 1.},
         { "Horizontal Position", 1.},
         { "Horizontal Scale", 1.},
         { "Sample Rate", 1.},
         { "Acquisition Mode", "SAMPLE"}
      };
      settings.connect("/Equipment/Trigger/Settings");

      fOdbSettings.connect("/Equipment/Trigger/Settings");
      fOdbSettings["Channel Enable"].resize(TEK_NCHANNEL);
      fOdbSettings["Channel Position"].resize(TEK_NCHANNEL);
      fOdbSettings["Channel Offset"].resize(TEK_NCHANNEL);
      fOdbSettings["Channel Scale"].resize(TEK_NCHANNEL);
      fOdbSettings["Channel Bandwidth"].resize(TEK_NCHANNEL);

      Connect(fOdbSettings["IP Address"], fOdbSettings["IP Port"]);
      AlignODB();

      fOdbSettings.set_trigger_hotlink(false);
      std::function<void(midas::odb&)> f = std::bind(&tek_midas::stateCallback, this, std::placeholders::_1);
      fOdbSettings.watch(f);

      if(IsPushMode())
         std::cout << "Push Mode" << std::endl;
      else
         std::cout << "Pull Mode" << std::endl;
   }

   ~tek_midas(){
   }

   void AlignODB(bool query=false){
      if(query)
         QueryState();

      for(int i=0; i<TEK_NCHANNEL; i++){
         fOdbSettings["Channel Enable"][i] = fChannelEnabled[i];
         if(fChannelEnabled[i]){
            fOdbSettings["Channel Position"][i] = fChannelPosition[i];
            fOdbSettings["Channel Offset"][i] = fChannelOffset[i];
            fOdbSettings["Channel Scale"][i] = fChannelScale[i];
            fOdbSettings["Channel Bandwidth"][i] = fChannelBandwidth[i];
         }
      }

      fOdbSettings["Horizontal Position"] = fHorizontalPosition;
      fOdbSettings["Horizontal Scale"] = fHorizontalScale;
      fOdbSettings["Sample Rate"] = fHorizontalSampleRate;
      fOdbSettings["Acquisition Mode"] = fAcquisitionMode.substr(0,5);
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
         if(fChannelEnabled[i]){
            if(channels.length())
               channels += ",";
            channels += "CH"+std::to_string(i+1);
         }
      }
      channels += '\n';
      std::cout << "Enabled Channels: " << channels;
      WriteCmd("DAT:SOU " + channels + "\n");

      if(IsPushMode()){
         WriteCmd("!t 300000\n");
      } else {
         WriteCmd("!t 10000\n");
      }
   }

   void BeginOfRun(){
      AlignODB();
   };

   void SetEventPointer(WORD* ptr){
      fPointer = (char*)ptr;
   };

   bool ConsumeChannel(int npt, int id){
      // LOG << "Consuming channel " << id <<std::endl;
      char* padc;

      /* create ADC0 bank */
      char bkname[] = "TEK0";
      bkname[3] += id;
      //bk_create(fPointer, bkname, TID_UINT8, (void **)&padc);
      bk_create(fPointer, bkname, TID_UINT16, (void **)&padc);

      int nbyte = 0;
      //fOutputStream << fEventNumber << ", " << id;
      while(nbyte < npt){
         //int size = (sizeof(buff)>(npt-nbyte))?(npt-nbyte):sizeof(buff);
         int size = npt-nbyte;
         int n = ReadFromSocket(padc, size);
	 if (n < 0){
            bk_close(fPointer, padc);
            return false;
	 }
         /*for(int i=0; i<(n/sizeof(unsigned char)); i++){
           fOutputStream << ", " << +(buff[i]);
           }*/
         nbyte += n;
         padc += n;
      }

      bk_close(fPointer, padc);
      return true;
   };

};


/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "Tektronix Frontend";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = TRUE;

/* a frontend status page is displayed with this frequency in ms    */
INT display_period = 0;//1000;

/* maximum event size produced by this frontend */
INT max_event_size = 2 * 1024 * 1024;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 2 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 2 * 2 * 1024 * 1024;
/*-- Function declarations -----------------------------------------*/

INT trigger_thread(void *param);

/*-- Equipment list ------------------------------------------------*/

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {

   {"Trigger",               /* equipment name */
    {1, 0,                   /* event ID, trigger mask */
     "SYSTEM",               /* event buffer */
     EQ_USER,                /* equipment type */
     0,                      /* event source (not used) */
     "MIDAS",                /* format */
     TRUE,                   /* enabled */
     RO_RUNNING,             /* read only when running */
     500,                    /* poll for 500ms */
     0,                      /* stop run after this event limit */
     0,                      /* number of sub events */
     0,                      /* don't log history */
     "", "", "",},
    NULL,                    /* readout routine */
    },

   {""}
};

/*-- Dummy routines ------------------------------------------------*/

INT poll_event(INT source, INT count, BOOL test)
{
   return 1;
};
INT interrupt_configure(INT cmd, INT source, PTYPE adr)
{
   return 1;
};

tek_midas* instrument;

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
   instrument = new tek_midas(true); //set to false for polling mode

   /* create a ring buffer for each thread */
   create_event_rb(0);

   /* create readout thread */
   ss_thread_create(trigger_thread, NULL);

   return CM_SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
   return CM_SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
   instrument->Start();
   return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
   instrument->Stop();
   return CM_SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

/*-- Resuem Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{

   /*if(! instrument->IsStreaming()){
	   instrument->AlignODB(true);
   }*/
   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

/*-- Event readout -------------------------------------------------*/

INT trigger_thread(void *param)
{
   EVENT_HEADER *pevent;
   WORD *pdata;
   int  i, status, exit = FALSE;
   INT rbh;
   
   /* tell framework that we are alive */
   signal_readout_thread_active(0, TRUE);

   /* set name of thread as seen by OS */
   ss_thread_set_name(std::string(equipment[0].name) + "RT");
   
   /* Initialize hardware here ... */
   printf("Start readout thread\n");
   
   /* Obtain ring buffer for inter-thread data exchange */
   rbh = get_event_rbh(0);
   
   while (is_readout_thread_enabled()) {

      if (!readout_enabled() && !instrument->IsStreaming()) {
         // do not produce events when run is stopped
         ss_sleep(10);
         continue;
      }

      if (instrument->HasEvent()) { // if event available, read it out

         // check once more in case state changed during the poll
         if (!is_readout_thread_enabled())
            break;

         // obtain buffer space
         do {
            status = rb_get_wp(rbh, (void **) &pevent, 0);
            if (status == DB_TIMEOUT) {
               ss_sleep(10);
               // check for readout thread disable, thread might be stop from main thread
               // in case Ctrl-C is hit for example
               if (!is_readout_thread_enabled()) {
                  exit = TRUE;
                  break;
               }
            }
         } while (status != DB_SUCCESS);

         if (exit)
            break;

         bm_compose_event_threadsafe(pevent, 1, 0, 0, &equipment[0].serial_number);
         pdata = (WORD *)(pevent + 1);
         
         /* init bank structure */
         bk_init32(pdata);
         
         instrument->SetEventPointer(pdata);
         instrument->ReadData();
         pevent->data_size = bk_size(pdata);

         /* send event to ring buffer */
         rb_increment_wp(rbh, sizeof(EVENT_HEADER) + pevent->data_size);
      }
   }
   
   /* tell framework that we are finished */
   signal_readout_thread_active(0, FALSE);
   
   printf("Stop readout thread\n");

   return 0;
}
