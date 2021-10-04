using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PowerLine
{
    public class TruckCar : Car
    {
        private int m_nMaxWeight;
        private int m_nWeight;

        /// <summary>
        /// Максимальный вес
        /// </summary>
        public int MaxWeight
        {
            get
            {
                return m_nMaxWeight;
            }

            set
            {
                if (value <= 0)
                    throw new ArgumentOutOfRangeException("Максимальный вес должен быть положительным");

                m_nMaxWeight = value;
            }
        }

        /// <summary>
        /// Текущий вес
        /// </summary>
        public int Weight
        {
            get
            {
                return m_nWeight;
            }

            set
            {
                if ((value < 0) || (value > MaxWeight))
                    throw new ArgumentOutOfRangeException("Вес должен быть положительным числом и не превышать максимальное значение");

                m_nWeight = value;
            }
        }

        /// <summary>
        /// Конструирует объект
        /// </summary>
        /// <param name="maxWeight">Максимальный вес</param>
        /// <param name="fuelCons">Потребление топлива</param>
        /// <param name="maxFuel">Емкость бака</param>
        /// <param name="speed">Скорость</param>
        public TruckCar(int maxWeight, decimal fuelCons, decimal maxFuel, decimal speed) : base(fuelCons, maxFuel, speed)
        {
            MaxWeight = maxWeight;
            Type = CarType.Truck;
        }

        /// <summary>
        /// Возвращает расстояние, которое автомобиль способен преодолеть.
        /// </summary>
        /// <param name="bMax">true, если требуется вычислить макс. расстояние, false - расстояние с текущим топливным баком</param>
        /// <returns>Возвращает расстояние в км.</returns>
        public override decimal CalculateDistance(bool bMax)
        {
            decimal d = base.CalculateDistance(bMax);

            if (d == 0)
                return d;

            d *= (decimal)Math.Pow((double)(0.96), (double)Weight / (double)200);

            return d;
        }

        /// <summary>
        /// Выводит на консоль информацию об автомобиле.
        /// </summary>
        public new void Dump()
        {
            base.Dump();

            System.Console.WriteLine($"Текущий вес:             {Weight} из {MaxWeight}");
        }

    }
}

