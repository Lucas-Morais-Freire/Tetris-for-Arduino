#ifndef NTENDO_PRIVATE_H
#define NTENDO_PRIVATE_H

#include "ntendo.h"

namespace ntd {

enum status : uint8_t {
    ACK_CONTROLLER,
    GAME
};

enum recv_status : uint8_t {
    SUCCESS,
    NO_RESPONSE
};

volatile uint8_t _scans_per_frame;
volatile bool _frame_ready = true;
uint8_t** volatile _temp_frame;
uint8_t** volatile _write_frame;
volatile uint8_t* _PORTt,* _PORTm,* _PORTb,* _DDRl,* _DDRr;
volatile uint64_t _frame_count = 0;
volatile status _state = ACK_CONTROLLER;
volatile uint8_t _recv_state = NO_RESPONSE;
volatile char* volatile _read_inputs;
volatile char* volatile _temp_inputs;
volatile uint8_t _read_len;
volatile uint8_t _temp_len;

}

#endif