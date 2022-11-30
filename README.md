# Library for the AM2302 Temperature and Humidity sensor.

## Motivation:

Most libraries use an active waiting scheme to read the data from the sensor (sleeping for 1us and checking for the state of the wire), but this approach just causes problems when you have a scheduler.

The code on this library makes a lot of assumptions, so please open an issue if you encounter any bugs.

## Using:

Two functions are exposed to the user: `am2302_init()` and `am2302_read()`.

```c
am2302_init() // Will configure the RMT driver used for reading of the sensor,
              // this **must** be called in the core that will perform the probing
              // i.e.: if you read the sensor on core one, call am2302_init() on
              // that same core.
```

```c
am2302_read() // Will perform the probing of the sensor, and set h and t with the
              // humidity and temperature values. The following errors can be given:
              //    ESP_ERR_INVALID_SIZE: If the number of bits read was different
              //                          from the expectet 42 bits.
              //    ESP_ERR_INVALID_RESPONSE: If the ringbuffer recieved from the
              //                              RMT driver was empty.
              //    ESP_ERR_INVALID_CRC: If the checksum does not match.
              // Values are retune in the metric system, as god intended.
```

Example:

```c

    am2302_init();

    while (1){
        uint16_t temp = 0;
        uint16_t humd = 0;

        am2302_read(&temp, &humd); // Not checking for the return value

        // Do something with temp and humd...
    }

```

The RMT channel and pin number are set using `menuconfig`.

```shell
idf.py menuconfig
```
