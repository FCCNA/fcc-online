import midas.client
import time

"""
this programs connect to midas and drives the Demand of /Equipment/caen_hv to perform an IV Measurement
"""

if __name__ == "__main__":
    client = midas.client.MidasClient("pyIVCurve")

    channel = 0;
    start = 30;
    end = 35;
    step = 0.25;
    
    # set to initial and switch on
    print(str(start) + " V")
    client.odb_set("/Equipment/caen_hv/Settings/Slot 0/VSet (V)[%d]" % (channel), start)
    client.odb_set("/Equipment/caen_hv/Settings/Slot 0/Pw[%d]" % (channel), True)
    time.sleep(10);
    
    # Read the value back
    #readback = client.odb_get("/pyexample/eg_float")
    i = 0;

    while start + (i * step) < end:
        print(str(start + (i * step)) + " V")
        client.odb_set("/Equipment/caen_hv/Settings/Slot 0/VSet (V)[%d]" % (channel), start + (i * step))
        time.sleep(10);
        meas = client.odb_get("/Equipment/caen_hv/Variables/VM00[%d]" %(channel))
        curr = client.odb_get("/Equipment/caen_hv/Variables/IM00[%d]" %(channel))
        print (str(meas) + "V " + str(curr) + "uA")
        i += 1

    client.odb_set("/Equipment/caen_hv/Settings/Slot 0/VSet (V)[%d]" % (channel), start)
    client.odb_set("/Equipment/caen_hv/Settings/Slot 0/Pw[%d]" % (channel), False)
    
    client.disconnect()
