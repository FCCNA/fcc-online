#!/bin/env python3
"""
This program can either connect to a running midas experiment or read a data file
"""
import argparse
import midas.client
import midas.file_reader
import sys
import matplotlib.pyplot as plt
import numpy as np

plt.style.use('_mpl-gallery')

def processEvent(event, odb_dict):
    bank_names = ", ".join(b.name for b in event.banks.values())
    print("Event # %s of type ID %s contains banks %s" % (event.header.serial_number, event.header.event_id, bank_names))

    for bank_name, bank in event.banks.items():
        if bank_name == "TEK0":
            if len(bank.data):
                x = np.linspace(0, len(bank.data), len(bank.data))
                y = np.array(bank.data)

                fig, ax = plt.subplots()
                ax.plot(x, y, linewidth=2.0)
                plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='FCC Naples analyzer')
    parser.add_argument('-i')
    args = parser.parse_args()

    if args.i is None:
        print("running online");
        # Create our client
        client = midas.client.MidasClient("pyanalyzer")
        # Define which buffer we want to listen for events on (SYSTEM is the 
        # main midas buffer).
        buffer_handle = client.open_event_buffer("SYSTEM")

        #dump the current ODB
        odb_dict = client.odb_get("/", recurse_dir=True)
        run_number = odb_dict["Runinfo"]["Run number"]
        print("We are looking at a file from run number %s" % run_number)

        # Request events from this buffer that match certain criteria. In this
        # case we will only be told about events with an "event ID" of 1.
        # sampling_type is such to avoid impacting the DAQ (default is midas.GET_ALL)
        request_id = client.register_event_request(buffer_handle, event_id = 1, sampling_type=midas.GET_NONBLOCKING)
        while True:
            # If there's an event ready, `event` will contain a `midas.event.Event`
            # object. If not, it will be None. If you want to block waiting for an
            # event to arrive, you could set async_flag to False.
            event = client.receive_event(buffer_handle, async_flag=True)
            
            if event is not None:
                processEvent(event, odb_dict);

            state = client.odb_get("/Runinfo/State")
            #if state == midas.STATE_RUNNING:
            #    print("The experiment is currently running")
            #elif state == midas.STATE_PAUSED:
            #    print("The experiment is currently paused")
            #elif state == midas.STATE_STOPPED:
            #    print("The experiment is currently stopped")
            #else:
            #    print("The experiment is in an unexpected run state")

            client.communicate(10)

        client.disconnect()

    else:
        print("reading file " + args.i);
        mfile = midas.file_reader.MidasFile(args.i)
        try:
            # Try to find the special midas event that contains an ODB dump.
            odb = mfile.get_bor_odb_dump()
            odb_dict = odb.data;
            
            # The full ODB is stored as a nested dict withing the `odb.data` member.
            run_number = odb_dict["Runinfo"]["Run number"]
            print("We are looking at a file from run number %s" % run_number)
        except RuntimeError:
            # No ODB dump found (mlogger was probably configured to not dump
            # the ODB at the start of each subrun).
            print("No begin-of-run ODB dump found")

        a = 0
        for event in mfile:
            a += 1;

            if event.header.is_midas_internal_event():
                print("Saw a special event")
                continue


            processEvent(event, odb_dict);

            ch = sys.stdin.read(1)
            if ch== 'q':
                exit()

