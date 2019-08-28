#define F_CPU 16000000UL   // частота 16 MHz
#include <avr/io.h>        // используем стандартную библиотеку (для языка Си) для данного микроконтроллера (Atmega328P)
#include <util/delay.h>    // delay.h для функции _delay_ms()
#include <avr/interrupt.h> // interrupt.h для макроса ISR, он нужен, чтобы создавать обработчики прерываний
#include <stdlib.h>        // stdlib.h для функции rand(), а ней реализован простейший алгоритм генерации (псевдо)случайных чисел
#define TRUE 1
#define FALSE 0            // некоторе макросы, чтобы в коде было меньше так называемых магических констант
#define WIDTH 8
#define HEIGHT 8

typedef struct
{
	unsigned char X : 3; // решил всё-таки попробовать битовые поля.
	unsigned char   : 1; // теперь нету проверок на выход за границы
	unsigned char Y : 3; // потому что переполнение сделает нужные значения само
} COORD;

#define G2R2_TYPE COORD      // тут подключаю свой файлик с четырьмя функциями для работы с массивами,
#include "Gena2018115rus2.c" // он требует объявить макрос G2R2_TYPE в котором будет имя типа структур, из которых будут массивы

// массив body будет хранить координаты змейки:
// пусть голова - элемент №0, первая клетка тела - элемент №1....
// каждый элемент массива хранит X и Y соответствующей клетки тела
// размер массива соответствует количеству клеток, составляющих поле
// в массив сразу записаны начальные координаты змейки
// явно не указанные координаты устанавливаются в 0, в соответствии со стандартом языка Си
// чтобы подчеркнуть, что произойдёт неявная инициализация, в конце перечисления оставлена запятая
COORD body[WIDTH * HEIGHT] = {{4, 3}, {3, 3}, {2, 3}, };

// т.к. поле 8 * 8, длина змейки не превысит 64 - тип char (-128 - 127)
// а т.к. лучше использовать беззнаковый тип - unsigned char (0 - 255)
unsigned char length = 3; // стартовая длина

enum
{
	RIGHT,
	UP,
	LEFT,
	DOWN
} direction = RIGHT; // эта переменная будет хранить направление змейки; стартовое направление - вправо

// key_pressed хранит, повернула ли змейка после последнего сдвига, это надо, чтобы нельзя было повернуть два раза между сдвигами
// (чтобы отменить поворот или повернуть на 180 градусов(случайно))
_Bool key_pressed = FALSE; // со старта = ложь, естественно

// apple хранит координаты яблока
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

// ISR - "функция" обработки прерывания
ISR(TIMER1_COMPA_vect) // прерывание таймера 1
{
	// dead установится в истину, когда змейка умрёт(врежется в себя, в данном случае); это надо, чтобы остановить движение
	static _Bool dead = FALSE;

	// сюда мы будем попадать при срабатывании (в нашем случае единственного настроенного) таймера (работа таймера подробнее описана ниже)
	
	// тут будем двигать змейку
	if (dead) return; // если она не мертва
	
	// вычисляем координаты в соответствии с направлением (увеличиваем или уменьшаем на 1 нужную коодинату)
	switch (direction)
	{
		COORD coord;
		case RIGHT: coord = (COORD){body[0].X + 1, body[0].Y}; goto unshift;
		case LEFT : coord = (COORD){body[0].X - 1, body[0].Y}; goto unshift;
		case UP   : coord = (COORD){body[0].X, body[0].Y - 1}; goto unshift;
		default   : coord = (COORD){body[0].X, body[0].Y + 1};
		unshift   : G2R2_UNSHIFT_COORD(body, &length, coord);
	}
	
	// и отрезаем хвост, сохраняя координаты отрезанного кусочка в last_pop
	// last_pop хранит координаты конца хвоста в предыдущем кадре, на это место добавляется клетка тела при съедании яблока
	COORD last_pop = G2R2_POP_COORD(body, &length);
	
	// проверка на столкновение с собой
	for (unsigned char i = 4; i < length; ++i)
	{
		// если координаты головы, совпадают с координатами какой-то клетки тела (кроме себя, поэтому отсчёт с одного в предыдущей строке)
		if ((body[0].X == body[i].X) && (body[0].Y == body[i].Y))
		{
			// то надо сдвинуть змейку назад, т.к. выше мы её сдвинули, не смотря на препядствие
			// возвращаем конец на место
			G2R2_PUSH_COORD(body, &length, last_pop);
			// возвращаем голову на место
			G2R2_SHIFT_COORD(body, &length);
			// убиваем
			dead = TRUE;
		}
	}
	 
	 // проверка на столкновение с яблоком: если координаты головы и яблока совпадают
	 if ((body[0].X == apple.X) && (body[0].Y == apple.Y))
	 {
		 PORTD |= 1 << 3; // начало звукового эффекта
		 
		 // то мы двигаем яблоко на случайное место
		APPLE_MOVE: apple_move();
		
		for (unsigned char i = 0; i < length; ++i)
		{
			// если новые координаты яблока совпадают с координатами змейки - пробуем ещё раз
			if ((body[i].X == apple.X) && (body[i].Y == apple.Y))
			{
				goto APPLE_MOVE;
			}
		}
		
		// увеличиваем длину змейки
		G2R2_PUSH_COORD(body, &length, last_pop);
		
		_delay_ms(1);
		PORTD &= ~(1 << 3);// конец звукового эффекта
	 }
	 
	 // ускорение змейки:
	 
	 // таймер считает до числа, первая половина которого хранится в регистре OCR1AL, а вторая в OCR1AH (подробнее смотреть ниже, где настройка таймера)
	 // это число беззнаковое, две половины по 8 бит
	 
	 union
	 {
		 #define T unsigned int
		 T i;
		 unsigned char c[sizeof(T)];
		 #undef T
	 } OCR = {ADC_convert() * 64};
	 
	 // записываем число на своё место (в регистры, на которые смотрит таймер)
	 OCR1AL = OCR.c[0];
	 OCR1AH = OCR.c[1];
	 
	 
	 // сдвинули змейку - сбрасываем key_pressed, чтобы разблокировать поворот
	 key_pressed = FALSE;
}

inline void loadbit(unsigned char bit)
{
	// D0 - импульс // D1 - затвор // D2 - данные // D3 - speaker
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

ISR(PCINT0_vect) // прерывание кнопок
{
	// сюда мы будем попадать при изменении логического состояния любой из кнопок (настройка такого поведения - ниже)
	
	// prevPINB будет хранить предыдущее логическое состояние кнопок
	// PINB - unsigned char, prevPINB - тоже, static, чтобы при выходе из обработчика прерывания значение сохранялось
	static unsigned char prevPINB = 0; // т.к. static, то 0 будет записан только при старте программы
	
	// если key_pressed установлен в "истина", то просто обновим prevPINB
	if (key_pressed) goto RET;
	
	// когда кнопка опускается, то логический уровень устанавливается в 0
	// если рассматривать PINB как число, то тогда оно уменьшается
	if ((PINB & ((1 << PINB2) | (1 << PINB3) | (1 << PINB4) | (1 << PINB5))) < (prevPINB & ((1 << PINB2) | (1 << PINB3) | (1 << PINB4) | (1 << PINB5)))) // т.е. "если хоть одна из кнопок опустилать"
	{
		//_delay_ms(30); // от дребезга кнопок // тут это не надо, т.к. несколько лишних нажатий на ту же кнопку ничего не изменят
		
		// тут мы уже уточняем, какая из кнопок была нажата (сравниваем конкретные биты)
		if ((prevPINB & (1 << PINB2)) != (PINB & (1 << PINB2))) // влево
		{
			// если поворот будет не в обратную сторону (и если это поворот), то поворачиваем и устанавливаем key_pressed в "истина"
			if ((direction != RIGHT) && (direction != LEFT))
			{
				direction = LEFT;
				key_pressed = TRUE;
			}
		}
		else if ((prevPINB & (1 << PINB3)) != (PINB & (1 << PINB3))) // вправо
		{
			if ((direction != LEFT) && (direction != RIGHT))
			{
				direction = RIGHT;
				key_pressed = TRUE;
			}
		}
		else if ((prevPINB & (1 << PINB4)) != (PINB & (1 << PINB4))) // вниз
		{
			if ((direction != UP) && (direction != DOWN))
			{
				direction = DOWN;
				key_pressed = TRUE;
			}
		}
		else if ((prevPINB & (1 << PINB5)) != (PINB & (1 << PINB5))) // вверх
		{
			if ((direction != DOWN) && (direction != UP))
			{
				direction = UP;
				key_pressed = TRUE;
			}
		}
	}
	
	RET: prevPINB = PINB; // обновляем prevPINB
}

// SetPixel будет зажигать светодиод по указанным координатам хотябы на одну миллисекунду(уже меньше)
void SetPixel(unsigned char X, unsigned char Y)
{
	PORTD |= 1 << 7;
	//      11010000
	PORTC |= ~0xD0; // и включаем на катодах
	//      11111100
	PORTB |= ~0xFC; // две строки, т.к. катоды разбросаны на два порта (т.е. надо работать с двумя регистрами)
	
	loadbyte(1 << (7 - X));
	
	// выключаем ток на нужном катоде (и подключаем к земле)
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
	
	_delay_ms(0.1); // останавливаем микроконтроллер на 1/10 миллисекунды
}

int main(void)
{
	
	ADCSRA |= 1 << ADEN; // вкл ацп
	ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2); // делитель 128
	ADMUX  |= 1 << REFS0; // ион - AVcc (напряжение питания)
	ADMUX  |= 4; // вход ADC4
	
	
	// TCCR1 - регистр режима (сам 16-ти битный) // 1 - т.к. первый таймер (16-ти битный)
	// именно в B нужные биты // режим - по совпадению
	// для CTC без генератора надо оставить выключенными
	// WGM10(Waveform Generator M?odulation), WGM11 и WGM13, а WGM12 включить (судя по таблице в даташите)
	
	// устанавливаем режим таймера - по совпадению. т.е. он будет срабатывать каждый раз, когда досчитает до некоторого числа
	TCCR1B |= (1 << WGM12);
	
	// TIMSK - timer mask
	// если в этот регистр ничего не забить, то таймер никогда не включится
	// OCIE1A - разрешение прерываний по совпадению с OCR1A
	
	// это число будет в OCR1A
	TIMSK1 |= (1 << OCIE1A);
	
	// число (в двух регистрах) на котором будем сбрасывать таймер (т.к. CTC)
	// 16'000'000 Hz / 256 = 0b1111'0100'0010'0100  около 60'000
	//OCR1AH = 0b11110100;
	//OCR1AL = 0b00100100;
	
	// OCR1A хранится в двух регистрах. устанавливаю максивальное значение, чтобы был больший потенциал, куда замедляться
	OCR1AH = 0xFF;
	OCR1AL = 0xFF;
	
	// включим делитель
	// CS№n - Clock Select bit (№ таймера)
	TCCR1B |= (1 << CS12); // делитель 256 // т.е. таймер будет срабатывать только 1 раз на 256 совпадений с OCR1A
	
	// включаем прерывания для кнопок
	PCICR |= (1 << PCIE0); // на порту B;
	//      00111100
	PCMSK0 |= 0x3C; // второй, 3, четвёртой и пятой ногах
	
	
	// настройка ножек
	
	DDRD = 0xFF; // аноды - на выход
	PORTD = 0; // пока выкл
	
	//    00101111
	DDRC |= 0x2F; // катоды 0 - 5 на выход // когда на вход - к земле не притянуть
	//    00000011
	DDRB |= 0x03; // катоды 6 и 7 на выход
	//     11010000
	PORTC &= 0xD0; // тут тоже пока выключу // катоды 0 - 5
	//     11111100
	PORTB &= 0xFC; // и 6 - 7
	
	//    11000011
	DDRB &= 0xC3; // кнопки на вход
	//     00111100
	PORTB |= 0x3C; // подтягиваем к ним резисторы на 20-50 килоОм
	
	
	{
		unsigned int seed;
		EEPROM_read(0, &seed, sizeof(seed));
		srand(seed + 3 + ADC_convert());
		seed = rand();
		EEPROM_write(0, &seed, sizeof(seed));
	}
	
	// стартовая позиция яблока
	apple_move();
	
	// set interrupt // стартуем(разрешаем) прерывания
	sei();
	
    for (;;)
    {
		// тут будем зажигать и тушить светодиоды
		
		//пробегаем на телу змейки
		for (unsigned char i = 0; i < length; ++i)
		{
			// X и Y переданы наоборот, чтобы повернуть изображение на 90 градусов.
			// это надо, т.к. при составлении схемы я забил на это, зная, что смогу повернуть програмно
			SetPixel(body[i].Y, body[i].X);
		}
		
		// и не забываем про яблоко
		SetPixel(apple.Y, apple.X);
    }
}
