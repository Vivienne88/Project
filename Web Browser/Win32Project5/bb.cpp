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

//콘솔
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
//소켓
#pragma comment(lib, "Ws2_32.lib")
//공용콘트롤
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
//ID
#define ID_BUTTON1 9000
#define ID_EDIT1 9001
//버퍼 크기
#define MAXLEN 1024
//함수원형
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("Project");

//버퍼 생성
char buf[MAXLEN];
char* rbuf = NULL;
char* buf_temp = NULL;
char* str_buffer = NULL;
wchar_t* uni = NULL;

//총 받은 패킷 수
int yPos_total = 0;
//텍스트에서 y값
int text_yPos = 0;
//텍스트 내의 공백 수
int back_total = 0;

//Win32 변수
HWND hEdit;
BITMAPINFO bmi;
//Syslink위한 Rect와 HWND
RECT* m_rect = NULL;
HWND* m_link = NULL;

//RECT변수 생성할 때, 계산해주는 함수
int CalculateRect(RECT* rect, int x, int y, int width, int height)
{
	//아무것도 안들어왔을 경우 예외처리
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

//멀티바이트에서 유니코드로 변환해주는 함수
int Multi2Uni(char* buffer, int buffer_size, wchar_t* output)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, buffer, strlen(buffer), 0, 0);
	MultiByteToWideChar(CP_UTF8, 0, buffer, strlen(buffer), output, wlen);
	setlocale(LC_ALL, "korean");
	
	//성공 실패 유무
	if (wlen)
		return 0;
	else
		return 1;
}

//Syslink Control 생성
HWND CreateSysLink(HWND hDlg, RECT rect, int ID, char* buffer, int buffer_size)
{
	//주소 최대 크기 대략 500
	char temp_str[500] = " ";

	//링크 만들기 위한 디폴드값
	strcat(temp_str, "<A HREF=\"");
	strcat(temp_str, buffer);
	strcat(temp_str, "\">");
	strcat(temp_str, buffer);
	strcat(temp_str, "</A>");

	//CreateWindow 인자로 넣기 위해 변형
	wchar_t* w_str = new wchar_t[500]();
	Multi2Uni(temp_str, strlen(temp_str), w_str);
	
	//Syslink Control 생성하고 Handler 반환
	return CreateWindowEx(0, WC_LINK,
		w_str,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		rect.left, rect.top, rect.right, rect.bottom,
		hDlg, (HMENU)ID, NULL, NULL);
}

//Bitmap 8개의 Bit로 생성
static HBITMAP Create8bppBitmap(HDC hdc, int width, int height, LPVOID pBits = NULL)
{
	//Bitmap 헤더
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

	//Bitmap 생성
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
	//예외처리
	if (uBitsPerPixel < 8) // NOT IMPLEMENTED YET
		return NULL;

	if (uBitsPerPixel == 8)
		return Create8bppBitmap(hDC, uWidth, uHeight, pBits);

	HBITMAP hBitmap = 0;
	if (!uWidth || !uHeight || !uBitsPerPixel)
		return hBitmap;
	
	//Bitmap 헤더
	//24일 이상일 경우 생략되는 것이 많다.
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

//unsigned char로 변형
unsigned char* char2unsigned(char* data)
{
	return reinterpret_cast<unsigned char*>(data);
}

enum option { FORWARD, BACKWARD };

//다음 또는 이전 줄을 반환하기 위해 계산해주는 함수
//전진할 경우 \n + 1
//후진할 경우 \n 2번 찾는다.
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

//img 또는 a href와 같은 태그 파싱하기 위한 함수
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
		//태그 문자열 일치
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
			//문자열 비교
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

//index번째 Parsing 내용을 가져오는 함수
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

//대칭 구조를 갖고 있는 함수를 파싱하기 위한 함수
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
		//태그 문자열 일치
		//start_flag 열리면 이제부터 저장 시작
		if (start_flag)
		{
			output[output_index++] = buffer[i + 1];
			//저장 끝나는 시점
			if (buffer[i + 2] == '"')
			{
				start_flag = 0;
				output[output_index++] = '\n';
			}
		}
		else
		{
			//Pattern과 Text가 일치할 경우
			if (buffer[i] == input[input_index++])
				count++;
			else
			{
				input_index = 0;
				count = 0;
			}
			//모든 조건을 만족할 경우 start_flag
			if (count == strlen(input) && buffer[i + 1] == '"' && strncmp(buffer + i + 2, "http://", 7) == 0)
			{
				count = 0;
				start_flag = 1;
			}
		}
	}

	return 0;
}

//내용만 나오도록 파싱 해주는 함수
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

//HTTP Parser용 Class
class Parser
{
public:
	char address[1024];
	char index[1024];

	//배열 초기화 생성자
	Parser()
	{
		for (int i = 0; i < 1024; i++)
		{
			address[i] = '\0';
			index[i] = '\0';
		}
	}

	//괄호 위치 반환하는 함수
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

	//0됨, 1안됨
	//option 0 일 경우 /
	//option 1 일 경우 None
	int HTTP_parser(char* buffer, int buf_size, bool option)
	{
		//HTTP가 있을 경우
		bool HTTP_flag = 0;
		bool WWW_flag = 0;
		bool Pos_flag = 0;
		int dot_count = 0;
		int Pos = 0;

		//대소문자 구분 HTTP가 있나 확인
		if (strncmp(buffer, "HTTP://", 7) == 0)
			HTTP_flag = 1;
		else if (strncmp(buffer, "Http://", 7) == 0)
			HTTP_flag = 1;
		else if (strncmp(buffer, "http://", 7) == 0)
			HTTP_flag = 1;
		else
			HTTP_flag = 0;

		//HTTP가 있을 경우 지우는 과정
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

		//괄호 위치
		Pos = Braket_Pos(buffer, buf_size, HTTP_flag, &dot_count);
		if (Pos == -1)
		{
			Pos = buf_size;
			Pos_flag = 0;
		}
		else
			Pos_flag = 1;

		//주소 가져오기
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

		//html 주소
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

//DNS서버와 연결하기 위한 함수
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
	//초기화
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

	//호스트 이름으로부터 ip를 가져오는 함수
	int get_dns(char* host_name)
	{
		//호스트 파싱
		if (host_name[strlen(host_name)-1] == '\n')
		{
			char* temp = new char[strlen(host_name) - 1];
			memset(temp, 0, strlen(host_name) - 1);
			memcpy(temp, host_name, strlen(host_name) - 1);
			remoteHost = gethostbyname(temp);
		}
		else
			remoteHost = gethostbyname(host_name);

		//오류 검사
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

	//ip 가져오기
	char* get_ip()
	{
		return inet_ntoa(addr);
	}

	//호스트명 가져오기
	char* get_hostname()
	{
		return remoteHost->h_name;
	}
};

//소켓 생성해주는 Class
class Socket
{
private:
	SOCKET sockfd;
	WSADATA wsaData;
	struct sockaddr_in addr;
	int total;
public:
	//연결을 담당하는 함수
	int Connect(char* ip, short int port)
	{
		//초기화
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

		//연결 시도
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

	//URI를 입력 받는 함수, 즉 요청하는 함수이다.
	int Insert(char* message)
	{
		strcpy(buf, message);

		//연결 시도
		int temp = 0;
		if (temp = send(sockfd, buf, MAXLEN, 0) < 0)
		{
			printf("Send Failed!\n");
			printf("%d\n", temp);
			return 1;
		}
		else
			printf("Send Complete!\n");

		//데이터 받기
		total = 0;
		rbuf = new char[MAXLEN]();
		memset(buf, 0, strlen(buf));

		while ((temp = recv(sockfd, buf, MAXLEN, 0)) > 0)
		{
			//버퍼 합치기
			memcpy(rbuf + total, buf, temp);
			total += temp;
			buf_temp = new char[total]();

			memcpy(buf_temp, rbuf, total);
			delete[] rbuf;
			
			//크기 늘리기
			rbuf = new char[MAXLEN + total]();
			memcpy(rbuf, buf_temp, total);
			delete[] buf_temp;
		}
		if (temp == -1)
			printf("Recv Error!\n");

		return 0;
	}

	//총 받은 패킷의 수
	int GetPacketNum()
	{
		return total;
	}

	//소켓 닫기
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
	//변수 선언
	HDC hdc;
	PAINTSTRUCT ps;
	char str[128];
	char new_str[500] = "";
	int parse_int = 0;
	RECT rt;
	GetClientRect(hWnd, &rt);
	HBITMAP bmp;
	HBITMAP hBitmap = 0;
	//클래스 선언
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

	//마우스 좌표
	switch (iMessage)
	{

	case WM_CREATE:
		hEdit = CreateWindow(TEXT("edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_AUTOHSCROLL, 10, 10, 200, 25, hWnd, (HMENU)ID_EDIT1, g_hInst, NULL);
		CreateWindow(TEXT("button"), TEXT("입력"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			10, 50, 100, 25, hWnd, (HMENU)ID_BUTTON1, g_hInst, NULL);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

			//버튼 클릭
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


			//Parsing 되었을 경우
			if (parse_int)
			{
				m_dns.Initialize();
				//주소 오류 처리
				if (m_dns.get_dns(m_parser.address) != 1)
				{
					//연결 및 파싱
					m_socket.Connect(m_dns.get_ip(), 80);
					//m_socket.Connect(m_dns.get_ip(), 443);
					
					strcat(new_str, "GET ");

					//요청 메세지 상황에 따라 파싱
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
					
					//요청하는 정보 출력
					printf("%s\n", new_str);
					//연결 시도
					m_socket.Insert(new_str);
					
					//Tag 내용으로 Parsing
					yPos_total = m_socket.GetPacketNum();
					str_buffer = new char[yPos_total]();
					Parse_Tag(rbuf, yPos_total, str_buffer);
					uni = new wchar_t[yPos_total];
					Multi2Uni(str_buffer, yPos_total, uni);
					
					//유니코드로 파싱된 내용 출력
					wprintf(L"%ls", uni);
					memset(str_buffer, 0, yPos_total);
					
					//전체 버퍼에서 링크와 관련된 태그 가져오기
					Parse_Tag2(rbuf, yPos_total, "a href=", str_buffer);
					
					//연결 종료
					m_socket.Close();
				}
				cout << "End Request\n" << endl;
				InvalidateRect(hWnd, NULL, true);
			}

			//SysLink 생성
			if (yPos_total)
			{
				int tag_temp = 1;
				//다수
				m_rect = new RECT[yPos_total]();
				m_link = new HWND[yPos_total]();

				RECT client_rect;
				GetClientRect(hWnd, &client_rect);

				//Syslink 여러개 생성
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
			//클릭 또는 RETURN 받았을 경우
			case NM_CLICK:

			case NM_RETURN:
			{
				PNMLINK pNMLink = (PNMLINK)IParam;
				LITEM   item = pNMLink->item;

				//Hyperlink 생성
				int tag_temp = 1;
				for (int i = 0; tag_temp == 1; i++)
				{
					tag_temp = Get_Parse_Tag2(str_buffer, strlen(str_buffer), i, rbuf);

					//ID 검사
					if (((LPNMHDR)IParam)->idFrom == (UINT)i)
					{
						ShellExecute(NULL, L"open", L"Iexplore.exe", item.szUrl, NULL, SW_SHOW);
						SetWindowText(hEdit, item.szUrl);
					}

					//Hyperlink 제거
					DestroyWindow(m_link[i]);
				}

				InvalidateRect(hWnd, &rt, 0);

				//RECT 정보 지우기
				delete[] m_rect;
				m_rect = NULL;

				//HWND 정보 지우기	
				delete[] m_link;
				m_link = NULL;

				//버튼 클릭 이벤트 발생
				SendMessage(hWnd, WM_COMMAND, ID_BUTTON1, NULL);
				break;
			}
		}

		break;

	case WM_PAINT:
		
		//화면에 내용 출력
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
			//줄어들 때
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
