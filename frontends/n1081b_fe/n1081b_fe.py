#!/usr/bin/env python3
"""
MIDAS interface for CAEN N1081B module

requires python3-pyserial
requires python3-parse
"""

#this is to include SDK
import sys
from os.path import dirname
sys.path.append(dirname(__file__) + '/SDK/src')
from N1081B_sdk import N1081B

import midas
import midas.frontend
import midas.event
import collections

class N1081BEquipment(midas.frontend.EquipmentBase):
    """
    We define an "equipment" for each logically distinct task that this frontend
    performs. For example, you may have one equipment for reading data from a
    device and sending it to a midas buffer, and another equipment that updates
    summary statistics every 10s.

    Each equipment class you define should inherit from 
    `midas.frontend.EquipmentBase`, and should define a `readout_func` function.
    If you're creating a "polled" equipment (rather than a periodic one), you
    should also define a `poll_func` function in addition to `readout_func`.
    """
    def __init__(self, client):
        # The name of our equipment. This name will be used on the midas status
        # page, and our info will appear in /Equipment/MyPeriodicEquipment in
        # the ODB.
        equip_name = "N1081B"
        
        # Define the "common" settings of a frontend. These will appear in
        # /Equipment/MyPeriodicEquipment/Common. The values you set here are
        # only used the very first time this frontend/equipment runs; after 
        # that the ODB settings are used.
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type = midas.EQ_PERIODIC
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 500
        default_common.period_ms = 1000                # readout time
        default_common.read_when = midas.RO_ALWAYS      # read also during run stop
        default_common.log_history = True               # log "Variables" into MIDAS History

        default_settings = {
            "Ip Address": "192.168.50.20",
            "Password": "password"
        }

        # You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, default_common, default_settings)

    def connect(self):
        #connect to device
        try:
            self.device = N1081B(self.settings["Ip Address"])
            self.device.connect()
            self.device.login(self.settings["Password"])
        except Exception as e:
            self.client.msg("Cannot connect: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to connect")

        
    def readout_func(self):
        """
        For a periodic equipment, this function will be called periodically
        (every 1s in this case). It should return either a `midas.event.Event`
        or None (if we shouldn't write an event).
        """

        event = midas.event.Event()
        section_list = [N1081B.Section.SEC_A, N1081B.Section.SEC_B, N1081B.Section.SEC_C, N1081B.Section.SEC_D]
        bank_list = ["SCAA", "SCAB", "SCAB", "SCAD"]
        for (section, bankname) in zip(section_list, bank_list):
            res = self.device.get_function_results(section)
            if res["Result"] == True:
                for key in res["data"]:
                    if key == "result" and res["data"][key] == "none":
                        pass
                    elif key == "counters":
                        values = [0.0 for i in range(4)]
                        for conn in res["data"]["counters"]:
                            values[conn["lemo"]] = conn["value"]
                        event.create_bank(bankname, midas.TID_FLOAT, values)
                        
            else:
                print("Error")
        return event

    def settings_changed_func(self):
        """
        You can define this function to be told about when the values in
        /Equipment/MyMultiPeriodicEquipment_1/Settings have changed.
        self.settings is updated automatically, and has already changed
        by this time this function is called.
        
        In this version, you just get told that a setting has changed
        (not specifically which setting has changed).
        """
        #self.client.msg("High-level: Prescale factor is now %d" % self.settings["Prescale factor"])
        #self.client.msg("High-level: Some array is now %s" % self.settings["Some array"])
        pass

    def detailed_settings_changed_func(self, path, idx, new_value):
        """
        You can define this function to be told about when the values in
        /Equipment/MyMultiPeriodicEquipment_1/Settings have changed.
        self.settings is updated automatically, and has already changed
        by this time this function is called.
        
        In this version you get told which setting has changed (down to
        specific array elements).
        """
        if idx is not None:
            self.client.msg("Low-level: %s[%d] is now %s" % (path, idx, new_value))
        else:
            self.client.msg("Low-level: %s is now %s" % (path, new_value))

class N1081BFrontend(midas.frontend.FrontendBase):
    """
    A frontend contains a collection of equipment.
    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
    """
    def __init__(self):
        # You must call __init__ from the base class.
        midas.frontend.FrontendBase.__init__(self, "n1081b_fe")
        
        
        # create equipement for this frontend
        n1081b_equip = N1081BEquipment(self.client)
        try:
            #connect to arduino
            n1081b_equip.connect();
        except:
            n1081b_equip.set_status("HW Error", status_color="redLight")
        else:
            # if successful, add it to quipments
            self.add_equipment(n1081b_equip)
            n1081b_equip.set_status("Ok", status_color="greenLight")
        
    def begin_of_run(self, run_number):
        """
        This function will be called at the beginning of the run.
        You don't have to define it, but you probably should.
        You can access individual equipment classes through the `self.equipment`
        dict if needed.
        """
        # notthing to do here
        #self.set_all_equipment_status("Ok", "greenLight")
        #self.client.msg("Frontend has seen start of run number %d" % run_number)
        return midas.status_codes["SUCCESS"]
        
    def end_of_run(self, run_number):
        # notthing to do here
        #self.set_all_equipment_status("Ok", "greenLight")
        #self.client.msg("Frontend has seen end of run number %d" % run_number)
        return midas.status_codes["SUCCESS"]
    
    def frontend_exit(self):
        """
        Most people won't need to define this function, but you can use
        it for final cleanup if needed.
        """
        #print("Goodbye from user code!")
        pass
        
if __name__ == "__main__":
    # The main executable is very simple - just create the frontend object,
    # and call run() on it.
    with N1081BFrontend() as my_fe:
        my_fe.run()
