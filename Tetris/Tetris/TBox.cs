using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows;

namespace Tetris
{
    // Интерфейс для уведомления главного окна об игровых событиях.
    public interface IBoxNotify
    {
        // Вызывается при перемещении фигуры в стакане.
        int OnShapeMove(float p);

        // Вызывается при перевороте фигуры в стакане.
        int OnShapeRotate(float p);

        // Вызывается при броске фигуры.
        int OnShapeDrop(float p);

        // Вызывается при падении фигуры на дно.
        int OnHitShape(float p);

        // Вызывается при взрыве строк (в начале анимации).
        int OnExplodeLines();

        // Вызывается при закреплении фигуры в стакане.
        int OnAttachShape(TShape s);
        
        // Вызывается при удалении (в конце анимации) строк.
        int OnDeleteRows(int count);

        // Вызывается при входе в режим паузы.
        int OnPause();

        // Вызывается при выходе из паузы.
        int OnResume();

        // Вызывается при попытке прервать игру.
        int OnAbort();

        // Вызывается при завершении игры, когда стакан полон.
        int OnGameOver();
    }

    struct TCell
    {
        public byte Value;     // 0 или 1
        public int brushIndex; // индекс кисти для заливки (не используется)
    }

    // Флаги для обновления картинки (перерисовка объектов Visual):
    enum BoxUpdateMode
    {
        modeUpdateNone = 0,   // ничего
        modeUpdateBox  = 1,   // стакан
        modeUpdateBricks = 2, // кирпичики
        modeUpdateShape = 4,  // фигуру в стакане
        modeUpdateGlass = 8   // стекло
    }

    // Состояния стакана (влияют на обработку):
    enum BoxState
    {
        stateNormal = 0,     // обычное
        stateShapeFall = 1,  // фигура падает на дно (игрок бросил ее, управление отключается, ускоренное падение)
        stateBoxSqueeze = 2, // удаление полных строк (анимация мигания блоков)
        stateGameOver = 3,   // конец игры
        statePaused = 4      // пауза
    }

    public class TBox : Canvas
    {
        // Размеры стакана и другие параметры из класса App:
        private int boxWidth, boxHeight, brickSize;

        // Ячейки в стакане:
        private TCell[] cellsData = null;

        // Видимость строк (для эффекта мигания):
        private bool[] rowsVisibility = null;

        // Видимость фигуры (для эффекта мигания на паузе)
        private bool shapeVisibility = false;

        // Номера полных строк (для удаления во время сжатия стакана):
        private List<int> solidRows = new List<int>();

        // Текущая фигура в стакане:
        private TShape currentShape = null;

        // Следующая фигура в стакане:
        private TShape nextShape = null;

        // Положение фигуры (x,y), (0,0) - левый верхний угол стакана:
        private int shapeX, shapeY;

        // Задержка при падении:
        private const float delayFactor = 0.1f;
        private float delayCurrent = 0.0f;

        // Задержка при мигании:
        private const float blinkFactor = 0.2f;
        private float blinkCurrent = 0.0f;
        private int blinkCount = 0;

        // Текущий режим обновления:
        private BoxUpdateMode boxUpdateMode = BoxUpdateMode.modeUpdateNone;

        // Текущее состояние:
        private BoxState boxState = BoxState.stateNormal;

        // Изображение стакана
        private DrawingVisual visualBox = new DrawingVisual();

        // Изображение кирпичей:
        private DrawingVisual visualBricks = new DrawingVisual();

        // Изображение фигуры:
        private DrawingVisual visualShape = new DrawingVisual();

        // Изображение стекла:
        private DrawingVisual visualGlass = new DrawingVisual();

        // Окно для просмотра следующей фигуры:
        private TShapePreview canvasNextShape = null;

        // Ссылка на интерфейс IBoxNotify для отправки уведомлений.
        private IBoxNotify boxNotify = null;

        // Индексатор для получения состояния ячейки в стакане:
        private byte this[int i, int j]
        {
            get
            {
                // 1. Все ячейки НАД стаканом считаются пустыми.
                if ( (j < 0) && (i >= 0) && (i < boxWidth))
                    return 0;

                // 2. Все ячейки ПОД стаканом считаются занятыми.
                if (j >= boxHeight)
                    return 1;

                // 3. Все ячейки слева и справа от стакана также считаются занятыми.
                if ((i < 0) || (i >= boxWidth))
                    return 1;

                return cellsData[j * boxWidth + i].Value;
            }
            set
            {
                // Если диапазон выходит за пределы стакана - исключение НЕ происходит.
                if ((i < 0) || (i >= boxWidth) || (j < 0) || (j >= boxHeight))
                    return;

                cellsData[j * boxWidth + i].Value = value;
            }
        }
        public TBox() : base()
        {
            boxWidth = ((App)(App.Current)).boxWidth;
            boxHeight = ((App)(App.Current)).boxHeight;
            brickSize = ((App)(App.Current)).brickSize;
            
            // Добавляем элементы в логическое и визуальное деревья:
            AddVisualChild(visualBox);
            AddVisualChild(visualBricks);
            AddVisualChild(visualShape);
            AddVisualChild(visualGlass);

            AddLogicalChild(visualBox);
            AddLogicalChild(visualBricks);
            AddLogicalChild(visualShape);
            AddLogicalChild(visualGlass);

            // Резервируем места для ячеек:
            cellsData = new TCell[boxWidth * boxHeight];
            rowsVisibility = new bool[boxHeight];

            ClipToBounds = true;

            ClearBox();
        }

        // Установка объекта, который будет принимать уведомления:
        public void SetNotifyTarget(IBoxNotify boxNotifyInterface)
        {
            if (boxNotifyInterface != null)
                boxNotify = boxNotifyInterface;
        }

        // Установка окна для отображения следующей фигуры:
        public void SetPreviewWindow(TShapePreview previewWindow)
        {
            if (previewWindow != null)
                canvasNextShape = previewWindow;
        }

        // Подготовка стакана, очистка данных:
        public void ClearBox()
        {
            // Обычное состояние, текущих фигур нет:
            currentShape = null;
            nextShape = null;
            boxState = BoxState.stateNormal;
            boxUpdateMode = BoxUpdateMode.modeUpdateNone;

            // Стакан пуст:
            for (int i = 0; i < boxWidth * boxHeight; i++)
                cellsData[i].Value = 0;

            // Рендеринг элементов:
            RenderBox();
            RenderShape();
            RenderBricks();
            RenderGlass();

            // Генерируем первую фигуру:
            nextShape = GenerateShape();
            delayCurrent = 0.0f;
            blinkCurrent = 0.0f;
            blinkCount = 0;
        }

        protected override Visual GetVisualChild(int index)
        {
            switch (index)
            {
                case 0: return visualBox;
                case 1: return visualBricks;
                case 2: return visualShape;
                case 3: return visualGlass;
                default: return null;
            }
        }

        protected override int VisualChildrenCount
        {
            get
            {
                return 4;
            }
        }

        private void RenderBox()
        {
            using (DrawingContext dc = visualBox.RenderOpen())
            {
                int i;

                // Рисуем бордюр их кирпичиков:
                Brush brushBorder = new LinearGradientBrush(Color.FromRgb(255, 255, 255), 
                    ((App)(App.Current)).GetGameColor(TGameColors.BorderColor), 45.0f);
                Pen penBorder = new Pen(new SolidColorBrush(Color.FromRgb(0, 0, 0)), 2.0);

                for(i = 0; i < boxHeight; i++)
                {
                    // Левая стенка:
                    dc.DrawRoundedRectangle(brushBorder, penBorder,
                        new Rect(new Point(0, i * brickSize),
                        new Size((double)brickSize, (double)brickSize)), 2, 2);
                    
                    // Правая стенка:
                    dc.DrawRoundedRectangle(brushBorder, penBorder,
                        new Rect(new Point( (boxWidth + 1)*brickSize, i * brickSize),
                        new Size((double)brickSize, (double)brickSize)), 2, 2);
                }

                // Дно:
                for (i = 0; i < boxWidth + 2; i++)
                {
                    dc.DrawRoundedRectangle(brushBorder, penBorder,
                        new Rect(new Point(i * brickSize, (boxHeight) * brickSize),
                        new Size((double)brickSize, (double)brickSize)), 2, 2);
                }

                // Рисуем сетку направляющих:
                Pen penGrid = new Pen(new SolidColorBrush(((App)(App.Current)).GetGameColor(TGameColors.GridColor)), 1.0);
                penGrid.DashStyle = DashStyles.Dash;

                for(i = 2; i < boxWidth + 1; i++)
                {
                    // Вертикальные направляющие:
                    dc.DrawLine(penGrid, new Point(brickSize * i, 0), new Point(brickSize * i, brickSize * boxHeight));
                }
                for (i = 0; i < boxHeight; i++)
                {
                    // Горизонтальные направляющие:
                    dc.DrawLine(penGrid, new Point(brickSize, i * brickSize), new Point(brickSize * (boxWidth + 1), i * brickSize));
                }
            }
        }
        private void RenderShape()
        {
            using (DrawingContext dc = visualShape.RenderOpen())
            {
                int i, j;

                if (currentShape != null)
                {
                    // Рисуем текущуюю фигуру:
                    Brush brushShape = new LinearGradientBrush(Color.FromRgb(255, 255, 255),
                            ((App)(App.Current)).colorList[currentShape.ColorIndex], 45.0f); ;

                    if (boxState == BoxState.statePaused)
                    {
                        blinkCurrent += blinkFactor;
                        if (blinkCurrent >= 1.0f)
                        {
                            // Когда игра стоит на паузе, периодически меняется видимость фигуры.
                            blinkCurrent = 0.0f;
                            shapeVisibility = !shapeVisibility;
                        };

                        if (shapeVisibility == false)
                        {
                            // Когда фигура невидима - выбираем пустую кисть.
                            brushShape = new SolidColorBrush(Color.FromArgb(0, 0, 0, 0));
                        }
                    };

                    Pen penShape = new Pen(new SolidColorBrush(Color.FromRgb(0, 0, 0)), 2.0);

                    for(i = 0; i < currentShape.Size; i++)
                    {
                        for(j = 0; j < currentShape.Size; j++)
                        {
                            if (currentShape[i, j] != 0)
                            {
                                dc.DrawRoundedRectangle(brushShape, penShape,
                                new Rect(new Point((shapeX + 1 + i) * brickSize, (shapeY + j) * brickSize),
                                new Size((double)brickSize, (double)brickSize)), 2, 2);
                            }
                        }
                    }
                }
            }
        }

        private void RenderBricks()
        {
            using (DrawingContext dc = visualBricks.RenderOpen())
            {
                int i, j;

                // Рисуем текущуюю фигуру:
                Brush brushBricks = new LinearGradientBrush(Color.FromRgb(255, 255, 255),
                    ((App)(App.Current)).GetGameColor(TGameColors.BricksColor), 45.0f);
                Pen penBricks = new Pen(new SolidColorBrush(Color.FromRgb(0, 0, 0)), 2.0);

                for (i = 0; i < boxWidth; i++)
                {
                    for (j = 0; j < boxHeight; j++)
                    {

                        // Если идет процесс сжатия стакана и видимость строки == false, пропускаем её.
                        if ((boxState == BoxState.stateBoxSqueeze) && (rowsVisibility[j] == false))
                            continue;

                        if (this[i, j] != 0)
                        {
                            dc.DrawRoundedRectangle(brushBricks, penBricks,
                            new Rect(new Point((1 + i) * brickSize, j * brickSize),
                            new Size((double)brickSize, (double)brickSize)), 2, 2);
                        }
                    }
                }
            }
        }
        private void RenderGlass()
        {
            using (DrawingContext dc = visualGlass.RenderOpen())
            {
                // Рисуем стекло поверх стакана и всех кирпичей:
                Pen penGlass = new Pen(new SolidColorBrush(Color.FromArgb(0, 0, 0, 0)), 0.0);
                GradientStopCollection gStop = new GradientStopCollection()
                { new GradientStop(((App)(App.Current)).GetGameColor(TGameColors.GlassColor0), 0.0f),
                  new GradientStop(((App)(App.Current)).GetGameColor(TGameColors.GlassColor1), 0.8f),
                  new GradientStop(((App)(App.Current)).GetGameColor(TGameColors.GlassColor0), 1.0f)};
                Brush brushGlass = new LinearGradientBrush(gStop, 0.0f); 

                dc.DrawRoundedRectangle(brushGlass, penGlass,
                new Rect(new Point(brickSize, 0),
                new Size((double)brickSize * boxWidth, (double)brickSize * boxHeight)), 0, 0);
            }
        }

        private TShape GenerateShape()
        {
            // Генерируем новую фигуру в стакане:
            int nShapes = ((App)(App.Current)).shapeList.Count;
            int n = ((App)(App.Current)).Random(0, nShapes);

            return ((App)(App.Current)).shapeList[n].RandomTransform();
        }

        // Метод возвращает true, если фигура s помещается в стакан по координатам (x,y).
        private bool IsShapeFit(TShape s, int x, int y)
        {
            byte r = 0;
            int i, j;

            for (i = 0; i < s.Size; i++)
            {
                for(j = 0; j < s.Size; j++)
                {
                    r |= (byte)(this[x + i, y + j] + s[i, j]);
                }
            }

            // Если второй бит не равен нулю, значит при сложении было переполнение и фигура не входит.
            return ((r & 2) > 0 ? false : true);
        }

        // Метод добавляет матрицу s в матрицу стакана по координатам (x,y):
        private void AttachShape(TShape s, int x, int y)
        {
            for (int i = 0; i < s.Size; i++)
            {
                for (int j = 0; j < s.Size; j++)
                {
                    this[x + i, y + j] +=  s[i, j];
                }
            }
        }

        // Метод возвращает true, если строка полностью заполнена:
        private bool IsRowSolid(int n)
        {
            if ((n < 0) || (n >= boxHeight))
                return false;

            int sum = 0;
            for (int i = 0; i < boxWidth; i++)
                sum += this[i, n];

            return (sum == boxWidth) ? true : false;
        }

        // Удаление строки в стакане. Верхние строки спускаются ниже.
        private void DestroyRow(int n)
        {
            int i;

            while (n > 0)
            {
                // Верхние строки "сползают" вниз:
                for (i = 0; i < boxWidth; i++)
                {
                    this[i, n] = this[i, n - 1];
                }
                n--;
            }

            // Самая верхняя строка очищается.
            for (i = 0; i < boxWidth; i++)
                this[i, 0] = 0;
        }

        // Обновление игрового поля: ------------------------------------------------------------------------
        public void Update()
        {
            // Получаем команду от генератора:
            GameCommands gameCommand = ((App)App.Current).commandGenerator.Generate().First();

            // Обрабатываем команду "ПАУЗА". На паузу игру можно ставить только из режима stateNormal:
            if (gameCommand == GameCommands.Pause)
            {
                // Попытка поставить / снять паузу.
                if (boxState == BoxState.statePaused)
                {
                    // Снимаем с паузы. 
                    boxState = BoxState.stateNormal;
                    blinkCurrent = 0.0f;

                    if (boxNotify != null)
                        boxNotify.OnResume();

                    boxUpdateMode |= BoxUpdateMode.modeUpdateShape;
                }
                else
                if (boxState == BoxState.stateNormal)
                {
                    // Ставим на паузу. Фигура начинает мигать.
                    boxState = BoxState.statePaused;
                    blinkCurrent = 0.0f;

                    if (boxNotify != null)
                        boxNotify.OnPause();

                    boxUpdateMode |= BoxUpdateMode.modeUpdateShape;
                }
            } else
            // Обрабатываем команду "Прервать игру". Срабатывает только при stateNormal:
            if(gameCommand == GameCommands.Abort)
            {
                if (boxNotify != null)
                    boxNotify.OnAbort();
            }

            if (boxState == BoxState.stateNormal)
            {
                // Обычное состояние. Управление включено, фигура падает.
                if (currentShape == null)
                {
                    // Текущей фигуры нет. Берем следующую.
                    currentShape = nextShape;
                    shapeY = -currentShape.Size;
                    shapeX = (boxWidth - currentShape.Size) / 2;
                    delayCurrent = 0.0f;

                    // Генерируем еще одну, на следующий раз.
                    nextShape = GenerateShape();
                    if (canvasNextShape != null)
                        canvasNextShape.SetShape(nextShape);

                    boxUpdateMode |= BoxUpdateMode.modeUpdateShape;
                } else
                {
                    // Фигура есть. 
                    // 1. Проверяем команды:
                    switch (gameCommand)
                    {
                        case GameCommands.MoveLeft:
                            {
                                if (IsShapeFit(currentShape, shapeX - 1, shapeY) == true)
                                {
                                    shapeX--;
                                    boxUpdateMode |= BoxUpdateMode.modeUpdateShape;

                                    boxNotify.OnShapeMove(CalculateShapeParameter(currentShape, shapeX));
                                }
                                break;
                            }
                        case GameCommands.MoveRight:
                            {
                                if (IsShapeFit(currentShape, shapeX + 1, shapeY) == true)
                                {
                                    shapeX++;
                                    boxUpdateMode |= BoxUpdateMode.modeUpdateShape;

                                    boxNotify.OnShapeMove(CalculateShapeParameter(currentShape, shapeX));
                                }
                                break;
                            }
                        case GameCommands.RotateCW:
                            {
                                TShape s = currentShape.Rotate90CW();
                                if (IsShapeFit(s, shapeX, shapeY) == true)
                                {
                                    currentShape = s;
                                    boxUpdateMode |= BoxUpdateMode.modeUpdateShape;

                                    boxNotify.OnShapeRotate(CalculateShapeParameter(currentShape, shapeX));
                                }
                                break;
                            }
                        case GameCommands.RotateCCW:
                            {
                                TShape s = currentShape.Rotate90CCW();
                                if (IsShapeFit(s, shapeX, shapeY) == true)
                                {
                                    currentShape = s;
                                    boxUpdateMode |= BoxUpdateMode.modeUpdateShape;

                                    boxNotify.OnShapeRotate(CalculateShapeParameter(currentShape, shapeX));
                                }
                                break;
                            }
                        case GameCommands.Drop:
                            {
                                boxState = BoxState.stateShapeFall;
                                boxNotify.OnShapeDrop(CalculateShapeParameter(currentShape, shapeX));

                                break;
                            }
                    }

                    // 2. Проверяем, может ли фигура упасть еще ниже:
                    if (IsShapeFit(currentShape, shapeX, shapeY + 1) == true)
                    {
                        // Фигура может упасть, сдвигаем ее, если delayCurrent >= 1.0f:
                        delayCurrent += delayFactor;

                        if (delayCurrent >= 1.0f)
                        {
                            delayCurrent = 0.0f;
                            shapeY++;
                            boxUpdateMode |= BoxUpdateMode.modeUpdateShape;
                        }
                    }
                    else
                    {
                        // Фигура упасть не может. Закрепляем ее и она превращается в кирпичи.
                        delayCurrent += delayFactor;

                        if (delayCurrent >= 1.0f)
                        {
                            AttachShape(currentShape, shapeX, shapeY);
                            if (boxNotify != null)
                            {
                                boxNotify.OnAttachShape(currentShape);
                                boxNotify.OnHitShape(CalculateShapeParameter(currentShape, shapeX));
                            }

                            // Проверяем, может быть какие-то строки нужно удалить:
                            // Строки для проверки - [shapeY, shapeY + shapeSize)
                            for (int i = shapeY; i < shapeY + currentShape.Size; i++)
                            {
                                if (IsRowSolid(i) == true)
                                    solidRows.Add(i);
                            }

                            if (solidRows.Count > 0)
                            {
                                // Какие-то строки нужно удалить. Включаем режим сжатия стакана.
                                for (int k = 0; k < boxHeight; k++)
                                    rowsVisibility[k] = true;
                                boxState = BoxState.stateBoxSqueeze;

                                boxNotify.OnExplodeLines();

                            }
                            else
                            if (shapeY < 0)
                            {
                                // Полных строк нет и стакан переполнился. Игра окончена.
                                boxState = BoxState.stateGameOver;

                                if (boxNotify != null)
                                    boxNotify.OnGameOver();
                            }

                            currentShape = null;
                            boxUpdateMode |= (BoxUpdateMode.modeUpdateShape | BoxUpdateMode.modeUpdateBricks);
                        }
                    }
                }
            } else
            if (boxState == BoxState.stateShapeFall)
            {
                // В данном режиме фигура падает каждое обновление, управление не работает.
                // Однако, при попытке перевернуть фигуру или сдвинуть её - переходим в обычный режим.

                boxUpdateMode |= BoxUpdateMode.modeUpdateShape;

                if ( (gameCommand == GameCommands.MoveLeft) || (gameCommand == GameCommands.MoveRight)
                    || (gameCommand == GameCommands.RotateCW) || (gameCommand == GameCommands.RotateCCW))
                {
                    // Попытка остановить падение:
                    boxState = BoxState.stateNormal;
                } else
                if (IsShapeFit(currentShape, shapeX, shapeY + 1) == true)
                {
                    // Можно падать еще ниже.
                    shapeY++;
                }
                else
                {
                    // Достигли дна. Переключаемся в обычный режим игры, где фигура будет закреплена.
                    boxState = BoxState.stateNormal;
                }
            }
            else
            if (boxState == BoxState.stateBoxSqueeze)
            {
                int i;

                // Накручиваем счетчик для мигания.
                blinkCurrent += blinkFactor;

                if (blinkCurrent >= 1.0f)
                {
                    // Пора изменить видимость строк и обнулить счетчик:
                    blinkCurrent = 0.0f;
                    blinkCount++;

                    for (i = 0; i < solidRows.Count; i++)
                        rowsVisibility[solidRows[i]] = !rowsVisibility[solidRows[i]];

                    if (blinkCount > 6)
                    {
                        // Фаза мигания закончена. Удаляем строки в стакане.

                        if (boxNotify != null)
                            boxNotify.OnDeleteRows(solidRows.Count);

                        for (i = 0; i < solidRows.Count; i++)
                        {
                            DestroyRow(solidRows[i]);
                        }

                        blinkCount = 0;
                        blinkCurrent = 0.0f;
                        solidRows.Clear();

                        boxState = BoxState.stateNormal;
                    }

                    boxUpdateMode |= BoxUpdateMode.modeUpdateBricks;
                }
            } else
            {
                if (boxState == BoxState.statePaused)
                {
                    // На паузе просто обновляем фигуру, чтобы она мигала.
                    boxUpdateMode |= BoxUpdateMode.modeUpdateShape;
                }
            }

            /////////////////////////////////////////////////////////////////////////////////////////////////
            // Рендерим элементы по необходимости:
            if ((boxUpdateMode & BoxUpdateMode.modeUpdateBox) > 0)
                RenderBox();
            if ((boxUpdateMode & BoxUpdateMode.modeUpdateShape) > 0)
                RenderShape();
            if ((boxUpdateMode & BoxUpdateMode.modeUpdateBricks) > 0)
                RenderBricks();
            if ((boxUpdateMode & BoxUpdateMode.modeUpdateGlass) > 0)
                RenderGlass();

            boxUpdateMode = BoxUpdateMode.modeUpdateNone;

            InvalidateVisual();
        }

        private float CalculateShapeParameter(TShape shape, int x)
        {
            int nBoxWidth = ((App)(App.Current)).boxWidth;
            float a1 = 1.0f - (float)shape.Size;
            float b1 = (float)nBoxWidth - 1.0f;
            float a2 = -1.0f;
            float b2 = 1.0f;
            float k = (float)x;
            float p;

            p = (k - a1) / (b1 - a1);
            k = (b2 - a2) * p + a2;

            return k;
        }
    }
}
