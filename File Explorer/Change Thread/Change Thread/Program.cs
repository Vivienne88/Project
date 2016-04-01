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

namespace Change_Thread
{
    //쓰레드 함수 옵션
    public enum ThreadMode { Single = 0, Multi = 1 };

    class Program
    {
        public class global
        {
            //public static bool AutoReset = true;
            public static List<char> HexaVector = new List<char>();
            public static string g_dir = null;
            public static bool g_cookie = true;
            public static EventWaitHandle latch = new EventWaitHandle(false, EventResetMode.AutoReset);
            public static EventWaitHandle latch2 = new EventWaitHandle(false, EventResetMode.AutoReset);
        }

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
            //해쉬 위한 변수 추가
            public Hashtable Hashcount;
            private static System.Object lockThis = new System.Object();
            private ManualResetEvent ieventX;
            public static int iCount = 0;
            public static int iMaxCount = 0;

            public WorkFunction(int MaxCount, ManualResetEvent eventX)
            {
                Hashcount = new Hashtable(MaxCount);
                iMaxCount = MaxCount;
                ieventX = eventX;
            }

            public static void Search(string f, string word)
            {
                try
                {
                    //HEXA 찾기
                    int offset = 0;
                    foreach (var line in File.ReadLines(f))
                    {
                        foreach (char character in line)
                        {
                            foreach (char c in global.HexaVector)
                            {
                                if (character == c)
                                {
                                    lock (lockThis)
                                    {                                        
                                        //offset 줘야함.
                                        File.AppendAllText(word + "Log.txt", f + " : Found " + c + " in " + offset + " Position " + Environment.NewLine);
                                    }
                                }
                                offset++;
                            }
                        }
                    }
                }
                catch (UnauthorizedAccessException ex) { }
                catch (Exception) { }
            }

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

                        if (d == temp)
                        {
                            Console.WriteLine("{0}", d);
                            File.AppendAllText(word + ".txt", d + Environment.NewLine);
                        }
                        //Console.WriteLine("{0}", d);

                        Dir_Search(d, word, mode);

                        try
                        {
                            foreach (string f in Directory.GetFiles(d))
                            {
                                string temp2;
                                string filename = Path.GetFileNameWithoutExtension(f);

                                if (d == "C:\\")
                                    temp2 = d + word;
                                else
                                    temp2 = d + "\\" + word;

                                //멀티 스레드 일 경우
                                if (Convert.ToInt32(mode) == 1)
                                {
                                    //file 정답일 경우
                                    if (word == "" || f == temp2 || word == filename)
                                    {

                                        //다음 Thread에게 dir을 넘겨주고
                                        global.g_dir = f;

                                        //다른 쓰레드 시작하길 기다린다.
                                        global.latch2.Set();

                                        EventWaitHandle.WaitAny(new WaitHandle[] { global.latch });

                                        Console.WriteLine("{0}", f);
                                        File.AppendAllText(word + "2.txt", f + Environment.NewLine);
                                    }
                                }
                                else
                                {   //file 정답일 경우
                                    if (f == temp2 || word == filename)
                                    {
                                        Console.WriteLine("{0}", f);
                                        Search(f, word);
                                    }
                                }
                                //Console.WriteLine("{0}", f);

                            }
                        }
                        catch (UnauthorizedAccessException) { }
                        catch (Exception) { }
                    }
                }
                catch (UnauthorizedAccessException) { }
                catch (Exception) { }
            }


            public void Work(object state)
            {

                // Console.WriteLine(" {0} {1} : ", Thread.CurrentThread.GetHashCode(), ((TaskInfo)state).Cookie);

                //Console.WriteLine("HashCount.Count=={0}, Thread.CurrentThread.GetHashCode()=={1}", Hashcount.Count, Thread.CurrentThread.GetHashCode());

                //0번일 경우 다른 함수

                lock (ieventX)
                {
                    if (!Hashcount.ContainsKey(Thread.CurrentThread.GetHashCode()))
                        Hashcount.Add(Thread.CurrentThread.GetHashCode(), 0);
                    Hashcount[Thread.CurrentThread.GetHashCode()] = ((int)Hashcount[Thread.CurrentThread.GetHashCode()]) + 1;

                    //Cookie가 아니고 Thread Number
                    if (((TaskInfo)state).Cookie == 0)
                        Dir_Search(((TaskInfo)state).dir, ((TaskInfo)state).word, ((TaskInfo)state).mode);
                    else
                        Search(((TaskInfo)state).dir, ((TaskInfo)state).word);
                    ieventX.Set();
                }

                if (((TaskInfo)state).Cookie == 0)
                {
                    Console.WriteLine("Directory Search Complete!");
                    global.g_cookie = false;
                    global.latch.Set();

                    //진행중인 쓰레드들 강제 종료
                    global.latch2.Set();
                }
            }
        }

        static void Main(string[] args)
        {
            int thread_num;

            //String pattern = @"\s-\s?[+*]?\s?-\s";
            //String pattern = "[,]+|\s+";
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
                    Console.WriteLine("파일이 존재하지 않아서 헥사코드 없이 진행합니다.");
                    HexaInput = null;
                }
            }
            else
            {
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
            /*
            else if (thread_num < 5)
            {
                global.AutoReset = false;
            }
            */

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

            do
            {
                for (int i = 0 + temp; i < thread_num; i++)
                {
                    if (global.g_cookie == true)
                    {
                        if (start_flag || eventX[i].WaitOne(0))
                        {
                            eventX[i] = new ManualResetEvent(false);
                            WorkFunction thread = new WorkFunction(thread_num, eventX[i]);

                            if (i == 0 && start_flag)
                            {
                                Ti[i] = new TaskInfo(i, "C:\\", input, ThreadMode.Multi);
                                ThreadPool.QueueUserWorkItem(new WaitCallback(thread.Work), Ti[i]);
                            }
                            else
                            {
                                global.g_dir = null;

                                //g_dir 가져올 때 까지 대기
                                global.latch.Set();

                                /*
                                if (!global.AutoReset)
                                    global.latch2 = new EventWaitHandle(false, EventResetMode.ManualReset);
                                */

                                EventWaitHandle.WaitAny(new WaitHandle[] { global.latch2 });

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
            //(!eventX[0].WaitOne(0));


            Console.WriteLine("All calculations are complete.");
            sw.Stop();
            Console.WriteLine(sw.ElapsedMilliseconds.ToString() + "ms");
            Console.WriteLine("Press any key to quit");
            Console.ReadKey();
        }
    }
}
