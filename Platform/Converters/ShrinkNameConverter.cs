﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml.Data;

namespace Platform.Converters
{
    class ShrinkNameConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            string name = (string)value;
            if (string.IsNullOrEmpty(name))
                return "";
            else if (name.Length > 6)
                return name.Substring(0, 6) + "...";
            else
                return name;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
