//������� G2R2_POP_G2R2_TYPE		������� ��������� ������� �� �������		����� �������� ������� �������
//������� G2R2_SHIFT_G2R2_TYPE		������� �� ������� ������ �������			����� �������� ������� �������
//��� ���������:
//1. ��������� �� ���������� ������
//2. ��������� �� int: ���������� ��������� � �������

//������� G2R2_UNSHIFT_G2R2_TYPE	��������� ������� � ������ �������			������ �� ����������
//������� G2R2_PUSH_G2R2_TYPE		��������� ������� � ����� �������			������ �� ����������
//��� ���������:
//1. ��������� �� ���������� ������
//2. ��������� �� int: ���������� ��������� � �������
//3. ����� �������

// ��������, ����� �� ����� �� � ��� �� ������� ��������, � �� ���� ������ ���������� namespace ����

#ifdef G2R2_simple_CAT
	#undef G2R2_simple_CAT
#endif
#ifdef G2R2_CAT
	#undef G2R2_CAT
#endif
//������������
#define G2R2_simple_CAT(str1, str2)		str1 ## str2
#define G2R2_CAT(str1, str2)			G2R2_simple_CAT(str1, str2)

// ��� ����� ������_�����^v, ����� ������������� ������� �� C++

//POP
inline G2R2_TYPE G2R2_CAT(G2R2_POP_, G2R2_TYPE)(G2R2_TYPE *arr, unsigned char *len)
{
	G2R2_TYPE res = arr[--(*len)]; // ���������� ��������� ������� � ��������� ����� �������
	arr[*len] = (G2R2_TYPE){ '\0' }; // ������� ��������� �������
	return res; // ���������� �� ��� ���������
}

//SHIFT
inline G2R2_TYPE G2R2_CAT(G2R2_SHIFT_, G2R2_TYPE)(G2R2_TYPE *arr, unsigned char *len)
{
	G2R2_TYPE res = arr[0]; // ���������� ������ ������
	for (unsigned char index = 0; index < *len - 1; ++index)
	{
		arr[index] = arr[index + 1]; // �������� ��� �������� �� ���� � ������� ������
	}
	arr[--(*len)] = (G2R2_TYPE){ '\0' }; // ��������� ����� ������� � ������� ��������� �������, �.�. ���� �� ��� �� �����, � ������� �� ���������
	return res; // ���������� �� ��� ���������
}

//UNSHIFT
inline void G2R2_CAT(G2R2_UNSHIFT_, G2R2_TYPE)(G2R2_TYPE *arr, unsigned char *len, G2R2_TYPE New)
{
	// index - ������ ����, �� ���� ����� �������
	for (unsigned char index = *len; index; --index)
	{
		arr[index] = arr[index - 1]; // �������� ��� �������� �� ���� � ������� �����
	}
	++(*len); // ����������� ����� �������
	arr[0] = New; // ��������� ����� ������� � ������
}

//PUSH
inline void G2R2_CAT(G2R2_PUSH_, G2R2_TYPE)(G2R2_TYPE *arr, unsigned char *len, G2R2_TYPE New)
{
	arr[(*len)++] = New; // ��������� ����� ������� � ����� ������� � ����������� ����� �������
}


#undef G2R2_TYPE // ������� G2R2_TYPE, ����� ����� ���� ����� (��� �� ������������) ���������� ���� ���� ��� ���
