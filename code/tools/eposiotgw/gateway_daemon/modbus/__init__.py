'''This module is responsible for building and interpreting Modbus messages.

Implements only the subset of Modbus we use.
'''

import struct


READ_HOLDING_REGISTER = 0x03
READ_COILS = 0x01
WRITE_HOLDING_REGISTER = 0x06
WRITE_SINGLE_COIL = 0x05


def chunks(sequence, n):
    '''Splits a sequence into evenly-sized chunks of size n

    Args:
        sequence (sequence): the sequence to split
        n (int): the size of the chunks

    Yields:
        list: a chunk
    '''
    for i in range(0, len(sequence), n):
        yield sequence[i:i+n]


def string_chunks(string, n):
    '''Splits a string into evenly-sized chunks of size n

    Args:
        string (str): the string to split
        n (int): the size of the chunks

    Returns:
        string: a list of strings (chunks)
    '''
    return [''.join(pieces) for pieces in chunks(string, n)]


def parse(message):
    '''Parse a message with Modbus.

    Args:
        message (str): a Modbus ASCII message.

    Returns:
        (int, int, bytes): a tuple consisting of address, command and data
    '''
    if len(message) <= 0:
        raise ValueError('Message with size <= 0')

    if message[0] != ':':
        raise ValueError('Modbus message should start with :')

    address = int(message[1:3], 16)

    command = int(message[3:5], 16)

    size = int(message[5:7], 16)

    data = message[7:-2]

    coordinates = message[15:]

    mac_hash = message[55:]

    scale = message[65:]

    byte_list = [int(byte, 16) for byte in string_chunks(data, 2)]
    data = b''.join(struct.pack('B', byte) for byte in byte_list)

    expected_lrc = address + command + size + sum(byte_list)
    expected_lrc = ((expected_lrc ^ 0xFF) + 1) & 0xFF

    lrc = int(message[-2:], 16)

    print(hex(address), command, size, data, message[-2], message[-1])

    if expected_lrc != lrc:
        raise ValueError('Modbus message lrc does not match: {} != {}'
                         .format(hex(expected_lrc), hex(lrc)))

    return address, command, data, coordinates, mac_hash, scale


def build(address, command, byte_list):
    '''Build a Modbus message.

    Args:
        address (int): the address, should be only one byte
        command (int): the modbus command, should be one of the constants
                       defined by this module
        byte_list (bytes): the data to use as payload, should be formatted
                           according to its use

    Returns:
        str: the Modbus message built as requested
    '''
    encoded_data = ''.join('{:02x}'.format(byte) for byte in byte_list)

    lrc = address + command + sum(byte_list)
    lrc = ((lrc ^ 0xFF) + 1) & 0xff

    return ':{:02x}{:02x}{}{:02x}'.format(address,
                                          command,
                                          encoded_data,
                                          lrc).upper() + '\r\n'
