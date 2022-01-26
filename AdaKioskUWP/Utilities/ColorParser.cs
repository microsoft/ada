using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI;

namespace AdaKioskUWP.Utilities
{
    internal class ColorParser
    {
        internal static Color? TryParseColor(string color)
        {
            bool isPossibleKnowColor;
            bool isNumericColor;
            string trimmedColor = MatchColor(color, out isPossibleKnowColor, out isNumericColor);

            if (isPossibleKnowColor == false && isNumericColor == false)
            {
                throw new FormatException();
            }

            //Is it a number?
            if (isNumericColor)
            {
                return TryParseHexColor(trimmedColor);
            }
            else
            {
                foreach(var name in KnownColors)
                {
                    if (string.Compare(name.Name, trimmedColor, StringComparison.OrdinalIgnoreCase) == 0)
                    {
                        return FromUInt32(name.Value);
                    }
                }
                return null;
            }
        }
        internal static Color FromUInt32(uint argb)// internal legacy sRGB interface
        {
            Color c1 = new Color();
            c1.A = (byte)((argb & 0xff000000) >> 24);
            c1.R = (byte)((argb & 0x00ff0000) >> 16);
            c1.G = (byte)((argb & 0x0000ff00) >> 8);
            c1.B = (byte)(argb & 0x000000ff);            
            return c1;
        }

        private static Color? TryParseHexColor(string trimmedColor)
        {
            try
            {
                int a, r, g, b;
                a = 255;

                if (trimmedColor.Length > 7)
                {
                    a = ParseHexChar(trimmedColor[1]) * 16 + ParseHexChar(trimmedColor[2]);
                    r = ParseHexChar(trimmedColor[3]) * 16 + ParseHexChar(trimmedColor[4]);
                    g = ParseHexChar(trimmedColor[5]) * 16 + ParseHexChar(trimmedColor[6]);
                    b = ParseHexChar(trimmedColor[7]) * 16 + ParseHexChar(trimmedColor[8]);
                }
                else if (trimmedColor.Length > 5)
                {
                    r = ParseHexChar(trimmedColor[1]) * 16 + ParseHexChar(trimmedColor[2]);
                    g = ParseHexChar(trimmedColor[3]) * 16 + ParseHexChar(trimmedColor[4]);
                    b = ParseHexChar(trimmedColor[5]) * 16 + ParseHexChar(trimmedColor[6]);
                }
                else if (trimmedColor.Length > 4)
                {
                    a = ParseHexChar(trimmedColor[1]);
                    a = a + a * 16;
                    r = ParseHexChar(trimmedColor[2]);
                    r = r + r * 16;
                    g = ParseHexChar(trimmedColor[3]);
                    g = g + g * 16;
                    b = ParseHexChar(trimmedColor[4]);
                    b = b + b * 16;
                }
                else
                {
                    r = ParseHexChar(trimmedColor[1]);
                    r = r + r * 16;
                    g = ParseHexChar(trimmedColor[2]);
                    g = g + g * 16;
                    b = ParseHexChar(trimmedColor[3]);
                    b = b + b * 16;
                }

                return (Color.FromArgb((byte)a, (byte)r, (byte)g, (byte)b));
            }
            catch { }
            return null;
        }


        private const int s_zeroChar = (int)'0';
        private const int s_aLower = (int)'a';
        private const int s_aUpper = (int)'A';

        static private int ParseHexChar(char c)
        {
            int intChar = (int)c;

            if ((intChar >= s_zeroChar) && (intChar <= (s_zeroChar + 9)))
            {
                return (intChar - s_zeroChar);
            }

            if ((intChar >= s_aLower) && (intChar <= (s_aLower + 5)))
            {
                return (intChar - s_aLower + 10);
            }

            if ((intChar >= s_aUpper) && (intChar <= (s_aUpper + 5)))
            {
                return (intChar - s_aUpper + 10);
            }
            throw new FormatException();
        }

        static internal string MatchColor(string colorString, out bool isKnownColor, out bool isNumericColor)
        {
            string trimmedString = colorString.Trim();

            if (((trimmedString.Length == 4) ||
                (trimmedString.Length == 5) ||
                (trimmedString.Length == 7) ||
                (trimmedString.Length == 9)) &&
                (trimmedString[0] == '#'))
            {
                isNumericColor = true;
                isKnownColor = false;
                return trimmedString;
            }
            else
                isNumericColor = false;

            isKnownColor = true;
            return trimmedString;
        }

        internal struct ColorName
        {
            public string Name;
            public uint Value;

            public ColorName(string name, uint value) {
                this.Name = name;
                this.Value = value;
            }
        }

        internal static ColorName[] KnownColors = new ColorName[] {
            // We've reserved the value "1" as unknown.  If for some odd reason "1" is added to the
            // list, redefined UnknownColor
            new ColorName("AliceBlue", 0xFFF0F8FF),
            new ColorName("AntiqueWhite", 0xFFFAEBD7),
            new ColorName("Aqua", 0xFF00FFFF),
            new ColorName("Aquamarine", 0xFF7FFFD4),
            new ColorName("Azure", 0xFFF0FFFF),
            new ColorName("Beige", 0xFFF5F5DC),
            new ColorName("Bisque", 0xFFFFE4C4),
            new ColorName("Black", 0xFF000000),
            new ColorName("BlanchedAlmond", 0xFFFFEBCD),
            new ColorName("Blue", 0xFF0000FF),
            new ColorName("BlueViolet", 0xFF8A2BE2),
            new ColorName("Brown", 0xFFA52A2A),
            new ColorName("BurlyWood", 0xFFDEB887),
            new ColorName("CadetBlue", 0xFF5F9EA0),
            new ColorName("Chartreuse", 0xFF7FFF00),
            new ColorName("Chocolate", 0xFFD2691E),
            new ColorName("Coral", 0xFFFF7F50),
            new ColorName("CornflowerBlue", 0xFF6495ED),
            new ColorName("Cornsilk", 0xFFFFF8DC),
            new ColorName("Crimson", 0xFFDC143C),
            new ColorName("Cyan", 0xFF00FFFF),
            new ColorName("DarkBlue", 0xFF00008B),
            new ColorName("DarkCyan", 0xFF008B8B),
            new ColorName("DarkGoldenrod", 0xFFB8860B),
            new ColorName("DarkGray", 0xFFA9A9A9),
            new ColorName("DarkGreen", 0xFF006400),
            new ColorName("DarkKhaki", 0xFFBDB76B),
            new ColorName("DarkMagenta", 0xFF8B008B),
            new ColorName("DarkOliveGreen", 0xFF556B2F),
            new ColorName("DarkOrange", 0xFFFF8C00),
            new ColorName("DarkOrchid", 0xFF9932CC),
            new ColorName("DarkRed", 0xFF8B0000),
            new ColorName("DarkSalmon", 0xFFE9967A),
            new ColorName("DarkSeaGreen", 0xFF8FBC8F),
            new ColorName("DarkSlateBlue", 0xFF483D8B),
            new ColorName("DarkSlateGray", 0xFF2F4F4F),
            new ColorName("DarkTurquoise", 0xFF00CED1),
            new ColorName("DarkViolet", 0xFF9400D3),
            new ColorName("DeepPink", 0xFFFF1493),
            new ColorName("DeepSkyBlue", 0xFF00BFFF),
            new ColorName("DimGray", 0xFF696969),
            new ColorName("DodgerBlue", 0xFF1E90FF),
            new ColorName("Firebrick", 0xFFB22222),
            new ColorName("FloralWhite", 0xFFFFFAF0),
            new ColorName("ForestGreen", 0xFF228B22),
            new ColorName("Fuchsia", 0xFFFF00FF),
            new ColorName("Gainsboro", 0xFFDCDCDC),
            new ColorName("GhostWhite", 0xFFF8F8FF),
            new ColorName("Gold", 0xFFFFD700),
            new ColorName("Goldenrod", 0xFFDAA520),
            new ColorName("Gray", 0xFF808080),
            new ColorName("Green", 0xFF008000),
            new ColorName("GreenYellow", 0xFFADFF2F),
            new ColorName("Honeydew", 0xFFF0FFF0),
            new ColorName("HotPink", 0xFFFF69B4),
            new ColorName("IndianRed", 0xFFCD5C5C),
            new ColorName("Indigo", 0xFF4B0082),
            new ColorName("Ivory", 0xFFFFFFF0),
            new ColorName("Khaki", 0xFFF0E68C),
            new ColorName("Lavender", 0xFFE6E6FA),
            new ColorName("LavenderBlush", 0xFFFFF0F5),
            new ColorName("LawnGreen", 0xFF7CFC00),
            new ColorName("LemonChiffon", 0xFFFFFACD),
            new ColorName("LightBlue", 0xFFADD8E6),
            new ColorName("LightCoral", 0xFFF08080),
            new ColorName("LightCyan", 0xFFE0FFFF),
            new ColorName("LightGoldenrodYellow", 0xFFFAFAD2),
            new ColorName("LightGreen", 0xFF90EE90),
            new ColorName("LightGray", 0xFFD3D3D3),
            new ColorName("LightPink", 0xFFFFB6C1),
            new ColorName("LightSalmon", 0xFFFFA07A),
            new ColorName("LightSeaGreen", 0xFF20B2AA),
            new ColorName("LightSkyBlue", 0xFF87CEFA),
            new ColorName("LightSlateGray", 0xFF778899),
            new ColorName("LightSteelBlue", 0xFFB0C4DE),
            new ColorName("LightYellow", 0xFFFFFFE0),
            new ColorName("Lime", 0xFF00FF00),
            new ColorName("LimeGreen", 0xFF32CD32),
            new ColorName("Linen", 0xFFFAF0E6),
            new ColorName("Magenta", 0xFFFF00FF),
            new ColorName("Maroon", 0xFF800000),
            new ColorName("MediumAquamarine", 0xFF66CDAA),
            new ColorName("MediumBlue", 0xFF0000CD),
            new ColorName("MediumOrchid", 0xFFBA55D3),
            new ColorName("MediumPurple", 0xFF9370DB),
            new ColorName("MediumSeaGreen", 0xFF3CB371),
            new ColorName("MediumSlateBlue", 0xFF7B68EE),
            new ColorName("MediumSpringGreen", 0xFF00FA9A),
            new ColorName("MediumTurquoise", 0xFF48D1CC),
            new ColorName("MediumVioletRed", 0xFFC71585),
            new ColorName("MidnightBlue", 0xFF191970),
            new ColorName("MintCream", 0xFFF5FFFA),
            new ColorName("MistyRose", 0xFFFFE4E1),
            new ColorName("Moccasin", 0xFFFFE4B5),
            new ColorName("NavajoWhite", 0xFFFFDEAD),
            new ColorName("Navy", 0xFF000080),
            new ColorName("OldLace", 0xFFFDF5E6),
            new ColorName("Olive", 0xFF808000),
            new ColorName("OliveDrab", 0xFF6B8E23),
            new ColorName("Orange", 0xFFFFA500),
            new ColorName("OrangeRed", 0xFFFF4500),
            new ColorName("Orchid", 0xFFDA70D6),
            new ColorName("PaleGoldenrod", 0xFFEEE8AA),
            new ColorName("PaleGreen", 0xFF98FB98),
            new ColorName("PaleTurquoise", 0xFFAFEEEE),
            new ColorName("PaleVioletRed", 0xFFDB7093),
            new ColorName("PapayaWhip", 0xFFFFEFD5),
            new ColorName("PeachPuff", 0xFFFFDAB9),
            new ColorName("Peru", 0xFFCD853F),
            new ColorName("Pink", 0xFFFFC0CB),
            new ColorName("Plum", 0xFFDDA0DD),
            new ColorName("PowderBlue", 0xFFB0E0E6),
            new ColorName("Purple", 0xFF800080),
            new ColorName("Red", 0xFFFF0000),
            new ColorName("RosyBrown", 0xFFBC8F8F),
            new ColorName("RoyalBlue", 0xFF4169E1),
            new ColorName("SaddleBrown", 0xFF8B4513),
            new ColorName("Salmon", 0xFFFA8072),
            new ColorName("SandyBrown", 0xFFF4A460),
            new ColorName("SeaGreen", 0xFF2E8B57),
            new ColorName("SeaShell", 0xFFFFF5EE),
            new ColorName("Sienna", 0xFFA0522D),
            new ColorName("Silver", 0xFFC0C0C0),
            new ColorName("SkyBlue", 0xFF87CEEB),
            new ColorName("SlateBlue", 0xFF6A5ACD),
            new ColorName("SlateGray", 0xFF708090),
            new ColorName("Snow", 0xFFFFFAFA),
            new ColorName("SpringGreen", 0xFF00FF7F),
            new ColorName("SteelBlue", 0xFF4682B4),
            new ColorName("Tan", 0xFFD2B48C),
            new ColorName("Teal", 0xFF008080),
            new ColorName("Thistle", 0xFFD8BFD8),
            new ColorName("Tomato", 0xFFFF6347),
            new ColorName("Transparent", 0x00FFFFFF),
            new ColorName("Turquoise", 0xFF40E0D0),
            new ColorName("Violet", 0xFFEE82EE),
            new ColorName("Wheat", 0xFFF5DEB3),
            new ColorName("White", 0xFFFFFFFF),
            new ColorName("WhiteSmoke", 0xFFF5F5F5),
            new ColorName("Yellow", 0xFFFFFF00),
            new ColorName("YellowGreen", 0xFF9ACD32),
            new ColorName("UnknownColor", 0x00000001)
        };
    }
}
