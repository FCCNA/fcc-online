#!/usr/bin/env python3
"""
Arduino-based readout of BME680 

requires python3-pyserial
"""

import midas
import midas.frontend
import midas.event
import collections

import serial
from simple_pid import PID

# flag used for debugging, swith to True to enable a verbose version
debug = False

class LaudaEquipment(midas.frontend.EquipmentBase):
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
        equip_name = "LaudaEquipment"
        
        # Define the "common" settings of a frontend. These will appear in
        # /Equipment/MyPeriodicEquipment/Common. The values you set here are
        # only used the very first time this frontend/equipment runs; after 
        # that the ODB settings are used.
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type = midas.EQ_PERIODIC
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 501
        default_common.period_ms = 60000                # readout time
        default_common.read_when = midas.RO_ALWAYS      # read also during run stop
        default_common.log_history = True               # log "Variables" into MIDAS History
        
        default_settings = {
            "Serial Port": "/dev/ttyUSB0", #ACM1
            "Serial Speed": 9600,
            "Names LAU0": ["Bath Temperature", "Status", "Cooling state", "SetPoint Temperature"],
            "Unit LAU0": ["C", "", "","C"],
            "SetPoint": 20.00,
            "Cooling": True,
            "Feedback": False,
            "Demand Feedback": 20.00
        }


        # You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, default_common, default_settings)

        
    def connect(self):
        #connect to serial port
        try:
            self.ser = serial.Serial(self.settings["Serial Port"], self.settings["Serial Speed"]);
        except Exception as e:
            self.client.msg("error open serial port: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to connect")


        try:
            self.ser.write(b'IN_SP_00\r\n')
            setpoint_temperature_string = self.ser.readline().decode()
            setpoint_temperature = float(setpoint_temperature_string)

            
        except Exception as e:
            self.client.msg("Error reading initial setpoint measurement: %s" % str(e), is_error = True)
            setpoint_temperature = 20.00

            
        try:
            self.ser.write(b'IN_MODE_02\r\n')
            cooling_state_string = self.ser.readline().decode()
            cooling_state = 1-float(cooling_state_string)

        except Exception as e:
            self.client.msg("Error reading initial cooling state: %s" % str(e), is_error = True)
            cooling_state = 0

            
        self.client.odb_set(f'{self.odb_settings_dir}/SetPoint', setpoint_temperature)
        self.client.odb_set(f'{self.odb_settings_dir}/Cooling', cooling_state)

    def readout_func(self):
        """
        For a periodic equipment, this function will be called periodically
        (every 60s in this case). It should return either a `midas.event.Event`
        or None (if we shouldn't write an event).
        """

        #IN_PV_00 (bath temperature)
        #STATUS (0 device is OK, -1 fault)
        #IN_SP_00 (temperature setpoint)
        #IN_MODE_02 (0 is device ON, 1 is device OFF-standby, the worst logic ever...)
        
        # read from lauda
        try :
            self.ser.write(b'IN_PV_00\r\n')
            bath_temperature_string = self.ser.readline().decode()
            bath_temperature = float(bath_temperature_string)

        except Exception as e:
            self.client.msg("Error reading bath temperature measurement: %s" % str(e), is_error = True)
            bath_temperature = float('nan')


        try:
            self.ser.write(b'STATUS\r\n')
            status_string = self.ser.readline().decode()
            status = float(status_string)

        except Exception as e:
            self.client.msg("Error reading chiller status: %s" % str(e), is_error = True)
            status = float('nan')

            
        try:
            self.ser.write(b'IN_SP_00\r\n')
            setpoint_temperature_string = self.ser.readline().decode()
            setpoint_temperature = float(setpoint_temperature_string)

        except Exception as e:
            self.client.msg("Error reading setpoint: %s" % str(e), is_error = True)
            setpoint_temperature = float('nan')

            
        try:
            self.ser.write(b'IN_MODE_02\r\n')
            cooling_state_string = self.ser.readline().decode()
            cooling_state = float(cooling_state_string)

        except Exception as e:
            self.client.msg("Error reading cooling state: %s" % str(e), is_error = True)
            cooling_state = float('nan')

        #trigger a feedback to control temperature in the box
        if self.client.odb_get(f'{self.odb_settings_dir}/Feedback'):

            pid = PID(0.2, 0., 50., self.client.odb_get(f"{self.odb_settings_dir}/Demand Feedback"))
            pid.output_limits = (-6., 6.)

            tbox = self.client.odb_get("/Equipment/ArduinoEquipment/Variables/ARD0[0]")

            output = pid(tbox)

            self.client.odb_set(f"{self.odb_settings_dir}/SetPoint", setpoint_temperature + output)                

            setpoint_temperature += output

        # Create an event
        event = midas.event.Event()

        # Create a bank (called "LAU0") which in this case will store 4 floats.
        # data can be a list, a tuple or a numpy array.
        data = [bath_temperature, status, cooling_state, setpoint_temperature]

        event.create_bank("LAU0", midas.TID_FLOAT, data)

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

        #OUT_SP_00_XXX.XX (setpoint transfer)
        #START and STOP
        
        if path == f'{self.odb_settings_dir}/SetPoint':
            
            digits = [char for char in str("{:.2f}".format(new_value)) if char.isdigit()]
            
            if len(digits) == 3:
                setvalue = digits[0]+"."+digits[1]+digits[2]
                command = "OUT_SP_00_00"+setvalue+"\r\n"
        
            elif len(digits) == 4:
                setvalue = digits[0]+digits[1]+"."+digits[2]+digits[3]
                command = "OUT_SP_00_0"+setvalue+"\r\n"

            elif len(digits) == 5:
                setvalue = digits[0]+digits[1]+digits[2]+"."+digits[3]+digits[4]
                command = "OUT_SP_00_"+setvalue+"\r\n"

            try:
                self.ser.write(command.encode())
                output = self.ser.readline().decode()
                if not output.startswith("OK"):
                    raise RuntimeError("Wrong response to SetPoint")
                                    
            except Exception as e:
                self.client.msg("Error writing setpoint: %s" % str(e), is_error = True)

            if debug: print("New SetPoint", new_value)

        elif path == f'{self.odb_settings_dir}/Cooling':

            if new_value == 1:
                command = "START\r\n"
                if debug: print("New cooling",new_value,command)

            elif new_value == 0:
                command = "STOP\r\n"
                if debug: print("New cooling",new_value,command)
            
            try:
                if debug: print("try to write the new cooling state...",command.encode())
                self.ser.write(command.encode())
                output = self.ser.readline().decode()
                if debug: print("OUTPUT",output)
                if not output.startswith("OK"):
                    raise RuntimeError("Wrong response to Cooling State")
                                    
            except Exception as e:
                self.client.msg("Error writing cooling state: %s" % str(e), is_error = True)

            if debug: print("New Cooling State", new_value)

            
            


class LaudaFrontend(midas.frontend.FrontendBase):
    """
    A frontend contains a collection of equipment.
    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
    """
    def __init__(self):
        # You must call __init__ from the base class.
        midas.frontend.FrontendBase.__init__(self, "lauda_fe")
        
        
        # create equipement for this frontend
        lauda_equip = LaudaEquipment(self.client)
        try:
            #connect to arduino
            lauda_equip.connect();
        except:
            lauda_equip.set_status("HW Error", status_color="redLight")
        else:
            # if successful, add it to quipments
            self.add_equipment(lauda_equip)
            lauda_equip.set_status("Ok", status_color="greenLight")
        
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
    with LaudaFrontend() as my_fe:
        my_fe.run()
