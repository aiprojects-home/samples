using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;

namespace Tetris
{
    // Структура для хранения одной записи в таблице очков.
    [Serializable]
    public class TRecordItem : IComparable
    {
        public int Rank { get; set; }
        public int SolidRows { get; set; }
        public int TotalShapes { get; set; }
        public int TotalScore { get; set; }
        public string PlayerName { get; set; }

        public int CompareTo(object obj)
        {
            TRecordItem item = obj as TRecordItem;

            if (item == null)
                throw new NullReferenceException();

            if (item.TotalScore < this.TotalScore)
                return -1;
            if (item.TotalScore > this.TotalScore)
                return 1;

            return 0;
        }
    }

    [Serializable]
    public class TRecordTable
    {
        private List<TRecordItem> recordTable = new List<TRecordItem>();
        public TRecordTable()
        {

        }

        // Метод добавляет новую запись в таблицу, при этом сокращая ее до maxItems записей и сортируя по очкам.
        public void AddRecord(TRecordItem item, int maxItems)
        {
            recordTable.Add(item);
            recordTable.Sort();

            recordTable = recordTable.Take(maxItems).ToList();

            for (int i = 0; i < recordTable.Count; i++)
                recordTable[i].Rank = (i + 1);
        }

        public ICollection<TRecordItem> GetItems()
        {
            return recordTable as ICollection<TRecordItem>;
        }

        // Сохраняет таблицу в файл.
        public void SaveToFile(string fileName)
        {
            try
            {
                using (FileStream fs = File.Create(fileName))
                {
                    BinaryFormatter bf = new BinaryFormatter();

                    bf.Serialize(fs, this);
                }
            }
            catch
            {
                // Перехватываем все исключения...
            }
        }

        // Загружает таблицу из файла.
        public static TRecordTable LoadFromFile(string fileName)
        {
            TRecordTable rt = null;

            using (FileStream fs = File.OpenRead(fileName))
            {
                BinaryFormatter bf = new BinaryFormatter();

                rt = (TRecordTable)bf.Deserialize(fs);
            }

            return rt;
        }
    }
}
