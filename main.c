#define F_CPU 16000000UL   // ������� 16 MHz
#include <avr/io.h>        // ���������� ����������� ���������� (��� ����� ��) ��� ������� ���������������� (Atmega328P)
#include <util/delay.h>    // delay.h ��� ������� _delay_ms()
#include <avr/interrupt.h> // interrupt.h ��� ������� ISR, �� �����, ����� ��������� ����������� ����������
#include <stdlib.h>        // stdlib.h ��� ������� rand(), � ��� ���������� ���������� �������� ��������� (������)��������� �����
#define TRUE 1
#define FALSE 0            // �������� �������, ����� � ���� ���� ������ ��� ���������� ���������� ��������
#define WIDTH 8
#define HEIGHT 8

typedef struct
{
	unsigned char X : 3; // ����� ��-���� ����������� ������� ����.
	unsigned char   : 1; // ������ ���� �������� �� ����� �� �������
	unsigned char Y : 3; // ������ ��� ������������ ������� ������ �������� ����
} COORD;

#define G2R2_TYPE COORD      // ��� ��������� ���� ������ � �������� ��������� ��� ������ � ���������,
#include "Gena2018115rus2.c" // �� ������� �������� ������ G2R2_TYPE � ������� ����� ��� ���� ��������, �� ������� ����� �������

// ������ body ����� ������� ���������� ������:
// ����� ������ - ������� �0, ������ ������ ���� - ������� �1....
// ������ ������� ������� ������ X � Y ��������������� ������ ����
// ������ ������� ������������� ���������� ������, ������������ ����
// � ������ ����� �������� ��������� ���������� ������
// ���� �� ��������� ���������� ��������������� � 0, � ������������ �� ���������� ����� ��
// ����� �����������, ��� ��������� ������� �������������, � ����� ������������ ��������� �������
COORD body[WIDTH * HEIGHT] = {{4, 3}, {3, 3}, {2, 3}, };

// �.�. ���� 8 * 8, ����� ������ �� �������� 64 - ��� char (-128 - 127)
// � �.�. ����� ������������ ����������� ��� - unsigned char (0 - 255)
unsigned char length = 3; // ��������� �����

enum
{
	RIGHT,
	UP,
	LEFT,
	DOWN
} direction = RIGHT; // ��� ���������� ����� ������� ����������� ������; ��������� ����������� - ������

// key_pressed ������, ��������� �� ������ ����� ���������� ������, ��� ����, ����� ������ ���� ��������� ��� ���� ����� ��������
// (����� �������� ������� ��� ��������� �� 180 ��������(��������))
_Bool key_pressed = FALSE; // �� ������ = ����, �����������

// apple ������ ���������� ������
COORD apple;

unsigned int ADC_convert(void)
{
	ADCSRA |= 1 << ADSC;
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

inline void EEPROM_write_byte(unsigned int dst, unsigned char byte)
{
	while (EECR & (1 << EEPE));
	EEAR = dst;
	EEDR = byte;
	EECR |= 1 << EEMPE;
	EECR |= 1 << EEPE;
}

inline unsigned char EEPROM_read_byte(unsigned int src)
{
	while (EECR & (1 << EEPE));
	EEAR = src;
	EECR |= 1 << EERE;
	while (EECR & (1 << EERE));
	return EEDR;
}

inline void EEPROM_write(unsigned int dst, void *src, unsigned int bytes_count)
{
	for (unsigned int i = 0; i < bytes_count; ++i)
	{
		EEPROM_write_byte(dst + i, ((unsigned char *)src)[i]);
	}
}

inline void EEPROM_read(unsigned int src, void *dst, unsigned int bytes_count)
{
	for (unsigned int i = 0; i < bytes_count; ++i)
	{
		((unsigned char *)dst)[i] = EEPROM_read_byte(src + i);
	}
}

inline void apple_move(void)
{
	*(unsigned char *)&apple = rand();
}

// ISR - "�������" ��������� ����������
ISR(TIMER1_COMPA_vect) // ���������� ������� 1
{
	// dead ����������� � ������, ����� ������ ����(�������� � ����, � ������ ������); ��� ����, ����� ���������� ��������
	static _Bool dead = FALSE;

	// ���� �� ����� �������� ��� ������������ (� ����� ������ ������������� ������������) ������� (������ ������� ��������� ������� ����)
	
	// ��� ����� ������� ������
	if (dead) return; // ���� ��� �� ������
	
	// ��������� ���������� � ������������ � ������������ (����������� ��� ��������� �� 1 ������ ���������)
	switch (direction)
	{
		COORD coord;
		case RIGHT: coord = (COORD){body[0].X + 1, body[0].Y}; goto unshift;
		case LEFT : coord = (COORD){body[0].X - 1, body[0].Y}; goto unshift;
		case UP   : coord = (COORD){body[0].X, body[0].Y - 1}; goto unshift;
		default   : coord = (COORD){body[0].X, body[0].Y + 1};
		unshift   : G2R2_UNSHIFT_COORD(body, &length, coord);
	}
	
	// � �������� �����, �������� ���������� ����������� ������� � last_pop
	// last_pop ������ ���������� ����� ������ � ���������� �����, �� ��� ����� ����������� ������ ���� ��� �������� ������
	COORD last_pop = G2R2_POP_COORD(body, &length);
	
	// �������� �� ������������ � �����
	for (unsigned char i = 4; i < length; ++i)
	{
		// ���� ���������� ������, ��������� � ������������ �����-�� ������ ���� (����� ����, ������� ������ � ������ � ���������� ������)
		if ((body[0].X == body[i].X) && (body[0].Y == body[i].Y))
		{
			// �� ���� �������� ������ �����, �.�. ���� �� � ��������, �� ������ �� �����������
			// ���������� ����� �� �����
			G2R2_PUSH_COORD(body, &length, last_pop);
			// ���������� ������ �� �����
			G2R2_SHIFT_COORD(body, &length);
			// �������
			dead = TRUE;
		}
	}
	 
	 // �������� �� ������������ � �������: ���� ���������� ������ � ������ ���������
	 if ((body[0].X == apple.X) && (body[0].Y == apple.Y))
	 {
		 PORTD |= 1 << 3; // ������ ��������� �������
		 
		 // �� �� ������� ������ �� ��������� �����
		APPLE_MOVE: apple_move();
		
		for (unsigned char i = 0; i < length; ++i)
		{
			// ���� ����� ���������� ������ ��������� � ������������ ������ - ������� ��� ���
			if ((body[i].X == apple.X) && (body[i].Y == apple.Y))
			{
				goto APPLE_MOVE;
			}
		}
		
		// ����������� ����� ������
		G2R2_PUSH_COORD(body, &length, last_pop);
		
		_delay_ms(1);
		PORTD &= ~(1 << 3);// ����� ��������� �������
	 }
	 
	 // ��������� ������:
	 
	 // ������ ������� �� �����, ������ �������� �������� �������� � �������� OCR1AL, � ������ � OCR1AH (��������� �������� ����, ��� ��������� �������)
	 // ��� ����� �����������, ��� �������� �� 8 ���
	 
	 union
	 {
		 #define T unsigned int
		 T i;
		 unsigned char c[sizeof(T)];
		 #undef T
	 } OCR = {ADC_convert() * 64};
	 
	 // ���������� ����� �� ��� ����� (� ��������, �� ������� ������� ������)
	 OCR1AL = OCR.c[0];
	 OCR1AH = OCR.c[1];
	 
	 
	 // �������� ������ - ���������� key_pressed, ����� �������������� �������
	 key_pressed = FALSE;
}

inline void loadbit(unsigned char bit)
{
	// D0 - ������� // D1 - ������ // D2 - ������ // D3 - speaker
	//             11111011
	PORTD = (PORTD & 0xFB) | ((bit & 1) << 2);
	PORTD |= 1;
	PORTD &= ~1;
}

inline void loadbyte(unsigned char byte)
{
	for (unsigned char i = 0; i < 8; ++i)
	{
		loadbit(byte >> i);
	}
	PORTD |= 1 << 1;
	PORTD &= ~(1 << 1);
}

ISR(PCINT0_vect) // ���������� ������
{
	// ���� �� ����� �������� ��� ��������� ����������� ��������� ����� �� ������ (��������� ������ ��������� - ����)
	
	// prevPINB ����� ������� ���������� ���������� ��������� ������
	// PINB - unsigned char, prevPINB - ����, static, ����� ��� ������ �� ����������� ���������� �������� �����������
	static unsigned char prevPINB = 0; // �.�. static, �� 0 ����� ������� ������ ��� ������ ���������
	
	// ���� key_pressed ���������� � "������", �� ������ ������� prevPINB
	if (key_pressed) goto RET;
	
	// ����� ������ ����������, �� ���������� ������� ��������������� � 0
	// ���� ������������� PINB ��� �����, �� ����� ��� �����������
	if ((PINB & ((1 << PINB2) | (1 << PINB3) | (1 << PINB4) | (1 << PINB5))) < (prevPINB & ((1 << PINB2) | (1 << PINB3) | (1 << PINB4) | (1 << PINB5)))) // �.�. "���� ���� ���� �� ������ ����������"
	{
		//_delay_ms(30); // �� �������� ������ // ��� ��� �� ����, �.�. ��������� ������ ������� �� �� �� ������ ������ �� �������
		
		// ��� �� ��� ��������, ����� �� ������ ���� ������ (���������� ���������� ����)
		if ((prevPINB & (1 << PINB2)) != (PINB & (1 << PINB2))) // �����
		{
			// ���� ������� ����� �� � �������� ������� (� ���� ��� �������), �� ������������ � ������������� key_pressed � "������"
			if ((direction != RIGHT) && (direction != LEFT))
			{
				direction = LEFT;
				key_pressed = TRUE;
			}
		}
		else if ((prevPINB & (1 << PINB3)) != (PINB & (1 << PINB3))) // ������
		{
			if ((direction != LEFT) && (direction != RIGHT))
			{
				direction = RIGHT;
				key_pressed = TRUE;
			}
		}
		else if ((prevPINB & (1 << PINB4)) != (PINB & (1 << PINB4))) // ����
		{
			if ((direction != UP) && (direction != DOWN))
			{
				direction = DOWN;
				key_pressed = TRUE;
			}
		}
		else if ((prevPINB & (1 << PINB5)) != (PINB & (1 << PINB5))) // �����
		{
			if ((direction != DOWN) && (direction != UP))
			{
				direction = UP;
				key_pressed = TRUE;
			}
		}
	}
	
	RET: prevPINB = PINB; // ��������� prevPINB
}

// SetPixel ����� �������� ��������� �� ��������� ����������� ������ �� ���� ������������(��� ������)
void SetPixel(unsigned char X, unsigned char Y)
{
	PORTD |= 1 << 7;
	//      11010000
	PORTC |= ~0xD0; // � �������� �� �������
	//      11111100
	PORTB |= ~0xFC; // ��� ������, �.�. ������ ���������� �� ��� ����� (�.�. ���� �������� � ����� ����������)
	
	loadbyte(1 << (7 - X));
	
	// ��������� ��� �� ������ ������ (� ���������� � �����)
	if (Y == 4)
	{
		PORTD &= ~(1 << 7);
	}
	else if (Y >= 6)
	{
		PORTB &= ~(1 << (Y - 6));
	}
	else
	{
		PORTC &= ~(1 << Y);
	}
	
	_delay_ms(0.1); // ������������� ��������������� �� 1/10 ������������
}

int main(void)
{
	
	ADCSRA |= 1 << ADEN; // ��� ���
	ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2); // �������� 128
	ADMUX  |= 1 << REFS0; // ��� - AVcc (���������� �������)
	ADMUX  |= 4; // ���� ADC4
	
	
	// TCCR1 - ������� ������ (��� 16-�� ������) // 1 - �.�. ������ ������ (16-�� ������)
	// ������ � B ������ ���� // ����� - �� ����������
	// ��� CTC ��� ���������� ���� �������� ������������
	// WGM10(Waveform Generator M?odulation), WGM11 � WGM13, � WGM12 �������� (���� �� ������� � ��������)
	
	// ������������� ����� ������� - �� ����������. �.�. �� ����� ����������� ������ ���, ����� ��������� �� ���������� �����
	TCCR1B |= (1 << WGM12);
	
	// TIMSK - timer mask
	// ���� � ���� ������� ������ �� ������, �� ������ ������� �� ���������
	// OCIE1A - ���������� ���������� �� ���������� � OCR1A
	
	// ��� ����� ����� � OCR1A
	TIMSK1 |= (1 << OCIE1A);
	
	// ����� (� ���� ���������) �� ������� ����� ���������� ������ (�.�. CTC)
	// 16'000'000 Hz / 256 = 0b1111'0100'0010'0100  ����� 60'000
	//OCR1AH = 0b11110100;
	//OCR1AL = 0b00100100;
	
	// OCR1A �������� � ���� ���������. ������������ ������������ ��������, ����� ��� ������� ���������, ���� �����������
	OCR1AH = 0xFF;
	OCR1AL = 0xFF;
	
	// ������� ��������
	// CS�n - Clock Select bit (� �������)
	TCCR1B |= (1 << CS12); // �������� 256 // �.�. ������ ����� ����������� ������ 1 ��� �� 256 ���������� � OCR1A
	
	// �������� ���������� ��� ������
	PCICR |= (1 << PCIE0); // �� ����� B;
	//      00111100
	PCMSK0 |= 0x3C; // ������, 3, �������� � ����� �����
	
	
	// ��������� �����
	
	DDRD = 0xFF; // ����� - �� �����
	PORTD = 0; // ���� ����
	
	//    00101111
	DDRC |= 0x2F; // ������ 0 - 5 �� ����� // ����� �� ���� - � ����� �� ���������
	//    00000011
	DDRB |= 0x03; // ������ 6 � 7 �� �����
	//     11010000
	PORTC &= 0xD0; // ��� ���� ���� ������� // ������ 0 - 5
	//     11111100
	PORTB &= 0xFC; // � 6 - 7
	
	//    11000011
	DDRB &= 0xC3; // ������ �� ����
	//     00111100
	PORTB |= 0x3C; // ����������� � ��� ��������� �� 20-50 ������
	
	
	{
		unsigned int seed;
		EEPROM_read(0, &seed, sizeof(seed));
		srand(seed + 3 + ADC_convert());
		seed = rand();
		EEPROM_write(0, &seed, sizeof(seed));
	}
	
	// ��������� ������� ������
	apple_move();
	
	// set interrupt // ��������(���������) ����������
	sei();
	
    for (;;)
    {
		// ��� ����� �������� � ������ ����������
		
		//��������� �� ���� ������
		for (unsigned char i = 0; i < length; ++i)
		{
			// X � Y �������� ��������, ����� ��������� ����������� �� 90 ��������.
			// ��� ����, �.�. ��� ����������� ����� � ����� �� ���, ����, ��� ����� ��������� ���������
			SetPixel(body[i].Y, body[i].X);
		}
		
		// � �� �������� ��� ������
		SetPixel(apple.Y, apple.X);
    }
}
