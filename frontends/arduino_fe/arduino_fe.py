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

class ArduinoEquipment(midas.frontend.EquipmentBase):
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
        equip_name = "ArduinoEquipment"
        
        # Define the "common" settings of a frontend. These will appear in
        # /Equipment/MyPeriodicEquipment/Common. The values you set here are
        # only used the very first time this frontend/equipment runs; after 
        # that the ODB settings are used.
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type = midas.EQ_PERIODIC
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 500
        default_common.period_ms = 60000                # readout time
        default_common.read_when = midas.RO_ALWAYS      # read also during run stop
        default_common.log_history = True               # log "Variables" into MIDAS History

        default_settings = {
            "Serial Port": "/dev/ttyUSB0",
            "Serial Speed": 115200,
            "Names ARD0": ["Temperature", "Humidity", "Atmospheric Pressure"],
            "Unit ARD0": ["C", "%", "hPa"],
            "Names ARD1": ["Temperature", "Humidity"],
            "Unit ARD1": ["C", "%"]
        }

        
        # You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, default_common, default_settings)

        # setup parser
        self.parser_bme = parse.compile('{temperature:f} *C, {pressure:f} hPa, {humidity:f} %\r\n')
        self.parser_sht = parse.compile('{temperature:f} *C, {humidity:f} %\r\n')


    def connect(self):
        #connect to serial port
        try:
            self.ser = serial.Serial(self.settings["Serial Port"], self.settings["Serial Speed"]);
        except Exception as e:
            self.client.msg("error open serial port: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to connect")

        # required to populate the ODB if writing by hand
        """
        # get a measurement
        try :
            self.ser.write(b'T');
            result = self.ser.readline().decode();
        except Exception as e:
            self.client.msg("Error reading measurement: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to get measurement")
        
        # parse data
        values = self.parser_bme.parse(result);

        if values is None :
            self.client.msg("Error parsing string \'%s\'" % result.rstrip(), is_error=True);
            raise ValueError("cannot parse string");

        temperature = values["temperature"]
        humidity = values["humidity"]
        pressure = values["pressure"]

        # copy default variables to ODB
        default_variables = collections.OrderedDict([
            ("Temperature", values["temperature"]),
            ("Atmospheric Pressure", values["pressure"]),
            ("Humidity", values["humidity"])
        ])
        self.client.odb_set(f"{self.odb_variables_dir}", default_variables, remove_unspecified_keys=False)
        """
        
    def readout_func(self):
        """
        For a periodic equipment, this function will be called periodically
        (every 60s in this case). It should return either a `midas.event.Event`
        or None (if we shouldn't write an event).
        """

        # read bme from arduino
        try :
            self.ser.write(b'T');
            result = self.ser.readline().decode();
        except Exception as e:
            self.client.msg("Error reading bme measurement: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to get measurement")
        
        # parse data
        values = self.parser_bme.parse(result);

        if values is None :
            self.client.msg("Error parsing bme string \'%s\'" % result.rstrip(), is_error=True);
            raise ValueError("cannot parse string");

        temperature_bme = values["temperature"]
        humidity_bme = values["humidity"]
        pressure_bme = values["pressure"]

        # Create an event
        event = midas.event.Event()

        # Create a bank (called "ARD0") which in this case will store 3 floats.
        # data can be a list, a tuple or a numpy array.
        data = [temperature_bme, humidity_bme, pressure_bme]

        event.create_bank("ARD0", midas.TID_FLOAT, data)

        #option to fill ODB by hand, instead that with "Log History"
        """
        #fill the ODB
        self.client.odb_set(f'{self.odb_variables_dir}/Temperature', round(temperature_bme, 3))
        self.client.odb_set(f'{self.odb_variables_dir}/Humidity', round(humidity_bme, 3))
        self.client.odb_set(f'{self.odb_variables_dir}/Atmospheric Pressure', round(pressure_bme, 3))"""

        # read sht from arduino
        try :
            self.ser.write(b'R');
            result = self.ser.readline().decode();
        except Exception as e:
            self.client.msg("Error reading bme measurement: %s" % str(e), is_error=True);
            raise RuntimeError("Fail to get measurement")

        # parse data
        values = self.parser_sht.parse(result);

        if values is None :
            self.client.msg("Error parsing sht string \'%s\'" % result.rstrip(), is_error=True);
            raise ValueError("cannot parse string");

        temperature_sht = values["temperature"]
        humidity_sht = values["humidity"]

        # Create a bank (called "ARD1") which in this case will store 2 floats.
        # data can be a list, a tuple or a numpy array.
        data = [temperature_sht, humidity_sht]

        event.create_bank("ARD1", midas.TID_FLOAT, data)

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

class ArduinoFrontend(midas.frontend.FrontendBase):
    """
    A frontend contains a collection of equipment.
    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
    """
    def __init__(self):
        # You must call __init__ from the base class.
        midas.frontend.FrontendBase.__init__(self, "arduino_fe")
        
        
        # create equipement for this frontend
        arduino_equip = ArduinoEquipment(self.client)
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
    with ArduinoFrontend() as my_fe:
        my_fe.run()
