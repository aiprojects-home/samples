using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace Tetris
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    /// 

    public enum TGameColors
    {
        BorderColor,
        GridColor,
        BricksColor,
        GlassColor0,
        GlassColor1,
        PreviewColor0,
        PreviewColor1
    }

    public partial class App : Application
    {
        // Владелец копии:
        public string ownerName = "";
        // Цвета:
        public List<Color> colorList = new List<Color>();

        // Список фигур, падающих в стакан:
        public List<TShape> shapeList = new List<TShape>(); 

        // Размеры стакана:
        public int boxWidth, boxHeight;

        // Размеры кирпича:
        public int brickSize;

        // Частота кадров:
        public int FPS;

        // Индексы цветов для игровых элементов:
        public int borderColor, gridColor, bricksColor, glassColor0, glassColor1, previewColor0, previewColor1;

        private Random r = new Random((int)DateTime.Now.Ticks & 0x0000FFFF);

        // Класс генератора команд:
        public TCommandGenerator commandGenerator = new TCommandGenerator();

        // Класс таблицы рекордов:
        public TRecordTable recordTable = null;

        public int Random(int min, int max)
        {
            return r.Next(min, max);
        }

        public Color GetGameColor(TGameColors c)
        {
            switch (c)
            {
                case TGameColors.BorderColor: return colorList[borderColor];
                case TGameColors.GridColor: return colorList[gridColor];
                case TGameColors.BricksColor: return colorList[bricksColor];
                case TGameColors.GlassColor0: return colorList[glassColor0];
                case TGameColors.GlassColor1: return colorList[glassColor1];
                case TGameColors.PreviewColor0: return colorList[previewColor0];
                case TGameColors.PreviewColor1: return colorList[previewColor1];
                default: return Color.FromRgb(0, 0, 0);
            }
        }

        protected override void OnStartup(StartupEventArgs e)
        {
            string strCurrentDirectory = Environment.CurrentDirectory;

            TLogger.Current.CreateLogFile(strCurrentDirectory + "\\startup.log");

            base.OnStartup(e);
        }

        protected override void OnExit(ExitEventArgs e)
        {

            TAudioEngine.Current.Dispose();
            TLogger.Current.Dispose();

            base.OnExit(e);
        }
    }
}
