/********************************************************************\

  Name:         vx2730_fe.cxx

\********************************************************************/

#undef NDEBUG // midas required assert() to be always enabled

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <assert.h> // assert()
#include "midas.h"
#include "mfe.h"
#include "msystem.h"

#include "CaenDigitizerMidas.h"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "caenfelib_fe";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms    */
INT display_period = 0; // 1000;

/* maximum event size produced by this frontend */
INT max_event_size = 10 * 1024 * 1024;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 2 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 2 * 10 * 1024 * 1024;

/*-- Function declarations -----------------------------------------*/

INT read_trigger_event(char *pevent, INT off);
INT read_periodic_event(char *pevent, INT off);

//INT trigger_thread(void *param);

/*-- Equipment list ------------------------------------------------*/

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {

    {
        "FeLibFrontend%03d", /* equipment name */
        {
            1,          /* event ID*/
            0,          /* trigger mask */
            "SYSTEM",   /* event buffer */
            EQ_MULTITHREAD,    /* equipment type */
            0,          /* event source (not used) */
            "MIDAS",    /* format */
            TRUE,       /* enabled */
            RO_RUNNING, /* read only when running */
            500,        /* poll for 500ms */
            0,          /* stop run after this event limit */
            0,          /* number of sub events */
            0,          /* don't log history */
            "",
            "",
            "",
        },
        read_trigger_event, /* readout routine */
    },

    {
        "FeLibPeriodic%03d", /* equipment name */
        {
            10,         /* event ID*/
            0,          /* trigger mask */
            "SYSTEM",   /* event buffer */
            EQ_PERIODIC,/* equipment type */
            0,          /* event source (not used) */
            "MIDAS",    /* format */
            TRUE,       /* enabled */
            RO_ALWAYS,  /* read only when running */
            10000,      /* read every 10s */
            0,          /* stop run after this event limit */
            0,          /* number of sub events */
            0,          /* don't log history */
            "",
            "",
            "",
        },
        read_periodic_event, /* readout routine */
    },

    {""}};

/********************************************************************\
                  Frontend callback routines

  The function frontend_init gets called when the frontend program
  is started. This routine should initialize the hardware, and can
  optionally install several callback functions:

  install_poll_event:
     Install a function which gets called to check if a new event is
     available for equipment of type EQ_POLLED.

  install_frontend_exit:
     Install a function which gets called when the frontend program
     finishes.

   install_begin_of_run:
      Install a function which gets called when a new run gets started.

   install_end_of_run:
      Install a function which gets called when a new run gets stopped.

   install_pause_run:
      Install a function which gets called when a new run gets paused.

   install_resume_run:
      Install a function which gets called when a new run gets resumed.

   install_frontend_loop:
      Install a function which gets called inside the main event loop
      as often as possible. This function gets all available CPU cycles,
      so in order not to take 100% CPU, this function can use the
      ss_sleep(10) function to give up some CPU cycles.

 \********************************************************************/

/*-- Frontend Init -------------------------------------------------*/
CaenDigitizerMidas *digitizer = nullptr;

/*-- Dummy routines ------------------------------------------------*/

INT poll_event(INT source, INT count, BOOL test)
{
    return digitizer->HasData();
    //return 0;
};
INT interrupt_configure(INT cmd, INT source, PTYPE adr)
{
    return 1;
};

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
  INT frontend_index = get_frontend_index();
  digitizer = new CaenDigitizerMidas(frontend_index, equipment);

  INT ret = digitizer->Initialize();
  if(ret != SUCCESS)
    return ret;
  
  // this is if using EQ_USER
  /* create a ring buffer for each thread */
  //create_event_rb(0);

  /* create readout thread */
  //ss_thread_create(trigger_thread, NULL);

  return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
    if (digitizer)
    {
        INT ret = digitizer->Terminate();
        if(ret != SUCCESS)
          return ret;

        delete digitizer;
        digitizer = nullptr;
    }
    return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  INT ret = digitizer->Configure();
  if(ret != SUCCESS)
    return ret;

  return digitizer->StartRun();
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
  return digitizer->StopRun();
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
    return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
    return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
    return SUCCESS;
}

/*------------------------------------------------------------------*/

/*-- Event readout -------------------------------------------------*/
INT read_trigger_event(char *pevent, INT off)
{
  return digitizer->ReadData(pevent);
  //return 0;
}

INT read_periodic_event(char *pevent, INT off)
{
  digitizer->Sync();
  return 0;
}

/*INT trigger_thread(void *param)
{
   EVENT_HEADER *pevent;
   WORD *pdata;
   int  i, status, exit = FALSE;
   INT rbh;
   
   // tell framework that we are alive 
   signal_readout_thread_active(0, TRUE);

   // set name of thread as seen by OS 
   ss_thread_set_name(std::string(equipment[0].name) + "RT");
   
   // Initialize hardware here ...
   printf("Start readout thread\n");
   
   // Obtain ring buffer for inter-thread data exchange
   rbh = get_event_rbh(0);
   
   while (is_readout_thread_enabled()) {

      if (!readout_enabled()) {
         // do not produce events when run is stopped
         ss_sleep(10);
         continue;
      }

      if (digitizer->HasData()) { // if event available, read it out

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
         
         INT size = digitizer->ReadData((char*)pdata);
         pevent->data_size = size;

         // send event to ring buffer
         rb_increment_wp(rbh, sizeof(EVENT_HEADER) + pevent->data_size);
      }
   }
   
   // tell framework that we are finished
   signal_readout_thread_active(0, FALSE);
   
   printf("Stop readout thread\n");

   return 0;
}*/
