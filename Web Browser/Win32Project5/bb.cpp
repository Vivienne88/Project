#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <string>
#include <iostream>
#include <winsock2.h>
#include <Windows.h>
#include <locale.h>
#include <atlstr.h> 
#include <wchar.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <gdiplusflat.h>
#include <shlobj.h>
#include <time.h>
#include <random>

using namespace std;
using namespace Gdiplus;
using namespace Gdiplus::DllExports;

//�̹���
#pragma comment (lib,"Gdiplus.lib")

//���� ����
#pragma comment (lib,"Shell32.lib")

//UTF-8 ��ȯ
#pragma execution_character_set("utf-8")

//�ܼ�
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

//����
#pragma comment(lib, "Ws2_32.lib")

//���� ��Ʈ��
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//������ ID
#define ID_BUTTON1 9000
#define ID_EDIT1 9001

//��Ʈ�� ����
#define MAXLEN 65537
#define ADDRESSLEN 2048

//�Լ� ����
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
int Multi2Uni(char* buffer, int buffer_size, wchar_t* output);

//���� ����
char buf[MAXLEN];
char MainFolder[ADDRESSLEN];
char Folder[ADDRESSLEN];
char filename[ADDRESSLEN];
char format[ADDRESSLEN];

char* rbuf = NULL;
char* buf_temp = NULL;
char* str_buffer = NULL;
char* header_buffer = NULL;
char* body_buffer = NULL;
char* img_buffer = NULL;
char* content_buffer = NULL;
wchar_t* uni = NULL;

//�� ���� ��Ŷ ��
int yPos_total = 0;
int yPos_body = 0;
int text_yPos = 0;
bool Error_flag = 0;

//�ؽ�Ʈ ���� ���� ��
int back_total = 0;

//Win32 ����
HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("Project");
HWND hEdit;
RECT* m_rect = NULL;
HWND* m_link = NULL;
WNDPROC oldEditProc;
BYTE* pImageBuffer;
SCROLLINFO si;

//GDI+ ����
Graphics* g;

int Count_Backtotal(char* buf, int buffer_size)
{
	back_total = 0;
	for (int i = 0; i < buffer_size; i++)
	{
		if (buf[i] == '\n')
			back_total ++;
	}
	return 1;
}

//���ϸ�, ���� Parsing
int GetFileName(char* buf, int buffer_size, char* filename, char* format)
{
	int bracketPos = 0;
	int dotPos = 0;
	int endPos = 0;
	bool jsp_flag = 0;
	//���۰� ���� ��츸
	if (buffer_size)
	{
		//���� �������ڸ��� �ִ� ��ȣ�� �� ��ġ ã��
		for (int i = 0; i < buffer_size; i++)
		{
			if (buf[i] == '/')
				bracketPos = i + 1;
			else if (buf[i] == '.')
				dotPos = i + 1;
		
			if (dotPos)
			{
				//jsp�� ���
				if (buf[i] == '?')
				{
					bracketPos = i + 1;
					dotPos = buffer_size;
					jsp_flag = 1;
				}
			}
		}

		//�õ� ����
		srand((unsigned int)time(NULL));

		if(dotPos > bracketPos)
			memcpy(filename, buf + bracketPos, dotPos - bracketPos - 1);
		else
			memcpy(filename, buf + bracketPos, bracketPos - dotPos - 1);

		//���� ���� �ֱ�
		for (int i = 0; i < 4; i++)
		{
			char temp[10];
			itoa(rand() % RAND_MAX, temp, 10);
			strcat(filename, temp);
		}
		strcat(filename, ".");

		//jsp�� ���
		if (jsp_flag)
			memcpy(format, "jpg", 3);
		else
			memcpy(format, buf + dotPos, buffer_size - dotPos - 1);
		return 1;
	}
	else
		return 0;
}

//�̹��� ���� �Լ�
int SaveStream(Image* image, WCHAR* type, WCHAR* filename)
{
	//��Ʈ�� ����
	IStream *Encodingbuffer;
	CreateStreamOnHGlobal(NULL, true, &Encodingbuffer);

	//���� ���� ����
	CLSID m_pngClsid;
	GetEncoderClsid(type, &m_pngClsid);

	//�̹��� ����
	image->Save(Encodingbuffer, &m_pngClsid, NULL);
	Bitmap *bitmap = new Bitmap(Encodingbuffer);
	bitmap->Save(filename, &m_pngClsid, NULL);
	return 1;
}

//�̹��� �ε� �Լ�
int LoadStream(char* buf, int buffer_size, Image* Ip)
{
	//��Ʈ�� ����
	IStream *m_pIStream_in;
	CreateStreamOnHGlobal(NULL, true, &m_pIStream_in);

	//��Ʈ���� ������ ����
	m_pIStream_in->Write(body_buffer, yPos_body, NULL);
	if (!m_pIStream_in)
	{
		cout << "Stream is Empty!" << endl;
		return 0;
	}

	//��Ʈ�� �������� �̹��� ��ü ����
	Ip = new Image(m_pIStream_in);
	return 1;
}

//�̹��� �ε� �� ��� �Լ�
int DrawStream(HDC hdc, char* buf, int buffer_size, int x, int y)
{
	//��Ʈ�� ����
	IStream *m_pIStream_in;
	CreateStreamOnHGlobal(NULL, true, &m_pIStream_in);

	//��Ʈ���� ������ ����
	m_pIStream_in->Write(body_buffer, yPos_body, NULL);
	if (!m_pIStream_in)
	{
		cout << "Stream is Empty!" << endl;
		return 0;
	}

	//��Ʈ�� �������� �̹��� ��ü ���� �� ���
	Gdiplus::Image image(m_pIStream_in);
	g = new Graphics(hdc);
	if(g!=NULL)
		g->DrawImage(&image, x, y);

	//���� ����
	char my_format[ADDRESSLEN];
	memset(my_format, 0, ADDRESSLEN);

	strcat(my_format, "image/");
	if (!strncmp(format, "jpg", 3) || !strncmp(format, "jsp", 3))
	{
		strcat(my_format, "jpeg");
		strcat(Folder, "jpg");
	}
	else
	{
		strcat(my_format, format);
		strcat(Folder, format);
	}

	//������ ���� ��� ���� ����
	if (format[0]!='\0' && Folder[0]!='\0')
	{
		//���� ���� 
		wchar_t Formatuni[20];
		wmemset(Formatuni, 0, 20);
		mbstowcs(Formatuni, my_format, 20);

		//���ϸ�
		wchar_t Folderuni[ADDRESSLEN];
		wmemset(Folderuni, 0, ADDRESSLEN);
		mbstowcs(Folderuni, Folder, ADDRESSLEN);

		//����
		SaveStream(&image, Formatuni, Folderuni);
		memcpy(Folder, MainFolder, ADDRESSLEN);
		return 1;
	}
	else
		return 0;
}

//����� �ٵ� �и��ؾߵǴ� ��ġ ��ȯ�ϴ� �Լ�
int FindHeaderPos(char* buf, int buf_size, int m_count)
{
	int count = 0;
	for (int i = 0; i < buf_size; i++)
	{
		if (buf[i] == '\n' || buf[i] == '\r')
			count++;
		else if (count == m_count)
			return i;
		else
			count = 0;
	}
	return 0;
}

//���� ���� �������ִ� �Լ�
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num = 0;          // number of image encoders 
	UINT size = 0;         // size of the image encoder array in bytes 

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
	{
		return -1;  // Failure 
	}

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
	{
		return -1;  // Failure 
	}
	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success 
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure 
}

//Rect �ʱ�ȭ���ִ� �Լ�
int CalculateRect(RECT* rect, int x, int y, int width, int height)
{
	if (rect)
	{
		rect->top = y;
		rect->bottom = y + height;
		rect->left = x;
		rect->right = x + width;
		return 1;
	}
	return 0;
}

//��Ƽ����Ʈ���� ���Ϲ���Ʈ�� ��ȯ���ִ� �Լ�
int Multi2Uni(char* buffer, int buffer_size, wchar_t* output)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, buffer, strlen(buffer), 0, 0);
	MultiByteToWideChar(CP_UTF8, 0, buffer, strlen(buffer), output, wlen);
	//wprintf(L"%ls", uni);
	if (wlen)
		return 0;
	else
		return 1;
}

//SysLink �������ִ� �Լ�
HWND CreateSysLink(HWND hDlg, RECT rect, int ID, char* buffer)
{
	char temp_str[3000];
	memset(temp_str, 0, 3000);

	//��ũ ���� �ۼ�
	strcat(temp_str, "<A HREF=\"");
	strcat(temp_str, buffer);
	strcat(temp_str, "\">");
	strcat(temp_str, buffer);
	strcat(temp_str, "</A>");

	//��Ƽ����Ʈ �����ڵ�� ����
	wchar_t* w_str = new wchar_t[3000]();
	Multi2Uni(temp_str, strlen(temp_str), w_str);
	
	//SysLink ����
	return CreateWindowEx(0, WC_LINK,
		w_str,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		rect.left, rect.top, rect.right, rect.bottom,
		hDlg, (HMENU)ID, NULL, NULL);
}

//�Ʒ� �Լ� ���� ����ü
enum option { FORWARD, BACKWARD };

//���ۿ��� ������, ������ ��ġ �������� �Լ�
int Set_yPos(char* buffer, int buffer_size, int yPos, enum option type)
{
	int m_count = 0;
	//���� ���� ��ġ���� �� �Ǵ� �ڿ� �ִ� \n ã��
	if (type == FORWARD)
	{
		for (int i = 0; i < buffer_size - yPos; i++)
		{
			if (buffer[yPos + i] == '\n')
				return yPos + i + 1;
		}
		return 0;
	}
	else if (type == BACKWARD)
	{
		for (int i = 0; i < yPos; i++)
		{
			if (buffer[yPos - i] == '\n')
				m_count++;
			if (m_count == 2)
				return yPos - i;
		}
		return 0;
	}
}

//��Ī�Ǵ� Tag �������� �Լ�
int Parse_Tag3(char* buffer, int buffer_size, char* input, char* output)
{
	bool tag_flag = 0;
	bool start_flag = 0;
	bool finish_flag = 0;
	int start_index = 0;
	int finish_index = 0;
	int count = 0;
	int input_index = 0;
	int output_index = 0;

	for (int i = 0; i < buffer_size; i++)
	{
		//�±� ���ڿ� ��ġ
		if (start_flag == 1 && finish_flag == 0)
		{
			output[output_index++] = buffer[i];
		}

		if (!tag_flag)
		{
			if (buffer[i] == '<')
				tag_flag = 1;
		}
		else
		{
			//���ڿ� ��
			if (start_flag)
			{
				if (buffer[i + 1] == input[input_index++])
					count++;
				else
				{
					input_index = 0;
					tag_flag = 0;
					count = 0;
				}
			}
			else if (buffer[i] == input[input_index++])
				count++;
			else
			{
				input_index = 0;
				tag_flag = 0;
				count = 0;
			}

			if (start_flag)
			{
				if (count == strlen(input) && buffer[i + 2] == '>')
				{
					finish_flag = 1;
					finish_index = i;
				}
				if (start_flag == 1 && finish_flag == 1)
				{
					start_flag = 0;
					finish_flag = 0;
				}
			}
			else if (count == strlen(input) && buffer[i + 1] == '>')
			{
				count = 0;
				input_index = 0;
				start_flag = 1;
				start_index = i;
				i++;
			}
		}
	}
	if (output_index)
	{
		memset(buffer, 0, strlen(buffer));
		strncpy(buffer, output, finish_index - start_index - strlen(input) - 2);
		//printf("%s\n", buffer);
		return 0;
	}
	else
		return 1;
}

//���ۿ��� index��° �ٿ� �ִ� ���� ��������
int Get_Parse_Tag2(char* input, int buffer_size, int index, char* output)
{
	int count = 0;
	int output_index = 0;
	int next_flag = 0;

	for (int i = 0; i < buffer_size; i++)
	{
		// '[' �� '+' �� ���� ����ó��
		if ((input[i] == '[' || input[i] == '+') && next_flag == 0)
		{
			next_flag = 1;
			index += 1;
			count += 1;
		}

		if (index == count && next_flag == 0)
			output[output_index++] = input[i];

		if (input[i] == '\n')
		{
			count++;
			if (next_flag)
				memset(output, 0, MAXLEN);
			output_index = 0;
			next_flag = 0;
		}

		if (count > index)
			return 1;
	}
	return 0;
}

enum Parse_option { HTTP_only, ALL };

//img src������ Tag�� " ���� " ��������
int Parse_Tag2(char* buffer, int buffer_size, char* input, char* output, enum Parse_option option)
{
	bool tag_flag = 0;
	bool start_flag = 0;
	bool finish_flag = 0;
	int start_index = 0;
	int finish_index = 0;
	int count = 0;
	int input_index = 0;
	int output_index = 0;
	int time = 0;

	//��,�ҹ��� ����
	//�ҹ��ڿ� diff��ŭ ���� �빮�� 
	int diff = 'a' - 'A';
	

	for (int i = 0; i < buffer_size; i++)
	{
		//�±� ���ڿ� ��ġ
		//start_flag ������ �������� ���� ����
		if (start_flag)
		{
			//amp;�� ������ ���
			if (time==0 && (!strncmp(buffer + i + 1, "amp;", 4)))
				time = 4;
			else if (buffer[i + 1] != '\r' && time == 0)
				output[output_index++] = buffer[i + 1];

			//amp; �Ľ� ���ؼ� ���� �ð� ���� ���� ����
			if (time > 0)
				time--;

			//���� ������ ����
			if (buffer[i + 2] == '"')
			{
				start_flag = 0;
				output[output_index++] = '\n';
			}
		}
		else
		{
			//Pattern�� Text�� ��ġ�� ���
			char temp = input[input_index] - diff;
			if (buffer[i] == input[input_index])
			{
				input_index++;
				count++;
			}
			else if (temp == buffer[i])
			{
				count++;
				input_index++;
			}
			else
			{
				input_index = 0;
				count = 0;
			}


			//HTTP�� ��������
			if (option == HTTP_only)
			{
				if (count == strlen(input) && buffer[i + 1] == '"' && strncmp(buffer + i + 2, "http://", 7) == 0)
				{
					count = 0;
					start_flag = 1;
				}
			}
			else
			{
				if (count == strlen(input) && buffer[i + 1] == '"')
				{
					count = 0;
					start_flag = 1;
				}
			}
			
		}
	}
	return 0;
}

int Parse_Tag(char* buffer, int buffer_size, char* output)
{
	bool start_flag = 0;
	int count = 0;
	int output_index = 0;

	for (int i = 0; i < buffer_size; i++)
	{
		if (start_flag)
		{
			if (buffer[i] == '<')
			{
				start_flag = 0;
				count = 0;
			}
			else if (buffer[i] == 'v' && buffer[i + 1] == 'a' && buffer[i + 2] == 'r')
				count = 2;
			else if (buffer[i] == '\n' && count == 0)
			{
				count++;
				//back_total++;
				output[output_index++] = '\n';
			}
			else if (buffer[i] == '\n')
				count++;
			else if (count < 2 && output_index > 0 && output[output_index - 1] != '\n')
			{
				output[output_index++] = buffer[i];
			}
			else if (count < 2 && output[output_index] != '\n')
			{
				output[output_index++] = buffer[i];
			}
		}
		if (buffer[i] == '>')
		{
			start_flag = 1;
		}

		if (buffer[i] == '\n')
			back_total++;
	}
	return 0;
}

class Parser
{
public:
	char address[ADDRESSLEN];
	char index[ADDRESSLEN];
	char port[10];

	//�迭 �ʱ�ȭ ������
	Parser()
	{
		for (int i = 0; i < ADDRESSLEN; i++)
		{
			address[i] = '\0';
			index[i] = '\0';
		}
		memset(port, 0, 10);
	}

	//: ��ġ ��ȯ�ϴ� �Լ�
	int Colon_Pos(char* buffer, int buf_size, bool HTTP_flag)
	{
		if (HTTP_flag)
		{
			for (int i = 7; i < buf_size; i++)
			{
				if (buffer[i] == ':')
				{
					return i;
				}
			}
		}
		else
		{
			for (int i = 0; i < buf_size; i++)
			{
				if (buffer[i] == ':')
				{
					return i;
				}
			}
		}
		return 0;
	}

	//��ȣ ��ġ ��ȯ�ϴ� �Լ�
	int Braket_Pos(char* buffer, int buf_size, bool HTTP_flag, int* dot_count)
	{
		int pos = 0;
		int count = 0;

		for (int i = 0; i < buf_size; i++)
		{
			if (HTTP_flag)
			{
				if (buffer[i] == '/')
					count++;
				if (buffer[i] == '.')
					(*dot_count)++;

				if (count == 3)
					return pos;
			}
			else
			{
				if (buffer[i] == '/')
					return pos;
				if (buffer[i] == '.')
					(*dot_count)++;
			}
			pos++;
		}
		return 0;
	}

	//0��, 1�ȵ�
	//option 0 �� ��� /
	//option 1 �� ��� None
	int HTTP_parser(char* buffer, int buf_size, bool option)
	{
		memset(address, 0, ADDRESSLEN);
		memset(index, 0, ADDRESSLEN);
		memset(port, 0, 10);

		//HTTP�� ���� ���
		bool HTTP_flag = 0;
		bool HTTPS_flag = 0;
		bool WWW_flag = 0;
		bool Pos_flag = 0;
		bool Colon_flag = 0;
		int dot_count = 0;
		int Pos = 0;
		int cPos = 0;

		if (strncmp(buffer, "HTTPS://", 8) == 0)
			HTTPS_flag = 1;
		else if (strncmp(buffer, "Https://", 8) == 0)
			HTTPS_flag = 1;
		else if (strncmp(buffer, "https://", 8) == 0)
			HTTPS_flag = 1;
		else
			HTTPS_flag = 0;

		if (strncmp(buffer, "HTTP://", 7) == 0)
			HTTP_flag = 1;
		else if (strncmp(buffer, "Http://", 7) == 0)
			HTTP_flag = 1;
		else if (strncmp(buffer, "http://", 7) == 0)
			HTTP_flag = 1;
		else
			HTTP_flag = 0;

		if (HTTP_flag)
			if (strncmp(buffer + 7, "www.", 4) == 0)
				WWW_flag = 1;
			else if (strncmp(buffer + 7, "WWW.", 4) == 0)
				WWW_flag = 1;
			else if (strncmp(buffer + 7, "Www.", 4) == 0)
				WWW_flag = 1;
			else
				WWW_flag = 0;
		else if(HTTPS_flag)
			if (strncmp(buffer + 8, "www.", 4) == 0)
				WWW_flag = 1;
			else if (strncmp(buffer + 8, "WWW.", 4) == 0)
				WWW_flag = 1;
			else if (strncmp(buffer + 8, "Www.", 4) == 0)
				WWW_flag = 1;
			else
				WWW_flag = 0;
		else
			if (strncmp(buffer, "www.", 4) == 0)
				WWW_flag = 1;
			else
				WWW_flag = 0;

		//��ȣ ��ġ
		Pos = Braket_Pos(buffer, buf_size, HTTP_flag, &dot_count);

		//��ȣ�� ���� ���
		if (Pos)
			Pos_flag = 1;
		else
			Pos = buf_size;

		// : ��ġ
		cPos = Colon_Pos(buffer, 5, HTTP_flag);

		// :�� ���� ���
		if (cPos)
			Colon_flag = 1;
		else
		{
			port[0] = '8';
			port[1] = '0';
			port[2] = '\n';
		}

		//�ּ� ��������
		if (HTTP_flag)
			if (cPos)
				strncpy(address, buffer + 7, cPos - 7);
			else
				strncpy(address, buffer + 7, Pos - 7);
		else
			if (cPos)
				strncpy(address, buffer, cPos);
			else
				strncpy(address, buffer, Pos);
		/*if (WWW_flag)
		strncpy(address, buffer, Pos);
		else
		{
		strncpy(address, buffer, Pos);
		}*/

		//Port ��������
		if (Colon_flag)
		{
			//:�� ��ġ������ 1���ϰ� ��
			strncpy(port, buffer + cPos + 1, Pos - cPos - 1);
			printf("%s\n", port);
		}
		//html �ּ�
		if (option)
		{
			if (Pos_flag)
				strcpy(index, buffer + Pos);
		}
		else
		{
			if (Pos_flag)
				strcpy(index, buffer + Pos);
			else
			{
				index[0] = '/';
				strcpy(index + 1, buffer + Pos);
			}
		}

		if (HTTPS_flag)
			return -1;
		else if (dot_count)
			return 1;
		else
			return 0;
	}
};

class DNS
{
private:
	WSADATA wsaData;
	int iResult;
	DWORD dwError;
	struct hostent *remoteHost;
	struct in_addr addr;
	char **pAlias;

public:
	int Initialize()
	{
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0)
		{
			printf("WSAStartup failed: %d\n", iResult);
			return 1;
		}
		else
			return 0;
	}

	int get_dns(char* host_name)
	{
		int k = 0;

		//ȣ��Ʈ �Ľ�
		if (host_name[strlen(host_name) - 1] == '\n')
		{
			char* temp = new char[strlen(host_name)];
			memset(temp, 0, strlen(host_name));
			memcpy(temp, host_name, strlen(host_name) - 1);
			remoteHost = gethostbyname(temp);
		}
		else
			remoteHost = gethostbyname(host_name);

		if (remoteHost == NULL)
		{
			dwError = WSAGetLastError();
			if (dwError != 0)
			{
				if (dwError == WSAHOST_NOT_FOUND)
				{
					printf("Host not found\n");
					return 1;
				}
				else if (dwError == WSANO_DATA)
				{
					printf("No data record found\n");
					return 1;
				}
				else
				{
					printf("Function failed with error: %ld\n", dwError);
					return 1;
				}
			}
		}
		else
		{
			printf("Function returned:\n");
			printf("\tOfficial name: %s\n", remoteHost->h_name);
			for (pAlias = remoteHost->h_aliases; *pAlias != 0; pAlias++)
			{
				printf("\tAlternate name #%d: %s\n", ++k, *pAlias);
			}
			printf("\tAddress type: ");
			switch (remoteHost->h_addrtype)
			{
			case AF_INET: printf("AF_INET\n");
				break;
			case AF_NETBIOS: printf("AF_NETBIOS\n");
				break;
			default:
				printf(" %d\n", remoteHost->h_addrtype);
				break;
			}
			printf("\tAddress length: %d\n", remoteHost->h_length);
			k = 0;
			if (remoteHost->h_addrtype == AF_INET)
			{
				while (remoteHost->h_addr_list[k] != 0)
				{
					addr.s_addr = *(u_long *)remoteHost->h_addr_list[k++];
					printf("\tIP Address #%d: %s\n", k, inet_ntoa(addr));
				}
			}
			else if (remoteHost->h_addrtype == AF_NETBIOS)
			{
				printf("NETBIOS address was returned\n");
			}
		}
		return 0;
	}

	char* get_ip()
	{
		return inet_ntoa(addr);
	}

	char* get_hostname()
	{
		return remoteHost->h_name;
	}
};

class Socket
{
private:
	SOCKET sockfd;
	WSADATA wsaData;
	struct sockaddr_in addr;
	int total;
public:
	int Connect(char* ip, short int port)
	{
		//�ʱ�ȭ
		memset((void*)&addr, 0x00, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(ip);
		addr.sin_port = htons(port);

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
		{
			return 1;
		}

		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			return 1;
		}

		int timeout = 500;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int));

		//����
		//if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
		if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			printf("Connection Failed!\n");
			WSACleanup();
			return 1;
		}
		else
		{
			printf("Connection Succeed!\n");
			return 0;
		}
	}

	int Insert(char* message)
	{
		strcpy(buf, message);

		//���� �õ�
		int temp = 0;
		if (temp = send(sockfd, buf, MAXLEN, 0) < 0)
		{
			printf("Send Failed!\n");
			printf("%d\n", temp);
			return 1;
		}
		else
			printf("Send Complete!\n");

		//������ �ޱ�
		total = 0;
		rbuf = new char[MAXLEN]();
		//buf_temp = new char[MAXLEN]();
		memset(buf, 0, strlen(buf));

		temp = 0;
		while ((temp = recv(sockfd, buf, MAXLEN, 0)) > 0)
		{
			//���� ��ġ��
			memcpy(rbuf + total, buf, temp);
			total += temp;
			buf_temp = new char[total]();

			memcpy(buf_temp, rbuf, total);
			delete[] rbuf;

			//ũ�� �ø���
			rbuf = new char[MAXLEN + total]();
			memcpy(rbuf, buf_temp, total);
			delete[] buf_temp;
		}
		return 0;
	}

	int GetPacketNum()
	{
		return total;
	}

	void Close()
	{
		//���� �ݱ�
		closesocket(sockfd);
		WSACleanup();
	}
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	//�׷��� ��ü
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

	//GDI+ �ʱ�ȭ
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	//�ѱ���
	setlocale(LC_ALL, "korean");

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.lpszMenuName = NULL;
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW | WS_VSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, (HMENU)NULL, hInstance, NULL);
	RECT rt;
	GetClientRect(hWnd, &rt);

	ShowWindow(hWnd, nCmdShow);

	//���� ����
	char Folder[ADDRESSLEN];
	memset(Folder, 0, ADDRESSLEN);

	//PATH ����
	strcat(Folder, "C:\\Images\\");
	SHCreateDirectoryExA(NULL, Folder, NULL);

	while (GetMessage(&Message, NULL, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	//GDI+ ����
	GdiplusShutdown(gdiplusToken);

	return (int)Message.wParam;
}

//EditText �޼��� �ޱ� ���� �Լ�
LRESULT CALLBACK subEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RETURN:
			SendMessage(GetParent(hWnd), WM_COMMAND, ID_BUTTON1, NULL);
			break;  //or return 0; if you don't want to pass it further to def proc
					//If not your key, skip to default:
		}
	default:
		return CallWindowProc(oldEditProc, hWnd, msg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM IParam)
{
	//���� ����
	HDC hdc;
	PAINTSTRUCT ps;
	char str[ADDRESSLEN];
	char new_str[1000] = "";
	int parse_int = 0;
	RECT rt;
	GetClientRect(hWnd, &rt);
	HBITMAP hBitmap = 0;
	bool flag301 = 0;

	//��ü ����
	Socket m_socket;
	DNS m_dns;
	Parser m_parser;
		
	//GDI+ Color
	Gdiplus::Color cl;
	cl.White;

	int x = 0;               // horizontal and vertical coordinates

	static int xClient;     // width of client area 
	static int yClient;     // height of client area 
	static int xClientMax;  // maximum width of client area 

	static int xChar;       // horizontal scrolling unit 
	static int yChar;       // vertical scrolling unit 

	static int yPos;
	static int xPos;
	int abcLength = 0;  // length of an abc[] item
	
	int j = 0;
	memset(str, 0, ADDRESSLEN);

	switch (iMessage)
	{
	case WM_CREATE:
		//����Ʈ ����
		hEdit = CreateWindow(TEXT("edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_AUTOHSCROLL, 10, 10, rt.right - 30 - 100, 25, hWnd, (HMENU)ID_EDIT1, g_hInst, NULL);
		//��ư ����
		CreateWindow(TEXT("button"), TEXT("�Է�"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			rt.right - 10 - 100, 10, 100, 25, hWnd, (HMENU)ID_BUTTON1, g_hInst, NULL);
		//����Ʈ �޼��� �ڵ鷯
		oldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)subEditProc);

		return 0;

	case WM_COMMAND:

		switch (LOWORD(wParam)) {
		case ID_BUTTON1:
			Error_flag = 0;
			//�ʱ�ȭ 
			//������ SysLink ������ ��� ��� ����
			if (m_link)
			{
				for (int i = 0; Get_Parse_Tag2(str_buffer, yPos_total, i, rbuf) == 1; i++)
				{
					DestroyWindow(m_link[i]);
				}
				delete[] m_rect;
				m_rect = NULL;
				delete[] m_link;
				m_link = NULL;
				InvalidateRect(hWnd, &rt, 1);
				UpdateWindow(hWnd);
			}
			//uni�ڵ� ������ ��� ����
			if (uni)
			{
				delete[] uni;
				uni = NULL;
			}
			
			memset(str, 0, ADDRESSLEN);
			GetWindowTextA(hEdit, str, ADDRESSLEN);

			do
			{
				//�ʱ�ȭ

				m_dns.Initialize();
				memset(m_parser.address, 0, ADDRESSLEN);
				memset(m_parser.index, 0, ADDRESSLEN);

				//ù �����
				if (j == 0)
				{
					parse_int = m_parser.HTTP_parser(str, strlen(str), 0);
	
					//�Ľ̿� �����Ͽ��� ���
					if (parse_int == -1)
					{
						MessageBox(hWnd, L"�˼��մϴ�.\nHTTPS�� �������� �ʽ��ϴ�.", L"�˸�â", MB_OK);
						break;
					}
					if (parse_int == 0)
					{
						MessageBox(hWnd, L"�ּҸ� �Է����ּ���.", L"�˸�â", MB_OK);
						break;
					}
				}		
				else
				{
					//rbuf���� m_parser.index, m_parser.address�� 
					printf("Before Parsing : %s\n", rbuf);
					m_parser.HTTP_parser(rbuf, strlen(rbuf), 0);
					memset(rbuf, 0, MAXLEN);
				}
					
				printf("Address : %s\n", m_parser.address);
				printf("Index : %s\n", m_parser.index);

				if (m_dns.get_dns(m_parser.address) != 1)
				{
					//���� �� �Ľ�
					m_socket.Connect(m_dns.get_ip(), atoi(m_parser.port));

					//��û �޼��� ��� ���� : new_str
					memset(new_str, 0, 1000);
					strcat(new_str, "GET ");

					//��û �޼��� ��Ȳ�� ���� �Ľ�
					if (m_parser.index[strlen(m_parser.index) - 1] == '\n')
					{

						char* temp = new char[strlen(m_parser.index)];
						memset(temp, 0, strlen(m_parser.index));
						memcpy(temp, m_parser.index, strlen(m_parser.index) - 1);
						strcat(new_str, temp);
					}
					else
						strcat(new_str, m_parser.index);

					if(j==0)
						strcat(new_str, " HTTP/1.1\r\n\r\n");
					else
					{
						strcat(new_str, " HTTP/1.1\r\n");
						//Header Info
						strcat(new_str, "Host: ");
						strcat(new_str, m_parser.address);
						strcat(new_str, "\r\n");
						strcat(new_str, "Connection: keep-alive\r\n");
						strcat(new_str, "Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n");
						strcat(new_str, "Accept-Language: ko-KR,ko;q=0.8,en-US;q=0.6,en;q=0.4\r\n\r\n");
					}

					printf("Request : %s\n", new_str);

					m_socket.Insert(new_str);

					yPos_total = m_socket.GetPacketNum();

					//Header, Body Parsing
					
					if (yPos_total > MAXLEN)
						yPos_total = MAXLEN;

					int HeaderPos = FindHeaderPos(rbuf, yPos_total, 4);
					
					//�ʱ�ȭ
					if (header_buffer != NULL)
						delete[] header_buffer;
					if (body_buffer != NULL)
						delete[] body_buffer;

					//��� ��������
					header_buffer = new char[HeaderPos]();
					memcpy(header_buffer, rbuf, HeaderPos);

					//�ٵ� ��������
					body_buffer = new char[yPos_total - HeaderPos]();
					yPos_body = yPos_total - HeaderPos;
					memcpy(body_buffer, rbuf + HeaderPos, yPos_body);

					//ó���� ��� �̹����� ������ ��ũ�� ���� �Ľ� ����
					if (j == 0)
					{
						str_buffer = new char[yPos_total]();
						img_buffer = new char[yPos_total]();
						//content_buffer = new char[yPos_total]();
						//uni = new WCHAR[yPos_total]();
						uni = new WCHAR[MAXLEN]();

						memset(str_buffer, 0, yPos_total);
						memset(img_buffer, 0, yPos_total);
						//memset(content_buffer, 0, yPos_total);

						//Parse_Tag(rbuf, yPos_total, content_buffer);
						Count_Backtotal(rbuf, MAXLEN);
						//printf("%s", content_buffer);

						//index���� ���ϸ� �������� Parser


						Parse_Tag2(rbuf, yPos_total, "a href=", str_buffer, HTTP_only);
						
						//301 Redirection ���� �߻���
						if (str_buffer[0] == '\0')
						{
							//Redirection �õ�
							Parse_Tag2(rbuf, yPos_total, "a href=", str_buffer, ALL);

							//Parsing �ȵ� ��� HTML ������ �ƴ� ���̴�. ��õ� �ߴ�
							if (str_buffer[0] != '\0')
							{
								memcpy(str + strlen(str) - 1, str_buffer, strlen(str_buffer) - 1);
								SetWindowTextA(hEdit, str);
								memset(str_buffer, 0, yPos_total);
								j = -1;
							}
						}
						else
						{
							Parse_Tag2(rbuf, yPos_total, "img src=", img_buffer, HTTP_only);
							m_socket.Close();
						}

						cout << img_buffer << endl;

						//str,img_buffer ��� ������� ������ ���� ���̴�.
						if (img_buffer[0] == '\0' && str_buffer[0] == '\0')
							Error_flag = 1;
						else
						{
							//������ ���� �ʾ��� ��� ���� ����
							memset(Folder, 0, ADDRESSLEN);
							memset(MainFolder, 0, ADDRESSLEN);
							
							//��� ����
							strcat(Folder, "C:\\Images\\");
							strcat(Folder, m_parser.address);
							
							//�ּҸ����� ���� ����
							SHCreateDirectoryExA(NULL, Folder, NULL);
							strcat(Folder, "\\");
							
							memcpy(MainFolder, Folder, ADDRESSLEN);
						}
					}
					else
					{
						//���� �����ϱ� ���� Parsing
						memset(rbuf, 0, MAXLEN);
						if (Get_Parse_Tag2(img_buffer, strlen(img_buffer), j, rbuf))
						{
							memset(filename, 0, ADDRESSLEN);
							memset(format, 0, ADDRESSLEN);
							GetFileName(rbuf, strlen(rbuf), filename, format);
							cout << "filename : " << filename << endl;
							cout << "format : " << format << endl;
							strcat(Folder, filename);
						}
					}
					
					if(!Error_flag)
						memset(rbuf, 0, MAXLEN);
					j++;
					InvalidateRect(hWnd, NULL, true);
					UpdateWindow(hWnd);
				}
				else //���� �������� ��� ����
				{
					if(j==0)
						MessageBox(hWnd, L"ȣ��Ʈ�� ã�� �� �����ϴ�.", L"�˸�â", MB_OK);
					InvalidateRect(hWnd, NULL, true);
					UpdateWindow(hWnd);
					break;
				}
			} while (Get_Parse_Tag2(img_buffer, strlen(img_buffer), j, rbuf) == 1 || j == 0);
			
			
			cout << "End Request\n" << endl;

	
			//g->Clear()
			//Scroll �ʱ�ȭ
			GetScrollInfo(hWnd, SB_VERT, &si);
			SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
			ScrollWindow(hWnd, 0, si.nPos, NULL, &rt);
			SetScrollPos(hWnd, SB_VERT, 0, 1);
			SendMessage(hWnd, WM_VSCROLL, NULL, NULL);
			InvalidateRect(hWnd, &rt, 1);
			UpdateWindow(hWnd);

			//SysLink ����
			if (yPos_total && !Error_flag)
			{
				//�ʱ�ȭ
				if (m_link != NULL)
					delete[] m_link;
				if (m_rect != NULL)
					delete[] m_rect;

				m_rect = new RECT[yPos_total]();
				m_link = new HWND[yPos_total]();

				RECT client_rect;
				GetClientRect(hWnd, &client_rect);

				int temp_x = 0;
				int temp_y = 0;
				memset(rbuf, 0, MAXLEN);

				for (int i = 0; Get_Parse_Tag2(str_buffer, yPos_total, i, rbuf) == 1; i++)
				{
					//������ ��ũ�� ���� Rectangle ���
					CalculateRect(m_rect + i, 0, 50 + (16 * i), client_rect.right, 16);
					
					//������ ��ũ ����
					m_link[i] = CreateSysLink(hWnd, m_rect[i], i, rbuf);
					
					//�ʱ�ȭ
					memset(rbuf, 0, MAXLEN);
				}
				
				//ȭ�� ����
				if (!Error_flag)
				{
					InvalidateRect(hWnd, &rt, 1);
					UpdateWindow(hWnd);
				}
			}

			break;

		//����Ʈ �ڵ鷯
		case ID_EDIT1:
			switch (HIWORD(wParam)) {
			//������ �Ǿ��� ��� ��������
			case EN_CHANGE:
				memset(str, 0, ADDRESSLEN);
				GetWindowTextA(hEdit, str, ADDRESSLEN);
				break;
			}
		}
		return 0;

	case WM_NOTIFY:

		switch (((LPNMHDR)IParam)->code)
		{

		case NM_CLICK:          // Fall through to the next case.

		case NM_RETURN:
		{
			PNMLINK pNMLink = (PNMLINK)IParam;
			LITEM   item = pNMLink->item;

			memset(rbuf, 0, yPos_total);

			//� �����۸�ũ�� ���ȴ��� Ȯ��
			for (int i = 0; Get_Parse_Tag2(str_buffer, yPos_total, i, rbuf) == 1; i++)
			{
				//ID�� ���� �����۸�ũ ����
				if (((LPNMHDR)IParam)->idFrom == (UINT)i)
				{
					//ShellExecute(NULL, L"open", L"Iexplore.exe", item.szUrl, NULL, SW_SHOW);
					SetWindowText(hEdit, item.szUrl);
				}

				//������ ��ũ ����
				DestroyWindow(m_link[i]);
			}

			//�ʱ�ȭ
			InvalidateRect(hWnd, &rt, 0);
			delete[] m_rect;
			m_rect = NULL;
			delete[] m_link;
			m_link = NULL;

			//��ư Ŭ�� �޼��� ������
			SendMessage(hWnd, WM_COMMAND, ID_BUTTON1, NULL);
			break;
		}
		}

		break;

	case WM_PAINT:
		
		hdc = BeginPaint(hWnd, &ps);
		//ȭ�� �����
		SetBkMode(hdc, TRANSPARENT);

		//���� �߻��Ͽ��� ���
		if (Error_flag)
		{
			RECT m_rt;
			m_rt.left = rt.left;
			m_rt.bottom = rt.bottom;
			m_rt.right = rt.right;
			m_rt.top = rt.top + 50;
			GetScrollInfo(hWnd, SB_VERT, &si);		
			DrawTextA(hdc, rbuf + text_yPos, -1, &m_rt, DT_LEFT);
			printf("%d", text_yPos);
		}
		else if (yPos_total)
		{
			//���ۿ� �ִ� �̹��� �׸���
			DrawStream(hdc, body_buffer, yPos_body, 0, 50);
		}

		EndPaint(hWnd, &ps);
		return 0;

	case WM_SIZE:

		// Retrieve the dimensions of the client area. 
		yClient = HIWORD(IParam);
		xClient = LOWORD(IParam);

		// Set the vertical scrolling range and page size
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE | SIF_PAGE;
		si.nMin = 0;
		si.nPage = 5;
		si.nMax = 1500 * si.nPage - 1;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		//ShellExecute(NULL, L"explore", L"http://www.google.com", NULL, NULL, SW_SHOWNORMAL);


	case WM_VSCROLL:
		// Get all the vertial scroll bar information.
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(hWnd, SB_VERT, &si);

		if (back_total)
		{
			si.nMax = back_total;
			si.nPage = 1;
		}
		else
			si.nPage = 5;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		
		// Save the position for comparison later on.
		yPos = si.nPos;
		switch (LOWORD(wParam))
		{

			// User clicked the HOME keyboard key.
		case SB_TOP:
			si.nPos = si.nMin;
			text_yPos = Set_yPos(rbuf, MAXLEN, text_yPos, BACKWARD);
			break;

			// User clicked the END keyboard key.
		case SB_BOTTOM:
			si.nPos = si.nMax;
			text_yPos = Set_yPos(rbuf, MAXLEN, text_yPos, FORWARD);
			break;

			// User clicked the top arrow.
		case SB_LINEUP:
			si.nPos -= 10;
			text_yPos = Set_yPos(rbuf, MAXLEN, text_yPos, BACKWARD);
			break;

			// User clicked the bottom arrow.
		case SB_LINEDOWN:
			si.nPos += 10;
			text_yPos = Set_yPos(rbuf, MAXLEN, text_yPos, FORWARD);
			break;

			// User clicked the scroll bar shaft above the scroll box.
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			text_yPos = Set_yPos(rbuf, MAXLEN, text_yPos, BACKWARD);
			break;

			// User clicked the scroll bar shaft below the scroll box.
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			text_yPos = Set_yPos(rbuf, MAXLEN, text_yPos, FORWARD);
			break;

			// User dragged the scroll box.
		case SB_THUMBTRACK:
			//�پ�� ��
			if (si.nTrackPos < si.nPos)
				for (int i = si.nTrackPos; i < si.nPos; i += si.nPage)
					text_yPos = Set_yPos(rbuf, MAXLEN, text_yPos, BACKWARD);
			else
				for (int i = si.nPos; i < si.nTrackPos; i += si.nPage)
					text_yPos = Set_yPos(rbuf, MAXLEN, text_yPos, FORWARD);
			si.nPos = si.nTrackPos;

			break;

		default:
			break;
		}

		// Set the position and then retrieve it.  Due to adjustments
		// by Windows it may not be the same as the value set.
		si.fMask = SIF_POS;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		GetScrollInfo(hWnd, SB_VERT, &si);

		// If the position has changed, scroll window and update it.
		if (si.nPos != yPos)
		{
			InvalidateRect(hWnd, &rt, 0);
			ScrollWindow(hWnd, 0, yPos - si.nPos, NULL, &rt);
			UpdateWindow(hWnd);
		}

		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	}

	return(DefWindowProc(hWnd, iMessage, wParam, IParam));
}
