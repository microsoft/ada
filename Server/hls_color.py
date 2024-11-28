# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.


class HlsColor:
    def __init__(self, hue, luminance, saturation):
        self.hue = hue
        self.luminance = luminance
        self.saturation = saturation

    def Lighten(self, percent):
        self.luminance *= 1.0 + percent
        if self.luminance > 1.0:
            self.luminance = 1.0

    def Darken(self, percent):
        self.luminance *= 1.0 - percent

    def ToHLS(self, red, green, blue):
        minval = min(red, min(green, blue))
        maxval = max(red, max(green, blue))

        mdiff = float(maxval - minval)
        msum = float(maxval + minval)

        self.luminance = msum / 510.0

        if maxval == minval:
            self.saturation = 0.0
            self.hue = 0.0
        else:
            rnorm = (maxval - red) / mdiff
            gnorm = (maxval - green) / mdiff
            bnorm = (maxval - blue) / mdiff

            if self.luminance <= 0.5:
                self.saturation = mdiff / msum
            else:
                self.saturation = mdiff / (510.0 - msum)

            if red == maxval:
                self.hue = 60.0 * (6.0 + bnorm - gnorm)

            if green == maxval:
                self.hue = 60.0 * (2.0 + rnorm - bnorm)

            if blue == maxval:
                self.hue = 60.0 * (4.0 + gnorm - rnorm)

            if self.hue > 360.0:
                self.hue = self.hue - 360.0

    def ToRGB(self):
        red = 0
        green = 0
        blue = 0

        if self.saturation == 0.0:
            red = int(self.luminance * 255)
            green = red
            blue = red
        else:
            rm1 = 0.0
            rm2 = 0.0

            if self.luminance <= 0.5:
                rm2 = self.luminance + self.luminance * self.saturation
            else:
                rm2 = self.luminance + self.saturation - self.luminance * self.saturation

            rm1 = 2.0 * self.luminance - rm2
            red = self.ToRGB1(rm1, rm2, self.hue + 120.0)
            green = self.ToRGB1(rm1, rm2, self.hue)
            blue = self.ToRGB1(rm1, rm2, self.hue - 120.0)

        return [red, green, blue]

    def ToRGB1(rm1, rm2, rh):
        if rh > 360.0:
            rh -= 360.0
        elif rh < 0.0:
            rh += 360.0

        if rh < 60.0:
            rm1 = rm1 + (rm2 - rm1) * rh / 60.0
        elif rh < 180.0:
            rm1 = rm2
        elif rh < 240.0:
            rm1 = rm1 + (rm2 - rm1) * (240.0 - rh) / 60.0

        return int(rm1 * 255)
