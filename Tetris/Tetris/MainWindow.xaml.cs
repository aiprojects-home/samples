using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml.Linq;
using System.Threading;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.ComponentModel;
using System.Globalization;

namespace Tetris
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private Thread LoadingThread = null;    // поток для загрузки данных
        private bool IsLoading = true;          // флаг загрузки

        public MainWindow()
        {
            InitializeComponent();
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);

            // После загрузки стартового окна запускаем поток загрузки данных:
            LoadingThread = new Thread(LoadGameData);
            LoadingThread.Start();
        }

        protected void LoadGameData()
        {
            // Загрузка данных:
            bool loadFailed = false;

            string appPath = System.IO.Path.GetDirectoryName(System.Environment.GetCommandLineArgs()[0]);

            try
            {
                PrintLogMessage("Загрузка игровых данных:");

                XDocument xmlGameData = XDocument.Load(appPath + "\\gamedata.xml");

                // Загрузка цветов: -------------------------------------------------------------------------
                PrintLogMessage("Загрузка таблицы цветов...");

                IEnumerable<XElement> Colors;

                try
                {
                    Colors = xmlGameData.Element("GameData").Element("Colors").Elements("Color");
                }
                catch 
                {
                    throw new Exception("Отсутствует секция <GameData><Colors>");
                }

                // Находим все элементы описывающие цвета:
                foreach (XElement e in Colors)
                {
                    XAttribute colorName = e.Attribute("Name");
                    XAttribute colorR = e.Attribute("R");
                    XAttribute colorG = e.Attribute("G");
                    XAttribute colorB = e.Attribute("B");
                    XAttribute colorA = e.Attribute("A");

                    if (colorName == null)
                        throw new Exception("Пропущен атрибут Name");
                    if (colorR == null)
                        throw new Exception("Пропущен атрибут R");
                    if (colorG == null)
                        throw new Exception("Пропущен атрибут G");
                    if (colorB == null)
                        throw new Exception("Пропущен атрибут B");
                    if (colorA == null)
                        throw new Exception("Пропущен атрибут A");

                    string ColorName = colorName.Value;
                    byte colorRed = byte.Parse(colorR.Value);
                    byte colorGreen = byte.Parse(colorG.Value);
                    byte colorBlue = byte.Parse(colorB.Value);
                    byte colorAlpha = byte.Parse(colorA.Value);

                    PrintLogMessage(string.Format("\tЦвет: {0}, {1:X2}{2:X2}{3:X2}{4:X2}",
                        ColorName, (int)colorR, (int)colorG, (int)colorB, (int)colorA));

                    // Добавляем в коллекцию:
                    ((App)App.Current).colorList.Add(Color.FromArgb(colorAlpha, colorRed, colorGreen, colorBlue));
                }

                // Чтение констант из общих настроек: -------------------------------------------------------
                PrintLogMessage("Чтение констант...");

                IEnumerable<XElement> Settings;
                try
                {
                    Settings = xmlGameData.Element("GameData").Element("General").Elements("Settings");
                }
                catch
                {
                    throw new Exception("Отсутствует секция <GameData><General>");
                }

                foreach (XElement e in Settings)
                {
                    XAttribute attrKey = e.Attribute("Key");
                    XAttribute attrValue = e.Attribute("Value");

                    if (attrKey == null)
                        throw new Exception("Пропущен атрибут Key");
                    if (attrValue == null)
                        throw new Exception("Пропущен атрибут Value");


                    switch (attrKey.Value.ToString())
                    {
                        case "Owner":
                            {
                                ((App)(App.Current)).ownerName = attrValue.Value.ToString();

                                PrintLogMessage(string.Format("\tOwner = {0}", ((App)(App.Current)).ownerName));
                                break;
                            }
                        case "FPS":
                            {
                                ((App)(App.Current)).FPS = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tFPS = {0}", ((App)(App.Current)).FPS));
                                break;
                            }
                        case "BoxWidth":
                            {
                                ((App)(App.Current)).boxWidth = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tBoxWidth = {0}", ((App)(App.Current)).boxWidth));
                                break;
                            }
                        case "BoxHeight":
                            {
                                ((App)(App.Current)).boxHeight = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tBoxHeight = {0}", ((App)(App.Current)).boxHeight));
                                break;
                            }
                        case "BrickSize":
                            {
                                ((App)(App.Current)).brickSize = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tBrickSize = {0}x{0}", ((App)(App.Current)).brickSize));
                                break;
                            }
                        case "BorderColor":
                            {
                                ((App)(App.Current)).borderColor = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tBorderColor = {0}", ((App)(App.Current)).borderColor));
                                break;
                            }
                        case "GridColor":
                            {
                                ((App)(App.Current)).gridColor = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tGridColor = {0}", ((App)(App.Current)).gridColor));
                                break;
                            }
                        case "BricksColor":
                            {
                                ((App)(App.Current)).bricksColor = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tBricksColor = {0}", ((App)(App.Current)).bricksColor));
                                break;
                            }
                        case "GlassColor0":
                            {
                                ((App)(App.Current)).glassColor0 = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tGlassColor0 = {0}", ((App)(App.Current)).glassColor0));
                                break;
                            }
                        case "GlassColor1":
                            {
                                ((App)(App.Current)).glassColor1 = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tGlassColor1 = {0}", ((App)(App.Current)).glassColor1));
                                break;
                            }
                        case "PreviewColor0":
                            {
                                ((App)(App.Current)).previewColor0 = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tPreviewColor0 = {0}", ((App)(App.Current)).previewColor0));
                                break;
                            }
                        case "PreviewColor1":
                            {
                                ((App)(App.Current)).previewColor1 = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tPreviewColor1 = {0}", ((App)(App.Current)).previewColor1));
                                break;
                            }
                        default:
                            {
                                break;
                            }

                    }

                }
     
                // Загрузка фигур: --------------------------------------------------------------------------
                PrintLogMessage("Загрузка фигур...");

                IEnumerable<XElement> Shapes;

                try
                {
                    Shapes = xmlGameData.Element("GameData").Element("Shapes").Elements("Shape");
                }
                catch
                {
                    throw new Exception("Отсутствует секция <GameData><Shapes>");
                }

                // Находим все элементы описывающие фигуры:
                foreach (XElement e in Shapes)
                {
                    XAttribute shapeNameAttr = e.Attribute("Name");
                    XAttribute shapeSizeAttr = e.Attribute("Size");
                    XAttribute shapeColorIndexAttr = e.Attribute("ColorIndex");

                    if (shapeNameAttr == null)
                        throw new Exception("Пропущен атрибут Name");
                    if (shapeSizeAttr == null)
                        throw new Exception("Пропущен атрибут Size");
                    if (shapeColorIndexAttr == null)
                        throw new Exception("Пропущен атрибут ColorIndex");

                    string shapeName = shapeNameAttr.Value;
                    int shapeSize = int.Parse(shapeSizeAttr.Value);
                    int colorIndex = int.Parse(shapeColorIndexAttr.Value);

                    if (colorIndex >= ((App)(App.Current)).colorList.Count)
                    {
                        throw new Exception("Значение атрибута ColorIndex выходит за границы диапазона");
                    }

                    PrintLogMessage(string.Format("\tФигура: {0}, размер: {1}x{1}", shapeName, shapeSize));

                    // Создаем новый класс для фигуры:
                    TShape newShape = new TShape(shapeSize, colorIndex);

                    // Теперь для каждой фигуры читаем маску:
                    var shapeData = e.Elements().Where((d) => (d.Name == "ShapeData"));

                    foreach (XElement d in shapeData)
                    {
                        XAttribute dataRow = d.Attribute("Row");

                        if (dataRow == null)
                            throw new Exception("Пропущен атрибут Row");

                        int rowNumber = int.Parse(dataRow.Value);

                        string rowData = d.Value;

                        PrintLogMessage(string.Format("\t\t Строка: {0}, данные: {1}", rowNumber, rowData));

                        // Сохраняем данные матрицы в новом классе:
                        for(int i = 0; i < rowData.Length; i++)
                        {
                            newShape[i, rowNumber] = byte.Parse(rowData[i].ToString());
                        }
                    }

                    ((App)(App.Current)).shapeList.Add(newShape);

                }

                PrintLogMessage(string.Format("Обработано фигур: {0}.", ((App)(App.Current)).shapeList.Count()));

                // Загрузка настроек управления: ------------------------------------------------------------
                PrintLogMessage("Загрузка настроек управления...");

                IEnumerable<XElement> Controls;

                try
                {
                    Controls = xmlGameData.Element("GameData").Element("Controls").Elements("Command");
                }
                catch
                {
                    throw new Exception("Отсутствует секция <GameData><Controls>");
                }

                XAttribute controlsTypeDelay = xmlGameData.Element("GameData").Element("Controls").Attribute("TypeDelay");
                XAttribute controlsTypeSpeed = xmlGameData.Element("GameData").Element("Controls").Attribute("TypeSpeed");

                if (controlsTypeDelay != null)
                {
                    ((App)(App.Current)).commandGenerator.TypeDelay = int.Parse(controlsTypeDelay.Value);
                    PrintLogMessage(string.Format("\tTypeDelay = {0}.", ((App)(App.Current)).commandGenerator.TypeDelay));
                }

                if (controlsTypeSpeed != null)
                {
                    ((App)(App.Current)).commandGenerator.TypeSpeed = int.Parse(controlsTypeSpeed.Value);
                    PrintLogMessage(string.Format("\tTypeSpeed = {0}.", ((App)(App.Current)).commandGenerator.TypeSpeed));
                }

                // Находим все элементы описывающие команды:
                foreach (XElement e in Controls)
                {
                    XAttribute controlsNameAttr = e.Attribute("Name");
                    XAttribute controlsKeyAttr = e.Attribute("Key");

                    if (controlsNameAttr == null)
                        throw new Exception("Пропущен атрибут Name");
                    if (controlsKeyAttr == null)
                        throw new Exception("Пропущен атрибут Key");

                    GameCommands gameCommand;
                    Key gameKey;

                    try
                    {
                        gameCommand = (GameCommands)Enum.Parse(typeof(GameCommands), controlsNameAttr.Value);
                    } catch
                    {
                        throw new Exception("Недопустимое имя команды!");
                    }

                    try
                    {
                        gameKey = (Key)Enum.Parse(typeof(Key), controlsKeyAttr.Value);
                    }
                    catch
                    {
                        throw new Exception("Недопустимое значение клавиши!");
                    }

                    switch (gameCommand)
                    {
                        case GameCommands.MoveLeft: // Эти две команды обрабатываются как TURBO
                        case GameCommands.MoveRight:
                            {
                                ((App)(App.Current)).commandGenerator.AddCommand(gameCommand, gameKey, true);
                                break;
                            }
                        default:
                            {
                                ((App)(App.Current)).commandGenerator.AddCommand(gameCommand, gameKey, false);
                                break;
                            }
                    }
                    PrintLogMessage(string.Format("\tКоманда = {0}, клавиша = {1}", gameCommand, gameKey));
                }

                // Загрузка таблицы рекордов: ---------------------------------------------------------------
                try
                {
                    PrintLogMessage("Загрузка таблицы рекордов...");
                    ((App)(App.Current)).recordTable = TRecordTable.LoadFromFile(appPath + "\\scores.dat");
                }
                catch
                {
                    // Загрузка прошла неудачно - создаем пустую таблицу.
                    PrintLogMessage("\tОшибка при загрузке таблицы рекордов - будет использована пустая таблица.");
                    ((App)(App.Current)).recordTable = new TRecordTable();
                }

                // Загрузка параметров XSE: -----------------------------------------------------------------
                PrintLogMessage("Загрузка настроек XSE...");

                IEnumerable<XElement> XSESettings;

                string strStorageFile = "";
                string strBankFile = "";
                int    nMaxVoices = 8, nMaxStreams = 1, nMaxBankSize = 1 * 1024 * 1024;
                bool   bXMode = true;
                float  fMusicVolume = 1.0f, fSFXVolume = 1.0f;

                try
                {
                    XSESettings = xmlGameData.Element("GameData").Element("XSE").Elements("Settings");
                }
                catch
                {
                    throw new Exception("Отсутствует секция <GameData><XSE>");
                }

                foreach (XElement e in XSESettings)
                {
                    XAttribute attrKey = e.Attribute("Key");
                    XAttribute attrValue = e.Attribute("Value");

                    if (attrKey == null)
                        throw new Exception("Пропущен атрибут Key");
                    if (attrValue == null)
                        throw new Exception("Пропущен атрибут Value");


                    switch (attrKey.Value.ToString())
                    {
                        case "StorageFile":
                            {
                                strStorageFile = appPath + "\\" + attrValue.Value.ToString();

                                PrintLogMessage(string.Format("\tStorageFile = {0}", strStorageFile));
                                break;
                            }
                        case "BankFile":
                            {
                                strBankFile = attrValue.Value.ToString();

                                PrintLogMessage(string.Format("\tBankFile = {0}", strBankFile));
                                break;
                            }
                        case "MaxVoices":
                            {
                                nMaxVoices = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tMaxVoices = {0}", nMaxVoices));
                                break;
                            }
                        case "MaxStreams":
                            {
                                nMaxStreams = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tMaxStreams = {0}", nMaxStreams));
                                break;
                            }
                        case "MaxBankSize":
                            {
                                nMaxBankSize = int.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tMaxBankSize = {0}", nMaxBankSize));
                                break;
                            }
                        case "XMode":
                            {
                                bXMode = bool.Parse(attrValue.Value.ToString());

                                PrintLogMessage(string.Format("\tXMode = {0}", bXMode));
                                break;
                            }
                        case "MusicVolume":
                            {
                                fMusicVolume = float.Parse(attrValue.Value.ToString(), CultureInfo.InvariantCulture);

                                PrintLogMessage(string.Format("\tMusicVolume = {0}", fMusicVolume));
                                break;
                            }
                        case "SFXVolume":
                            {
                                fSFXVolume = float.Parse(attrValue.Value.ToString(), CultureInfo.InvariantCulture);

                                PrintLogMessage(string.Format("\tSFXVolume = {0}", fSFXVolume));
                                break;
                            }
                        default:
                            {
                                break;
                            }

                    }

                }

                try
                {
                    PrintLogMessage("Инициализация XSE...");

                    TAudioEngine.Current.MusicVolume = fMusicVolume;
                    TAudioEngine.Current.SFXVolume = fSFXVolume;

                    TAudioEngine.Current.Initialize(strStorageFile, strBankFile, nMaxVoices, 
                        nMaxStreams, nMaxBankSize, bXMode);

                } catch (Exception e)
                {
                    PrintLogMessage("Ошибка инициализации XSE. Отладочная информация:\n");
                    PrintLogMessage(string.Format("{0}", e.Message));
                    PrintLogMessage("Звук будет отключен.");
                }

            }
            catch (Exception e)
            {
                PrintLogMessage("Ошибка при загрузке игровых данных! ( " + e.Message + " )");
                loadFailed = true;
            }

            IsLoading = false;

            if (loadFailed)
            {
                // Ошибка загрузки, выходим:
                PrintLogMessage("Нажмите \"Закрыть\" для выхода.");
                Dispatcher.Invoke(() => { btnClose.Visibility =  Visibility.Visible; });
            }
            else
            {
                // Загрузка прошла успешно - закрываем это окно и открываем основное:
                Dispatcher.Invoke(() => {
                    GameWindow gameWindow = new GameWindow();
                    App.Current.MainWindow = gameWindow;
                    gameWindow.Show();
                    this.Close();
                });
            };
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            base.OnClosing(e);

            // Во время загрузки нельзя закрывать окно:
            if (IsLoading)
                e.Cancel = true;
        }

        private void OnCloseButton(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void PrintLogMessage(string msg)
        {
            Dispatcher.Invoke(() => {
                txtLog.Text += string.Format("{0}\n", msg);
                svLog.ScrollToBottom();
            });

            TLogger.Current.WriteLine(msg);
        }
    }

}
