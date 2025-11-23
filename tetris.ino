#include "headers/ntendo.h"
#include "headers/tetrimino.h"

int main() {
    // portas das saidas das matrizes
    DDRA = 0xFF;
    DDRC = 0xFF;
    DDRL = 0xFF;
    // setup da API
    ntendo.set_ports(&PORTA, &PORTC, &PORTL, &DDRF, &DDRK);
    ntendo.begin(60);

    // variaveis utilizadas para logica
    bool     frame[24][16];
    char*    inputs;
    uint8_t  len;
    uint64_t curr_frame;

    // setup do jogo
    memset(frame, false, sizeof(frame));
    enum state : uint8_t {
        SPAWN,
        FALL,
        LOCK,
        CLEAR_LINES,
        MOVE_BOARD,
        OVER,
        PAUSE
    };

    tetrimino curr_piece;
    tetrimino next_piece;

    const uint8_t max_lock_delay = 30;
    const uint8_t max_DAS_counter = 16;

    state game_state = SPAWN;
    uint8_t level = 0;
    uint8_t R_DAS_counter = 0;
    uint8_t L_DAS_counter = 0;
    uint8_t frames_per_drop = 2;
    uint8_t drop_counter = 0;
    uint8_t lines_cleared = 0;
    uint8_t score = 0;
    uint8_t lock_delay = 0;
    uint8_t lock_delay_resets = 0;

    uint8_t destroy_len = 0;
    uint8_t to_destroy[5];
    randomSeed(TCNT1);

    uint8_t block_to_clear = 0;
    bool moved = false;

    while (true) {
        inputs     = ntendo.get_inputs();
        len        = ntendo.get_input_len();
        curr_frame = ntendo.get_frame_count();
        for (int j = 0; j < 11; j++) {
            frame[3][j] = false;
        }
        for (int i = 3; i < 24; i++) {
            frame[i][10] = false;
        }

        switch (game_state) {
        case LOCK:
            for (uint8_t i = 23; i > 3; i--) {
                bool destroy_line = true;
                for (uint8_t j = 0; j < 10; j++) {
                    if (!frame[i][j]) {
                        destroy_line = false;
                    }
                }
                if (destroy_line) {
                    to_destroy[destroy_len] = i;
                    destroy_len++;
                }
            }
            
            if (destroy_len > 0) {
                game_state = CLEAR_LINES;
            } else {
                game_state = SPAWN;
            }
            break;
        case SPAWN:
            curr_piece = tetrimino();

            if (!curr_piece.check_validity(frame) || curr_piece.check_lock(frame)) {
                game_state = OVER;
            } else {
                drop_counter = 0;
                lock_delay = 0;
                lock_delay_resets = 0;
                L_DAS_counter = 0;
                R_DAS_counter = 0;
                
                destroy_len = 0;
                for (uint8_t i = 0; i < 5; i++) {
                    to_destroy[i] = 3;
                }
                
                block_to_clear = 0;
                
                level = lines_cleared/10;
                
                curr_piece.place(frame);
                game_state = FALL;
            }
            break;
        case CLEAR_LINES:
            for (uint8_t i = 0; i < destroy_len; i++) {
                frame[to_destroy[i]][block_to_clear] = false;
            }
            block_to_clear++;
            if (block_to_clear == 10) {
                game_state = MOVE_BOARD;
            }
            break;
        case MOVE_BOARD:
            for (uint8_t k = 0; k < destroy_len; k++) {
                for (uint8_t i = to_destroy[k]; i >= to_destroy[k + 1] + k + 2; i--) {
                    for (uint8_t j = 0; j < 10; j++) {
                        frame[i][j] = frame[i - 1 - k][j];
                    }
                }
                to_destroy[k + 1] += k + 1;
            }
            game_state = SPAWN;
            break;
        case PAUSE:
            for (uint8_t i = 0; i < len; i++) {
                if (inputs[i] == 's') {
                    game_state = FALL;
                    break;
                }
            }
            break;
        case OVER:
            for (uint8_t i = 0; i < len; i++) {
                if (inputs[i] == 's') {
                    memset(frame, false, sizeof(frame));
                    game_state = SPAWN;
                    break;
                }
            }
            break;
        case FALL:
            curr_piece.unplace(frame);
            
            bool reset_R_DAS = true;
            bool reset_L_DAS = true;
            bool moved = false;
            bool reset_G = true;
            for (uint8_t i = 0; i < len; i++) {
                // processar entradas
                switch (inputs[i]) {
                case 'r':
                    curr_piece.ypos++;
                    if (!curr_piece.check_validity(frame)) {
                        curr_piece.ypos--;
                    } else {
                        lock_delay = 0;
                        moved = true;
                    }
                    break;
                case 'R':
                    if (R_DAS_counter == max_DAS_counter) {
                        curr_piece.ypos++;
                        if (!curr_piece.check_validity(frame)) {
                            curr_piece.ypos--;
                        } else {
                            lock_delay = 0;
                            moved = true;
                        }
                        R_DAS_counter = 10;
                        L_DAS_counter = 0;
                    } else {
                        R_DAS_counter++;
                    }
                    reset_R_DAS = false;
                    break;
                case 'l':
                    curr_piece.ypos--;
                    if (!curr_piece.check_validity(frame)) {
                        curr_piece.ypos++;
                    } else {
                        lock_delay = 0;
                        moved = true;
                    }
                    break;
                case 'L':
                    if (L_DAS_counter == max_DAS_counter) {
                        curr_piece.ypos--;
                        if (!curr_piece.check_validity(frame)) {
                            curr_piece.ypos++;
                        } else {
                            lock_delay = 0;
                            moved = true;
                        }
                        L_DAS_counter = 10;
                        R_DAS_counter = 0;
                    } else {
                        L_DAS_counter++;
                    }
                    reset_L_DAS = false;
                    break;
                case 'a':
                    if (curr_piece.rotate_r(frame)) {
                        moved = true;
                        lock_delay = 0;
                    }
                    break;
                case 'b':
                    if (curr_piece.rotate_l(frame)) {
                        moved = true;
                        lock_delay = 0;
                    }
                    break;
                case 'd':
                case 'D':
                    frames_per_drop = 2;
                    reset_G = false;
                    break;
                case 's':
                    game_state = PAUSE;
                    break;
                }
            }
            if (reset_R_DAS) {
                R_DAS_counter = 0;
            }
            if (reset_L_DAS) {
                L_DAS_counter = 0;
            }
            if (reset_G) {
                frames_per_drop = 48; // modificar
            }
            
            if (curr_piece.check_lock(frame)) {
                if (moved) {
                    lock_delay_resets++;
                }
                if (lock_delay == max_lock_delay || lock_delay_resets == 15) {
                    curr_piece.place(frame);
                    game_state = LOCK;
                } else {
                    lock_delay++;
                    curr_piece.place(frame);
                }
            } else {
                if (drop_counter >= frames_per_drop) {
                    curr_piece.xpos++;
                    curr_piece.place(frame);
                    drop_counter = 0;
                    lock_delay = 0;
                } else {
                    drop_counter++;
                    curr_piece.place(frame);
                }
            }
            break;
        }

        for (int j = 0; j < 10; j++) {
            frame[3][j] = true;
            frame[2][j] = false;
        }
        for (int i = 3; i < 24; i++) {
            frame[i][10] = true;
        }

        ntendo.frame_ready(frame);
    }

    return 0;
}