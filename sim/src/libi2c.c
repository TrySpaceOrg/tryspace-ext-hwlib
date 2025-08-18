#include <stdint.h>
#include <stdlib.h>

/* hwlib API */
#include "libi2c.h"
#include "simulith.h"

/*
 * Helper: Map an i2c_bus_info_t to a unique TCP port.
 * Uses SIMULITH_I2C_BASE_PORT + (bus_id * 100) + device_addr
 * Example: base_port = 7000, Bus 0 Device 0x50 -> 7050, Bus 1 Device 0x51 -> 7151
 */
#define HWLIB_I2C_MAX_DEVICES 256

static void make_simulith_i2c_address(char* out, size_t outlen, int bus_id, int device_addr) 
{
    int port = SIMULITH_I2C_BASE_PORT + (bus_id * 100) + device_addr;
    snprintf(out, outlen, "tcp://tryspace-director:%d", port);
    OS_printf("HWLIB: make_simulith_i2c_address: Bus %d Device 0x%02X -> %s\n", bus_id, device_addr, out);
}

/*
 * Simulith I2C device storage: indexed by a hash of bus_id and device address
 */
static i2c_device_t *simulith_i2c_devices[HWLIB_I2C_MAX_DEVICES] = {0};

static int get_device_index(int bus_id, int device_addr)
{
    return (bus_id * 128) + device_addr; // Simple hash, assumes bus_id < 2 and device_addr < 128
}

int32_t i2c_master_init(i2c_bus_info_t* device)
{
    int32_t status = I2C_SUCCESS;

    if (!device) 
    {
        OS_printf("HWLIB: i2c_master_init: device is NULL\n");
        return I2C_ERROR;
    }

    int bus_id = (int)(device->handle);
    int device_addr = (int)(device->addr);
    int idx = get_device_index(bus_id, device_addr);

    if (idx < 0 || idx >= HWLIB_I2C_MAX_DEVICES) 
    {
        OS_printf("HWLIB: i2c_master_init: invalid device index %d (bus %d, addr 0x%02X)\n", idx, bus_id, device_addr);
        return I2C_ERROR;
    }

    if (!simulith_i2c_devices[idx]) 
    {
        i2c_device_t *i2c_dev = (i2c_device_t *)calloc(1, sizeof(i2c_device_t));
        if (!i2c_dev) 
        {
            OS_printf("HWLIB: i2c_master_init: failed to allocate i2c_device_t\n");
            return I2C_ERROR;
        }
        
        // Set logical name for logging
        snprintf(i2c_dev->name, sizeof(i2c_dev->name), "I2C%d_0x%02X", bus_id, device_addr);
        make_simulith_i2c_address(i2c_dev->address, sizeof(i2c_dev->address), bus_id, device_addr);
        i2c_dev->is_server = 0; // Always connect, never bind
        i2c_dev->bus_id = bus_id;
        i2c_dev->device_addr = device_addr;
        simulith_i2c_devices[idx] = i2c_dev;
    }

    i2c_device_t *i2c_dev = simulith_i2c_devices[idx];
    status = simulith_i2c_init(i2c_dev);
    if(status == SIMULITH_I2C_SUCCESS)
    {
        device->isOpen = I2C_OPEN;
    }
    else
    {
        OS_printf("HWLIB: simulith_i2c_init failed with status %d\n", status);
        device->isOpen = I2C_CLOSED;
        status = I2C_ERROR;
    }
    return status;
}

/* nos i2c transaction */
int32_t i2c_master_transaction(i2c_bus_info_t* device, uint8_t addr, void * txbuf, uint8_t txlen,
                               void * rxbuf, uint8_t rxlen, uint16_t timeout)
{
    if (!device) return I2C_ERROR;
    
    int bus_id = (int)(device->handle);
    int device_addr = (int)(device->addr);
    int idx = get_device_index(bus_id, device_addr);
    
    if (idx < 0 || idx >= HWLIB_I2C_MAX_DEVICES || !simulith_i2c_devices[idx]) 
    {
        OS_printf("HWLIB: i2c_master_transaction: invalid device\n");
        return I2C_ERROR;
    }
    
    i2c_device_t *i2c_dev = simulith_i2c_devices[idx];
    int32_t status = simulith_i2c_transaction(i2c_dev, (const uint8_t*)txbuf, txlen, (uint8_t*)rxbuf, rxlen);
    
    if(status != SIMULITH_I2C_SUCCESS)
    {
        OS_printf("HWLIB: simulith_i2c_transaction failed with status %d\n", status);
        return I2C_ERROR;
    }
    return I2C_SUCCESS;
}

int32_t i2c_read_transaction(i2c_bus_info_t* device, uint8_t addr, void * rxbuf, uint8_t rxlen, uint8_t timeout)
{
    if (!device) return I2C_ERROR;
    
    int bus_id = (int)(device->handle);
    int device_addr = (int)(device->addr);
    int idx = get_device_index(bus_id, device_addr);
    
    if (idx < 0 || idx >= HWLIB_I2C_MAX_DEVICES || !simulith_i2c_devices[idx]) 
    {
        OS_printf("HWLIB: i2c_read_transaction: invalid device\n");
        return I2C_ERROR;
    }
    
    i2c_device_t *i2c_dev = simulith_i2c_devices[idx];
    int32_t status = simulith_i2c_read(i2c_dev, (uint8_t*)rxbuf, rxlen);
    
    if(status < 0)
    {
        OS_printf("HWLIB: simulith_i2c_read failed with status %d\n", status);
        return I2C_ERROR;
    }
    return I2C_SUCCESS;
}

int32_t i2c_write_transaction(i2c_bus_info_t* device, uint8_t addr, void * txbuf, uint8_t txlen, uint8_t timeout)
{
    if (!device) return I2C_ERROR;
    
    int bus_id = (int)(device->handle);
    int device_addr = (int)(device->addr);
    int idx = get_device_index(bus_id, device_addr);
    
    if (idx < 0 || idx >= HWLIB_I2C_MAX_DEVICES || !simulith_i2c_devices[idx]) 
    {
        OS_printf("HWLIB: i2c_write_transaction: invalid device\n");
        return I2C_ERROR;
    }
    
    i2c_device_t *i2c_dev = simulith_i2c_devices[idx];
    int32_t status = simulith_i2c_write(i2c_dev, (const uint8_t*)txbuf, txlen);
    
    if(status < 0)
    {
        OS_printf("HWLIB: simulith_i2c_write failed with status %d\n", status);
        return I2C_ERROR;
    }
    return I2C_SUCCESS;
}

int32_t i2c_multiple_transaction(i2c_bus_info_t* device, uint8_t addr, struct i2c_rdwr_ioctl_data* rdwr_data, uint16_t timeout)
{
    // For now, return error as this is more complex to implement with the current architecture
    OS_printf("HWLIB: i2c_multiple_transaction: not implemented in simulith mode\n");
    return I2C_ERROR;
}

int32_t i2c_master_close(i2c_bus_info_t* device) 
{
    if (!device) return I2C_ERROR;
    
    int bus_id = (int)(device->handle);
    int device_addr = (int)(device->addr);
    int idx = get_device_index(bus_id, device_addr);
    
    if (idx < 0 || idx >= HWLIB_I2C_MAX_DEVICES || !simulith_i2c_devices[idx]) 
    {
        return I2C_ERROR;
    }
    
    i2c_device_t *i2c_dev = simulith_i2c_devices[idx];
    int32_t status = simulith_i2c_close(i2c_dev);
    
    if(status < 0)
    {
        OS_printf("HWLIB: simulith_i2c_close failed with status %d\n", status);
    }
    
    free(simulith_i2c_devices[idx]);
    simulith_i2c_devices[idx] = NULL;
    device->isOpen = I2C_CLOSED;
    
    return (status == SIMULITH_I2C_SUCCESS) ? I2C_SUCCESS : I2C_ERROR;
}
