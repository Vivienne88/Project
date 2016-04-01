using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.IO;
using System.Security.Principal;
using System.Security.Permissions;
using System.Diagnostics;
using System.Collections;
using System.Text.RegularExpressions;

namespace New_Search_File
{
    //Thread 탐색시 옵션
    public enum ThreadMode { Single = 0, Multi = 1 };

    class Program
    {
        public class global
        {
            //패턴 담는 Vector
            public static List<char> HexaVector = new List<char>();
            //다음 Thread에게 줘야할 Direcotry
            public static string g_dir = null;
            //Thread의 번호
            public static bool g_cookie = true;
            //Thread 순서를 맞추기 위한 Wait Handler
            public static EventWaitHandle latch = new EventWaitHandle(false, EventResetMode.AutoReset);
            public static EventWaitHandle latch2 = new EventWaitHandle(false, EventResetMode.AutoReset);
        }

        //Thread가 수행하는 Callback 함수에 필요한 정보
        public class TaskInfo
        {
            public int Cookie;
            public string dir;
            public string word;
            public ThreadMode mode;
            public TaskInfo(int iCookie, string idir, string iword, ThreadMode imode)
            {
                Cookie = iCookie;
                dir = idir;
                word = iword;
                mode = imode;
            }
        }

        class WorkFunction
        {
            //Thread Pool에 사용될 Hashtable
            public Hashtable Hashcount;
            //동기화 위한 Lock
            private static System.Object lockThis = new System.Object();
            //멤버 변수
            private ManualResetEvent ieventX;
            public static int iCount = 0;
            public static int iMaxCount = 0;

            public WorkFunction(int MaxCount, ManualResetEvent eventX)
            {
                Hashcount = new Hashtable(MaxCount);
                iMaxCount = MaxCount;
                ieventX = eventX;
            }

            //하나의 Directory안에 File들의 Hex Code를 찾는 부분
            public static void Search(string dir, string word)
            {
                try
                {
                    foreach (string f in Directory.GetFiles(dir))
                    {
                        string temp;
                        string filename = Path.GetFileNameWithoutExtension(f);
                        //파일 열 때 필요한 경로
                        if (dir == "C:\\")
                            temp = dir + word;
                        else
                            temp = dir + "\\" + word;
                        
                        //모든 파일에 대해
                        if (word == "")
                        {
                            //HEXA 찾기
                            foreach (var line in File.ReadLines(f))
                            {
                                int offset = 0;
                                int offset2 = 0;
                                int index = 0;

                                foreach (char character in line)
                                {
                                    foreach (char c in global.HexaVector)
                                    {
                                        if (character == c)
                                        {
                                            offset2 = index;
                                            lock(lockThis)
                                            {
                                                File.AppendAllText(word + "Log.txt", dir+"\\"+f+" : "+ c + Environment.NewLine);
                                            }
                                        }
                                        else
                                            offset = index;
                                    }
                                    index++;
                                }
                            }
                        }
                        else if (f == temp || (word == filename)) //특정 파일에 대해
                        {
                            Console.WriteLine("{0}", f);

                            foreach (var line in File.ReadLines(f))
                            {
                                int offset = 0;
                                int offset2 = 0;
                                int index = 0;

                                foreach (char character in line)
                                {
                                    foreach (char c in global.HexaVector)
                                    {
                                        if (character == c)
                                        {
                                            offset2 = index;
                                            File.AppendAllText(word + "Log.txt", dir + "\\" + f + " : " + c + Environment.NewLine);
                                        }
                                        else
                                            offset = index;
                                    }
                                    index++;
                                }
                            }
                        }
                    }
                }
                catch (UnauthorizedAccessException ex){ }
                catch (Exception ex) { }
                //관리자 접근 제한 및 System Path 270글자로 제한된 운영체제에서 오류방지
            }

            //하나의 Directory를 탐색하는 경우
            public static void Dir_Search(string dir, string word, ThreadMode mode)
            {
                try
                {
                    foreach (string d in Directory.GetDirectories(dir))
                    {
                        string temp;
                        if (dir == "C:\\")
                            temp = dir + word;
                        else
                            temp = dir + "\\" + word;

                        //멀티스레드일 경우
                        if (mode == ThreadMode.Multi)
                        {
                            if (d == temp)
                            {
                                Console.WriteLine("{0}", d);
                            }
                            
                            //다음 Thread에게 dir을 넘겨주고
                            global.g_dir = d;

                            //다른 쓰레드 시작하길 기다린다.
                            global.latch2.Set();

                            EventWaitHandle.WaitAny(new WaitHandle[] { global.latch });
                            
                            Dir_Search(d, word, mode);
                        }
                        else if(mode == ThreadMode.Single)
                        {
                            //입력한 단어와 동일할 경우
                            if (d == temp)
                            {
                                Console.WriteLine("{0}", d);
                                File.AppendAllText(word + ".txt", d + Environment.NewLine);
                            }

                            //다음 디렉토리 및 파일 재귀적으로 탐색
                            Dir_Search(d, word, mode);
                            Search(d, word);
                        }
                    }
                }
                catch (UnauthorizedAccessException ex){ }
                catch (Exception ex){ }
            }

            public void Work(object state)
            {
                lock (ieventX)
                {
                    if (!Hashcount.ContainsKey(Thread.CurrentThread.GetHashCode()))
                        Hashcount.Add(Thread.CurrentThread.GetHashCode(), 0);
                    Hashcount[Thread.CurrentThread.GetHashCode()] = ((int)Hashcount[Thread.CurrentThread.GetHashCode()]) + 1;

                    //Thread 0번 일 경우 Directory 찾는 일을 할당
                    if (((TaskInfo)state).Cookie == 0)
                        Dir_Search(((TaskInfo)state).dir, ((TaskInfo)state).word, ((TaskInfo)state).mode);
                    else //나머지 경우 각 파일에 대해 Hex Code 찾는 일을 할당
                        Search(((TaskInfo)state).dir, ((TaskInfo)state).word);

                    //할당된 일이 끝났음을 알림
                    ieventX.Set();
                }

                //Directory 찾는 스레드가 종료되었을 경우
                if (((TaskInfo)state).Cookie == 0)
                {
                    Console.WriteLine("Directory Search Complete!");
                    global.g_cookie = false;
                    global.latch.Set();
                    global.latch2.Set();
                }
            }
        }

        static void Main(string[] args)
        {
            //변수 선언 및 입력
            int thread_num;
            String pattern = @"\s+|[,]+|0x";

            string HexaInput;
            Console.WriteLine("헥사 코드를 입력하세요.");
            HexaInput = Console.ReadLine();

            Console.WriteLine("쓰레드의 갯수를 입력하세요.");
            thread_num = Convert.ToInt32(Console.ReadLine());
            string[] str = Directory.GetDirectories("C:\\");
            
            string input;
            Console.WriteLine("파일명을 입력하세요.");
            input = Console.ReadLine();
            
            //파일일 경우
            if (input == "FILE" || input == "file" || input == "File")
            {
                if (File.Exists(HexaInput))
                {
                    string contents = File.ReadAllText(HexaInput);
                    input = null;
                    String[] elements = Regex.Split(contents, pattern);
                    foreach (var element in elements)
                        if (element != "")
                        {
                            int value = Convert.ToInt32(element, 16);
                            global.HexaVector.Add((char)value);
                        }
                }
                else
                {
                    Console.WriteLine ("파일이 존재하지 않아서 헥사코드 없이 진행합니다.");
                    HexaInput = null;
                }
            }
            else//입력 받은 경우
            {
                //정규표현식으로 나눈다.
                //사용자가 입력을 Space 한번으로 안준다는 가정 때문에 넣었습니다.
                //한번의 Space로 정규화된 입력이면 String.Split(" ");을 사용했을 것입니다.
                String[] elements = Regex.Split(HexaInput, pattern);
                foreach (var element in elements)
                    if (element != "")
                    {
                        int value = Convert.ToInt32(element, 16);
                        global.HexaVector.Add((char)value);
                    }
            }

            //속도 측정 시작
            Stopwatch sw = new Stopwatch();
            sw.Start();

            //Thread가 하나일 경우 예외처리
            if (thread_num == 1)
            {
                WorkFunction.Dir_Search("C:\\", input, ThreadMode.Single);
                Console.WriteLine("All calculations are complete.");
                sw.Stop();
                Console.WriteLine(sw.ElapsedMilliseconds.ToString() + "ms");
                Console.WriteLine("Press any key to quit");
                Console.ReadKey();
                return;
            }

            string[] dir = Directory.GetDirectories("C:\\");

            TaskInfo[] Ti = new TaskInfo[thread_num];

            //파일 존재할 경우 삭제
            if (File.Exists(input + ".txt"))
            {
                File.Delete(input + ".txt");
                File.Delete(input + "Log.txt");
            }

            //Event 끝났는지 유무 확인
            ManualResetEvent[] eventX = new ManualResetEvent[thread_num];

            var tokenSource = new CancellationTokenSource();
            var token = tokenSource.Token;
            bool start_flag = true;
            int temp = 0;

            //Pool에서 반복적으로 일이 끝났는지 확인하고
            //끝났을 경우 새로운 매개변수를 줘서 탐색을 한다.
            do
            {
                for (int i = 0 + temp; i < thread_num; i++)
                {
                    if (global.g_cookie == true)
                    {
                        if (start_flag || eventX[i].WaitOne(0))
                        {
                            //초기화
                            eventX[i] = new ManualResetEvent(false);
                            WorkFunction thread = new WorkFunction(thread_num, eventX[i]);

                            //처음의 경우 예외처리, Directory주는 Thread 실행을 위함
                            if (i == 0 && start_flag)
                            {
                                Ti[i] = new TaskInfo(i, "C:\\", input, ThreadMode.Multi);
                                ThreadPool.QueueUserWorkItem(new WaitCallback(thread.Work), Ti[i]);
                            }
                            else//File 읽는 Thread 실행
                            {
                                global.g_dir = null;

                                //g_dir 가져올 때 까지 대기
                                global.latch.Set();
                                
                                //Directory 가져올 때 까지 대기
                                EventWaitHandle.WaitAny(new WaitHandle[] { global.latch2 });

                                //Directory 가져왔을 경우, 각 파일에 대해 Hex Code 찾기 수행
                                if (global.g_dir != null)
                                {
                                    Ti[i] = new TaskInfo(i, global.g_dir, input, ThreadMode.Multi);
                                    ThreadPool.QueueUserWorkItem(new WaitCallback(thread.Work), Ti[i]);
                                }
                                else
                                    eventX[i].Set();
                            }
                        }

                    }

                    if (start_flag && i == thread_num - 1)
                    {
                        start_flag = false;
                        temp = 1;
                    }
                }
            } while (Convert.ToBoolean(WaitHandle.WaitAny(new[] { eventX[0], token.WaitHandle }, 0)));
            //File Thread가 멈출 때 까지

            //시간 측정 완료
            Console.WriteLine("All calculations are complete.");
            sw.Stop();
            Console.WriteLine(sw.ElapsedMilliseconds.ToString() + "ms");

            //Release 버전에서 강제 종료됨을 방지
            Console.WriteLine("Press any key to quit");
            Console.ReadKey();
        }
    }
}
