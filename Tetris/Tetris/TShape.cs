using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace Tetris
{
    // Класс для хранения информации о фигурах, которые падают в стакан.
    public class TShape : ICloneable
    {
        private byte[] shapeData = null; // матрица
        private int shapeSize = 0;       // размер матрицы (высота = ширина)
        private int colorIndex = 0;      // номер цвета

        public int Size
        {
            get { return shapeSize; }
        }

        public int ColorIndex
        {
            get { return colorIndex; }
        }

        public TShape(int shapeSize, int colorIndex)
        {
            // Размер не должен быть слишком большим:
            if ((shapeSize <= 0) || (shapeSize > 10))
                throw new ArgumentOutOfRangeException();

            // Выделяем память под матрицу:
            this.shapeSize = shapeSize;
            this.colorIndex = colorIndex;
            shapeData = new byte[shapeSize * shapeSize];
        }

        public byte this[int i, int j] // i - горизонтальная координата в матрице, j - вертикальная
        {
            set
            {
                if ( (i < 0 ) || (j < 0) || (i >= shapeSize) || (j >= shapeSize))
                {
                    throw new ArgumentOutOfRangeException();
                }

                shapeData[j * shapeSize + i] = value;
            }
            get
            {
                if ((i < 0) || (j < 0) || (i >= shapeSize) || (j >= shapeSize))
                {
                    throw new ArgumentOutOfRangeException();
                }

                return shapeData[j * shapeSize + i];
            }
        }

        object ICloneable.Clone()
        {
            TShape newShape = new TShape(this.shapeSize, this.colorIndex);
            Array.Copy(this.shapeData, newShape.shapeData, this.shapeData.Length);

            return newShape;
        }

        // Поворот на 90 градусов по часовой:
        public TShape Rotate90CW()
        {
            TShape newShape = new TShape(this.shapeSize, this.colorIndex);

            for(int i = 0; i < shapeSize; i++)
                for(int j = 0; j < shapeSize; j++)
                {
                    newShape[shapeSize - j - 1, i] = this[i, j];
                }

            return newShape;
        }
        // Поворот на 90 градусов против часовой:
        public TShape Rotate90CCW()
        {
            TShape newShape = new TShape(this.shapeSize, this.colorIndex);

            for (int i = 0; i < shapeSize; i++)
                for (int j = 0; j < shapeSize; j++)
                {
                    newShape[j, shapeSize - i - 1] = this[i, j];
                }

            return newShape;
        }
        // Поворот на 180 градусов:
        public TShape Rotate180()
        {
            TShape newShape = new TShape(this.shapeSize, this.colorIndex);

            for (int i = 0; i < shapeSize; i++)
                for (int j = 0; j < shapeSize; j++)
                {
                    newShape[shapeSize - 1 - i, shapeSize - 1 - j] = this[i, j];
                }

            return newShape;
        }
        // Горизонтальное отражение:
        public TShape FlipH()
        {
            TShape newShape = new TShape(this.shapeSize, this.colorIndex);

            for (int i = 0; i < shapeSize; i++)
                for (int j = 0; j < shapeSize; j++)
                {
                    newShape[shapeSize - 1 - i, j] = this[i, j];
                }

            return newShape;
        }
        public TShape FlipV()
        {
            TShape newShape = new TShape(this.shapeSize, this.colorIndex);

            for (int i = 0; i < shapeSize; i++)
                for (int j = 0; j < shapeSize; j++)
                {
                    newShape[i, shapeSize - 1 - j] = this[i, j];
                }

            return newShape;
        }

        // Случайное преобразование:
        public TShape RandomTransform()
        {
            int t = ((App)(App.Current)).Random(0, 5);

            switch (t)
            {
                case 0: return Rotate90CW();
                case 1: return Rotate90CCW();
                case 2: return Rotate180();
                case 3: return FlipH();
                case 4: return FlipV();
                default: return null;

            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////

    public class TShapePreview : Canvas
    {
        private DrawingVisual visualShape = new DrawingVisual();

        public TShapePreview() : base()
        {
            AddVisualChild(visualShape);
            AddLogicalChild(visualShape);

            ClipToBounds = true;
        }

        protected override int VisualChildrenCount
        {
            get
            {
                return 1;
            }
        }

        protected override Visual GetVisualChild(int index)
        {
            switch (index)
            { 
                case 0: return visualShape;
                default: return null;
            }
        }

        // Установка фигуры для просмотра:
        public void SetShape(TShape s)
        {
            if (s == null)
                return;

            using (DrawingContext dc = visualShape.RenderOpen())
            {
                int cellSizeX = (int)Width / s.Size;
                int cellSizeY = (int)Height / s.Size;

                int i, j;

                Brush brushNull = new LinearGradientBrush(Color.FromRgb(255, 255, 255), 
                    ((App)(App.Current)).GetGameColor(TGameColors.PreviewColor0), 45.0f);
                Brush brushShape = new LinearGradientBrush(Color.FromRgb(255, 255, 255),
                    ((App)(App.Current)).GetGameColor(TGameColors.PreviewColor1), 45.0f);
                Pen penBorder = new Pen(new SolidColorBrush(Color.FromRgb(0, 0, 0)), 1.0);

                for (i = 0; i < s.Size; i++)
                {
                    for (j = 0; j < s.Size; j++)
                    {
                        if (s[i, j] != 0)
                        {
                            dc.DrawRoundedRectangle(brushShape, penBorder,
                            new Rect(new Point(i * cellSizeX, j * cellSizeY),
                            new Size((double)cellSizeX, (double)cellSizeY)), 2, 2);
                        }
                        else
                        {
                            dc.DrawRoundedRectangle(brushNull, penBorder,
                            new Rect(new Point(i * cellSizeX, j * cellSizeY),
                            new Size((double)cellSizeX, (double)cellSizeY)), 2, 2);
                        }
                    }
                }
            }
        }
    }
}
