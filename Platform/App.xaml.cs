﻿using Platform.Models;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.Storage.Streams;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using WinRTExceptions;

namespace Platform
{
    /// <summary>
    /// 提供特定于应用程序的行为，以补充默认的应用程序类。
    /// </summary>
    sealed partial class App : Application
    {
        /// <summary>
        /// 公共访问实例对象
        /// </summary>
        static public App Client;

        /// <summary>
        /// 本地资源记录
        /// </summary>
        public struct Record
        {
            public string Name;
            public string Password;
        }
        public List<Record> LocalRecords;
        public Windows.Media.Playback.MediaPlayer BackgroundMusic = new Windows.Media.Playback.MediaPlayer();

        /// <summary>
        /// 观察集
        /// </summary>
        public ObservableCollection<PokemenViewer>    OnlinePokemens;
        public ObservableCollection<OnlineUserViewer> OnlineUsers;
        public ObservableCollection<UserInfoViewer>   RankedUsers;
        public ObservableCollection<PokemenViewer>    RankedPokemens;

        /// <summary>
        /// 通知RankPage显示对应用户的小精灵
        /// </summary>
        public Semaphore IsRankedPokemensReady;

        /// <summary>
        /// 连接控制
        /// </summary>
        public bool IsOnConnection;
        public Task NetDriver;

        /// <summary>
        /// 对战控制
        /// </summary>
        public bool IsOnBattle;
        public Task BattleDriver;

        /// <summary>
        /// 后台内核实例对象
        /// </summary>
        public Kernel.Core Core;

        /// <summary>
        /// 初始化单一实例应用程序对象。这是执行的创作代码的第一行，
        /// 已执行，逻辑上等同于 main() 或 WinMain()。
        /// </summary>
        public App()
        {
            this.InitializeComponent();
            this.Suspending += OnSuspending;
            Client = this;
            InitializeLocalEnvironments();
            LoadLocalRecords();

            UnhandledException += App_UnhandledException;
        }

        private async void OnUnhandledException(object sender, Windows.UI.Xaml.UnhandledExceptionEventArgs e)
        {
            e.Handled = true;
            await new Windows.UI.Popups.MessageDialog("Application Unhandled Exception:\r\n" + e.Exception.Message, "爆了 :(")
                .ShowAsync();
        }

        /// <summary>
        /// Should be called from OnActivated and OnLaunched
        /// </summary>
        private void RegisterExceptionHandlingSynchronizationContext()
        {
            ExceptionHandlingSynchronizationContext
                .Register()
                .UnhandledException += SynchronizationContext_UnhandledException;
        }

        private async void SynchronizationContext_UnhandledException(object sender, WinRTExceptions.UnhandledExceptionEventArgs e)
        {
            e.Handled = true;

            await new Windows.UI.Popups.MessageDialog("Synchronization Context Unhandled Exception:\r\n" + e.Exception.Message)
                .ShowAsync();
        }

        private async void App_UnhandledException(object sender, Windows.UI.Xaml.UnhandledExceptionEventArgs e)
        {
            e.Handled = true;

            await new Windows.UI.Popups.MessageDialog("Application Unhandled Exception:\r\n" + e.Exception.Message)
                .ShowAsync();
        }
        

        /// <summary>
        /// 初始化本地环境
        /// </summary>
        public void InitializeLocalEnvironments()
        {
            LocalRecords = new List<Record>();

            OnlinePokemens = new ObservableCollection<Models.PokemenViewer>();
            OnlineUsers = new ObservableCollection<Models.OnlineUserViewer>();

            Core = new Kernel.Core();
        }

        public const string FilenameOfLocalRecords = "records.dat";
        /// <summary>
        /// 加载本地资源
        /// </summary>
        async public void LoadLocalRecords()
        {
            try
            {
                IBuffer localBuffer
                    = await FileIO.ReadBufferAsync(
                        await ApplicationData.Current.LocalFolder.GetFileAsync(
                            FilenameOfLocalRecords
                            )
                        );
                using (DataReader data = DataReader.FromBuffer(localBuffer))
                {
                    string records =
                        data.ReadString(localBuffer.Length);
                    if (!string.IsNullOrEmpty(records))
                    {
                        string[] nameAndPasswords
                            = records.Split(' ');
                        foreach (var item in nameAndPasswords)
                        {
                            if (!string.IsNullOrEmpty(item))
                            {
                                string[] vs = item.ToString().Split(',');
                                if (!string.IsNullOrEmpty(vs[0])
                                    && !string.IsNullOrEmpty(vs[1]))
                                {
                                    LocalRecords.Add(
                                        new Record { Name = vs[0], Password = vs[1] }
                                        );
                                }
                            }
                        }
                    }
                    /* DEBUG */
                    foreach (var item in LocalRecords)
                    {
                        Debug.WriteLine(item.Name + ' ' + item.Password);
                    }
                }
            }
            catch (Exception)
            {
                await ApplicationData.Current.LocalFolder.CreateFileAsync(
                    FilenameOfLocalRecords,
                    CreationCollisionOption.ReplaceExisting
                    );
            }
        }

        /// <summary>
        /// 在应用程序由最终用户正常启动时进行调用。
        /// 将在启动应用程序以打开特定文件等情况下使用。
        /// </summary>
        /// <param name="e">有关启动请求和过程的详细信息。</param>
        protected override void OnLaunched(LaunchActivatedEventArgs e)
        {
            RegisterExceptionHandlingSynchronizationContext();
            Frame rootFrame = Window.Current.Content as Frame;

            // 不要在窗口已包含内容时重复应用程序初始化，
            // 只需确保窗口处于活动状态
            if (rootFrame == null)
            {
                // 创建要充当导航上下文的框架，并导航到第一页
                rootFrame = new Frame();

                rootFrame.NavigationFailed += OnNavigationFailed;

                if (e.PreviousExecutionState == ApplicationExecutionState.Terminated)
                {
                    //TODO: 从之前挂起的应用程序加载状态
                }

                // 将框架放在当前窗口中
                Window.Current.Content = rootFrame;
            }

            if (e.PrelaunchActivated == false)
            {
                if (rootFrame.Content == null)
                {
                    // 当导航堆栈尚未还原时，导航到第一页，
                    // 并通过将所需信息作为导航参数传入来配置
                    // 参数
                    rootFrame.Navigate(typeof(MainPage), e.Arguments);
                }
                // 确保当前窗口处于活动状态
                Window.Current.Activate();
            }
        }

        protected override void OnActivated(IActivatedEventArgs args)
        {
            RegisterExceptionHandlingSynchronizationContext();

            base.OnActivated(args);
        }

        /// <summary>
        /// 导航到特定页失败时调用
        /// </summary>
        ///<param name="sender">导航失败的框架</param>
        ///<param name="e">有关导航失败的详细信息</param>
        void OnNavigationFailed(object sender, NavigationFailedEventArgs e)
        {
            throw new Exception("Failed to load Page " + e.SourcePageType.FullName);
        }

        /// <summary>
        /// 在将要挂起应用程序执行时调用。  在不知道应用程序
        /// 无需知道应用程序会被终止还是会恢复，
        /// 并让内存内容保持不变。
        /// </summary>
        /// <param name="sender">挂起的请求的源。</param>
        /// <param name="e">有关挂起请求的详细信息。</param>
        private void OnSuspending(object sender, SuspendingEventArgs e)
        {
            var deferral = e.SuspendingOperation.GetDeferral();
            //TODO: 保存应用程序状态并停止任何后台活动
            deferral.Complete();
        }
    }
}
