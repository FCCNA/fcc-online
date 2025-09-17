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

import serial
import parse

from enum import IntEnum

class ArduinoState(IntEnum):
    Idle = 0,
    Moving = 1,
    Error = 2

class ArdutableEquipment(midas.frontend.EquipmentBase):
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
        equip_name = "ArdutableEquipment"
        
        # Define the "common" settings of a frontend. These will appear in
        # /Equipment/MyPeriodicEquipment/Common. The values you set here are
        # only used the very first time this frontend/equipment runs; after 
        # that the ODB settings are used.
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type = midas.EQ_PERIODIC
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 501
        default_common.period_ms = 1000                # readout time
        default_common.read_when = midas.RO_ALWAYS      # read also during run stop
        #default_common.log_history = True               # log "Variables" into MIDAS History

        default_settings = {
            "Serial Port": "/dev/ttyUSB0",
            "Serial Speed": 9600,
            "Editable": "Demand",
            "Names": "Arduino stage",
            "Unit Demand": "deg",
            "Unit State": "",
            "Unit Position": "deg",
            "Format Demand": "%f0",
            "Format State": "%f0",
            "Format Position": "%f3",
            "Grid display": True
        }
        
        # You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, default_common, default_settings)

        # Write demand to ODB
        self.client.odb_set(f'{self.odb_variables_dir}/Demand', float(0))
        self.client.odb_set(f'{self.odb_variables_dir}/State', int(0))
        self.client.odb_set(f'{self.odb_variables_dir}/Position', float(0))
        self.client.odb_watch(f'{self.odb_variables_dir}/Demand', self.demand_callback)

        # Write LED variable to ODB
        self.client.odb_set(f'{self.odb_variables_dir}/Led', int(0))  # 0=off, 1=on
        self.client.odb_watch(f'{self.odb_variables_dir}/Led', self.led_callback)

        self.state = ArduinoState.Idle

        # setup parser
        self.parser = parse.compile('Current position: {pos:f}°\r\n')

    def demand_callback(self, client, path, odb_value):
        #print("New demand %s is %s" % (path, odb_value))
        if self.state != ArduinoState.Idle:
            self.client.msg("Tried to move while still moving", is_error=True);
            return

        self.state = ArduinoState.Moving
        pos = self.client.odb_get(f'{self.odb_variables_dir}/Position')

        try :
            if pos > odb_value:
                self.ser.write(f'i{int(pos-odb_value):03d}\n'.encode('UTF-8'));
            else:
                self.ser.write(f'a{int(odb_value-pos):03d}\n'.encode('UTF-8'));
        except Exception as e:
            self.client.msg("Error moving: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to get measurement")

    def led_callback(self, client, path, odb_value):
        """Callback when the LED value changes in ODB"""
        if self.state != ArduinoState.Idle:
            self.client.msg("Tried to change LED while moving", is_error=True)
            return

        try:
            if int(odb_value) == 1:
                self.ser.write(b'led_on\n')
            else:
                self.ser.write(b'led_off\n')
        except Exception as e:
            self.client.msg(f"Error sending LED command: {e}", is_error=True)
            raise RuntimeError("Fail to send LED command")


    def connect(self):
        #connect to serial port
        try:
            self.ser = serial.Serial(self.settings["Serial Port"], self.settings["Serial Speed"]);
        except Exception as e:
            self.client.msg("error open serial port: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to connect")

        
    def readout_func(self):
        """
        For a periodic equipment, this function will be called periodically
        (every 1s in this case). It should return either a `midas.event.Event`
        or None (if we shouldn't write an event).
        """

        if self.state == ArduinoState.Idle:
            # read position from arduino
            try :
                self.ser.write(b'pos\n');
                result = self.ser.readline().decode();
            except Exception as e:
                self.client.msg("Error reading position: %s" % str(e), is_error=True);
                raise RuntimeError("Fail to get measurement")

            # parse data
            values = self.parser.parse(result);

            if values is None :
                self.client.msg("Error parsing string \'%s\'" % result.rstrip(), is_error=True);
                raise ValueError("cannot parse string");
            
            self.client.odb_set(f'{self.odb_variables_dir}/Position', round(values["pos"], 3))
        elif self.state == ArduinoState.Moving:
            # check movement is complete
            try :
                if self.ser.in_waiting > 0: # to be tuned
                    s = self.ser.readline().decode();

                    if(s.startswith("Moved ")):
                        self.state = ArduinoState.Idle
                #else:
                #    print("still moving")
            except Exception as e:
                self.client.msg("Error checking end of movement: %s" % str(e), is_error=True);
                raise RuntimeError("Fail to read")

        # always update the state
        self.client.odb_set(f'{self.odb_variables_dir}/State', self.state.value)

        #return event
        return None

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

class ArdutableFrontend(midas.frontend.FrontendBase):
    """
    A frontend contains a collection of equipment.
    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
    """
    def __init__(self):
        # You must call __init__ from the base class.
        midas.frontend.FrontendBase.__init__(self, "ardutable_fe")
        
        
        # create equipement for this frontend
        arduino_equip = ArdutableEquipment(self.client)
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
    with ArdutableFrontend() as my_fe:
        my_fe.run()
