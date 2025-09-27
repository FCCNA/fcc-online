#!/usr/bin/env python3
"""
Arduino-based readout of BME680 

requires python3-pyserial
requires python3-parse
"""

import midas
import midas.frontend
import midas.event
import collections

import pyvisa

class KeithleyEquipment(midas.frontend.EquipmentBase):
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
        equip_name = "KeithleyEquipment_%s" % midas.frontend.frontend_index
        
        # Define the "common" settings of a frontend. These will appear in
        # /Equipment/MyPeriodicEquipment/Common. The values you set here are
        # only used the very first time this frontend/equipment runs; after 
        # that the ODB settings are used.
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type = midas.EQ_PERIODIC
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 600 + midas.frontend.frontend_index
        default_common.period_ms = 60000                # readout time
        default_common.read_when = midas.RO_ALWAYS      # read also during run stop
        default_common.log_history = True               # log "Variables" into MIDAS History

        default_settings = {
            "Resource": "",
            "Editable": "Demand,Output", 
            "Names KEIT": ["Voltage", "Current"],
            "Unit KEIT": ["V", "A"],
            "Format KEIT": ["%f2", "%e2"],
            "Grid display": False
        }

        
        # You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, default_common, default_settings)

        self.client.odb_set(f'{self.odb_variables_dir}/Demand', float(0))
        self.client.odb_watch(f'{self.odb_variables_dir}/Demand', self.demand_callback)
        self.client.odb_set(f'{self.odb_variables_dir}/Output', False)
        self.client.odb_watch(f'{self.odb_variables_dir}/Output', self.output_callback)

    def demand_callback(self, client, path, odb_value):
        try :
            self.device.write('SOUR:VOLT %f'%(odb_value))
        except Exception as e:
            self.client.msg("Error writing to VISA device: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to write to device")

    def output_callback(self, client, path, odb_value):
        try :
            if odb_value :
                self.device.write('OUTP:STAT 1')
            else:
                self.device.write('OUTP:STAT 0')

        except Exception as e:
            self.client.msg("Error writing to VISA device: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to write to device")


    def connect(self):
        try:
            rm = pyvisa.ResourceManager()
            self.device = rm.open_resource(self.settings["Resource"])
        except Exception as e:
            self.client.msg("error connecting to VISA device: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to connect")
        
        
    def readout_func(self):
        """
        For a periodic equipment, this function will be called periodically
        (every 60s in this case). It should return either a `midas.event.Event`
        or None (if we shouldn't write an event).
        """
        voltage = 0;
        current = 0;
        demand = 0;
        output = False;
        try :
            voltage = float(self.device.query('MEAS:VOLT?'))
            current = float(self.device.query('MEAS:CURR?'))
            demand = float(self.device.query('SOUR:VOLT?'))
            if self.device.query('OUTP:STAT?').startswith('1'):
                output = True
            else:
                output = False
        except Exception as e:
            self.client.msg("Error reading from VISA device: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to get measurement")

        event = midas.event.Event()
        data = [voltage, current]

        event.create_bank("KEIT", midas.TID_FLOAT, data)


        self.client.odb_stop_watching(f'{self.odb_variables_dir}/Demand')
        self.client.odb_set(f'{self.odb_variables_dir}/Demand', demand)
        self.client.odb_watch(f'{self.odb_variables_dir}/Demand', self.demand_callback)

        self.client.odb_stop_watching(f'{self.odb_variables_dir}/Output')
        self.client.odb_set(f'{self.odb_variables_dir}/Output', output)
        self.client.odb_watch(f'{self.odb_variables_dir}/Output', self.output_callback)
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

class KeithleyFrontend(midas.frontend.FrontendBase):
    """
    A frontend contains a collection of equipment.
    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
    """
    def __init__(self):
        # You must call __init__ from the base class.
        fe_name = "keithley_fe_%i" % midas.frontend.frontend_index
        midas.frontend.FrontendBase.__init__(self, fe_name)
        
        # create equipement for this frontend
        arduino_equip = KeithleyEquipment(self.client)
        try:
            #connect to arduino
            arduino_equip.connect();
        except:
            arduino_equip.set_status("HW Error", status_color="redLight")
        else:
            # if successful, add it to quipments
            self.add_equipment(arduino_equip)
            arduino_equip.set_status("Ok", status_color="greenLight")
        
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
    midas.frontend.parse_args()

    with KeithleyFrontend() as my_fe:
        my_fe.run()
