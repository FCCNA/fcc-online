/********************************************************************\

  Name:         midas_UI.cxx

\********************************************************************/

#undef NDEBUG // midas required assert() to be always enabled

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <assert.h> // assert()
#include "midas.h"
#include "msystem.h"

#include "mfe.h"
#include "cane.h"
#include "odbxx.h"

class cane_midas : public cane
{

    midas::odb fOdbSettings{};
    midas::odb fOdbVariables{};

public:
    cane_midas()
    {
        midas::odb settings = {
            {"IP Address", "192.168.50.22"},
            {"IP Port", 4000},
            {"Channel Enable", false},
            {"Channel Position", 1.},
            {"Channel Offset", 1.},
            {"Channel Scale", 1.},
            {"Channel Bandwidth", 1.},
            {"Horizontal Position", 1.},
            {"Horizontal Scale", 1.},
            {"Sample Rate", 1.},
            {"Acquisition Mode", "SAMPLE"}};
        settings.connect("/Equipment/FRATM/Settings");

        fOdbSettings.connect("/Equipment/FRATM/Settings");
        fOdbSettings["Channel Enable"].resize(DOG_NCHANNEL);
        fOdbSettings["Channel Position"].resize(DOG_NCHANNEL);
        fOdbSettings["Channel Offset"].resize(DOG_NCHANNEL);
        fOdbSettings["Channel Scale"].resize(DOG_NCHANNEL);
        fOdbSettings["Channel Bandwidth"].resize(DOG_NCHANNEL);

        midas::odb variables = {
            {"Channel Status", "Qualcosa"},
            {"Channel Info", "Qualcosa"}};
        variables.connect("/Equipment/FRATM/Variables");
        fOdbVariables.connect("/Equipment/FRATM/Variables");
        fOdbVariables["Channel Status"].resize(DOG_NCHANNEL);
        fOdbVariables["Channel Info"].resize(DOG_NCHANNEL);
    }

    void StateCallback(midas::odb &o)
    {
        std::cout << "ODB State changed: " << o.get_full_path() << " -> " << o << std::endl;
    }
    void SetupCallback()
    {
        fOdbSettings.watch([this](midas::odb &arg)
                           { this->StateCallback(arg); });
    }

    void Start()
    {
        Connect(fOdbSettings["IP Address"], fOdbSettings["IP Port"]);
        std::cout << "Cane Start()" << std::endl;
        AlignODB();
        fOdbSettings.set_trigger_hotlink(false);
        SetupCallback();
    }

    void Stop()
    {
        std::cout << "Cane Stop()" << std::endl;
    }

    void AlignODB()
    {
        QueryState();
        for (int i = 0; i < DOG_NCHANNEL; i++)
        {
            fOdbSettings["Channel Enable"][i] = fChannelEnabled[i];
            if (fChannelEnabled[i])
            {
                fOdbSettings["Channel Position"][i] = fChannelPosition[i];
                fOdbSettings["Channel Offset"][i] = fChannelOffset[i];
                fOdbSettings["Channel Scale"][i] = fChannelScale[i];
                fOdbSettings["Channel Bandwidth"][i] = fChannelBandwidth[i];
                fOdbVariables["Channel Info"][i] = fChannelInfo[i];
                fOdbVariables["Channel Status"][i] = fChannelStatus[i];
            }
        }

        fOdbSettings["Horizontal Position"] = fHorizontalPosition;
        fOdbSettings["Horizontal Scale"] = fHorizontalScale;
        fOdbSettings["Sample Rate"] = fHorizontalSampleRate;
        fOdbSettings["Acquisition Mode"] = fAcquisitionMode.substr(0, 5);
    }
};

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "michiamog_fe";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = TRUE;

/* a frontend status page is displayed with this frequency in ms    */
INT display_period = 0; // 1000;

/* maximum event size produced by this frontend */
INT max_event_size = 2 * 1024 * 1024;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 2 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 2 * 2 * 1024 * 1024;

/*-- Function declarations -----------------------------------------*/

INT read_trigger_event(char *pevent, INT off);

/*-- Equipment list ------------------------------------------------*/

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {

    {
        "FRATM", /* equipment name */
        {
            1,          /* event ID*/
            0,          /* trigger mask */
            "SYSTEM",   /* event buffer */
            EQ_POLLED,  /* equipment type */
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

/*-- Dummy routines ------------------------------------------------*/

INT poll_event(INT source, INT count, BOOL test)
{
    return 1;
};
INT interrupt_configure(INT cmd, INT source, PTYPE adr)
{
    return 1;
};

void my_cout(std::string my_string)
{
    std::cout << my_string << std::endl;
}

/*-- Frontend Init -------------------------------------------------*/
cane_midas *myCane;

INT frontend_init() // Dividere in due parti: Init e Configurazione
{
    my_cout("init");
    myCane = new cane_midas();
    myCane->Start();
    return CM_SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
    my_cout("exit");
    if (myCane)
    {
        myCane->Stop();
        delete myCane;
        myCane = nullptr;
    }
    return CM_SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
    my_cout("bor");
    myCane->AlignODB();
    return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
    my_cout("eor");
    return CM_SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
    my_cout("pausa");
    return CM_SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
    my_cout("resume");
    return CM_SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop() // Viene chiamata sempre
{
    return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

/*-- Event readout -------------------------------------------------*/
INT read_trigger_event(char *pevent, INT off)
{
    UINT32 *pdata;

    /* init bank structure */
    bk_init(pevent);

    /* create a bank called ADC0 */
    bk_create(pevent, "ADC0", TID_UINT32, (void **)&pdata);

    /* following code "simulates" some ADC data */
    for (int i = 0; i < 4; i++)
        *pdata++ = rand() % 1024 + rand() % 1024 + rand() % 1024 + rand() % 1024;

    bk_close(pevent, pdata);

    /* create another bank called TDC0 */
    bk_create(pevent, "TDC0", TID_UINT32, (void **)&pdata);

    /* following code "simulates" some TDC data */
    for (int i = 0; i < 4; i++)
        *pdata++ = rand() % 1024 + rand() % 1024 + rand() % 1024 + rand() % 1024;

    bk_close(pevent, pdata);

    /* limit event rate to 100 Hz. In a real experiment remove this line */
    ss_sleep(10);

    return bk_size(pevent);
}
