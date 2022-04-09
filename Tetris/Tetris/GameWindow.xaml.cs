using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace Tetris
{
    /// <summary>
    /// Interaction logic for GameWindow.xaml
    /// </summary>
    /// 

    enum GameState
    {
        stateMenu = 0,    // меню
        stateInGame = 1,  // игра
        stateAborted = 2  // игра прервана (не нуждается в вызове Update()
    }

    public partial class GameWindow : Window, IBoxNotify
    {
        private Timer updateTimer = null;
        GameState gameState = GameState.stateMenu;
        private int totalShapes, solidRows, totalScore;

        public GameWindow()
        {
            InitializeComponent();

            // Устанавливаем размеры холста:
            boxCanvas.Width = (((App)(App.Current)).boxWidth + 2) * ((App)(App.Current)).brickSize;
            boxCanvas.Height = (((App)(App.Current)).boxHeight + 1) * ((App)(App.Current)).brickSize;

            // Устанавлваем таймер для обновления:
            updateTimer = new Timer(GameUpdate);
            updateTimer.Change(0, 1000 / ((App)(App.Current)).FPS);

            // Связываем окна стакана и просмотра:
            boxCanvas.SetPreviewWindow(shapeCanvas);

            // Устанавливаем клавиши управления в интерфейс:
            foreach (GameCommandState cs in ((App)(App.Current)).commandGenerator.GetCommands())
            {
                switch (cs.Command)
                {
                    case GameCommands.MoveLeft:
                        {
                            lblMoveLeftKey.Content = cs.commandKey.ToString().ToUpper();
                            break;
                        }
                    case GameCommands.MoveRight:
                        {
                            lblMoveRightKey.Content = cs.commandKey.ToString().ToUpper();
                            break;
                        }
                    case GameCommands.Drop:
                        {
                            lblDropKey.Content = cs.commandKey.ToString().ToUpper();
                            break;
                        }
                    case GameCommands.RotateCW:
                        {
                            lblRotateCWKey.Content = cs.commandKey.ToString().ToUpper();
                            break;
                        }
                    case GameCommands.RotateCCW:
                        {
                            lblRotateCCWKey.Content = cs.commandKey.ToString().ToUpper();
                            break;
                        }
                    case GameCommands.Pause:
                        {
                            lblPauseKey.Content = cs.commandKey.ToString().ToUpper();
                            break;
                        }
                    case GameCommands.Abort:
                        {
                            lblAbortKey.Content = cs.commandKey.ToString().ToUpper();
                            break;
                        }
                }
            }

            // Настраиваем получение уведомлений:
            boxCanvas.SetNotifyTarget(this);
            
            // Ставим имя владельца:
            lblOwner.Content = $"Владелец копии: {((App)(App.Current)).ownerName}";

            if (TAudioEngine.Current.IsInitialized != true)
            {
                // Звуковой движок не удалось запустить - гасим конпку звукового режима.
                chkNoSound.IsChecked = true;
                chkNoSound.IsEnabled = false;

            }

            TAudioEngine.Current.PlayMenuTrack();
            
        }

        // Очистка данных перед игрой.
        private void InitGame()
        {
            boxCanvas.ClearBox();

            lblTotalScore.Content = 0;
            lblTotalShapes.Content = 0;
            lblSolidRows.Content = 0;

            totalScore = 0;
            totalShapes = 0;
            solidRows = 0;
        }
        private void GameUpdate(object state)
        {
            if (gameState == GameState.stateInGame)
            {
                this.Dispatcher.Invoke(() =>
               {
                   boxCanvas.Update();
               });
            }
        }

        protected override void OnClosed(EventArgs e)
        {
            base.OnClosed(e);
            if (updateTimer != null)
                updateTimer.Dispose();
        }


        // НАЧАЛО ИГРЫ:
        private void OnStartGame(object sender, RoutedEventArgs e)
        {
            TAudioEngine.Current.PlayGameTrack();
            TAudioEngine.Current.PlayPushButton();

            InitGame();
            ChangeLayout(GameState.stateInGame);
            gameState = GameState.stateInGame;
        }

        // ТАБЛИЦА РЕКОРДОВ:
        private void OnShowScores(object sender, RoutedEventArgs e)
        {
            TAudioEngine.Current.PlayPushButton();

            ScoreWindow sw = new ScoreWindow();

            sw.Owner = this;
            sw.ShowDialog();
        }

        // ВЫХОД ИЗ ИГРЫ:
        private void OnQuit(object sender, RoutedEventArgs e)
        {
            TAudioEngine.Current.PlayPushButton();

            MessageBoxResult r = MessageBox.Show("Выйти в Windows?",
                "Выход", MessageBoxButton.YesNo, MessageBoxImage.Question);

            if (r == MessageBoxResult.Yes)
                Close();
        }

        // ЗВУКОВОЙ РЕЖИМ:
        private void OnSoundMode(object sender, RoutedEventArgs e)
        {
            TAudioEngine.Current.SetMuteMode(chkNoSound.IsChecked ?? true);
        }

        // Смена раскладки.
        private void ChangeLayout(GameState s)
        {
            switch (s)
            {
                case GameState.stateInGame: // включить игровой слой
                    {
                        layerMenu.Visibility = Visibility.Collapsed;
                        layerGame.Visibility = Visibility.Visible;
                        lblPause.Visibility = Visibility.Collapsed;

                        break;
                    }
                case GameState.stateMenu: // включить слой меню
                    {
                        layerMenu.Visibility = Visibility.Visible;
                        layerGame.Visibility = Visibility.Collapsed;
                        lblPause.Visibility = Visibility.Collapsed;

                        break;
                    }
            }

            CenterWindow();
        }
        private void CenterWindow()
        {
            InvalidateArrange();
            UpdateLayout();
            this.Left = (System.Windows.SystemParameters.PrimaryScreenWidth - this.ActualWidth) / 2;
            this.Top = (System.Windows.SystemParameters.PrimaryScreenHeight - this.ActualHeight) / 2;
        }

        // Приём уведомлений от TBox : ----------------------------------------------------------------------

        int IBoxNotify.OnShapeMove(float p)
        {
            TAudioEngine.Current.PlayMoveShape(p);
            return 0;
        }

        int IBoxNotify.OnShapeRotate(float p)
        {
            TAudioEngine.Current.PlayRotateShape(p);
            return 0;
        }
        int IBoxNotify.OnShapeDrop(float p)
        {
            TAudioEngine.Current.PlayDropShape(p);
            return 0;
        }

        int IBoxNotify.OnHitShape(float p)
        {
            TAudioEngine.Current.PlayHitShape(p);
            return 0;
        }

        int IBoxNotify.OnExplodeLines()
        {
            TAudioEngine.Current.PlayLineExplode();

            return 0;
        }

        int IBoxNotify.OnAttachShape(TShape s)
        {
            // Упала новая фигура, увеличиваем счетчик и добавляем очки (+5 очков помноженные на размер).
            totalShapes++;
            totalScore += (s.Size * 5);
            lblTotalShapes.Content = totalShapes.ToString();
            lblTotalScore.Content = totalScore.ToString();

            return 0;
        }

        int IBoxNotify.OnDeleteRows(int count)
        {
            // Увеличиваем количество полных строк и добавляем очки (+50 очков помноженные на количество строк
            // в квадрате; таким образом чем больше строк набрано за раз, тем больше прибавка)

            solidRows += count;
            totalScore += (count * count * 50);
            lblSolidRows.Content = solidRows.ToString();
            lblTotalScore.Content = totalScore.ToString();

            return 0;
        }

        int IBoxNotify.OnPause()
        {
            // Показываем значок паузы.
            lblPause.Visibility = Visibility.Visible;

            TAudioEngine.Current.PlayPause(true);

            return 0;
        }

        int IBoxNotify.OnResume()
        {
            // Скрываем значок паузы.
            lblPause.Visibility = Visibility.Collapsed;

            TAudioEngine.Current.PlayPause(false);

            return 0;
        }

        int IBoxNotify.OnAbort()
        {
            // Приостанавливаем обновление и спрашиваем:
            gameState = GameState.stateAborted;

            MessageBoxResult r = MessageBox.Show("Выйти в главное меню (статистика не будет сохранена)?",
                "Прервать игру", MessageBoxButton.YesNo, MessageBoxImage.Warning);

            if (r == MessageBoxResult.Yes)
            {
                // Выходим из игры в меню.
                gameState = GameState.stateMenu;
                ChangeLayout(GameState.stateMenu);

                TAudioEngine.Current.Silence();

                return 0;
            }

            // Возвращаемся:
            gameState = GameState.stateInGame;

            return 0;
        }

        int IBoxNotify.OnGameOver()
        {
            string appPath = System.IO.Path.GetDirectoryName(System.Environment.GetCommandLineArgs()[0]);

            // Показываем общий счет, сохраняем статистику в файле и выходим в главное меню.

            TAudioEngine.Current.Silence();

            MessageBox.Show(string.Format("Игра окончена.\nОбщий счет: {0}", totalScore), "Игра окончена", 
                MessageBoxButton.OK, MessageBoxImage.Information);

            ((App)(App.Current)).recordTable.AddRecord(new TRecordItem()
            { PlayerName = System.Environment.UserName, SolidRows = solidRows, TotalShapes = totalShapes, TotalScore = totalScore}, 10);

            ((App)(App.Current)).recordTable.SaveToFile(appPath + "\\scores.dat");

            // Выходим в главное меню:
            gameState = GameState.stateMenu;
            ChangeLayout(GameState.stateMenu);

            return 0;
        }
    }
}
