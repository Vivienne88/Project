#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <string>
#include <iostream>
#include <winsock2.h>
#include <Windows.h>
#include <locale.h>
#include <atlstr.h> 
#include <wchar.h>
#include <commctrl.h>

using namespace std;

//�ܼ�
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
//����
#pragma comment(lib, "Ws2_32.lib")
//������Ʈ��
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
//ID
#define ID_BUTTON1 9000
#define ID_EDIT1 9001
//���� ũ��
#define MAXLEN 1024
//�Լ�����
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("Project");

//���� ����
char buf[MAXLEN];
char* rbuf = NULL;
char* buf_temp = NULL;
char* str_buffer = NULL;
wchar_t* uni = NULL;

//�� ���� ��Ŷ ��
int yPos_total = 0;
//�ؽ�Ʈ���� y��
int text_yPos = 0;
//�ؽ�Ʈ ���� ���� ��
int back_total = 0;

//Win32 ����
HWND hEdit;
BITMAPINFO bmi;
//Syslink���� Rect�� HWND
RECT* m_rect = NULL;
HWND* m_link = NULL;

//RECT���� ������ ��, ������ִ� �Լ�
int CalculateRect(RECT* rect, int x, int y, int width, int height)
{
	//�ƹ��͵� �ȵ����� ��� ����ó��
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

//��Ƽ����Ʈ���� �����ڵ�� ��ȯ���ִ� �Լ�
int Multi2Uni(char* buffer, int buffer_size, wchar_t* output)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, buffer, strlen(buffer), 0, 0);
	MultiByteToWideChar(CP_UTF8, 0, buffer, strlen(buffer), output, wlen);
	setlocale(LC_ALL, "korean");
	
	//���� ���� ����
	if (wlen)
		return 0;
	else
		return 1;
}

//Syslink Control ����
HWND CreateSysLink(HWND hDlg, RECT rect, int ID, char* buffer, int buffer_size)
{
	//�ּ� �ִ� ũ�� �뷫 500
	char temp_str[500] = " ";

	//��ũ ����� ���� �����尪
	strcat(temp_str, "<A HREF=\"");
	strcat(temp_str, buffer);
	strcat(temp_str, "\">");
	strcat(temp_str, buffer);
	strcat(temp_str, "</A>");

	//CreateWindow ���ڷ� �ֱ� ���� ����
	wchar_t* w_str = new wchar_t[500]();
	Multi2Uni(temp_str, strlen(temp_str), w_str);
	
	//Syslink Control �����ϰ� Handler ��ȯ
	return CreateWindowEx(0, WC_LINK,
		w_str,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		rect.left, rect.top, rect.right, rect.bottom,
		hDlg, (HMENU)ID, NULL, NULL);
}

//Bitmap 8���� Bit�� ����
static HBITMAP Create8bppBitmap(HDC hdc, int width, int height, LPVOID pBits = NULL)
{
	//Bitmap ���
	BITMAPINFO *bmi = (BITMAPINFO *)malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256);
	BITMAPINFOHEADER &bih(bmi->bmiHeader);
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = width;
	bih.biHeight = height;
	bih.biPlanes = 1;
	bih.biBitCount = 8;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = 0;
	bih.biXPelsPerMeter = 14173;
	bih.biYPelsPerMeter = 14173;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;

	for (int I = 0; I <= 255; I++)
	{
		bmi->bmiColors[I].rgbBlue = bmi->bmiColors[I].rgbGreen = bmi->bmiColors[I].rgbRed = (BYTE)I;
		bmi->bmiColors[I].rgbReserved = 0;
	}

	//Bitmap ����
	void *Pixels = NULL;
	HBITMAP hbmp = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, &Pixels, NULL, 0);

	if (pBits != NULL)
	{
		BYTE* pbBits = (BYTE*)pBits;
		BYTE *Pix = (BYTE *)Pixels;
		memcpy(Pix, pbBits, width * height);
	}
	free(bmi);

	return hbmp;
}

static HBITMAP CreateBitmapFromPixels(HDC hDC, UINT uWidth, UINT uHeight, UINT uBitsPerPixel, LPVOID pBits)
{
	//����ó��
	if (uBitsPerPixel < 8) // NOT IMPLEMENTED YET
		return NULL;

	if (uBitsPerPixel == 8)
		return Create8bppBitmap(hDC, uWidth, uHeight, pBits);

	HBITMAP hBitmap = 0;
	if (!uWidth || !uHeight || !uBitsPerPixel)
		return hBitmap;
	
	//Bitmap ���
	//24�� �̻��� ��� �����Ǵ� ���� ����.
	LONG lBmpSize = uWidth * uHeight * (uBitsPerPixel / 8);
	BITMAPINFO bmpInfo = { 0 };
	bmpInfo.bmiHeader.biBitCount = uBitsPerPixel;
	bmpInfo.bmiHeader.biHeight = uHeight;
	bmpInfo.bmiHeader.biWidth = uWidth;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	
	// Pointer to access the pixels of bitmap
	UINT * pPixels = 0;
	hBitmap = CreateDIBSection(hDC, (BITMAPINFO *)&
		bmpInfo, DIB_RGB_COLORS, (void **)&
		pPixels, NULL, 0);

	if (!hBitmap)
		return hBitmap;

	memcpy(pPixels, pBits, lBmpSize);

	return hBitmap;
}

//unsigned char�� ����
unsigned char* char2unsigned(char* data)
{
	return reinterpret_cast<unsigned char*>(data);
}

enum option { FORWARD, BACKWARD };

//���� �Ǵ� ���� ���� ��ȯ�ϱ� ���� ������ִ� �Լ�
//������ ��� \n + 1
//������ ��� \n 2�� ã�´�.
int Set_yPos(wchar_t* buffer, int buffer_size, int yPos, enum option type)
{
	int m_count = 0;
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
	return 0;
}

//img �Ǵ� a href�� ���� �±� �Ľ��ϱ� ���� �Լ�
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

//index��° Parsing ������ �������� �Լ�
int Get_Parse_Tag2(char* input, int buffer_size, int index, char* output)
{
	int count = 0;
	int output_index = 0;
	for (int i = 0; i < buffer_size; i++)
	{
		if (index == count)
			output[output_index++] = input[i];

		if (input[i] == '\n')
			count++;

		if (count > index)
			return 1;
	}
	return 0;
}

//��Ī ������ ���� �ִ� �Լ��� �Ľ��ϱ� ���� �Լ�
int Parse_Tag2(char* buffer, int buffer_size, char* input, char* output)
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
		//start_flag ������ �������� ���� ����
		if (start_flag)
		{
			output[output_index++] = buffer[i + 1];
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
			if (buffer[i] == input[input_index++])
				count++;
			else
			{
				input_index = 0;
				count = 0;
			}
			//��� ������ ������ ��� start_flag
			if (count == strlen(input) && buffer[i + 1] == '"' && strncmp(buffer + i + 2, "http://", 7) == 0)
			{
				count = 0;
				start_flag = 1;
			}
		}
	}

	return 0;
}

//���븸 �������� �Ľ� ���ִ� �Լ�
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
				back_total++;
				output[output_index++] = '\n';
			}
			else if (buffer[i] == '\n')
				count++;
			else if (count < 2)
			{
				output[output_index++] = buffer[i];
			}
		}
		if (buffer[i] == '>')
		{
			start_flag = 1;
		}
	}
	return 0;
}

//HTTP Parser�� Class
class Parser
{
public:
	char address[1024];
	char index[1024];

	//�迭 �ʱ�ȭ ������
	Parser()
	{
		for (int i = 0; i < 1024; i++)
		{
			address[i] = '\0';
			index[i] = '\0';
		}
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
		return -1;
	}

	//0��, 1�ȵ�
	//option 0 �� ��� /
	//option 1 �� ��� None
	int HTTP_parser(char* buffer, int buf_size, bool option)
	{
		//HTTP�� ���� ���
		bool HTTP_flag = 0;
		bool WWW_flag = 0;
		bool Pos_flag = 0;
		int dot_count = 0;
		int Pos = 0;

		//��ҹ��� ���� HTTP�� �ֳ� Ȯ��
		if (strncmp(buffer, "HTTP://", 7) == 0)
			HTTP_flag = 1;
		else if (strncmp(buffer, "Http://", 7) == 0)
			HTTP_flag = 1;
		else if (strncmp(buffer, "http://", 7) == 0)
			HTTP_flag = 1;
		else
			HTTP_flag = 0;

		//HTTP�� ���� ��� ����� ����
		if (HTTP_flag)
			if (strncmp(buffer + 7, "www.", 4) == 0)
				WWW_flag = 1;
			else if (strncmp(buffer + 7, "WWW.", 4) == 0)
				WWW_flag = 1;
			else if (strncmp(buffer + 7, "Www.", 4) == 0)
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
		if (Pos == -1)
		{
			Pos = buf_size;
			Pos_flag = 0;
		}
		else
			Pos_flag = 1;

		//�ּ� ��������
		if (HTTP_flag)
			if (WWW_flag)
				strncpy(address, buffer + 7, Pos - 7);
			else
				strncpy(address, buffer + 7, Pos - 7);
		else
			if (WWW_flag)
				strncpy(address, buffer, Pos);
			else
				strncpy(address, buffer, Pos);

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
		if (dot_count)
			return 1;
		else
			return 0;
	}
};

//DNS������ �����ϱ� ���� �Լ�
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
	//�ʱ�ȭ
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

	//ȣ��Ʈ �̸����κ��� ip�� �������� �Լ�
	int get_dns(char* host_name)
	{
		//ȣ��Ʈ �Ľ�
		if (host_name[strlen(host_name)-1] == '\n')
		{
			char* temp = new char[strlen(host_name) - 1];
			memset(temp, 0, strlen(host_name) - 1);
			memcpy(temp, host_name, strlen(host_name) - 1);
			remoteHost = gethostbyname(temp);
		}
		else
			remoteHost = gethostbyname(host_name);

		//���� �˻�
		int i = 0;
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
				printf("\tAlternate name #%d: %s\n", ++i, *pAlias);
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
			i = 0;
			if (remoteHost->h_addrtype == AF_INET)
			{
				while (remoteHost->h_addr_list[i] != 0)
				{
					addr.s_addr = *(u_long *)remoteHost->h_addr_list[i++];
					printf("\tIP Address #%d: %s\n", i, inet_ntoa(addr));
				}
			}
			else if (remoteHost->h_addrtype == AF_NETBIOS)
			{
				printf("NETBIOS address was returned\n");
			}
		}
		return 0;
	}

	//ip ��������
	char* get_ip()
	{
		return inet_ntoa(addr);
	}

	//ȣ��Ʈ�� ��������
	char* get_hostname()
	{
		return remoteHost->h_name;
	}
};

//���� �������ִ� Class
class Socket
{
private:
	SOCKET sockfd;
	WSADATA wsaData;
	struct sockaddr_in addr;
	int total;
public:
	//������ ����ϴ� �Լ�
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

		//���� �õ�
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

	//URI�� �Է� �޴� �Լ�, �� ��û�ϴ� �Լ��̴�.
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
		memset(buf, 0, strlen(buf));

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
		if (temp == -1)
			printf("Recv Error!\n");

		return 0;
	}

	//�� ���� ��Ŷ�� ��
	int GetPacketNum()
	{
		return total;
	}

	//���� �ݱ�
	void Close()
	{
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

	printf("Hello world\n");
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

	while (GetMessage(&Message, NULL, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM IParam)
{
	//���� ����
	HDC hdc;
	PAINTSTRUCT ps;
	char str[128];
	char new_str[500] = "";
	int parse_int = 0;
	RECT rt;
	GetClientRect(hWnd, &rt);
	HBITMAP bmp;
	HBITMAP hBitmap = 0;
	//Ŭ���� ����
	Socket m_socket;
	DNS m_dns;
	Parser m_parser;

	void* bits;
	int x = 0, y;               // horizontal and vertical coordinates
								
	SCROLLINFO si;//SCROLLBAR

	static int xClient;     // width of client area 
	static int yClient;     // height of client area 
	static int xClientMax;  // maximum width of client area 

	static int xChar;       // horizontal scrolling unit 
	static int yChar;       // vertical scrolling unit 

	static int yPos;
	static int xPos;
	int abcLength = 0;  // length of an abc[] item
	HRESULT hr;

	//���콺 ��ǥ
	switch (iMessage)
	{

	case WM_CREATE:
		hEdit = CreateWindow(TEXT("edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_AUTOHSCROLL, 10, 10, 200, 25, hWnd, (HMENU)ID_EDIT1, g_hInst, NULL);
		CreateWindow(TEXT("button"), TEXT("�Է�"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			10, 50, 100, 25, hWnd, (HMENU)ID_BUTTON1, g_hInst, NULL);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

			//��ư Ŭ��
		case ID_BUTTON1:

			//InvalidateRect(hWnd, &rt, TRUE);
			if (uni)
			{
				delete[] uni;
				uni = NULL;
			}
			GetWindowTextA(hEdit, str, 128);
			printf("%s\n", str);
			memset(m_parser.address, 0, MAXLEN);
			memset(m_parser.index, 0, MAXLEN);
			memset(new_str, 0, 500);
			parse_int = m_parser.HTTP_parser(str, strlen(str), 0);
			printf("%s\n", m_parser.address);
			printf("%s\n", m_parser.index);


			//Parsing �Ǿ��� ���
			if (parse_int)
			{
				m_dns.Initialize();
				//�ּ� ���� ó��
				if (m_dns.get_dns(m_parser.address) != 1)
				{
					//���� �� �Ľ�
					m_socket.Connect(m_dns.get_ip(), 80);
					//m_socket.Connect(m_dns.get_ip(), 443);
					
					strcat(new_str, "GET ");

					//��û �޼��� ��Ȳ�� ���� �Ľ�
					if (m_parser.index[strlen(m_parser.index) - 1] == '\n')
					{
						char* temp = new char[strlen(m_parser.index) - 1];
						memset(temp, 0, strlen(m_parser.index) - 1);
						memcpy(temp, m_parser.index, strlen(m_parser.index) - 1);
						strcat(new_str, temp);
					}
					else
						strcat(new_str, m_parser.index);

					strcat(new_str, " HTTP/1.1\r\n\r\n");
					//strcat(new_str, "Content-Type : image/gif;\r\n\r\n");
					
					//��û�ϴ� ���� ���
					printf("%s\n", new_str);
					//���� �õ�
					m_socket.Insert(new_str);
					
					//Tag �������� Parsing
					yPos_total = m_socket.GetPacketNum();
					str_buffer = new char[yPos_total]();
					Parse_Tag(rbuf, yPos_total, str_buffer);
					uni = new wchar_t[yPos_total];
					Multi2Uni(str_buffer, yPos_total, uni);
					
					//�����ڵ�� �Ľ̵� ���� ���
					wprintf(L"%ls", uni);
					memset(str_buffer, 0, yPos_total);
					
					//��ü ���ۿ��� ��ũ�� ���õ� �±� ��������
					Parse_Tag2(rbuf, yPos_total, "a href=", str_buffer);
					
					//���� ����
					m_socket.Close();
				}
				cout << "End Request\n" << endl;
				InvalidateRect(hWnd, NULL, true);
			}

			//SysLink ����
			if (yPos_total)
			{
				int tag_temp = 1;
				//�ټ�
				m_rect = new RECT[yPos_total]();
				m_link = new HWND[yPos_total]();

				RECT client_rect;
				GetClientRect(hWnd, &client_rect);

				//Syslink ������ ����
				for (int i = 0; tag_temp == 1; i++)
				{
					memset(rbuf, 0, yPos_total);
					tag_temp = Get_Parse_Tag2(str_buffer, strlen(str_buffer), i, rbuf);
					CalculateRect(m_rect + i, 0, 0 + (16 * i), client_rect.right, 16);
					m_link[i] = CreateSysLink(hWnd, m_rect[i], i, rbuf, strlen(str_buffer));
				}
			}

			break;

		case ID_EDIT1:
			switch (HIWORD(wParam)) {
			case EN_CHANGE:
				GetWindowTextA(hEdit, str, 128);
			}
		}
		return 0;
	case WM_NOTIFY:

		switch (((LPNMHDR)IParam)->code)
		{
			//Ŭ�� �Ǵ� RETURN �޾��� ���
			case NM_CLICK:

			case NM_RETURN:
			{
				PNMLINK pNMLink = (PNMLINK)IParam;
				LITEM   item = pNMLink->item;

				//Hyperlink ����
				int tag_temp = 1;
				for (int i = 0; tag_temp == 1; i++)
				{
					tag_temp = Get_Parse_Tag2(str_buffer, strlen(str_buffer), i, rbuf);

					//ID �˻�
					if (((LPNMHDR)IParam)->idFrom == (UINT)i)
					{
						ShellExecute(NULL, L"open", L"Iexplore.exe", item.szUrl, NULL, SW_SHOW);
						SetWindowText(hEdit, item.szUrl);
					}

					//Hyperlink ����
					DestroyWindow(m_link[i]);
				}

				InvalidateRect(hWnd, &rt, 0);

				//RECT ���� �����
				delete[] m_rect;
				m_rect = NULL;

				//HWND ���� �����	
				delete[] m_link;
				m_link = NULL;

				//��ư Ŭ�� �̺�Ʈ �߻�
				SendMessage(hWnd, WM_COMMAND, ID_BUTTON1, NULL);
				break;
			}
		}

		break;

	case WM_PAINT:
		
		//ȭ�鿡 ���� ���
		hdc = BeginPaint(hWnd, &ps);
		//DrawText(hdc, uni + (text_yPos), -1, &rt, DT_LEFT);
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
		

	case WM_VSCROLL:
		// Get all the vertial scroll bar information.
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(hWnd, SB_VERT, &si);
		if (back_total)
		{
			si.nMax = back_total;
			si.nPage = (si.nMax / back_total);
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
			text_yPos = Set_yPos(uni, yPos_total, text_yPos, BACKWARD);
			break;

			// User clicked the END keyboard key.
		case SB_BOTTOM:
			si.nPos = si.nMax;
			text_yPos = Set_yPos(uni, yPos_total, text_yPos, FORWARD);
			break;

			// User clicked the top arrow.
		case SB_LINEUP:
			si.nPos -= 10;
			text_yPos = Set_yPos(uni, yPos_total, text_yPos, BACKWARD);
			break;

			// User clicked the bottom arrow.
		case SB_LINEDOWN:
			si.nPos += 10;
			text_yPos = Set_yPos(uni, yPos_total, text_yPos, FORWARD);
			break;

			// User clicked the scroll bar shaft above the scroll box.
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			text_yPos = Set_yPos(uni, yPos_total, text_yPos, BACKWARD);
			break;

			// User clicked the scroll bar shaft below the scroll box.
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			text_yPos = Set_yPos(uni, yPos_total, text_yPos, FORWARD);
			break;

			// User dragged the scroll box.
		case SB_THUMBTRACK:
			//�پ�� ��
			if (si.nTrackPos < si.nPos)
				for (int i = si.nTrackPos; i < si.nPos; i += si.nPage)
					text_yPos = Set_yPos(uni, yPos_total, text_yPos, BACKWARD);
			else
				for (int i = si.nPos; i < si.nTrackPos; i += si.nPage)
					text_yPos = Set_yPos(uni, yPos_total, text_yPos, FORWARD);
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
