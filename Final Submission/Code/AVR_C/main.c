#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/* 
 * Pin Mapping for ATmega2560 (Arduino Mega):
 * Motors:
 * ENA: Pin 10 -> PB4 (OC2A)
 * ENB: Pin 5  -> PE3 (OC3A)
 * IN1: Pin 9  -> PH6
 * IN2: Pin 8  -> PH5
 * IN3: Pin 7  -> PH4
 * IN4: Pin 6  -> PH3
 * 
 * Sensors:
 * Front: Trig 24 -> PA2, Echo 25 -> PA3
 * Right: Trig 26 -> PA4, Echo 27 -> PA5
 * Left:  Trig 28 -> PA6, Echo 29 -> PA7
 * 
 * Buttons:
 * ON:  Pin 30 -> PC7
 * OFF: Pin 31 -> PC6
 */

// Thresholds
#define WALL_THRESHOLD 10
#define FRONT_CLEAR_THRESHOLD 20
#define MOTOR_SPEED 150

// Function Prototypes
void init_gpio(void);
void init_pwm(void);
void set_motors(uint8_t left_speed, uint8_t left_dir, uint8_t right_speed, uint8_t right_dir);
long get_distance(uint8_t trig_pin, uint8_t echo_pin);
void stop_motors(void);
void move_forward(void);
void turn_left(void);
void turn_right(void);
void turn_around(void);

// Direction constants
#define FORWARD 1
#define BACKWARD 0

void init_gpio(void) {
    // Motors direction pins as output
    DDRH |= (1 << PH6) | (1 << PH5) | (1 << PH4) | (1 << PH3);
    DDRB |= (1 << PB4); // ENA
    DDRE |= (1 << PE3); // ENB

    // Sensors
    DDRA |= (1 << PA2) | (1 << PA4) | (1 << PA6); // Trig pins as output
    DDRA &= ~((1 << PA3) | (1 << PA5) | (1 << PA7)); // Echo pins as input

    // Buttons
    DDRC &= ~((1 << PC7) | (1 << PC6)); // Input
    PORTC |= (1 << PC7) | (1 << PC6);   // Pull-up
}

void init_pwm(void) {
    // Timer 2 for ENA (PB4 / OC2A)
    // Fast PWM, non-inverting
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS22); // Prescaler 64

    // Timer 3 for ENB (PE3 / OC3A)
    // Fast PWM 8-bit, non-inverting
    TCCR3A = (1 << COM3A1) | (1 << WGM30);
    TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30); // Prescaler 64
}

void set_motors(uint8_t left_speed, uint8_t left_dir, uint8_t right_speed, uint8_t right_dir) {
    OCR2A = left_speed;
    OCR3A = right_speed;

    if (left_dir == FORWARD) {
        PORTH |= (1 << PH6);
        PORTH &= ~(1 << PH5);
    } else {
        PORTH &= ~(1 << PH6);
        PORTH |= (1 << PH5);
    }

    if (right_dir == FORWARD) {
        PORTH |= (1 << PH4);
        PORTH &= ~(1 << PH3);
    } else {
        PORTH &= ~(1 << PH4);
        PORTH |= (1 << PH3);
    }
}

void stop_motors(void) {
    OCR2A = 0;
    OCR3A = 0;
    PORTH &= ~((1 << PH6) | (1 << PH5) | (1 << PH4) | (1 << PH3));
}

long get_distance(uint8_t trig_bit, uint8_t echo_bit) {
    PORTA &= ~(1 << trig_bit);
    _delay_us(2);
    PORTA |= (1 << trig_bit);
    _delay_us(10);
    PORTA &= ~(1 << trig_bit);

    uint32_t count = 0;
    uint32_t timeout = 100000; // Timeout to prevent hanging

    while (!(PINA & (1 << echo_bit)) && count < timeout) count++;
    if (count == timeout) return 999;

    count = 0;
    while ((PINA & (1 << echo_bit)) && count < timeout) {
        count++;
        _delay_us(1);
    }
    
    // Distance in cm = (time in us * 0.034) / 2
    return (long)(count * 0.034 / 2);
}

void move_forward(void) {
    set_motors(MOTOR_SPEED, FORWARD, MOTOR_SPEED, FORWARD);
}

void turn_left(void) {
    set_motors(MOTOR_SPEED, BACKWARD, MOTOR_SPEED, FORWARD);
    _delay_ms(400);
    stop_motors();
    _delay_ms(100);
}

void turn_right(void) {
    set_motors(MOTOR_SPEED, FORWARD, MOTOR_SPEED, BACKWARD);
    _delay_ms(400);
    stop_motors();
    _delay_ms(100);
}

void turn_around(void) {
    turn_right();
    turn_right();
}

int main(void) {
    init_gpio();
    init_pwm();

    uint8_t running = 0;

    while (1) {
        // Check ON button (PC7, active low)
        if (!(PINC & (1 << PC7))) {
            _delay_ms(50);
            if (!(PINC & (1 << PC7))) {
                running = 1;
            }
        }

        // Check OFF button (PC6, active low)
        if (!(PINC & (1 << PC6))) {
            _delay_ms(50);
            if (!(PINC & (1 << PC6))) {
                running = 0;
                stop_motors();
            }
        }

        if (running) {
            long distL = get_distance(PA6, PA7);
            long distF = get_distance(PA2, PA3);
            long distR = get_distance(PA4, PA5);

            if (distL > WALL_THRESHOLD) {
                turn_left();
                move_forward();
                _delay_ms(200); // Move a bit into the new path
            } else if (distF > FRONT_CLEAR_THRESHOLD) {
                move_forward();
            } else if (distR > WALL_THRESHOLD) {
                turn_right();
                move_forward();
                _delay_ms(200);
            } else {
                turn_around();
                move_forward();
                _delay_ms(200);
            }
        }
        _delay_ms(50);
    }
    return 0;
}
