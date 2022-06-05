using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PowerLine
{
    public class PassengerCar : Car
    {
        private int m_nMaxPassengers;
        private int m_nPassengers;

        /// <summary>
        /// Максимальное число пассажиров
        /// </summary>
        public int MaxPassengers
        {
            get
            {
                return m_nMaxPassengers;
            }

            set
            {
                if (value <= 0)
                    throw new ArgumentOutOfRangeException("Максимальное число пассажиров должно быть положительным");

                m_nMaxPassengers = value;
            }
        }

        /// <summary>
        /// Число пассажиров
        /// </summary>
        public int Passengers
        {
            get
            {
                return m_nPassengers;
            }

            set
            {
                if ( (value < 0) || (value > MaxPassengers))
                    throw new ArgumentOutOfRangeException("Число пассажиров должно быть положительным числом и не превышать максимальное значение");

                m_nPassengers = value;
            }
        }

        /// <summary>
        /// Конструирует объект
        /// </summary>
        /// <param name="maxPass">Максимальное число пассажиров</param>
        /// <param name="fuelCons">Потребление топлива</param>
        /// <param name="maxFuel">Емкость бака</param>
        /// <param name="speed">Скорость</param>
        public PassengerCar(int maxPass, decimal fuelCons, decimal maxFuel, decimal speed) : base(fuelCons, maxFuel, speed)
        {
            MaxPassengers = maxPass;
            Type = CarType.Passenger;
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
           
            d *= (decimal)Math.Pow((double)(0.94) , (double)Passengers);

            return d;
        }

        /// <summary>
        /// Выводит на консоль информацию об автомобиле.
        /// </summary>
        public new void Dump()
        {
            base.Dump();

            System.Console.WriteLine($"Количество пассажиров:   {Passengers} из {MaxPassengers}");
        }

    }
}
