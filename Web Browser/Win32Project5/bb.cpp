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

using namespace std;
using namespace Gdiplus;
using namespace Gdiplus::DllExports;

//이미지
#pragma comment (lib,"Gdiplus.lib")

//UTF-8 변환
#pragma execution_character_set("utf-8")

//콘솔
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

//소켓
#pragma comment(lib, "Ws2_32.lib")

//공용 컨트롤
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//윈도우 ID
#define ID_BUTTON1 9000
#define ID_EDIT1 9001

//스트링 길이
#define MAXLEN 65537
#define ADDRESSLEN 2048

//함수 원형
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("Project");

//버퍼 생성
char buf[MAXLEN];
char* rbuf = NULL;
char* buf_temp = NULL;
char* str_buffer = NULL;
char* header_buffer = NULL;
char* body_buffer = NULL;
char* img_buffer = NULL;
wchar_t* uni = NULL;
BITMAPINFO bmi;
SCROLLINFO si;

//총 받은 패킷 수
int yPos_total = 0;
int yPos_body = 0;
int text_yPos = 0;

//텍스트 내의 공백 수
int back_total = 0;

//Win32 변수
HWND hEdit;
RECT* m_rect = NULL;
HWND* m_link = NULL;
WNDPROC oldEditProc;
BYTE* pImageBuffer;


//이미지 저장 함수
int SaveStream(Image image, WCHAR* type, WCHAR* filename)
{
	IStream *Encodingbuffer;
	CreateStreamOnHGlobal(NULL, true, &Encodingbuffer);

	CLSID m_pngClsid;
	GetEncoderClsid(type, &m_pngClsid);

	image.Save(Encodingbuffer, &m_pngClsid, NULL);
	Bitmap *bitmap = new Bitmap(Encodingbuffer);

	bitmap->Save(filename, &m_pngClsid, NULL);
	return 1;
}

//이미지 로드 함수
int LoadStream(char* buf, int buffer_size, Image* Ip)
{
	IStream *m_pIStream_in;
	CreateStreamOnHGlobal(NULL, true, &m_pIStream_in);

	m_pIStream_in->Write(body_buffer, yPos_body, NULL);
	if (!m_pIStream_in)
	{
		cout << "Stream is Empty!" << endl;
		return 0;
	}
	Ip = new Image(m_pIStream_in);
	return 1;
}

//이미지 로드 후 출력 함수
int DrawStream(HDC hdc, char* buf, int buffer_size, int x, int y)
{
	IStream *m_pIStream_in;
	CreateStreamOnHGlobal(NULL, true, &m_pIStream_in);

	m_pIStream_in->Write(body_buffer, yPos_body, NULL);
	if (!m_pIStream_in)
	{
		cout << "Stream is Empty!" << endl;
		return 0;
	}
	Gdiplus::Image image(m_pIStream_in);
	Graphics* g = new Graphics(hdc);
	if(g!=NULL)
		g->DrawImage(&image, x, y);
	return 1;
}

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

//Rect 초기화해주는 함수
int CalculateRect(RECT* rect, int x, int y, int width, int height)
{
	if (rect)
	{
		rect->top = y;
		//마이너스 일 수도 있음
		rect->bottom = y + height;
		rect->left = x;
		//마이너스 일 수도 있음
		rect->right = x + width;
		return 1;
	}
	return 0;
}

//멀티바이트에서 유니바이트로 변환해주는 함수
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

//SysLink 생성해주는 함수
HWND CreateSysLink(HWND hDlg, RECT rect, int ID, char* buffer)
{
	char temp_str[3000];
	memset(temp_str, 0, 3000);

	strcat(temp_str, "<A HREF=\"");
	strcat(temp_str, buffer);
	strcat(temp_str, "\">");
	strcat(temp_str, buffer);
	strcat(temp_str, "</A>");

	//string new_str;
	//new_str = char2string(temp_str);

	wchar_t* w_str = new wchar_t[3000]();
	Multi2Uni(temp_str, strlen(temp_str), w_str);
	//wcscat(w_str,);

	return CreateWindowEx(0, WC_LINK,
		w_str,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		rect.left, rect.top, rect.right, rect.bottom,
		hDlg, (HMENU)ID, NULL, NULL);
}

//아래 함수 위한 열거체
enum option { FORWARD, BACKWARD };

//버퍼에서 다음줄, 이전줄 위치 가져오는 함수
int Set_yPos(char* buffer, int buffer_size, int yPos, enum option type)
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

//대칭되는 Tag 가져오는 함수
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

//버퍼에서 index번째 줄에 있는 원소 가져오기
int Get_Parse_Tag2(char* input, int buffer_size, int index, char* output)
{
	int count = 0;
	int output_index = 0;
	int next_flag = 0;

	for (int i = 0; i < buffer_size; i++)
	{
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

//img src종류의 Tag에 " " 가져오기
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
			if (buffer[i + 1] != '\r')
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
	}

	//변환
	//ANSI to UNI


	return 0;
}



class Parser
{
public:
	char address[ADDRESSLEN];
	char index[ADDRESSLEN];
	char port[10];

	//배열 초기화 생성자
	Parser()
	{
		for (int i = 0; i < ADDRESSLEN; i++)
		{
			address[i] = '\0';
			index[i] = '\0';
		}
		memset(port, 0, 10);
	}

	//: 위치 반환하는 함수
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
		return 0;
	}

	//0됨, 1안됨
	//option 0 일 경우 /
	//option 1 일 경우 None
	int HTTP_parser(char* buffer, int buf_size, bool option)
	{
		memset(address, 0, ADDRESSLEN);
		memset(index, 0, ADDRESSLEN);
		memset(port, 0, 10);

		//HTTP가 있을 경우
		bool HTTP_flag = 0;
		bool WWW_flag = 0;
		bool Pos_flag = 0;
		bool Colon_flag = 0;
		int dot_count = 0;
		int Pos = 0;
		int cPos = 0;

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
		else
			if (strncmp(buffer, "www.", 4) == 0)
				WWW_flag = 1;
			else
				WWW_flag = 0;

		//괄호 위치
		Pos = Braket_Pos(buffer, buf_size, HTTP_flag, &dot_count);

		//괄호가 있을 경우
		if (Pos)
			Pos_flag = 1;
		else
			Pos = buf_size;

		// : 위치
		cPos = Colon_Pos(buffer, 5, HTTP_flag);

		// :가 있을 경우
		if (cPos)
			Colon_flag = 1;
		else
		{
			port[0] = '8';
			port[1] = '0';
			port[2] = '\n';
		}

		//주소 가져오기
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

		//Port 가져오기
		if (Colon_flag)
		{
			//:의 위치때문에 1더하고 뺌
			strncpy(port, buffer + cPos + 1, Pos - cPos - 1);
			printf("%s\n", port);
		}
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

		//호스트 파싱
		if (host_name[strlen(host_name) - 1] == '\n')
		{
			char* temp = new char[strlen(host_name)];
			memset(temp, 0, strlen(host_name));
			memcpy(temp, host_name, strlen(host_name) - 1);
			remoteHost = gethostbyname(temp);
		}
		else
			remoteHost = gethostbyname(host_name);
	
		//여기 수정중
		//getaddrinfo()
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

		int timeout = 500;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int));

		//연결
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
		//buf_temp = new char[MAXLEN]();
		memset(buf, 0, strlen(buf));

		temp = 0;
		while ((temp = recv(sockfd, buf, MAXLEN, 0)) > 0)
		{
			//버퍼 합치기
			//memcpy(rbuf + total, buf, strlen(buf));
			memcpy(rbuf + total, buf, temp);
			total += temp;
			buf_temp = new char[total]();

			memcpy(buf_temp, rbuf, total);
			delete[] rbuf;
			//rbuf = NULL;

			//크기 늘리기
			rbuf = new char[MAXLEN + total]();
			memcpy(rbuf, buf_temp, total);
			delete[] buf_temp;
			//buf_temp = NULL;
		}

		/*if (temp == SOCKET_ERROR)
		{
		WSAGetLastError();
		printf("Recv Error!\n");
		}*/
		/*uni = new wchar_t[total];
		Multi2Uni(rbuf, total, uni);

		for (int i = 0; i < 5000; i++)
		{
		printf("%c", uni[i]);
		}*/

		//printf("%s", rbuf);

		//wprintf(L"%ls", uni);
		return 0;
	}

	int GetPacketNum()
	{
		return total;
	}

	void Close()
	{
		//소켓 닫기
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

	//한국어
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

	while (GetMessage(&Message, NULL, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}


	return (int)Message.wParam;
}

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
	//변수 선언
	HDC hdc;
	PAINTSTRUCT ps;
	char str[128];
	char index[128];
	char new_str[1000] = "";
	int parse_int = 0;
	RECT rt;
	GetClientRect(hWnd, &rt);
	HBITMAP bmp;
	HBITMAP hBitmap = 0;
	BITMAP cBitmap;
	//클래스 선언
	Socket m_socket;
	DNS m_dns;
	Parser m_parser;
	//image m_image;

	//BITMAP
	BITMAPINFO* bmi;
	BITMAPFILEHEADER* bmfh;
	BITMAPINFOHEADER* bmih;
	void* bits;
	int x = 0, y;               // horizontal and vertical coordinates

	HWND hwndTest;

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
	int mX;
	int mY;

	//그래픽 객체
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

	//m_image.Initialize();
	int j = 0;

	switch (iMessage)
	{
	case WM_CREATE:
		hEdit = CreateWindow(TEXT("edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_AUTOHSCROLL, 10, 10, rt.right - 30 - 100, 25, hWnd, (HMENU)ID_EDIT1, g_hInst, NULL);
		CreateWindow(TEXT("button"), TEXT("입력"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			rt.right - 10 - 100, 10, 100, 25, hWnd, (HMENU)ID_BUTTON1, g_hInst, NULL);
		oldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)subEditProc);

		return 0;

	case WM_COMMAND:

		switch (LOWORD(wParam)) {
		case ID_BUTTON1:

			if (m_link)
			{
				for (int i = 0; Get_Parse_Tag2(str_buffer, yPos_total, i, rbuf) == 1; i++)
				{
					DestroyWindow(m_link[i]);
				}
				//RECT 정보 지우기
				delete[] m_rect;
				m_rect = NULL;
				//HWND 정보 지우기	
				delete[] m_link;
				m_link = NULL;
				InvalidateRect(hWnd, &rt, 0);
			}

			//InvalidateRect(hWnd, &rt, TRUE);
			if (uni)
			{
				delete[] uni;
				uni = NULL;
			}
			GetWindowTextA(hEdit, str, 128);

			//printf("Input : %s\n", str);
			//printf("Address : %s\n", m_parser.address);
			//printf("Index : %s\n", m_parser.index);

			//GDI+ 초기화
			GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

			//Parsing 되었을 경우
			do
			{
				m_dns.Initialize();
				memset(m_parser.address, 0, ADDRESSLEN);
				memset(m_parser.index, 0, ADDRESSLEN);

				if (j==0)
					parse_int = m_parser.HTTP_parser(str, strlen(str), 0);
				else
				{
					//rbuf에서 m_parser.index, m_parser.address값 
					printf("Before Parsing : %s\n", rbuf);
					m_parser.HTTP_parser(rbuf, strlen(rbuf), 0);
					memset(rbuf, 0, MAXLEN);
				}
					
				printf("Address : %s\n", m_parser.address);
				printf("Index : %s\n", m_parser.index);
					

				if (m_dns.get_dns(m_parser.address) != 1)
				{
					//연결 및 파싱
					m_socket.Connect(m_dns.get_ip(), atoi(m_parser.port));
					memset(new_str, 0, 1000);
					strcat(new_str, "GET ");

					//요청 메세지 상황에 따라 파싱
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
					int HeaderPos = FindHeaderPos(rbuf, yPos_total, 4);
					//printf("%d\n", HeaderPos);

					//있을 경우 삭제
					if (header_buffer != NULL)
						delete[] header_buffer;
					if (body_buffer != NULL)
						delete[] body_buffer;

					//헤더 가져오기
					header_buffer = new char[HeaderPos]();
					memcpy(header_buffer, rbuf, HeaderPos);
					//printf("Header : %s\n",header_buffer);

					//바디 가져오기
					body_buffer = new char[yPos_total - HeaderPos]();
					yPos_body = yPos_total - HeaderPos;
					memcpy(body_buffer, rbuf + HeaderPos, yPos_body);
					//printf("Body : %s\n", body_buffer);
					//cout << "Body : " << body_buffer << endl;


					//Parse_Tag(rbuf, yPos_total, str_buffer);
					//uni = new wchar_t[yPos_total];
					//Multi2Uni(str_buffer, yPos_total, uni);

					//wprintf(L"%ls", uni);
					//대칭 Tag 찾는 것
					//Parse_Tag3(rbuf, total, "body", str_buffer);
					//memset(rbuf, 0, total);
					//printf("%s", str_buffer);

					//Img 같은 Tag 찾는 것
					if (j == 0)
					{
						str_buffer = new char[yPos_total]();
						img_buffer = new char[yPos_total]();

						memset(str_buffer, 0, yPos_total);
						memset(img_buffer, 0, yPos_total);

						Parse_Tag2(rbuf, yPos_total, "a href=", str_buffer);
						//printf("str_buffer : %s\n", str_buffer);
						Parse_Tag2(rbuf, yPos_total, "img src=", img_buffer);
						//printf("img_buffer : %s\n", img_buffer);

					}

					//printf("%s", str_buffer);

					//memset(rbuf, 0, m_socket.GetPacketNum() + strlen(buf));
					//Get_Parse_Tag2(str_buffer, strlen(str_buffer), 0, rbuf);
					//printf("====");

					j++;
					memset(rbuf, 0, MAXLEN);

					InvalidateRect(hWnd, NULL, true);
					UpdateWindow(hWnd);
					//m_socket.Close();
				}
				else//연결 실패했을 경우 종료
					break;
			} while (Get_Parse_Tag2(img_buffer, strlen(img_buffer), j, rbuf) == 1);
			
			cout << "End Request\n" << endl;
			//Invalidate

			//GDI+ 종료
			GdiplusShutdown(gdiplusToken);

			/*RedrawWindow(hWnd, &rt, NULL, RDW_ERASENOW);
			InvalidateRect(hWnd, NULL, false);
			UpdateWindow(hWnd);*/

			//}str_buffer

			//Scroll 초기화
			GetScrollInfo(hWnd, SB_VERT, &si);
			ScrollWindow(hWnd, 0, si.nPos, NULL, &rt);
			SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
			SetScrollPos(hWnd, SB_VERT, 0, 1);
			InvalidateRect(hWnd, &rt, true);
			//InvalidateRect(&rt);
			UpdateWindow(hWnd);

			//SysLink 생성
			if (yPos_total)
			{
				//다수
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
					CalculateRect(m_rect + i, 0, 50 + (16 * i), client_rect.right, 16);
					//CalculateRect(m_rect + i, 0, 0+(15*i), strlen(rbuf)*8, 15);
					m_link[i] = CreateSysLink(hWnd, m_rect[i], i, rbuf);
					memset(rbuf, 0, MAXLEN);
				}

				InvalidateRect(hWnd, &rt, 1);
				UpdateWindow(hWnd);

				//InvalidateRect(hWnd, &rt, 0);
				//UpdateWindow(hWnd);

				//하나
				/*memset(rbuf, 0, yPos_total);
				tag_temp = Get_Parse_Tag2(str_buffer,  yPos_total, 0, rbuf);
				CalculateRect(m_rect, 0, 0, client_rect.right, 15);
				CreateSysLink(hWnd, m_rect[0], 0, rbuf, strlen(str_buffer));
				*/
			}

			break;

			//EDIT
		case ID_EDIT1:
			switch (HIWORD(wParam)) {
			case EN_CHANGE:
				GetWindowTextA(hEdit, str, 128);
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

			//Hyperlink 생성
			memset(rbuf, 0, yPos_total);

			for (int i = 0; Get_Parse_Tag2(str_buffer, yPos_total, i, rbuf) == 1; i++)
			{
				//ID 검사
				if (((LPNMHDR)IParam)->idFrom == (UINT)i)
				{
					//ShellExecute(NULL, L"open", L"Iexplore.exe", item.szUrl, NULL, SW_SHOW);
					SetWindowText(hEdit, item.szUrl);
				}
				DestroyWindow(m_link[i]);
			}

			InvalidateRect(hWnd, &rt, 0);
			//RECT 정보 지우기
			delete[] m_rect;
			m_rect = NULL;
			//HWND 정보 지우기	
			delete[] m_link;
			m_link = NULL;
			SendMessage(hWnd, WM_COMMAND, ID_BUTTON1, NULL);
			break;
		}
		}

		break;

		/*case WM_LBUTTONDOWN:
		POINT pt;
		GetCursorPos(&pt);
		mX = pt.x;
		mY = pt.y;

		if (PtInRegion(CreateRectRgnIndirect(&rt) , mX, mY) != 0)
		{
		MessageBox(hWnd, L"You have clicked INSIDE hRgn", L"Notify", MB_OK);
		}
		else
		MessageBox(hWnd, L"You have clicked OUTSIDE hRgn", L"Notify", MB_OK);
		return 0;
		*/

	case WM_PAINT:


		//DrawText(hdc, uni + (text_yPos), -1, &rt, DT_LEFT);

		//m_image.Printimage();

		hdc = BeginPaint(hWnd, &ps);

		//받은 패킷이 존재하면
		if (yPos_total)
		{
			//버퍼에 있는 이미지 그리기
			//GET하는 것을 함수로바꿔서 여러번 요청 출력
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
			text_yPos = Set_yPos(str_buffer, yPos_total, text_yPos, BACKWARD);
			break;

			// User clicked the END keyboard key.
		case SB_BOTTOM:
			si.nPos = si.nMax;
			text_yPos = Set_yPos(str_buffer, yPos_total, text_yPos, FORWARD);
			break;

			// User clicked the top arrow.
		case SB_LINEUP:
			si.nPos -= 10;
			text_yPos = Set_yPos(str_buffer, yPos_total, text_yPos, BACKWARD);
			break;

			// User clicked the bottom arrow.
		case SB_LINEDOWN:
			si.nPos += 10;
			text_yPos = Set_yPos(str_buffer, yPos_total, text_yPos, FORWARD);
			break;

			// User clicked the scroll bar shaft above the scroll box.
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			text_yPos = Set_yPos(str_buffer, yPos_total, text_yPos, BACKWARD);
			break;

			// User clicked the scroll bar shaft below the scroll box.
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			text_yPos = Set_yPos(str_buffer, yPos_total, text_yPos, FORWARD);
			break;

			// User dragged the scroll box.
		case SB_THUMBTRACK:
			//줄어들 때
			if (si.nTrackPos < si.nPos)
				for (int i = si.nTrackPos; i < si.nPos; i += si.nPage)
					text_yPos = Set_yPos(str_buffer, yPos_total, text_yPos, BACKWARD);
			else
				for (int i = si.nPos; i < si.nTrackPos; i += si.nPage)
					text_yPos = Set_yPos(str_buffer, yPos_total, text_yPos, FORWARD);
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
