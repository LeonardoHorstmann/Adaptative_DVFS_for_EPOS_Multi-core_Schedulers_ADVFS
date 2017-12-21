from setuptools import setup, find_packages

VERSION = '0.1'

setup(
    name='gateway_daemon',
    version=VERSION,
    packages=find_packages(),
    entry_points={
        'console_scripts': [
            'gateway_daemon = gateway_daemon.__main__:run',
        ],
    },
    install_requires=[
        'Flask==0.10.1',
        'pyserial==2.7',
        'tornado==4.2.1',
        'requests==2.8.0',
    ],
)
