# Writing firmwares for ST-Link dongles

Writing firmwares for ST-Link dongles is a bit tricky due to previous bootloader execution. We need to set up our linker script correctly, and tweak a few things here and there.

## Linker script

Since I'm using [unicore-mx](), my linker file looks like this :

```
/* Generic linker script for STM32F103RBT6/STM32F103R8T6 with 0x4000 bootloader */
MEMORY {
       rom (rx) : ORIGIN = 0x08004000, LENGTH = 128K-0x4000
       ram (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

INCLUDE libucmx_stm32f1.ld
```

Nothing fancy here. The only important this is that our code is offset by `0x4000` (keeping space for the bootloader which is stored in the `0x08000000`-`0x08004000` region).

## Cleaning NVIC table

USB communication is processed on ST-Link firmware using interrupts. When bootloading to 3rd-party code, the NVIC interrupt addresses are incorrect because they are refering to functions present in ST's ST-Link firmware. NVIC table reset can be done in a few lines of C :

```c
uint8_t i;

for (i = 0; i < 255; i++) {
  nvic_disable_irq(i);
}
```

## USB re-enumeration

If you want to use the USB interface, you should use other VID/PID than ST's ones to prevent conflicts with their software. USB re-enumeration is done by bringing the USB bus to SE0. In practicle only USB D+ line needs to be pulled low for more than 10ms :

```
gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
              GPIO_CNF_OUTPUT_OPENDRAIN, GPIO12);
gpio_clear(GPIOA, GPIO12);
  
delay_us(20000);
```

## GPIOs

GPIOs is the weird thing about those cheap ST-Link dongle. The pin configuration is inconsistent across multiple vendors, and you'll see multiple GPIOs connected to the same SWD signal. I assume this is used to pull-up/down signals with a lower impedance than STM32's built-in GPIO pull-up/down.
