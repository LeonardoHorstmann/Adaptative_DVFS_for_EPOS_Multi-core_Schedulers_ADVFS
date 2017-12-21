// EPOS ARM Cortex I2C Mediator Declarations

#include __MODEL_H

#ifndef __cortex_i2c_h__
#define __cortex_i2c_h__

#include <i2c.h>

__BEGIN_SYS

// WARNING: TODO: It looks like this class only implements Master operation
class I2C : private Machine_Model, private I2C_Common {
public:
    I2C(Role r = I2C_Common::MASTER, char port_sda = 'B', unsigned int pin_sda = 1, char port_scl = 'B', unsigned int pin_scl = 0)
    : _base(r == I2C_Common::MASTER ? reinterpret_cast<Log_Addr*>(I2C_MASTER_BASE) : reinterpret_cast<Log_Addr*>(I2C_SLAVE_BASE)) {
        Machine_Model::i2c_config(port_sda, pin_sda, port_scl, pin_scl);
        if(r == I2C_Common::MASTER) {
            reg(I2C_CR) = I2C_CR_MFE; //0x10;
            reg(I2C_TPR) = 0x3; // For a system clock = 32MHz, 400.000
        } else {
            reg(I2C_CR) = I2C_CR_SFE; //0x20;
        }
    }

    bool ready_to_put() { return !(reg(I2C_STAT) & I2C_STAT_BUSY); }
    bool ready_to_get() { return ready_to_put(); }

    // returns false if an error has occurred
    bool put(unsigned char slave_address, const char * data, unsigned int size) {
        bool ret = true;
        // Specify the slave address and that the next operation is a write (last bit = 0)
        reg(I2C_SA) = (slave_address << 1) & 0xFE;
        for(unsigned int i = 0; i < size; i++) {
            if(i == 0) //first byte to be sent
                ret = send_byte(slave_address, data[i], I2C_CTRL_RUN | I2C_CTRL_START);
            else if(i + 1 == size) //last byte to be sent
                ret = send_byte(slave_address, data[i], I2C_CTRL_RUN | I2C_CTRL_STOP);
            else
                ret = send_byte(slave_address, data[i], I2C_CTRL_RUN);

            if(ret)
                return false; //an error has occurred
        }
        return ret;
    }

    //returns true if an error has occurred, false otherwise.
    bool put(unsigned char slave_address, char data) {
        // Specify the slave address and that the next operation is a write (last bit = 0)
        reg(I2C_SA) = (slave_address << 1) & 0xFE;
        return send_byte(slave_address, data, I2C_CTRL_RUN | I2C_CTRL_START | I2C_CTRL_STOP);
    }

    bool get(char slave_address, char *data) {
        // Specify the slave address and that the next operation is a read (last bit = 1)
        reg(I2C_SA) = (slave_address << 1) | 0x01;
        return get_byte(slave_address, data, I2C_CTRL_RUN | I2C_CTRL_START | I2C_CTRL_STOP);

    }

    bool get(char slave_address, char *data, unsigned int size) {
        unsigned int i;
        bool ret = true;
        // Specify the slave address and that the next operation is a read (last bit = 1)
        reg(I2C_SA) = (slave_address << 1) | 0x01;
        for(i = 0; i < size; i++, data++) {
            if(i == 0) {
                ret = get_byte(slave_address, data, I2C_CTRL_START | I2C_CTRL_RUN | I2C_CTRL_ACK);
            } else if(i + 1 == size) {
                ret = get_byte(slave_address, data, I2C_CTRL_STOP | I2C_CTRL_RUN);
            } else {
                ret = get_byte(slave_address, data, I2C_CTRL_RUN | I2C_CTRL_ACK);
            }
            if(!ret) return ret;
        }
        return ret;
    }

private:
    bool send_byte(unsigned char slave_address, char data, int mode) {
        reg(I2C_DR) = data;
        reg(I2C_CTRL) = mode;
        while(!ready_to_put());
        return !(reg(I2C_STAT) & I2C_STAT_ERROR);
    }
    bool get_byte(unsigned char slave_address, char *data, int mode) {
        reg(I2C_CTRL) = mode;
        while(!ready_to_get());
        if(reg(I2C_STAT) & I2C_STAT_ERROR) {
            return false;
        } else {
            *data = reg(I2C_DR);
            return true;
        }
    }

private:
    volatile Log_Addr * _base;
    volatile Reg32 & reg(unsigned int o) { return reinterpret_cast<volatile Reg32 *>(_base)[o / sizeof(Reg32)]; }

};

__END_SYS

#endif
