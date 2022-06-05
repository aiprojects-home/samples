using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PowerLine
{
    public enum CarType
    {
        Generic = 0,
        Passenger = 1,
        Truck = 2
    }

    public class Car
    {
        /// <summary>
        /// Тип автомобиля.
        /// </summary>
        public CarType Type { get; set; }

        /// <summary>
        /// Потребление топлива (литр/км)
        /// </summary>
        public decimal FuelConsumption { get; set; }

        /// <summary>
        /// Объем бака (л).
        /// </summary>
        public decimal MaxFuel { get; set; }

        /// <summary>
        /// Текущий уровень топлива (л).
        /// </summary>
        public decimal CurrentFuel { get; set; }

        /// <summary>
        /// Макс. скорость (км/ч)
        /// </summary>
        public decimal Speed { get; set; }

        /// <summary>
        /// Конструирует объект.
        /// </summary>
        /// <param name="fuelCons">Потребление топлива</param>
        /// <param name="maxFuel">Максимальный запас бака</param>
        /// <param name="speed">Скорость</param>
        public Car(decimal fuelCons, decimal maxFuel, decimal speed)
        {
            Type = CarType.Generic;
            FuelConsumption = fuelCons;
            MaxFuel = maxFuel;
            Speed = speed;
        }

        /// <summary>
        /// Возвращает расстояние, которое автомобиль способен преодолеть.
        /// </summary>
        /// <param name="bMax">true, если требуется вычислить макс. расстояние, false - расстояние с текущим топливным баком</param>
        /// <returns>Возвращает расстояние в км.</returns>
        virtual public decimal CalculateDistance(bool bMax)
        {
            return (bMax == true ? MaxFuel / FuelConsumption : CurrentFuel / FuelConsumption); 
        }

        /// <summary>
        /// Выводит на консоль информацию об автомобиле.
        /// </summary>
        public void Dump()
        {
            System.Console.WriteLine($"Тип автомобиля:          {Type}");
            System.Console.WriteLine($"Скорость:                {Speed}км/час");
            System.Console.WriteLine($"Потребление топлива:     {FuelConsumption}л/км");
            System.Console.WriteLine($"Макс. запас топлива:     {MaxFuel}л");
            System.Console.WriteLine($"Текущий уровень топлива: {CurrentFuel}л");
            System.Console.WriteLine($"Максимальное расстояние: {CalculateDistance(true):F2}км");
            System.Console.WriteLine($"Оставшееся расстояние:   {CalculateDistance(false):F2}км");
        }

        /// <summary>
        /// Возвращает время в часах, которое потребуется для преодоления заданного расстояния с текущим баком.
        /// </summary>
        /// <param name="distance">Расстояние в километрах</param>
        /// <returns>Время в часах или 0, если расстояние невозможно преодолеть</returns>
        public decimal CalculateTime(decimal distance)
        {
            if (distance <= CalculateDistance(false))
            {
                // Расстояние можно преодолеть, топлива хватает.
                return (distance / Speed);
            };

            return 0;
        }
   }
}
