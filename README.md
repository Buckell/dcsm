# DMX Controller Serial Management Protocol (DCSM)
#

> #### [Latest Specification](https://cdn.goddard.systems/dcsm/specification/latest)
> This is the latest specification of DCSM. Read this for documentation such as how to use
> DCSM, implementation details, and more.

## About

DCSM is a protocol designed for controlling USB DMX controllers. Over other protocols,
DCSM has a couple of key benefits. DCSM includes two separate interfaces: a more human-friendly
command interface and a machine-focused direct control interface. This offers the flexibility of
controlling DCSM devices from any device with a serial monitor (which is available on almost
every device with a USB port, including most phones) even if there is no native software 
implementation for the direct control DCSM protocol. DCSM is designed for buffered DMX devices,
which means constant signalling is not required. DCSM also has many different instructions
available within the direct control interface. One of the benefits of this is the ability to
only change specific addresses instead of needed to transmit entire universes of data, which 
frees up bandwidth for other messages.

## Command Interface

The command interface is for use through programs like serial monitors. Due to the ubiquity of
serial monitors, this interface is designed to offer maximum compatibility with all USB hosts.
This also means that DCSM devices can be controlled with Android phones installed with a serial
monitor application (at least with devices feature USB-C ports; unfortunately, iPhones do not 
support such a utility through their lightning port, nor is the iOS software as permissive;
maybe someday a USB-C iPhone will have such a capability).

### Some Command Examples

Here are some examples of commands through the command interface.

```
set 20 thru 40 @ 50%
set 1/20 thru 1/30 @ 255
get 2/400 thru 2/450
patch 4 to 5
```
*If you have ever used an ETC Element 2 or similar lighting board, the command syntax may be
slightly familiar to you. The command syntax served as inspiration for DCSM.*

## Direct Control Interface

For integration with programs, a more capable and powerful direct control (DC) interface exists.
The DC interface offers an instruction set that allows near full control of the device in the 
most efficient way possible. Such instructions include:

* **Set Universe Data** `setu` - Set all the values of a universe at once. Useful for repeatedly
  updating the data of a universe from an external source (like sACN or ArtNet).

* **Set Universe Address To Value** `setutv` - Set specified addresses in a universe to a single
  value. Useful for situations where a user selects a range of addresses/channels in a program to
  set to one singular value. Uses a bitmask for increased efficiency when targeting large ranges 
  of addresses.

* **Get Universe Data** `getu` - Get all values of a single universe. Mostly used for 
  synchronization.

## Other Useful Features

### Patching

DCSM devices support both input and output functionality. For DCSM devices featuring an input port 
and one or more output ports, DCSM devices support "patching" an input universe to one or more 
output universes. Traditional output universes can also be used as the input for a patch, wherein
values set in that universe will simply be copied to the output universe of the patch. This allows
DCSM devices with multiple output ports to <u>act as a DMX splitter</u>.

But even in devices with an input port and only one output port, patching is still useful.

**Masking** is the ability to override the values being transferred from an input universe to an 
output universe. Masking uses mask universe, which are special universes that contain both standard
universe data and masking flags that mark which addresses in the mask universe are "masking."
Masking addresses/values in the mask universe, when applied to a patch, will override the values in
an input universe when being transferred to an output universe. 

This allows inserting a DCSM device in a DMX line and changing the values of specific DMX addresses 
without affecting the rest of the address range. 