#include <stdio.h>
#include <iostream>
#include "dsproto_sy4527/caenhv_fe_class.h"
#include "midas.h"

/* This frontend controls a CAEN SYXX27 hivh voltage crate (e.g. SY4527).
 * It is built in several layers:
 * - The C CAENHVWrapper provided by CAEN, which is painful to use.
 * - caenhvwrapper_cxx.cxx that provides an object-oriented interface to the
 *   CAEN library, but doesn't do anything related to midas.
 * - caenhv_fe_class.cxx, which provides the midas frontend/ODB layer.
 * - This file, which provides the specific configuration for an
 *   experiment.
 */

#define SYSTEM_TYPE N1470

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "caenhv_fe";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = TRUE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 0;

/* maximum event size produced by this frontend */
INT max_event_size = 100000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 10 * max_event_size;

extern HNDLE hDB;

BOOL equipment_common_overwrite = false;

/* The main object we'll delegate all our work to */
CaenHVFE hv;

/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();
INT poll_event(INT source, INT count, BOOL test);
INT interrupt_configure(INT cmd, INT source, PTYPE adr);

INT read_event(char *pevent, INT off);

/*-- Equipment list ------------------------------------------------*/
#undef USE_INT
#define EQUIP_NAME "caen_hv"

EQUIPMENT equipment[] = {
      { EQUIP_NAME, /* equipment name */
         { 400, 0, /* event ID, trigger mask */
         "SYSTEM", /* write events to system buffer */
         EQ_PERIODIC, /* equipment type */
         0, /* event source */
         "MIDAS", /* format */
         TRUE, /* enabled */
         RO_ALWAYS | RO_ODB, /* write events only when running */
         60000, /* read every so many msec */
         0, /* stop run after this event limit */
         0, /* number of sub events */
         1, /* log history */
         "", "", "", },
      read_event, /* readout routine */
   },

   { "" }
};

INT read_event(char *pevent, INT off) {
   return hv.record_state(pevent, off);
}

INT frontend_init() {
   // Ask to sync ODB/crate status every 50ms at most.
   hv.set_sync_period_ms(50);

   // Only log voltage and current in buffers.
   hv.set_log_param("VMon", "VM");
   hv.set_log_param("IMonH", "IM");

   // Don't expose V1/I1 in ODB at all
   //hv.set_ignore_param("V1Set");
   //hv.set_ignore_param("I1Set");

   // Now set up the crate/ODB.
   // See CAENHV_SYSTEM_TYPE_t in CAENHVWrapper.h for allowed values of 3rd param.
   return hv.frontend_init(EQUIP_NAME, hDB, SYSTEM_TYPE);
}

INT frontend_exit() {
   return hv.frontend_exit();
}

INT begin_of_run(INT run_number, char *error) {
   return SUCCESS;
}

INT end_of_run(INT run_number, char *error) {
   return SUCCESS;
}

INT pause_run(INT run_number, char *error) {
   return SUCCESS;
}

INT resume_run(INT run_number, char *error) {
   return SUCCESS;
}

INT frontend_loop() {
   return hv.sync();
}

INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
 is available. If test equals TRUE, don't return. The test
 flag is used to time the polling */
{
   if (test) {
      for (int i = 0; i < count - 1; i++) {
         ss_sleep(2);
      }
   }

   ss_sleep(2);

   return 0;
}

/*-- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source, PTYPE adr) {
   switch (cmd) {
   case CMD_INTERRUPT_ENABLE:
      break;
   case CMD_INTERRUPT_DISABLE:
      break;
   case CMD_INTERRUPT_ATTACH:
      break;
   case CMD_INTERRUPT_DETACH:
      break;
   }
   return SUCCESS;
}
