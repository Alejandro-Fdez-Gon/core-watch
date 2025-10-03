# Core Watch: System Timing and Control

This project implements the **Core Watch**, a foundational system timing and control module for embedded devices, developed for the **Digital Systems II** course. It focuses on using **Finite State Machines (FSMs)** and **concurrency** on a **Raspberry Pi** platform to manage a digital clock, a matrix keypad, and an LCD display.

## 💡 Main Objective

The main goal of this project is to develop a fully functional system that serves as a **core timing mechanism** (Core Watch). This core is the foundation for more complex embedded electronic systems, such as electronic organizers, thermostats, or monitoring devices.

The developed system must meet the following minimum requirements:

1.  Implement a **digital clock**.
2.  Allow **time setting** and **resetting**.
3.  Utilize a **matrix keypad** as the input interface.
4.  Display the time and system status via an **LCD** (Liquid-Crystal Display).

## 🛠️ Technology and Methodology

The project development focuses on applying digital systems concepts and programming in an embedded environment.

### Hardware and Software
* **Target Platform (HW):** Raspberry Pi 1 Model B+ (or other compatible models).
* **Development Environment (IDE):** Eclipse.
* **Programming Language:** C/C++.
* **Tools/Libraries:**
    * **`fsm` library:** For Finite State Machine (FSM) implementation.
    * **`tmr` library:** For handling periodic and non-periodic timers.
    * **`wiringPi` library** (or equivalent): For handling GPIOs and threads.

### Key Concepts
* **Finite State Machines (FSM):** Intensive use to model the behavior of the clock, the matrix keypad, and the main system (`CoreWatch`).
* **Embedded/IoT Systems:** The project introduces programming for embedded systems.
* **Concurrency:** Use of **Threads** for handling data input (keypad).
* **General Purpose Input/Output (GPIO):** Direct manipulation of pins to interact with the Matrix Keypad and the LCD.

## 🚀 Development Phases (Versions)

The project was developed incrementally, ensuring functionality step-by-step.

* **Version 1: Clock Library**
    * **Focus:** Isolated development of the clock library using FSM and periodic timing. Output printed to PC console.
    * **Platform:** PC (Emulation).
* **Version 2: CoreWatch System**
    * **Focus:** Creation of the main code, introduction of the system's FSM, and implementation of **threads** for PC keyboard use (emulated).
    * **Platform:** PC (Emulation).
* **Version 3: Matrix Keypad (HW)**
    * **Focus:** Integration of the matrix keypad with its own FSM and **transition to Raspberry Pi** (HW).
    * **Platform:** Raspberry Pi.
* **Version 4: LCD Display**
    * **Focus:** Replacement of terminal printing with **visualization on the LCD**. Completion of minimum specifications.
    * **Platform:** Raspberry Pi.
* **Improvements**
    * **Focus:** Implementation of additional features for an enhanced grade.
    * **Platform:** Raspberry Pi.

## ⚙️ Setup and Execution

### Prerequisites

* **PC Environment:** A development environment based on **Eclipse** with tools for compilation and debugging.
* **Hardware Environment:** A **Raspberry Pi** board (version 1 Model B+ is recommended).
* **Peripherals:** Matrix Keypad module and LCD (Liquid-Crystal Display).

### Compilation and Deployment

1.  **Environment Configuration:** Ensure the `systemConfig.h` file is configured for the target environment (`ENTORNO_LABORATORIO` or `ENTORNO_PC`).
2.  **Cross-Compilation (for RPi):** (Details specific to the GCC toolchain or Eclipse IDE).
3.  **Execution:** The final code must be functional on the actual Raspberry Pi hardware.
