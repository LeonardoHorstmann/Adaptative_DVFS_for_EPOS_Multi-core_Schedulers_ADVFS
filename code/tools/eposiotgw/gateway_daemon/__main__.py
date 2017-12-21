from threading import Thread

from gateway_daemon.http import run_server
from gateway_daemon.serial import run_serial

def run():
    '''Builds every thread needed and joins them'''
    a = Thread(target=run_server)
    b = Thread(target=run_serial)
    a.start()
    b.start()
    a.join()
    b.join()

if __name__ == '__main__':
    run()
