'''Tests for the modbus package.'''


from unittest import TestCase

from gateway_daemon.modbus import parse, build

TEST_MESSAGE = ':A0036162636465666768696A6BFB'


class ModbusTest(TestCase):
    '''Test fixture for the modbus package.'''
    def test_has_to_start_with_colon(self):
        '''A modbus message always starts with ":"'''
        with self.assertRaises(ValueError):
            parse('hello')

    def test_returns_correct_address(self):
        '''The first 2 bytes are the address.'''
        address, _, _ = parse(TEST_MESSAGE)
        self.assertEqual(address, 0xA0)

    def test_returns_correct_command(self):
        '''The bytes after the address encode the command.'''
        _, command, _ = parse(TEST_MESSAGE)
        self.assertEqual(command, 0x03)

    def test_returns_correct_message(self):
        '''There should be a message of correct length.'''
        _, _, message = parse(TEST_MESSAGE)
        self.assertEqual(message, b'abcdefghijk')

    def test_build_correctly(self):
        '''Should build a message as expected.'''
        encoded = build(0xA0, 0x03, b'abcdefghijk')
        self.assertEqual(encoded, TEST_MESSAGE)
