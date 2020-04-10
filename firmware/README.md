# Firmware



## Building the firmware

In order to build the firmware, several steps are needed:


### 1. Install [PlatformIO](https://platformio.org/platformio-ide)

These instructions assume that PlatformIO is installed as an extension of *Visual Studio Code*.


### 2. Add board definitions

Add the board definitions `genericSTM32F042F6.json` and `nucleo_f042k6_ex.json` in the [support](../support) directory to `boards` directory of the PlatformIO installation (`.platformio` in your home directory). You might need to create the `boards` directory if it doesn't exist yet.


### 3. Copy OpenOCD configuration

Copy the OpenOCD configuration `stm32f042f6_stlink.cfg` in the [support](../support) directory to `~\.platformio\packages\tool-openocd\scripts\board`.


### 4. Modify file `libopencm3.py`

Apply the below changes to `libopencm3.py` in `~\.platformio\platforms\ststm32\builder\frameworks\libopencm3`:

At line 124, insert:

```
    incre = re.compile(r"^INCLUDE\s+\"?([^\.]+\.ld)\"?", re.M)
```

At about line 133, replace:

```
                return fp.read()
```
with:
```
                return incre.sub(r'_INCLUDE_ \1', fp.read())
```


### 5. Build

Either from within Visual Studio Code (click *Build* icon in status bar or click the *Build* in project task in the *PLATFORMIO* view) or from a shell:
```
pio run
```



## Uploading the firmware

In order to upload the firmware, a ST-Link (or a JLink) adapter is needed. Connect the adapter to the SWD debug port and upload, either from PlatformIO (upload icon in status bar, *Upload* task in *PLATFORMIO* view) or by running:

```
pio run -t upload
```

An ST-Link adapter can provide sufficient power to the USB-to-Serial adapter for programming it.

