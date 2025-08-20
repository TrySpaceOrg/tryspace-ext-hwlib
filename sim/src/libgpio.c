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
#include <stdio.h>
#include <string.h>

/* hwlib API */
#include "libgpio.h"

/* simulith API */
#include "simulith_gpio.h"

// Storage for GPIO device structures mapped to gpio_info_t devices  
static gpio_device_t gpio_devices[256]; // Support up to 256 pins
static int device_count = 0;

// Helper function to get or create simulith device for gpio_info_t
static gpio_device_t* get_simulith_device(gpio_info_t* device)
{
    if (!device) return NULL;
    
    // Search for existing device
    for (int i = 0; i < device_count; i++) {
        if (gpio_devices[i].pin == device->pin) {
            return &gpio_devices[i];
        }
    }
    
    // Create new device if not found
    if (device_count >= 256) {
        return NULL; // Too many devices
    }
    
    gpio_device_t* sim_device = &gpio_devices[device_count++];
    sim_device->pin = device->pin;
    sim_device->direction = device->direction;
    sim_device->init = 0;
    
    // Create unique name and address based on pin
    snprintf(sim_device->name, sizeof(sim_device->name), "gpio%d", device->pin);
    snprintf(sim_device->address, sizeof(sim_device->address), "tcp://localhost:%d", 
             SIMULITH_GPIO_BASE_PORT + device->pin);
    
    // Default to client mode (can be overridden by environment or configuration)
    sim_device->is_server = 0;
    
    return sim_device;
}

int32_t gpio_init(gpio_info_t* device) 
{    
    if (!device) return GPIO_ERROR;
    if (device->pin > 255) return GPIO_ERROR; // Pin limit check
    
    gpio_device_t* sim_device = get_simulith_device(device);
    if (!sim_device) {
        return GPIO_ERROR;
    }
    
    int result = simulith_gpio_init(sim_device);
    if (result == SIMULITH_GPIO_SUCCESS) {
        device->isOpen = GPIO_OPEN;
        return GPIO_SUCCESS;
    }
    
    return GPIO_ERROR;
}

int32_t gpio_read(gpio_info_t* device, uint8_t* value)
{
    if (!device || !value || device->isOpen != GPIO_OPEN) return GPIO_ERROR;
    
    gpio_device_t* sim_device = get_simulith_device(device);
    if (!sim_device) return GPIO_ERROR;
    
    int result = simulith_gpio_read(sim_device, value);
    if (result == SIMULITH_GPIO_SUCCESS) {
        return GPIO_SUCCESS;
    }
    
    return GPIO_ERROR;
}

int32_t gpio_write(gpio_info_t* device, uint8_t value)
{
    if (!device || device->isOpen != GPIO_OPEN) return GPIO_ERROR;
    if (value > 1) return GPIO_ERROR; // Value validation
    
    gpio_device_t* sim_device = get_simulith_device(device);
    if (!sim_device) return GPIO_ERROR;
    
    int result = simulith_gpio_write(sim_device, value);
    if (result == SIMULITH_GPIO_SUCCESS) {
        return GPIO_SUCCESS;
    }
    
    return GPIO_ERROR;
}

int32_t gpio_close(gpio_info_t* device)
{
    if (!device) return GPIO_ERROR;
    
    gpio_device_t* sim_device = get_simulith_device(device);
    if (!sim_device) return GPIO_ERROR;
    
    int result = simulith_gpio_close(sim_device);
    if (result == SIMULITH_GPIO_SUCCESS) {
        device->isOpen = GPIO_CLOSED;
        return GPIO_SUCCESS;
    }
    
    return GPIO_ERROR;
}
