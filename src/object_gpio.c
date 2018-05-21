#include "liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* mraa header */
#include "mraa/gpio.h"

int num_pins = 2;
int pins[2] = {16, 18};


typedef struct _gpio_instance_
{
    struct _gpio_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t shortID;               // matches lwm2m_list_t::id
	uint16_t gpioPinID;
	uint8_t gpioPinDir;	
} gpio_instance_t;


static uint8_t gpio_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_data_t ** dataArrayP,
                               lwm2m_object_t * objectP)
{
	
	gpio_instance_t * targetP;

    targetP = (gpio_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
	
	uint16_t pin_id = targetP->gpioPinID;
	
	
	//MRAA BEGIN

	mraa_result_t status = MRAA_SUCCESS;
	mraa_gpio_context gpio;
	mraa_init();

	gpio = mraa_gpio_init(pin_id);
    if (gpio == NULL) {
        fprintf(stderr, "Failed to initialize GPIO %d\n", pin_id);
        mraa_deinit();
        return COAP_404_NOT_FOUND;
	}

	uint8_t pin_value =  mraa_gpio_read(gpio);
	if(pin_value == -1){
		fprintf(stderr, "Failed to read value of GPIO %d\n", pin_id);
        mraa_deinit();
        return COAP_404_NOT_FOUND;
	}

    mraa_gpio_dir_t dir;
    status = mraa_gpio_read_dir(gpio, &dir);
    if (status != MRAA_SUCCESS) {
        fprintf(stderr, "Failed to read direction of GPIO %d\n", pin_id);
        return COAP_404_NOT_FOUND;
    }

    uint8_t pin_dir = (int)dir;

	status = mraa_gpio_close(gpio);
    if (status != MRAA_SUCCESS) {
        fprintf(stderr, "Failed to close GPIO %d\n", pin_id);
	}

	mraa_deinit();

	//MRAA END

	
	*dataArrayP = lwm2m_data_new(3);
	*numDataP = 3;

	(*dataArrayP)[0].id = 0;
	(*dataArrayP)[1].id = 1;
    (*dataArrayP)[2].id = 2;
    lwm2m_data_encode_int(pin_id, *dataArrayP);
	lwm2m_data_encode_int(pin_value, *dataArrayP + 1);
	lwm2m_data_encode_int(pin_dir, *dataArrayP + 2);
	

    return COAP_205_CONTENT;
}


static uint8_t gpio_write(uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         lwm2m_object_t * objectP)
{
	gpio_instance_t * targetP;
    int i;

    targetP = (gpio_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;


	int64_t value;

    if (1 != lwm2m_data_decode_int(dataArray, &value) || value > 1 || value < 0)
	{
		return COAP_400_BAD_REQUEST;
	}
	
    int valueToSet = (int)value;
    int dataIdToChange = dataArray[0].id;
    uint16_t pin_id = targetP->gpioPinID;


    //MRAA BEGIN

    if(dataIdToChange == 0)
    {
        return COAP_405_METHOD_NOT_ALLOWED;
    }
    else if(dataIdToChange == 1)    // pin_value
    {
        mraa_result_t status = MRAA_SUCCESS;
        mraa_gpio_context gpio;
        mraa_init();

        gpio = mraa_gpio_init(pin_id);
        if (gpio == NULL) {
            fprintf(stderr, "Failed to initialize GPIO %d\n", pin_id);
            mraa_deinit();
            return COAP_404_NOT_FOUND;
        }


        status = mraa_gpio_write(gpio, valueToSet);
        if (status != MRAA_SUCCESS) {
            mraa_deinit();
            return COAP_404_NOT_FOUND;
        }

        mraa_deinit();
    }
    else if(dataIdToChange == 2)    // pin_dir
    {
        mraa_result_t status = MRAA_SUCCESS;
        mraa_gpio_context gpio;
        mraa_init();

        gpio = mraa_gpio_init(pin_id);
        if (gpio == NULL) {
            fprintf(stderr, "Failed to initialize GPIO %d\n", pin_id);
            mraa_deinit();
            return COAP_404_NOT_FOUND;
        }


        status = mraa_gpio_dir(gpio, valueToSet);
        if (status != MRAA_SUCCESS) {
            mraa_deinit();
            return COAP_404_NOT_FOUND;
        }

        mraa_deinit();
    }
    else
        return COAP_400_BAD_REQUEST;

    //MRAA END

	return COAP_204_CHANGED;	
}


lwm2m_object_t * get_gpio_object(void)
{
    lwm2m_object_t * myObj;

    myObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != myObj)
    {
        int i;
		
		int gpio_pins_number = 2;
        gpio_instance_t * targetP;

        memset(myObj, 0, sizeof(lwm2m_object_t));
		
		// ID obiektu
        myObj->objID = 31128;
        for (i=0 ; i < num_pins ; i++)
        {
            targetP = (gpio_instance_t *)lwm2m_malloc(sizeof(gpio_instance_t));
            if (NULL == targetP) return NULL;
            memset(targetP, 0, sizeof(gpio_instance_t));
			targetP->shortID = pins[i];
			targetP->gpioPinID = pins[i];
            myObj->instanceList = LWM2M_LIST_ADD(myObj->instanceList, targetP);
        }


        myObj->readFunc = gpio_read;
        myObj->writeFunc = gpio_write;
    }

    return myObj;
}


void free_gpio_object(lwm2m_object_t * object)
{
    LWM2M_LIST_FREE(object->instanceList);
    if (object->userData != NULL)
    {
        lwm2m_free(object->userData);
        object->userData = NULL;
    }
    lwm2m_free(object);
}

