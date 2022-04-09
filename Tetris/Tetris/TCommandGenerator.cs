using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace Tetris
{
    public enum GameCommands
    {
        None = 0,      // ничего
        MoveLeft = 1,  // сдвиг фигуры влево (TURBO)
        MoveRight = 2, // сдвиг фигуры вправо (TURBO)
        RotateCW = 3,  // поворот по часовой
        RotateCCW = 4, // поворот против часовой
        Drop = 5,      // бросить на дно стакана
        Pause = 6,     // пауза
        Abort = 7      // прервать игру
    }

    public struct GameCommandState
    {
        public GameCommands Command;    // команда
        public Key          commandKey; // клавиша на клавиатуре
        public bool         turboMode;  // true, если команда поддерживает повторную генерацию
        public bool         keyPressed; // true, если клавиша была нажата
        public int          genTimes;   // количество сгенерированных команд за одно нажатие клавиши
        public int          genSkipped; // количество пропущенных генераций за одно нажатие клавиши
    }

    // Класс для генерации игровых комманд на основе данных о состоянии клавиатуры.
    // Команды типа TURBO генерируются как только нажата соотв. клавиша. Затем идет отсчет задержки TypeDelay для
    // повторной (второй) генерации и потом, если клавиша все еще нажата - генерация идет с задержкой
    // TypeSpeed.
    // Обычные команды генерируются при нажатии клавиши и только один раз. Для повторной генерации
    // клавиша должна быть отпущена и повторно нажата.
    public class TCommandGenerator
    {
        public int TypeDelay // Задержка после первой генерации, если клавиши еще нажата.
        { get; set; }
        public int TypeSpeed // Задержка для дальнейших генераций, если клавиша еще нажата.
        { get; set; }

        // Список команд для генерации.
        private List<GameCommandState> commandStates = new List<GameCommandState>();

        public TCommandGenerator()
        {
            // Скорость генерации для TURBO по-умолчанию.
            TypeDelay = 5;
            TypeSpeed = 2;
        }

        // Метод добавляет новую команду в список генерации.
        public void AddCommand(GameCommands c, Key k, bool isTurbo)
        {
            GameCommandState gcs;

            // При добавлении предполагаем, что клавиша не нажата и команда ни разу не генерировалась.
            gcs.Command = c;
            gcs.commandKey = k;
            gcs.turboMode = isTurbo;
            gcs.keyPressed = false;
            gcs.genTimes = 0;
            gcs.genSkipped = 0;

            commandStates.Add(gcs);
        }

        public List<GameCommands> Generate()
        {
            List<GameCommands> genCommands = new List<GameCommands>();

            for(int i = 0; i < commandStates.Count; i++)
            {
                GameCommandState currentState = commandStates[i];
                bool isPressed = Keyboard.IsKeyDown(currentState.commandKey);

                if (currentState.turboMode == false)
                {
                    // Обычная команда
                    if ( (isPressed == true) && (currentState.keyPressed == false) )
                    {
                        // Клавиша нажата, но в предыдущий раз она была отпущена. Генерируем команду.
                        genCommands.Add(currentState.Command);
                    }

                    // Обновляем информацию в генераторе:
                    currentState.keyPressed = isPressed;
                    commandStates[i] = currentState;
                }
                else
                {
                    // TURBO-команда
                    if ((isPressed == true) && (currentState.keyPressed == false))
                    {
                        // Клавиша нажата, но в предыдущий раз она была отпущена. Генерируем первую команду.
                        genCommands.Add(currentState.Command);

                        // Обновляем информацию в генераторе:
                        currentState.keyPressed = isPressed;
                        currentState.genSkipped = 0;           // начинаем отсчет для задержки
                        currentState.genTimes = 1;             // команда сгенерирована 1 раз

                        commandStates[i] = currentState;
                    } else
                    if ((isPressed == true) && (currentState.keyPressed == true))
                    {
                        // Клавиша нажата и удерживается какое-то время. Генерируем команду, если счетчик
                        // накрутил достаточное количество пропусков.

                        currentState.genSkipped++;

                        if (currentState.genTimes == 1)
                        {
                            // Для второй генерации пользуемся счетчиком TypeDelay:
                            if (currentState.genSkipped >= TypeDelay)
                            {
                                // Накрутили достаточно. Генерируем команду.
                                genCommands.Add(currentState.Command);

                                // Обновляем генератор:
                                currentState.genSkipped = 0;
                                currentState.genTimes++;
                            }
                        }
                        else
                        if (currentState.genTimes > 1)
                        {
                            // Для третьей и последующих генераций пользуемся счетчиком TypeSpeed:
                            if (currentState.genSkipped >= TypeSpeed)
                            {
                                // Накрутили достаточно. Генерируем команду.
                                genCommands.Add(currentState.Command);

                                // Обновляем генератор:
                                currentState.genSkipped = 0;
                                currentState.genTimes++;
                            }
                        }

                        commandStates[i] = currentState;
                    } else
                    if (isPressed == false)
                    {
                        // Клавиша отпущена. Команды не будет. Обновляем генератор.
                        currentState.keyPressed = isPressed;
                        commandStates[i] = currentState;
                    }
                }
            }

            // Если команд так и не нашлось - добавляем пустую.
            if (genCommands.Count == 0)
                genCommands.Add(GameCommands.None);

            return genCommands;
        }

        // Итератор для перебора зарегистрированных комманд.
        public IEnumerable <GameCommandState> GetCommands()
        {
            for (int i = 0; i < commandStates.Count; i++)
                yield return commandStates[i];
        }
    }
}
