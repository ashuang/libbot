The libbot project provides a set of libraries, tools, and algorithms that are designed to facilitate robotics research. Among others, these include convenience functions for maintaining coordinate transformations, tools for parameter management and process administration, and a 3D visualization library. Initially developed at MIT for the 2007 DARPA Grand Challenge, libbot is designed to provide a core set of capabilities for robotics applications and has been utilized on a number of different robotic vehicles.

The libbot library follows the <a href='http://sourceforge.net/p/pods/home/Home/'>pods</a> build architecture guidlines.

# Features #

The libbot library consists of several pieces of software (aka "pods") that offer different functionality:

| **bot-core** | Provides core library that includes widely useful variable definitions and convenience functions related to such things as rotation representations, rigid-body coordinate transformations, mathematical operations, and time synchronization. |
|:-------------|:-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **bot-frames** | Provides a library for maintaining and querying coordinate transformations defined by kinematic chains.                                                                                                                                        |
| **bot-param** | Provides a library for maintaining and sharing configuration parameters stored as key/value pairs in a configuration file.                                                                                                                     |
| **bot-vis**  | Provides a 3D visualization library that uses OpenGL and GTK+ to enable data visualization in a graphical user interace.                                                                                                                       |
| **bot-lcmgl** | Provides a set of libraries in C, Java, and Python for transmitting OpenGL commands over <a href='http://code.google.com/p/lcm/'>LCM</a> together with routines for receiving and rendering these commands.                                    |
| **bot-lcm-utils** | Provides utilities for working with <a href='http://code.google.com/p/lcm/'>LCM</a> message passing and data marshalling, including tools to interface with LCM log files and to _tunnel_ LCM messages between different networks.             |
| **bot-procman** | Provides tools to manage multiple processes that are distributed over one or more computers.                                                                                                                                                   |

# Requirements #

Libbot currently works on the GNU/Linux and OS X operating systems. The requirements are different for each pod (see the README in each pod's top-level directory). The following lists the entire set of requirements.

  * CMake
  * Glib 2.0+
  * <a href='http://lcm.googlecode.com'>LCM</a>
  * Java (Sun JDK or OpenJDK strongly preferred)
  * Python
  * GTK 2.0+
  * OpenGL
  * GLUT
  * PyGTK