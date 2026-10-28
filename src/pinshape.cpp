#include "stdafx.h"
#include "util.h"
#include "pinshape.h"
#include "resource.h"


PinShape::PinShape() : bmp(0), rgn(0)
{
    sz.cx = sz.cy = 1;
}


PinShape::~PinShape()
{
    DeleteObject(bmp);
    DeleteObject(rgn);
}


// Load the pin bitmap as a DIB section, so pixel colors are exact
// regardless of the display color depth.
static HBITMAP loadPinBmp()
{
    return HBITMAP(LoadImage(app.inst, MAKEINTRESOURCE(IDB_PIN),
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
}


bool PinShape::initImage(COLORREF clr)
{
    if (bmp) {
        DeleteObject(bmp);
        bmp = 0;
    }

    const COLORREF clrMap[][2] = {
        { Util::Clr::white,  Util::Clr::light(clr) },
        { Util::Clr::silver, clr                   },
        { Util::Clr::gray,   Util::Clr::dark(clr)  }
    };
    return (bmp = loadPinBmp())
        && Util::Gfx::remapBmpColors(bmp, clrMap, ARRAYSIZE(clrMap));
}


bool PinShape::initShape()
{
    if (rgn) {
        DeleteObject(rgn);
        rgn = 0;
        sz.cx = sz.cy = 1;
    }

    if (HBITMAP bmp = loadPinBmp()) {
        if (!Util::Gfx::getBmpSize(bmp, sz))
            sz.cx = sz.cy = 1;
        rgn = Util::Gfx::makeRegionFromBmp(bmp, RGB(255,0,255));
        DeleteObject(bmp);
    }

    return rgn != 0;
}
