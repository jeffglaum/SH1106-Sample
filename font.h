
#include "gfxfont.h"

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    void SetFont(const GFXfont *f);
    void DrawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
    void WriteChar(uint8_t c);
    
#ifdef	__cplusplus
}
#endif /* __cplusplus */

