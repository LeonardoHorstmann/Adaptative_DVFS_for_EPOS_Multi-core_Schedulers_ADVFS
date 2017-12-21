'''This module is responsible for handling http connections from a client

This module supports running a server (based on tornado+flask) for handling
connections from clients interested in communicating with the wireless
sensor network, such as a SCADA component.
'''

import struct
import json
import serial

from flask import Flask, request
from tornado.wsgi import WSGIContainer
from tornado.httpserver import HTTPServer
from tornado.ioloop import IOLoop
from gateway_daemon.mote import Mote
from gateway_daemon.modbus import (WRITE_HOLDING_REGISTER,
                                   WRITE_SINGLE_COIL,
                                   build)


app = Flask(__name__)


def write_single_coil(number, data):
    '''Create data for building a "Write Single Coil" operation

    Args:
        number (int): the coil address
        data (bool): what to write to the coil

    Returns:
        (int, bytes): the operation and the packed data
    '''
    data = json.loads(data)
    return WRITE_SINGLE_COIL, struct.pack('!HH', number, data)


def write_holding_register(number, data):
    '''Create data for building a "Write Holding Register" operation

    Args:
        number (int): the register address
        data (int): what to write to the register, up to a short

    Returns:
        (int, bytes): the operation and the packed data
    '''
    data = int(float(data))
    return WRITE_HOLDING_REGISTER, struct.pack('!HH', number, data)


MODBUS_FUNCTIONS = {
    'numeric': write_holding_register,
    'binary': write_single_coil,
}


def decompose(name):
    '''Decompose a request parameter name into its intentions.

    Args:
        name (str): the parameter name, should be of the form
                    W_A_D_N
                    where W is a description of what is to be
                    altered, A is a list of addresses separated
                    by "-", D is the data type (numeric or binary)
                    and N is the number of the channel to alter

    Returns:
        (list, int, int): a list of addresses (ints), the modbus operation type
                          and the register or coil address.Cr
    '''
    _, address, data_type, number = name.split('_')
    addresses = address.split('-')
    return ([int(addr, 16) for addr in addresses],
            data_type,
            int(number, 16))


def modbus_data(data_type, number, data):
    '''Dispatches data to the correct handling function for an operation.

    Args:
        data_type (str): "numeric" or "binary"
        number (int): the address of the coil or register
        data (int): what to write (may be bool)
    '''
    return MODBUS_FUNCTIONS[data_type](number, data)



@app.route('/network/', methods=['POST'])
def network():
    '''This function handles actuator requests for the network.'''
    print(request.values)

    serial_connection = serial.Serial('/dev/ttyS2', 115200, timeout=None)
    mote = Mote(serial_connection)

    for key in request.values:
        data = request.values[key]

        addresses, data_type, number = decompose(key)

        function, data = modbus_data(data_type, number, data)

        for address in addresses:
            print(address, function, number, data)
            modbus = build(address, function, data)
            print("Writing to serial:", modbus)
            mote.put(modbus.encode('ascii'))

    return ''


def run_server():
    '''This function starts and runs a http server. Can be run inside a thread.

    Never returns.
    '''
    http_server = HTTPServer(WSGIContainer(app))
    http_server.listen(5000)
    IOLoop.instance().start()
