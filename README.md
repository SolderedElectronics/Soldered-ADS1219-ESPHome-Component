# Soldered ADS1219 ESPHome Component

| ![ADS1219 ADC](https://cms.soldered.com/products/333380/media/333380_photo_8c082e.webp) |
| :-------------------------------------------------------------------------------------: |
|                       [ADS1219 ADC](https://www.solde.red/333380)                        |

Precision 24-bit delta-sigma ADC board based on the Texas Instruments ADS1219, with a programmable gain amplifier
(gain 1 or 4), four inputs usable as four single-ended or two differential channels, selectable data rates from 20 to
1000 SPS and an internal 2.048 V reference. It runs from 2.3 V - 5.5 V, connects over I2C and is part of the
[Qwiic ecosystem](https://soldered.com/collections/qwiic-ecosystem).

External ESPHome component for the Soldered ADS1219 board. It is a port of the
[Soldered ADS1219 Arduino library](https://github.com/SolderedElectronics/Soldered-ADS1219-Arduino-Library) and works
the same way as ESPHome's built-in [`ads1115`](https://esphome.io/components/sensor/ads1115/) component: one `ads1219`
hub per chip, and one [sensor](https://esphome.io/components/sensor/) per input channel, each publishing a voltage in
volts.

Every sensor runs its own single-shot conversion with its own multiplexer, gain and data rate, so channels with
different settings can share one chip. Conversions are queued and run one after another without blocking the ESPHome
main loop.

## Repository Contents

- **components/** - the ESPHome external component (Python config + C++ implementation)
- **examples/** - example YAML configs showing how to use the component

## Usage

Reference this repo directly from your own ESPHome YAML (no need to clone it locally):

```yaml
external_components:
  - source: github://SolderedElectronics/Soldered-ADS1219-ESPHome-Component
    components: [ads1219]

i2c:
  sda: GPIO21
  scl: GPIO22

ads1219:
  - id: adc
    address: 0x40

sensor:
  - platform: ads1219
    ads1219_id: adc
    name: "AIN0"
    multiplexer: A0_GND
    update_interval: 1s

  - platform: ads1219
    ads1219_id: adc
    name: "AIN2 - AIN3"
    multiplexer: A2_A3
    gain: 4
    update_interval: 5s
```

See [`examples/basic.yaml`](examples/basic.yaml) for a full working example with the internal reference, and [`examples/external_reference.yaml`](examples/external_reference.yaml) for one with an external reference on REFP / REFN.

### Input range

The measurable range is +/- VREF / gain, so +/- 2.048 V at gain 1 and +/- 0.512 V at gain 4 with the internal
reference. Independent of the reference, no input may go below AGND - 0.1 V or above AVDD + 0.1 V, and a single-ended
measurement (`Ax_GND`) can only read positive voltages, so it covers 0 V - VREF / gain. To measure up to the supply
voltage, connect a reference to the REFP / REFN pins and set `voltage_reference: external` with the matching
`reference_voltage`.

### Configuration variables

#### Hub (`ads1219:`)

- **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types#config-id)): ID of the hub, used by the sensors
  in `ads1219_id`. Required if there is more than one ADS1219.
- **address** (*Optional*, int): I2C address of the board. Defaults to `0x40`; can be set to `0x40` - `0x4F` with the
  A0 / A1 address pins.
- **voltage_reference** (*Optional*, string): `internal` (fixed 2.048 V) or `external` (voltage between the REFP and
  REFN pins). Defaults to `internal`.
- **reference_voltage** (*Optional*, voltage): voltage between REFP and REFN, e.g. `3.3V`. Required with
  `voltage_reference: external`, not allowed with `internal`.
- **i2c_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types#config-id)): I2C bus to use, if there is
  more than one.

The chip is soft-reset on every boot, so no state from a previous run is kept.

#### Sensor (`platform: ads1219`)

- **ads1219_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types#config-id)): ID of the hub. Required if
  there is more than one ADS1219.
- **multiplexer** (**Required**, string): input to measure, one of:
  - `A0_GND`, `A1_GND`, `A2_GND`, `A3_GND` - single-ended, input against AGND
  - `A0_A1`, `A2_A3`, `A1_A2` - differential, first input minus second
  - `SHORTED` - both inputs shorted to AVDD / 2, reads the offset of the ADC itself
- **gain** (*Optional*, int): PGA gain, `1` or `4`. Defaults to `1`.
- **sample_rate** (*Optional*, int): data rate in samples per second, `20`, `90`, `330` or `1000`. Defaults to `20`
  (lowest noise, ~50 ms per conversion).
- **update_interval** (*Optional*, [Time](https://esphome.io/guides/configuration-types#config-time)): how often to
  read the channel. Defaults to `60s`.
- All other options from [Sensor](https://esphome.io/components/sensor/#config-sensor) (`name`, `filters`,
  `accuracy_decimals`, ...).

### Hardware design

Hardware design files for this board are not published yet.

### Documentation

Access library documentation [here](https://docs.soldered.com/).

### About Soldered

<img src="https://raw.githubusercontent.com/SolderedElectronics/Soldered-Generic-Arduino-Library/dev/extras/Soldered-logo-color.png" alt="soldered-logo" width="500"/>

At Soldered, we design and manufacture a wide selection of electronic products to help you turn your ideas into acts and bring you one step closer to your final project. Our products are intented for makers and crafted in-house by our experienced team in Osijek, Croatia. We believe that sharing is a crucial element for improvement and innovation, and we work hard to stay connected with all our makers regardless of their skill or experience level. Therefore, all our products are open-source. Finally, we always have your back. If you face any problem concerning either your shopping experience or your electronics project, our team will help you deal with it, offering efficient customer service and cost-free technical support anytime. Some of those might be useful for you:

- [Web Store](https://www.soldered.com/shop)
- [Tutorials & Projects](https://soldered.com/learn)
- [Documentation](https://docs.soldered.com)

### Open-source license

Soldered invests vast amounts of time into hardware & software for these products, which are all open-source. Please support future development by buying one of our products.

Check license details in the LICENSE file. Long story short, use these open-source files for any purpose you want to, as long as you apply the same open-source licence to it and disclose the original source. No warranty - all designs in this repository are distributed in the hope that they will be useful, but without any warranty. They are provided "AS IS", therefore without warranty of any kind, either expressed or implied. The entire quality and performance of what you do with the contents of this repository are your responsibility. In no event, Soldered (TAVU) will be liable for your damages, losses, including any general, special, incidental or consequential damage arising out of the use or inability to use the contents of this repository.

## Have fun!

And thank you from your fellow makers at Soldered Electronics.
