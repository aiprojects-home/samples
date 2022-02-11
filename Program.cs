using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Net.Http;
using System.Text.RegularExpressions;

namespace TIApp
{
    class Program
    {
        private static string m_strResponse = null; // internal cache for request result
        private const double m_Epsilon = 0.0001f;   // epsilon for zero-equality check

        static void  Main()
        {
            // Starting demo.
            ConvertDemo();

            // Waiting for keyboard input to prevent window to close (because of async call).
            Console.ReadLine();
        }

        /// <summary>
        /// Method sends a request to www.cbr.ru to retreive currency rate.
        /// </summary>
        /// <param name="c_type">Currency code</param>
        /// <param name="bUseCache">True, if no additional requests required.</param>
        /// <returns>Currency rate or zero if error</returns>
        static async Task<double> GetCurrencyRate(string c_type, bool bUseCache = true)
        {
            if (c_type == null)
            {
                // Prevent passing NULL.
                return 0;
            }

            string responseBody;

            if ((bUseCache == false) || (m_strResponse == null))
            {
                // First request or cache ignoring.
                try
                {
                    HttpClient client = new HttpClient();

                    responseBody = await client.GetStringAsync("http://www.cbr.ru/scripts/XML_daily.asp");
                    m_strResponse = responseBody;
                }
                catch
                {
                    // Bad request. Cache is invalid.
                    m_strResponse = null;
                    return 0;
                }
            } else
            {
                // Using cache.
                responseBody = m_strResponse;
            }

            string strPrefix = "<Valute ID=\"" + c_type + "\">";
            string strPostfix = "</Valute>";
            string strPattern = strPrefix + "<NumCode>(.*)</NumCode><CharCode>(.*)</CharCode><Nominal>(.*)</Nominal><Name>(.*)</Name><Value>(.*)</Value></Valute>";

            // Extracting part with currency we are interested in.

            int nStartPos = responseBody.IndexOf(strPrefix);

            if (nStartPos < 0)
            {
                return 0;
            };

            int nStopPos = responseBody.IndexOf(strPostfix, nStartPos);

            if (nStopPos < 0)
            {
                return 0;
            }

            responseBody = responseBody.Substring(nStartPos, nStopPos - nStartPos + strPostfix.Length);

            // Applying regular expression to extract values.

            Match result = Regex.Match(responseBody, strPattern, RegexOptions.IgnoreCase | RegexOptions.Singleline);

            if (!result.Success)
            {
                // Fail. No match.
                return 0;
            }

            // Parsing data. No additional check for brievity.

            double nominal = double.Parse(result.Groups[3].ToString());
            double value = double.Parse(result.Groups[5].ToString());

            return value / nominal;
        }

        /// <summary>
        /// Method converts one type of currency to another.
        /// </summary>
        /// <param name="from_type">Type of source currency</param>
        /// <param name="to_type">Type of destination currency</param>
        /// <param name="count">Initial amount to convert</param>
        /// <returns>Initial amount converted to destination type or zero if error</returns>
        static async Task<double> ConvertCurrency(string from_type, string to_type, double count)
        {

            // Getting rates of two currencies.
            double rate_from = await GetCurrencyRate(from_type);
            double rate_to = await GetCurrencyRate(to_type);

            if ( (System.Math.Abs(rate_from) <= m_Epsilon ) || (System.Math.Abs(rate_to) <= m_Epsilon))
            {
                // Something bad happened.
                return 0;
            }

            // Returning their ratio multiplied by initial count.
            double k = rate_from / rate_to * count;

            return k;
        }
        
        /// <summary>
        /// Just a demo method.
        /// </summary>
        /// <returns></returns>
        static async Task ConvertDemo()
        {
            // HUF = "R01135"
            // NOK = "R01535"
            // USD = "R01235"
            // EUR = "R01239"

            double result = await ConvertCurrency("R01535", "R01135", 1.0);

            if (System.Math.Abs(result) <= m_Epsilon)
            {
                Console.WriteLine("NOK : HUF. Conversion error!");
            }
            else
            {
                Console.WriteLine("1 NOK = {0:f4} HUF", result);
            };

            result = await ConvertCurrency("R01235", "R01239", 1.0);

            if (System.Math.Abs(result) <= m_Epsilon)
            {
                Console.WriteLine("USD : EUR. Conversion error!");
            }
            else
            {
                Console.WriteLine("1 USD = {0:f4} EUR", result);
            };

        }
    }
}
