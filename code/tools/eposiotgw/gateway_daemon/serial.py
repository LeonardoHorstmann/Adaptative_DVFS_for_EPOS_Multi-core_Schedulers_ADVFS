import serial
import struct
import requests
import traceback
import sys
import json

from gateway_daemon.mote import Mote
from gateway_daemon.modbus import parse


MODBUS_FUNCTIONS = {
    3 : 'numeric',
    1 : 'binary'
}
def clean_data(data):
    '''Remove /r/n'''
    while len(data) > 0 and (data[-1] == '\r' or data[-1] == '\n'):
        data = data[:-1]
    return data


def free_data(data):
    try:
        register, cmd, bytes, coordinates, mac_hash, scale = parse(data)
        command = MODBUS_FUNCTIONS[cmd]
    except ValueError:
        raise
    except KeyError:
        pass
    try:
        offset = int.from_bytes(bytes[:2], byteorder='big')
        value = int.from_bytes(bytes[2:], byteorder='big')
        timestamp = int.from_bytes(bytes[5:], byteorder='big')
        tstp_si = int.frombytes(bytes[15:], byteorder='big')
    except struct.error:
        raise
    return (hex(register).split('x')[1].upper(), command, str(offset),
str(value), timestamp, coordinates, tstp_si, str(mac_hash), scale)

def mount_request(data):
    try:
        register, command, offset, value, timestamp, coordinates, tstp_si, mac_hash, scale = free_data(data)
        post = {
            "name": str(register+command+offset),
            "value": float(value),
            "timestamp": int(timestamp),
            "tags": {
                "coordinate_x": coordinates[0],
                "coordinate_y": coordinates[1],
                "coordinate_z": coordinates[2],
                "coordinate_t": coordinates[3],
                "tstp_si": tstp_si,
                "mac_hash": mac_hash,
                "spatial_scale": scale[0],
                "temporal_scale": scale[1]
            }
        }
        return post
    except KeyError:
        raise
    except ValueError:
        raise
    except struct.error:
        raise

def make_request(data):
    data = clean_data(data)
    try:
        post = mount_request(data)
        done_post = requests.post('http://your_kairosdb_server:8080/api/v1/datapoints', data = json.dumps(post))
    except (KeyboardInterrupt, SystemExit):
        raise
    except:
        print("Error in message", data, file=sys.stderr)
        traceback.print_exc()

def run_serial():
    serial_connection = serial.Serial('/dev/ttyS2', 115200, timeout=None)
    mote = Mote(serial_connection)
    while True:
        data = mote.readline()
        print("Read from serial:", data)
        make_request(data)
