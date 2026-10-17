# Autonomous Maze-Solving Robot (Bare-Metal AVR C)

Embedded Systems (CSE234) final project — Alamein International University.

A 4WD robot that finds its way out of a maze with no human input. The firmware runs on an
**ATmega2560** and is written in register-level AVR C (no Arduino libraries). Three HC-SR04
ultrasonic sensors feed an **LSRB** (Left → Straight → Right → Back) wall-following algorithm,
and an L298N H-bridge drives the motors from hardware-timer PWM.

## Hardware

| Part | Details |
| :--- | :--- |
| Controller | ATmega2560 (Arduino Mega) |
| Sensors | 3× HC-SR04 ultrasonic (front, left, right) |
| Actuators | 4× DC motors via L298N H-bridge |
| PWM | Timer 2 → OC2A (PB4, left), Timer 3 → OC3A (PE3, right) |
| Controls | Start / stop push-buttons (PC7 / PC6) |

## Behaviour

`STOPPED → EXPLORING → FINISHED`. While exploring, the robot reads all three distances and:

1. turns left if the left is open,
2. otherwise goes straight if the front is open,
3. otherwise turns right if the right is open,
4. otherwise turns 180° (dead end).

## Repository layout

```
Final Submission/
├── Code/
│   ├── AVR_C/main.c                       # bare-metal firmware (submitted version)
│   └── Arduino/maze_solver_robot/*.ino    # Arduino reference sketch
├── Diagrams/                              # flowchart & state machine
└── Documentation/                         # project report (EN, MD/PDF) and Arabic documentation
```

## Build & flash

```bash
avr-gcc -mmcu=atmega2560 -Os -o main.elf "Final Submission/Code/AVR_C/main.c"
avr-objcopy -O ihex -R .eeprom main.elf main.hex
avrdude -c wiring -p m2560 -P <PORT> -b 115200 -D -U flash:w:main.hex:i
```
