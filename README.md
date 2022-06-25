# IRIX O2 avstream

## Table of Contents
- [IRIX O2 avstream](#irix_o2-avstream)
  - [Table of Contents](#table-of-contents)
  - [Description](#description)
  - [Requirements](#requirements)
  - [Example Video](#example-video)

## Description

This repository provides a plugin and demonstration showcasing what a Silicon 
Graphics workstation can achieve.  Utilizing OpenInventor and dmedia, avstream 
renders live video onto a cube in an examiner viewer window.

The install.sh script should take care of installing the OpenInventor extension, library, and building of the example.

This was developed and tested only on O2/O2+ machines with AV1 modules.

Documentation for the plugin can be found on the 'SGI General Demos 1 of 2 (05/2001)' disc in the demos_O2.sw.src sub-package. File path to plugin files is "/usr/share/src/demos_O2/lib/SoDMBufferVideoEngine".  Documentation is viewed via web browser using the provided html files.

## Requirements

- IRIX 6.5.22 (may not need to be this specific)
- OpenInventor 2.1.5 (may not need to be this specific)
- dmedia, vl, al
- AV input (component video in, analog audio L and R)

## Example Video

https://user-images.githubusercontent.com/11632779/171271514-94b366a5-d339-4520-a7a4-beff32b2ea23.mp4
