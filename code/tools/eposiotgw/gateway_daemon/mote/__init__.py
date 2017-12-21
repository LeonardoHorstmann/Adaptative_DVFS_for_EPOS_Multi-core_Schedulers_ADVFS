import threading
import time
import serial

class Mote:
    lock = threading.Lock()

    inter_write_time = 0.40 # seconds
    last_write = 0

    def __init__(self, serial):
        self.serial = serial
        Mote.lock.acquire()
        self.serial.close()
        self.serial.open()
        Mote.lock.release()

    def put(self, s):
        Mote.lock.acquire()
        while (time.time() - Mote.last_write) <= Mote.inter_write_time:
            pass
        self.serial.write(s)
        self.serial.flush()
        Mote.last_write = time.time()
        Mote.lock.release()

    def readline(self):
        while True:
            try:
                Mote.lock.acquire()
                ret = str(self.serial.readline(), 'utf8')
                Mote.lock.release()
                return ret
            except serial.serialutil.SerialException:
                Mote.lock.release()
            except UnicodeDecodeError:
                Mote.lock.release()
