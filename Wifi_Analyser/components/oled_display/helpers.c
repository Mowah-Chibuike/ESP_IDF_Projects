#include "display_driver.h"
#include "i2c_comms.h"


uint8_t DD_FRAMEBUFFER[DD_PAGES][DD_COLUMNS] = {0};

void dd_set_pixel(int x, int y, bool state)
{
    int column = x;
    int page = y / DD_PAGES;
    int row = y % (sizeof(uint8_t) * 8);

    if (x < DD_WIDTH && y < DD_HEIGHT)
    {
        if (state)
            DD_FRAMEBUFFER[page][column] |= (1 << row);
        else
            DD_FRAMEBUFFER[page][column] &= ~(1 << row);
    }
}

uint8_t get_pixel(int x, int y)
{
     if (x < 0 || x >= DD_WIDTH || y < 0 || y >= DD_HEIGHT)
        return 0;
    
    int column = x;
    int page = y / DD_PAGES;
    int row = y % (sizeof(uint8_t) * 8);

    return((DD_FRAMEBUFFER[page][column] & (1 << row)));
}

void dd_clear()
{
    for (int i = 0; i < DD_WIDTH; i++)
    {
        for (int j = 0; j < DD_HEIGHT; j++)
        {
            dd_set_pixel(i, j, false);
        }
        
    }
    
}

void dd_draw_bitmap(int x, int y, int width, int height, const uint8_t *bitmap)
{
    int size = (width * height) >> 3;

    int originalX = x;
    int originalY = y;

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            uint8_t mask = (0x80 >> j);
            bool pixel_state = (bitmap[i] & mask) != 0;

            dd_set_pixel(x, y, pixel_state);

            x++;

            if ((x - originalX) == width)
            {
                y++;
                x = originalX;
            }

            if ((y - originalY) >= height)
                return;
        }
    }
}

void dd_draw_hline(int y)
{
    for (int i = 0; i < DD_WIDTH; i++)
    {
        dd_set_pixel(i, y, true);
    }
     
}

void dd_draw_rectangle(int x, int y, int width, int height)
{
    for (int i = y; i < y + height; i++)
    {
        for (int j = x; j < x + width; j++)
        {
            dd_set_pixel(j, i, true);
        }
    }
}

void dd_render_text(int x, int y, font_size_t size, char *text)
{
    const uint8_t *drawChar;
    const  uint8_t *font;

    int printX = x;
    int printY = y;

    int target;
    int font_width;
    int font_height;

    if (y >= DD_HEIGHT)
        return;
    
    switch (size)
    {
        case SIZE16: {
            font = CGA16;
            font_width = font_height = 16;
            break;
        }

        case SIZE8:
        default: {
            font = CGA8;
            font_width = font_height = 8;
            break;
        }
    }

    for (int i = 0; i < strlen(text); i++)
    {
        if (printX + font_width >= DD_WIDTH)
        {
            // printX = 0;
            // printY += font_height;
            break;
        }

        if (y >= DD_HEIGHT)
        return;

        target = ((font_width * font_height) >> 3) * (text[i] - ' ');
        drawChar = font + target;

        dd_draw_bitmap(printX, printY, font_width, font_height, drawChar);
        printX += font_width;  
    }
}

void print_framebuffer()
{
    for (int y = 0; y < DD_HEIGHT; y++)
    {
        for (int x = 0; x < DD_WIDTH; x++)
        {
            uint8_t pixel = get_pixel(x, y);
            if (pixel != 0)
                putchar('1');
            else
                putchar('0');
        }
        putchar('\n');
    }
}

void update_oled()
{
    for (int i = 0; i < DD_PAGES; i++)
    {
        ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_SET_PAGE_ADDRESS_0 + i, 0));
        ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_SET_LOWER_COL, 0));
        ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_SET_UPPER_COL, 0 ));
        ESP_ERROR_CHECK(i2c_send_data_to_dsp(DD_FRAMEBUFFER[i], 128));
    }
}

