/* Copyright (C) 2009 - 2019 National Aeronautics and Space Administration. All Foreign Rights are Reserved to the U.S. Government.

This software is provided "as is" without any warranty of any, kind either express, implied, or statutory, including, but not
limited to, any warranty that the software will conform to, specifications any implied warranties of merchantability, fitness
for a particular purpose, and freedom from infringement, and any warranty that the documentation will conform to the program, or
any warranty that the software will be error free.

In no event shall NASA be liable for any damages, including, but not limited to direct, indirect, special or consequential damages,
arising out of, resulting from, or in any way connected with the software or its documentation.  Whether or not based upon warranty,
contract, tort or otherwise, and whether or not loss was sustained from, or arose out of the results of, or use of, the software,
documentation or services provided hereunder

ITC Team
NASA IV&V
ivv-itc@lists.nasa.gov
*/

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* hwlib API */
#include "libspi.h"

/* simulith API */
#include "simulith_spi.h"

// Storage for SPI device structures mapped to spi_info_t devices
static spi_device_t spi_devices[MAX_SPI_BUSES * 8]; // Max 8 CS per bus
static int device_count = 0;

// Helper function to get or create simulith device for spi_info_t
static spi_device_t* get_simulith_device(spi_info_t* device)
{
    if (!device) return NULL;
    
    // Search for existing device
    for (int i = 0; i < device_count; i++) {
        if (spi_devices[i].bus_id == device->bus && spi_devices[i].cs_id == device->cs) {
            return &spi_devices[i];
        }
    }
    
    // Create new device if not found
    if (device_count >= (MAX_SPI_BUSES * 8)) {
        return NULL; // Too many devices
    }
    
    spi_device_t* sim_device = &spi_devices[device_count++];
    sim_device->bus_id = device->bus;
    sim_device->cs_id = device->cs;
    sim_device->init = 0;
    
    // Create unique name and address based on bus and CS
    snprintf(sim_device->name, sizeof(sim_device->name), "spi%d_cs%d", device->bus, device->cs);
    snprintf(sim_device->address, sizeof(sim_device->address), "tcp://localhost:%d", 
             SIMULITH_SPI_BASE_PORT + (device->bus * 8) + device->cs);
    
    // Default to client mode (can be overridden by environment or configuration)
    sim_device->is_server = 0;
    
    return sim_device;
}

int32_t spi_init_dev(spi_info_t* device)
{
    if (!device) return SPI_ERROR;
    
    spi_device_t* sim_device = get_simulith_device(device);
    if (!sim_device) {
        return SPI_ERROR;
    }
    
    int result = simulith_spi_init(sim_device);
    if (result == SIMULITH_SPI_SUCCESS) {
        device->isOpen = SPI_DEVICE_OPEN;
        return SPI_SUCCESS;
    }
    
    return SPI_ERROR;
}

/* nos spi chip select */
int32_t spi_select_chip(spi_info_t* device)
{
    // In simulith, chip select is implicit in addressing, so this is a no-op
    if (!device) return SPI_ERROR;
    return SPI_SUCCESS;
}

/* nos spi chip unselect */
int32_t spi_unselect_chip(spi_info_t* device)
{
    // In simulith, chip select is implicit in addressing, so this is a no-op
    if (!device) return SPI_ERROR;
    return SPI_SUCCESS;
}

/* nos spi write */
int32_t spi_write(spi_info_t* device, uint8_t data[], const uint32_t numBytes)
{
    if (!device || device->isOpen != SPI_DEVICE_OPEN) return SPI_ERROR;
    
    spi_device_t* sim_device = get_simulith_device(device);
    if (!sim_device) return SPI_ERROR;
    
    int result = simulith_spi_write(sim_device, data, numBytes);
    if (result < 0) return SPI_ERROR;
    
    return SPI_SUCCESS;
}

/* nos spi read */
int32_t spi_read(spi_info_t* device, uint8_t data[], const uint32_t numBytes)
{
    if (!device || device->isOpen != SPI_DEVICE_OPEN) return SPI_ERROR;
    
    spi_device_t* sim_device = get_simulith_device(device);
    if (!sim_device) return SPI_ERROR;
    
    int result = simulith_spi_read(sim_device, data, numBytes);
    if (result < 0) return SPI_ERROR;
    
    return SPI_SUCCESS;
}

int32_t spi_transaction(spi_info_t* device, uint8_t *txBuff, uint8_t * rxBuffer, uint32_t length, uint16_t delay, uint8_t bits, uint8_t deselect)
{
    if (!device || device->isOpen != SPI_DEVICE_OPEN) return SPI_ERROR;
    
    spi_device_t* sim_device = get_simulith_device(device);
    if (!sim_device) return SPI_ERROR;
    
    int result = simulith_spi_transaction(sim_device, txBuff, length, rxBuffer, length);
    if (result != SIMULITH_SPI_SUCCESS) return SPI_ERROR;
    
    return SPI_SUCCESS;
}

int32_t spi_close_device(spi_info_t* device)
{
    if (!device) return SPI_ERROR;
    
    spi_device_t* sim_device = get_simulith_device(device);
    if (!sim_device) return SPI_ERROR;
    
    int result = simulith_spi_close(sim_device);
    if (result == SIMULITH_SPI_SUCCESS) {
        device->isOpen = SPI_DEVICE_CLOSED;
        return SPI_SUCCESS;
    }
    
    return SPI_ERROR;
}
