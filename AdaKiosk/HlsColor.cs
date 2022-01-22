using System;
using System.Windows.Media;

namespace AdaSimulation
{

    public class HlsColor
    {
        private double hue = 0;
        private double luminance = 0;
        private double saturation = 0;

        /// <summary>
        /// Constructs an instance of the class from the specified
        /// System.Drawing.Color
        /// </summary>
        /// <param name="c">The System.Drawing.Color to use to initialize the
        /// class</param>
        public HlsColor(Color c)
        {
            ToHLS(c.R, c.G, c.B);
        }

        /// <summary>
        /// Constructs an instance of the class with the specified hue, luminance
        /// and saturation values.
        /// </summary>
        /// <param name="hue">The Hue (between 0.0 and 360.0)</param>
        /// <param name="luminance">The Luminance (between 0.0 and 1.0)</param>
        /// <param name="saturation">The Saturation (between 0.0 and 1.0)</param>
        /// <exception cref="ArgumentOutOfRangeException">If any of the H,L,S
        /// values are out of the acceptable range (0.0-360.0 for Hue and 0.0-1.0
        /// for Luminance and Saturation)</exception>
        public HlsColor(double hue, double luminance, double saturation)
        {
            this.Hue = hue;
            this.Luminance = luminance;
            this.Saturation = saturation;
        }

        /// <summary>
        /// Constructs an instance of the class with the specified red, green and
        /// blue values.
        /// </summary>
        /// <param name="red">The red component.</param>
        /// <param name="green">The green component.</param>
        /// <param name="blue">The blue component.</param>
        public HlsColor(byte red, byte green, byte blue)
        {
            ToHLS(red, green, blue);
        }

        /// <summary>
        /// Constructs an instance of the class using the settings of another
        /// instance.
        /// </summary>
        /// <param name="HlsColor">The instance to clone.</param>
        public HlsColor(HlsColor hls)
        {
            this.luminance = hls.luminance;
            this.hue = hls.hue;
            this.saturation = hls.saturation;
        }

        /// <summary>
        /// Constructs a new instance of the class initialised to black.
        /// </summary>
        public HlsColor()
        {
        }

        /// <summary>
        /// Gets or sets the Luminance (0.0 to 1.0) of the colour.
        /// </summary>
        public double Luminance
        {
            get
            {
                return luminance;
            }
            set
            {
                luminance = Math.Max(0, Math.Min(1, value));
            }
        }

        /// <summary>
        /// Gets or sets the Hue (0.0 to 360.0) of the color.
        /// </summary>
        public double Hue
        {
            get
            {
                return hue;
            }
            set
            {
                hue = Math.Max(0, Math.Min(360, value));
            }
        }

        /// <summary>
        /// Gets or sets the Saturation (0.0 to 1.0) of the color.
        /// </summary>
        public double Saturation
        {
            get
            {
                return saturation;
            }
            set
            {
                saturation = Math.Max(0, Math.Min(1, value));
            }
        }

        /// <summary>
        /// Gets or sets the Color as a System.Drawing.Color instance
        /// </summary>
        public Color RgbColor
        {
            get
            {
                return ToRGB();
            }
            set
            {                
                ToHLS(value.R, value.G, value.B);
            }
        }

        /// <summary>
        /// Lightens the colour by the specified amount by modifying
        /// the luminance (for example, 0.2 would lighten the colour by 20%)
        /// </summary>
        public void Lighten(float percent)
        {
            luminance *= (1.0f + percent);
            if (luminance > 1.0f)
            {
                luminance = 1.0f;
            }
        }

        /// <summary>
        /// Darkens the colour by the specified amount by modifying
        /// the luminance (for example, 0.2 would darken the colour by 20%)
        /// </summary>
        public void Darken(float percent)
        {
            luminance *= (1 - percent);
        }

        private void ToHLS(byte red, byte green, byte blue)
        {
            byte minval = Math.Min(red, Math.Min(green, blue));
            byte maxval = Math.Max(red, Math.Max(green, blue));

            float mdiff = (float)(maxval - minval);
            float msum = (float)(maxval + minval);

            luminance = msum / 510.0f;

            if (maxval == minval)
            {
                saturation = 0.0f;
                hue = 0.0f;
            }
            else
            {
                float rnorm = (maxval - red) / mdiff;
                float gnorm = (maxval - green) / mdiff;
                float bnorm = (maxval - blue) / mdiff;

                saturation = (luminance <= 0.5f) ? (mdiff / msum) : (mdiff /
                 (510.0f - msum));

                if (red == maxval)
                {
                    hue = 60.0f * (6.0f + bnorm - gnorm);
                }
                if (green == maxval)
                {
                    hue = 60.0f * (2.0f + rnorm - bnorm);
                }
                if (blue == maxval)
                {
                    hue = 60.0f * (4.0f + gnorm - rnorm);
                }
                if (hue > 360.0f)
                {
                    hue = hue - 360.0f;
                }
            }
        }

        private Color ToRGB()
        {
            byte red, green, blue;
            if (saturation == 0.0)
            {
                red = (byte)(luminance * 255.0F);
                green = red;
                blue = red;
            }
            else
            {
                double rm1;
                double rm2;

                if (luminance <= 0.5f)
                {
                    rm2 = luminance + luminance * saturation;
                }
                else
                {
                    rm2 = luminance + saturation - luminance * saturation;
                }
                rm1 = 2.0f * luminance - rm2;
                red = ToRGB1(rm1, rm2, hue + 120.0f);
                green = ToRGB1(rm1, rm2, hue);
                blue = ToRGB1(rm1, rm2, hue - 120.0f);
            }
            return new Color() { A = 255, R = red, G = green, B = blue };
        }

        static private byte ToRGB1(double rm1, double rm2, double rh)
        {
            if (rh > 360.0f)
            {
                rh -= 360.0f;
            }
            else if (rh < 0.0f)
            {
                rh += 360.0f;
            }

            if (rh < 60.0f)
            {
                rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;
            }
            else if (rh < 180.0f)
            {
                rm1 = rm2;
            }
            else if (rh < 240.0f)
            {
                rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;
            }

            return (byte)(rm1 * 255);
        }

    }
}