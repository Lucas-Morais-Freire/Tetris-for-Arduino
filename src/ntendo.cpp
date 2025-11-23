#include "../headers/ntendo_private.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

namespace ntd {

void ntendo_::set_ports(volatile uint8_t *PORTt,
                        volatile uint8_t *PORTm,
                        volatile uint8_t *PORTb,
                        volatile uint8_t *DDRl,
                        volatile uint8_t *DDRr) {
    _PORTt = PORTt;
    _PORTm = PORTm;
    _PORTb = PORTb;
    _DDRl = DDRl;
    _DDRr = DDRr;

    *_PORTb = 0b10000000;
}

void ntendo_::begin(uint8_t frame_rate) {
    _scans_per_frame = 60/frame_rate;              // calcular quandos scans teremos por frame

    _temp_frame = new uint8_t*[24];                // alocacao dinamica da matriz temporaria
    _temp_frame[0] = new uint8_t[2*24];            //
    for (int i = 1; i < 24; i++) {                 //
        _temp_frame[i] = _temp_frame[i - 1] + 2;   //
    }
    memset((void*)(_temp_frame[0]), 0, 48);        // inicializa-la com valores zerados

    _write_frame = new uint8_t*[24];               // alocacao dinamica da matriz de escrita
    _write_frame[0] = new uint8_t[2*24];           //
    for (int i = 1; i < 24; i++) {                 //
        _write_frame[i] = _write_frame[i - 1] + 2; //
    }
    memset((void*)(_write_frame[0]), 0, 48);       // inicializa-la com valores zerados

    _read_inputs = new char[256];                  // alocar memoria para as entradas de leitura
    _temp_inputs = new char[256];                  // alocar memoria para as entradas temporarias

    cli();
    // UART3 config
    UCSR3A |= (1<<U2X3);                           // velocidade dupla
    UCSR3B |= (1<<RXCIE3)|(1<<RXEN3)|(1<<TXEN3);   // en recv intr, en recv, en trans
    UCSR3C |= (1<<UCSZ30)|(1<<UCSZ31);             // char size 8
    UCSR3C &= ~(1 << UCPOL3);                      // rising edge
    UBRR3   = 0;                                   // baud rate 2Mbps
    
    // UART0 config (debug)
    UCSR0A |= (1<<U2X0);                           // velocidade dupla
    UCSR0B |= (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);   // en recv intr, en recv, en trans
    UCSR0C |= (1<<UCSZ00)|(1<<UCSZ01);             // char size 8
    UCSR0C &= ~(1 << UCPOL0);                      // rising edge
    UBRR0   = 0;                                   // baud rate 2Mbps

    // timer 1 config
    // TCCR1A default
    TCCR1B |= (1<<WGM12)|(1<<CS10); // CTC mode, prescaler 1
    OCR1A = 11110;                  // interrupcao a cada 11111/(16M) = 0.0006944375 segundos (aprox. 1/(60*24))
    TIMSK1 |= (1<<OCIE1A);          // enable intr com reg A

    sei();
    while(_frame_ready);
}

void ntendo_::frame_ready(bool (&frame)[24][16]) { // conversao de uma matriz de booleanos para uma matriz de bits
    volatile uint8_t byte;
    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 2; j++) {
            byte = 0;
            for (int k = 0; k < 8; k++) {
                if (frame[i][(j << 3) + k]) {
                    byte |= (1 << k);
                }
            }
            _temp_frame[i][j] = byte;
        }
    }
    _frame_ready = true;
    while(_frame_ready);
}

uint64_t ntendo_::get_frame_count() {
    return _frame_count;
}

char* ntendo_::get_inputs() {
    return (char*)_read_inputs;
}

uint8_t ntendo_::get_input_len() {
    return _read_len;
}

}

ISR(TIMER1_COMPA_vect) {
    static uint8_t line = 0;
    static volatile uint8_t scan_count = ntd::_scans_per_frame;
    /* A taxa de atualizacao da tela eh constante em 60 fps, porem
       a taxa de atualizacao do jogo pode ser menor. Assim, poderemos
       fazer mais de um scan ate a troca do frame. */

    if (scan_count == ntd::_scans_per_frame) {
        scan_count = 0;           // resetar a contagem de scans
        switch (ntd::_state) {    // dependendo do estado do sistema:

        case ntd::ACK_CONTROLLER: // verificar se controle responde:
            switch (ntd::_recv_state) {              // verificar que tipo de resposta foi obtida na transmissao
            case ntd::NO_RESPONSE:                   // se sem resposta
                /// modificar lcd para "sem controle" (TODO)

                while (!(UCSR3A & (1 << UDRE3)));    // enviar byte de ack
                UDR3 = 'a';
                break;

            case ntd::SUCCESS: // se recebeu com sucesso:
                ntd::_state = ntd::GAME;             // modificar estado para de jogo
                ntd::_recv_state = ntd::NO_RESPONSE; // resetar estado de recepcao
                while (!(UCSR3A & (1 << UDRE3)));    // solicitar as entradas registradas
                UDR3 = 's';
                ntd::_frame_ready = false;           // liberar logica do jogo para construcao do proximo frame
                break;
            }
            break;

        case ntd::GAME: // processamento grafico do jogo
            if (ntd::_frame_ready) {                               // se o frame esta pronto:
                uint8_t** volatile swap_frames = ntd::_temp_frame; // trocar os frames
                ntd::_temp_frame = ntd::_write_frame;
                ntd::_write_frame = swap_frames;
                ntd::_frame_count++;                               // incrementar contador de frames

                switch (ntd::_recv_state) {                        // verificar recepcao de entradas do controle
            
                case ntd::NO_RESPONSE:                             // se sem resposta:
                    // modificar lcd para "sem controle" (TODO)

                    ntd::_state = ntd::ACK_CONTROLLER;             // mudar estado para verificacao do controle
                    while (!(UCSR3A & (1 << UDRE3)));              // enviar byte de ack
                    UDR3 = 'a';
                    break;

                case ntd::SUCCESS: // se recebeu com sucesso:
                    volatile char* volatile swap_inputs = ntd::_temp_inputs; // trocar as entradas
                    ntd::_temp_inputs = ntd::_read_inputs;
                    ntd::_read_inputs = swap_inputs;
                    
                    ntd::_read_len = ntd::_temp_len;                         // carregar novo comprimento da string

                    ntd::_recv_state = ntd::NO_RESPONSE;                     // resetar estado de recepcao
                    while (!(UCSR3A & (1 << UDRE3)));                        // pedir para enviar entradas
                    UDR3 = 's';

                    ntd::_frame_ready = false;                               // liberar para logica do jogo
                    break;
                }
            } else { // se o frame nao esta pronto
                while (!(UCSR0A & (1 << UDRE0))); // enviar 'n' no UART0
                UDR0 = 'n';
            }
            break;
        }
    }

    uint8_t temp = *ntd::_PORTb >> 7;                         // fazer o shift do bit que coloca 5V para uma linha
    *ntd::_PORTb = (*ntd::_PORTb << 1) | (*ntd::_PORTm >> 7); //
    *ntd::_PORTm = (*ntd::_PORTm << 1) | (*ntd::_PORTt >> 7); //
    *ntd::_PORTt = (*ntd::_PORTt << 1) | temp;                //

    *ntd::_DDRl = ntd::_write_frame[line][0];                 // definir se em cada bit das colunas tera 0V ou alta impedancia
    *ntd::_DDRr = ntd::_write_frame[line][1];                 //

    if (line == 23) {                                         // se chegamos na ultima linha
        scan_count++;                                         // incrementamos a contagem de scans
        line = 0;                                             // e resetamos a linha para o topo
    } else {
        line++;                                               // se nao, apenas incrementar o numero da linha
    }
}

// 3, r, a, l

// logica do jogo
// troca de contexto
// 3 (desabilita a interrupcao)
// r, a, l sem troca de contexto
// reabilito a interrupcao

ISR(USART3_RX_vect) {
    UCSR3B &= ~(1<<RXCIE3); // desabilito a interrupcao de recebimento
    uint8_t val;

    switch (ntd::_state) { // executar com base no estado do sistema
    case ntd::ACK_CONTROLLER:
        val = UDR3;        // receber o byte
        if (val == 'k') {  // se foi um k, sucesso
            ntd::_recv_state = ntd::SUCCESS;
        } else {
            ntd::_recv_state = ntd::NO_RESPONSE;
        }
        break;

    case ntd::GAME:
        ntd::_temp_len = UDR3;
        for (int i = 0; i < ntd::_temp_len; i++) { // escrever na string os caracteres recebidos (3)
            while(!(UCSR3A & (1<<RXC3))) {         // preso no loop enquanto nao ha caractere novo recebido
                if (TIFR1 & (1<<OCF1A)) {          // se nao ha caractere novo, e chegou a hora de trocar de linha
                    UCSR3B |= (1<<RXCIE3);         // reabilitar a interrupcao de recebimento
                    return;                        // matar a funcao
                }
            }
            ntd::_temp_inputs[i] = UDR3;           // quando um byte novo chega, escrever na string
        }
        ntd::_recv_state = ntd::SUCCESS;           // obteve sucesso na comunicacao
        break;
    }
    UCSR3B |= (1<<RXCIE3); // reabilito a interrupcao
}