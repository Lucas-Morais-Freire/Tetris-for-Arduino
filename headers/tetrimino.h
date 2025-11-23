#ifndef TETRIMINO_H
#define TETRIMINO_H
#include <avr/io.h>

struct tetrimino {
    int  xpos, ypos;
    char type;
    bool layout[5][4];
    uint8_t size;
    uint8_t rot_state;

    const static int I_r_wkdata[4][5][2];
    const static int I_l_wkdata[4][5][2];
    const static int r_wkdata[4][5][2];
    const static int l_wkdata[4][5][2];
    char get_random_piece() const;

    tetrimino() : tetrimino(get_random_piece()) {}
    tetrimino(char type);
    bool check_validity(bool (&frame)[24][16]);
    void place(bool (&frame)[24][16]);
    void unplace(bool (&frame)[24][16]);
    bool check_lock(bool (&frame)[24][16]);
    bool rotate_l(bool (&frame)[24][16]);
    bool rotate_r(bool (&frame)[24][16]);
};

#endif // TETRIMINO_H