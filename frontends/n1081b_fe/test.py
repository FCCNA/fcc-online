import sys
from os.path import dirname
sys.path.append(dirname(__file__) + '/SDK/src')

from N1081B_sdk import N1081B
N1081B_device = N1081B("192.168.50.20")
N1081B_device.connect()
N1081B_device.login('password')

for section in [N1081B.Section.SEC_A, N1081B.Section.SEC_B, N1081B.Section.SEC_C, N1081B.Section.SEC_D]:
    print("-- SECTION --")
    print(N1081B_device.get_input_configuration(section))
    print(N1081B_device.get_output_configuration(section))
    print(N1081B_device.get_function_configuration(section))
    print(N1081B_device.get_function_results(section))
