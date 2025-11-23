#include "../headers/tetrimino.h"
#include <avr/io.h>
#include <string.h>
#include <Arduino.h>

// dados de wall kick
const int tetrimino::I_r_wkdata[4][5][2] = {
    {{0,0}, { 0,-2}, { 0, 1}, { 1,-2}, {-2, 1}},
    {{0,0}, { 0,-1}, { 0, 2}, {-2,-1}, { 1, 2}},
    {{0,0}, { 0, 2}, { 0,-1}, {-1, 2}, { 2,-1}},
    {{0,0}, { 0, 1}, { 0,-2}, { 2, 1}, {-1,-2}} 
};

const int tetrimino::I_l_wkdata[4][5][2] = {
    {{0,0}, { 0,-1}, { 0, 2}, {-2,-1}, { 1, 2}},
    {{0,0}, { 0, 2}, { 0,-1}, {-1, 2}, { 2,-1}},
    {{0,0}, { 0, 1}, { 0,-2}, { 2, 1}, {-1,-2}},
    {{0,0}, { 0,-2}, { 0, 1}, { 1,-2}, {-2, 1}} 
};

const int tetrimino::r_wkdata[4][5][2] = {
    {{0,0}, { 0,-1}, {-1,-1}, { 2, 0}, { 2,-1}},
    {{0,0}, { 0, 1}, { 1, 1}, {-2, 0}, {-2, 1}},
    {{0,0}, { 0, 1}, {-1, 1}, { 2, 0}, { 2, 1}},
    {{0,0}, { 0,-1}, { 1,-1}, {-2, 0}, {-2,-1}}
};

const int tetrimino::l_wkdata[4][5][2] = {
    {{0,0}, { 0, 1}, {-1, 1}, { 2, 0}, { 2, 1}},
    {{0,0}, { 0, 1}, { 1, 1}, {-2, 0}, {-2, 1}},
    {{0,0}, { 0,-1}, {-1,-1}, { 2, 0}, { 2,-1}},
    {{0,0}, { 0,-1}, { 1,-1}, {-2, 0}, {-2,-1}}
};

tetrimino::tetrimino(char type) {
    this->type = type;
    rot_state = 0;
    memset(layout, false, sizeof(layout));
    switch (type) {
    case 'I':
        xpos = 2;
        ypos = 3;
        for (uint8_t i = 0; i < 4; i++) {
          layout[1][i] = true;
        }
        size = 4;
        break;
    case 'J':
        xpos = 2;
        ypos = 3;
        layout[0][0] = true;
        for (uint8_t i = 0; i < 3; i++) {
          layout[1][i] = true;
        }
        size = 3;
        break;
    case 'L':
        xpos = 2;
        ypos = 3;
        layout[0][2] = true;
        for (uint8_t i = 0; i < 3; i++) {
          layout[1][i] = true;
        }
        size = 3;
        break;
    case 'O':
        xpos = 2;
        ypos = 4;
        layout[0][0] = true; layout[0][1] = true;
        layout[1][0] = true; layout[1][1] = true;
        size = 2;
        break;
    case 'S':
        xpos = 2;
        ypos = 3;
        layout[0][1] = true; layout[0][2] = true;
        layout[1][0] = true; layout[1][1] = true;
        size = 3;
        break;
    case 'T':
        xpos = 2;
        ypos = 3;
        layout[0][1] = true;
        for (uint8_t i = 0; i < 3; i++) {
          layout[1][i] = true;
        }
        size = 3;
        break;
    case 'Z':
        xpos = 2;
        ypos = 3;
        layout[1][1] = true; layout[1][2] = true;
        layout[0][0] = true; layout[0][1] = true;
        size = 3;
    }
}

bool tetrimino::check_validity(bool (&board)[24][16]) {
    for (uint8_t i = 0; i < size; i++) {
        for (uint8_t j = 0; j < size; j++) {
            if (layout[i][j]) {
                if (i + xpos > 23 || j + ypos < 0 || j + ypos > 9) {
                    return false;
                } else if (board[i + xpos][j + ypos]) {
                    return false;
                }
            }
        }
    }
    return true;
}

void tetrimino::place(bool (&board)[24][16]) {
    for (uint8_t i = 0; i < size; i++) {
        for (uint8_t j = 0; j < size; j++) {
            if (layout[i][j]) {
                board[xpos + i][ypos + j] = true;
            }
        }
    }
}

void tetrimino::unplace(bool (&board)[24][16]) {
    for (uint8_t i = 0; i < size; i++) {
        for (uint8_t j = 0; j < size; j++) {
            if (layout[i][j]) {
                board[xpos + i][ypos + j] = false;
            }
        }
    }
}

bool tetrimino::check_lock(bool (&board)[24][16]) {
    // checar se a peca esta no fundo do quadro
    switch (rot_state) {
    case 0:
        if (xpos == 22) {
            return true;
        }
        break;
    case 1:
    case 3:
        switch (type) {
        case 'I':
            if (xpos == 20) {
                return true;
            }
            break;
        case 'O':
            if (xpos == 22) {
                return true;
            }
            break;
        default:
            if (xpos == 21) {
                return true;
            }
        }
        break;
    case 2:
        if (type == 'O') {
            if (xpos == 22) {
                return true;
            }
        } else {
            if (xpos == 21) {
                return true;
            }
        }
        break;
    }
    
    // checar se ha espaco ocupado abaixo da peca
    for (uint8_t i = 4; i > 0; i--) {
        for (uint8_t j = 0; j < 4; j++) {
            if (!layout[i][j] && layout[i - 1][j] && board[xpos + i][ypos + j]) {
                return true;
            }
        }
    }
    return false;
}

bool tetrimino::rotate_r(bool (&board)[24][16]) {
    tetrimino temp_piece(type);
    temp_piece.xpos = xpos;
    temp_piece.ypos = ypos;
    
    for (uint8_t i = 0; i < size; i++) {
        for (uint8_t j = 0; j < size; j++) {
            temp_piece.layout[j][size - i - 1] = layout[i][j];
        }
    }
    temp_piece.rot_state = rot_state == 3 ? 0 : rot_state + 1;
    
    int ogx = temp_piece.xpos;
    int ogy = temp_piece.ypos;
    if (type == 'I') {
        for (uint8_t i = 0; i < 5; i++) {
            temp_piece.xpos = ogx + I_r_wkdata[rot_state][i][0];
            temp_piece.ypos = ogy + I_r_wkdata[rot_state][i][1];
            if (temp_piece.check_validity(board)) {
                xpos = temp_piece.xpos;
                ypos = temp_piece.ypos;
                for (uint8_t j = 0; j < size; j++) {
                    for (uint8_t k = 0; k < size; k++) {
                        layout[j][k] = temp_piece.layout[j][k];
                    }
                }
                rot_state = temp_piece.rot_state;
                return true;
            }
        }
    } else {
        for (uint8_t i = 0; i < 5; i++) {
            temp_piece.xpos = ogx + r_wkdata[rot_state][i][0];
            temp_piece.ypos = ogy + r_wkdata[rot_state][i][1];
            if (temp_piece.check_validity(board)) {
                xpos = temp_piece.xpos;
                ypos = temp_piece.ypos;
                for (uint8_t j = 0; j < size; j++) {
                    for (uint8_t k = 0; k < size; k++) {
                        layout[j][k] = temp_piece.layout[j][k];
                    }
                }
                rot_state = temp_piece.rot_state;
                return true;
            }
        }
    }
    return false;
}

bool tetrimino::rotate_l(bool (&board)[24][16]) {
    tetrimino temp_piece(type);
    temp_piece.xpos = xpos;
    temp_piece.ypos = ypos;
    
    for (uint8_t i = 0; i < size; i++) {
        for (uint8_t j = 0; j < size; j++) {
            temp_piece.layout[size - j - 1][i] = layout[i][j];
        }
    }
    temp_piece.rot_state = rot_state == 0 ? 3 : rot_state - 1;
    
    int ogx = temp_piece.xpos;
    int ogy = temp_piece.ypos;
    if (type == 'I') {
        for (uint8_t i = 0; i < 5; i++) {
            temp_piece.xpos = ogx + I_l_wkdata[rot_state][i][0];
            temp_piece.ypos = ogy + I_l_wkdata[rot_state][i][1];
            if (temp_piece.check_validity(board)) {
                xpos = temp_piece.xpos;
                ypos = temp_piece.ypos;
                for (uint8_t j = 0; j < size; j++) {
                    for (uint8_t k = 0; k < size; k++) {
                        layout[j][k] = temp_piece.layout[j][k];
                    }
                }
                rot_state = temp_piece.rot_state;
                return true;
            }
        }
    } else {
        for (uint8_t i = 0; i < 5; i++) {
            temp_piece.xpos = ogx + l_wkdata[rot_state][i][0];
            temp_piece.ypos = ogy + l_wkdata[rot_state][i][1];
            if (temp_piece.check_validity(board)) {
                xpos = temp_piece.xpos;
                ypos = temp_piece.ypos;
                for (uint8_t j = 0; j < size; j++) {
                    for (uint8_t k = 0; k < size; k++) {
                        layout[j][k] = temp_piece.layout[j][k];
                    }
                }
                rot_state = temp_piece.rot_state;
                return true;
            }
        }
    }
    return false;
}

char tetrimino::get_random_piece() const {
    uint8_t p = random(0, 7);
    switch (p) {
    case 0:
        return 'I';
    case 1:
        return 'J';
    case 2:
        return 'L';
    case 3:
        return 'O';
    case 4:
        return 'S';
    case 5:
        return 'T';
    case 6:
        return 'Z';
    }
}