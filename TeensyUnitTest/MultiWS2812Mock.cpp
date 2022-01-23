// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "MultiWS2812Mock.h"
#include "Color.h"
#include "Bitmap.h"
#include "TestWindow.h"

void MultiWS2812::show()
{
	if (window != nullptr && buffer != nullptr)
	{
		int w = ledsPerStrip;
		int h = numStrips;
		Bitmap bitmap(w, h);

		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				uint32_t* pixel = (uint32_t*)(buffer + (x * numStrips) + y);
				Color c = Color::from(*pixel);
				bitmap.SetPixel(x, y, c.r, c.g, c.b);
			}
		}
		window->DrawBitmap(bitmap);
	}
}