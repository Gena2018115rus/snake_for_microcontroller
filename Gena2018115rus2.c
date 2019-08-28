//Функция G2R2_POP_G2R2_TYPE		удаляет последний элемент из массива		Вернёт удалённый элемент массива
//Функция G2R2_SHIFT_G2R2_TYPE		удаляет из массива первый элемент			Вернёт удалённый элемент массива
//Они принимают:
//1. Указатель на изменяемый массив
//2. Указатель на int: количество элементов в массиве

//Функция G2R2_UNSHIFT_G2R2_TYPE	добавляет элемент в начало массива			Ничего не возвращает
//Функция G2R2_PUSH_G2R2_TYPE		добавляет элемент в конец массива			Ничего не возвращает
//Они принимают:
//1. Указатель на изменяемый массив
//2. Указатель на int: количество элементов в массиве
//3. Новый элемент

// префиксы, чтобы уж точно ни с чем не совпали названия, в Си ведь только глобальный namespace есть

#ifdef G2R2_simple_CAT
	#undef G2R2_simple_CAT
#endif
#ifdef G2R2_CAT
	#undef G2R2_CAT
#endif
//конкатенация
#define G2R2_simple_CAT(str1, str2)		str1 ## str2
#define G2R2_CAT(str1, str2)			G2R2_simple_CAT(str1, str2)

// тут будут хитрые_штуки^v, чтобы сымитироваать шаблоны из C++

//POP
inline G2R2_TYPE G2R2_CAT(G2R2_POP_, G2R2_TYPE)(G2R2_TYPE *arr, unsigned char *len)
{
	G2R2_TYPE res = arr[--(*len)]; // запоминаем последний элемент и уменьшаем длину массива
	arr[*len] = (G2R2_TYPE){ '\0' }; // стираем последний элемент
	return res; // возвращаем то что запомнили
}

//SHIFT
inline G2R2_TYPE G2R2_CAT(G2R2_SHIFT_, G2R2_TYPE)(G2R2_TYPE *arr, unsigned char *len)
{
	G2R2_TYPE res = arr[0]; // запоминаем первый элмент
	for (unsigned char index = 0; index < *len - 1; ++index)
	{
		arr[index] = arr[index + 1]; // сдвигаем все элементы на один в сторону начала
	}
	arr[--(*len)] = (G2R2_TYPE){ '\0' }; // уменьшаем длину массива и стираем последний элемент, т.к. выше мы его не стёрли, в отличии от остальных
	return res; // возвращаем то что запомнили
}

//UNSHIFT
inline void G2R2_CAT(G2R2_UNSHIFT_, G2R2_TYPE)(G2R2_TYPE *arr, unsigned char *len, G2R2_TYPE New)
{
	// index - индекс того, на кого будем двигать
	for (unsigned char index = *len; index; --index)
	{
		arr[index] = arr[index - 1]; // сдвигаем все элементы на один в сторону конца
	}
	++(*len); // увеличиваем длину массива
	arr[0] = New; // вставляем новый элемент в начало
}

//PUSH
inline void G2R2_CAT(G2R2_PUSH_, G2R2_TYPE)(G2R2_TYPE *arr, unsigned char *len, G2R2_TYPE New)
{
	arr[(*len)++] = New; // вставляем новый элемент в конец массива и увеличиваем длину массива
}


#undef G2R2_TYPE // удаляем G2R2_TYPE, чтобы можно было сразу (той же конструкцией) подключить этот файл ещё раз
