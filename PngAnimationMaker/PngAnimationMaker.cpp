#define _CRT_SECURE_NO_WARNINGS
#define RAYGUI_TEXTSPLIT_MAX_ITEMS 10000
#define RAYGUI_TEXTSPLIT_MAX_TEXT_SIZE 100000
#include <iostream>
#include <raylib.h>
#include <string>
#define RAYGUI_IMPLEMENTATION
#define GUI_FILE_DIALOGS_IMPLEMENTATION
#include "gui_file_dialogs.h"
#include <vector>
#include <cmath>
#include <queue>

Color dark = { 45, 45, 45, 255 };
Color dark_gray = { 50, 51, 50, 255 };
Color linecolor = { 58, 58, 58, 255 };
Color right_dark = { 43, 43, 43, 255 };
Color bottom_dark = { 25, 26, 25, 255 };

int ScreenWidth = 1200;
int ScreenHeight = 600;

int scale_up = 0;
bool update_texture = false;
Color current_color = WHITE;
bool erasing = false;
bool picking = false;
bool snipping = false;
bool filling = false;
int tolerance = 0;
float originX = 0, originY = 0, recX = 0, recY = 0, realX = 0, realY = 0, realW = 0, realH = 0;
bool selected_area = false;
bool button_down = false;
bool in_dialog = false;
float timer = 0.0f;
int count = 1;
bool animating = false;
float alpha_amount = 1;
bool history = true;
int selected_action = -1, action_index, cache_action = -1;
bool update_cancel = false;
std::string actions = "";
std::vector<Image> image_cache;

bool PlayerButton(Rectangle bounds, const char* text, bool enabled)
{
    GuiState state = guiState;
    bool pressed = false;

    // Update control
    //--------------------------------------------------------------------
    if ((state != STATE_DISABLED) && !guiLocked)
    {
        Vector2 mousePoint = GetMousePosition();

        // Check button state
        if (CheckCollisionPointRec(mousePoint, bounds))
        {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) state = STATE_PRESSED;
            else state = STATE_FOCUSED;

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) pressed = true;
        }
    }
    //--------------------------------------------------------------------
    // 
    // Draw control
    //--------------------------------------------------------------------
    if (enabled) {
        DrawRectangleRounded(bounds, 0.5, 32, Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR + state * 5)), guiAlpha));
        GuiDrawText(text, GetTextBounds(BUTTON, bounds), GuiGetStyle(BUTTON, TEXT_ALIGNMENT), (state != STATE_PRESSED ? Fade(GetColor(GuiGetStyle(BUTTON, TEXT + (state * 3))), guiAlpha) : RAYWHITE));
    }
    else {
        DrawRectangleRounded(bounds, 0.5, 32, Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR + STATE_FOCUSED * 5)), guiAlpha));
        GuiDrawText(text, GetTextBounds(BUTTON, bounds), GuiGetStyle(BUTTON, TEXT_ALIGNMENT), RAYWHITE);
    }
    //------------------------------------------------------------------

    return pressed;
}

bool GuiButtonRound(Rectangle bounds, const char* text)
{
    GuiState state = guiState;
    bool pressed = false;

    // Update control
    //--------------------------------------------------------------------
    if ((state != STATE_DISABLED) && !guiLocked)
    {
        Vector2 mousePoint = GetMousePosition();

        // Check button state
        if (CheckCollisionPointRec(mousePoint, bounds))
        {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) state = STATE_PRESSED;
            else state = STATE_FOCUSED;

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) pressed = true;
        }
    }
    //--------------------------------------------------------------------
    // 
    // Draw control
    //--------------------------------------------------------------------
    DrawRectangleRounded(bounds, 0.5, 12, Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR + state * 5)), guiAlpha));
    GuiDrawText(text, GetTextBounds(BUTTON, bounds), GuiGetStyle(BUTTON, TEXT_ALIGNMENT), (state != STATE_PRESSED ? Fade(GetColor(GuiGetStyle(BUTTON, TEXT + (state * 3))), guiAlpha) : RAYWHITE));
    //------------------------------------------------------------------

    return pressed;
}

bool GuiTextBoxFix(Rectangle bounds, char* text, int textSize, bool editMode)
{
    GuiState state = guiState;
    bool pressed = false;
    int textWidth = GetTextWidth(text);
    Rectangle textBounds = GetTextBounds(TEXTBOX, bounds);
    int textAlignment = editMode && textWidth >= textBounds.width ? TEXT_ALIGN_RIGHT : GuiGetStyle(TEXTBOX, TEXT_ALIGNMENT);

    Rectangle cursor = {
        bounds.x + GuiGetStyle(TEXTBOX, TEXT_PADDING) + GetTextWidth(text) + 2,
        bounds.y + bounds.height / 2 - GuiGetStyle(DEFAULT, TEXT_SIZE),
        4,
        (float)GuiGetStyle(DEFAULT, TEXT_SIZE) * 2
    };

    if (cursor.height >= bounds.height) cursor.height = bounds.height - GuiGetStyle(TEXTBOX, BORDER_WIDTH) * 2;
    if (cursor.y < (bounds.y + GuiGetStyle(TEXTBOX, BORDER_WIDTH))) cursor.y = bounds.y + GuiGetStyle(TEXTBOX, BORDER_WIDTH);

    // Update control
    //--------------------------------------------------------------------
    if ((state != STATE_DISABLED) && !guiLocked)
    {
        Vector2 mousePoint = GetMousePosition();

        if (editMode)
        {
            state = STATE_PRESSED;

            int key = GetCharPressed();      // Returns codepoint as Unicode
            int keyCount = (int)strlen(text);
            int byteSize = 0;
            const char* textUTF8 = CodepointToUTF8(key, &byteSize);

            // Only allow keys in range [32..125]
            if ((keyCount + byteSize) < textSize)
            {
                float maxWidth = (bounds.width - (GuiGetStyle(TEXTBOX, TEXT_INNER_PADDING) * 2));

                if (key >= 32)
                {
                    for (int i = 0; i < byteSize; i++)
                    {
                        text[keyCount] = textUTF8[i];
                        keyCount++;
                    }

                    text[keyCount] = '\0';
                }
            }

            // Delete text
            if (keyCount > 0)
            {
                if (IsKeyPressed(KEY_BACKSPACE))
                {
                    while ((keyCount > 0) && ((text[--keyCount] & 0xc0) == 0x80));
                    text[keyCount] = '\0';
                }
            }

            if (IsKeyPressed(KEY_ENTER) || (!CheckCollisionPointRec(mousePoint, bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) pressed = true;

            // Check text alignment to position cursor properly
            if (textAlignment == TEXT_ALIGN_CENTER) cursor.x = bounds.x + GetTextWidth(text) / 2 + bounds.width / 2 + 1;
            else if (textAlignment == TEXT_ALIGN_RIGHT) cursor.x = bounds.x + bounds.width - GuiGetStyle(TEXTBOX, TEXT_INNER_PADDING) - GuiGetStyle(TEXTBOX, BORDER_WIDTH);
        }
        else
        {
            if (CheckCollisionPointRec(mousePoint, bounds))
            {
                state = STATE_FOCUSED;
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) pressed = true;
            }
        }
    }
    //--------------------------------------------------------------------
    // Draw control
    //--------------------------------------------------------------------
    if (state == STATE_PRESSED)
    {
        DrawRectangleRounded(bounds, 0.2, 64, Fade(GetColor(GuiGetStyle(TEXTBOX, BASE_COLOR_PRESSED)), guiAlpha));
    }
    else if (state == STATE_DISABLED)
    {
        DrawRectangleRounded(bounds, 0.2, 64, Fade(GetColor(GuiGetStyle(TEXTBOX, BASE_COLOR_DISABLED)), guiAlpha));
    }
    else {
        DrawRectangleRounded(bounds, 0.2, 64, Fade(GetColor(GuiGetStyle(TEXTBOX, BORDER + (state * 3))), guiAlpha));
    }

    // in case we edit and text does not fit in the textbox show right aligned and character clipped, slower but working
    while (textWidth >= textBounds.width && *text)
    {
        int bytes = 0;
        GetCodepoint(text, &bytes);
        text += bytes;
        textWidth = GetTextWidth(text);
    }
    GuiDrawText(text, textBounds, textAlignment, Fade(GetColor(GuiGetStyle(TEXTBOX, TEXT + (state * 3))), guiAlpha));

    // Draw cursor
    if (editMode) GuiDrawRectangle(cursor, 0, BLANK, Fade(GetColor(GuiGetStyle(TEXTBOX, BORDER_COLOR_PRESSED)), guiAlpha));
    //--------------------------------------------------------------------

    return pressed;
}

void GuiPanelModified(Rectangle bounds, const char* text)
{
#if !defined(RAYGUI_PANEL_BORDER_WIDTH)
#define RAYGUI_PANEL_BORDER_WIDTH   1
#endif

    GuiState state = guiState;

    // Text will be drawn as a header bar (if provided)
    Rectangle statusBar = { bounds.x, bounds.y, bounds.width, (float)RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT };
    if ((text != NULL) && (bounds.height < RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT * 2.0f)) bounds.height = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT * 2.0f;

    if (text != NULL)
    {
        // Move panel bounds after the header bar
        bounds.y += (float)RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT - 1;
        bounds.height -= (float)RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT + 1;
    }

    // Draw control
    //--------------------------------------------------------------------
    if (text != NULL) GuiStatusBar(statusBar, text);  // Draw panel header as status bar

    GuiDrawRectangle(bounds, RAYGUI_PANEL_BORDER_WIDTH, Fade(GetColor(GuiGetStyle(DEFAULT, (state == STATE_DISABLED) ? BORDER_COLOR_DISABLED : LINE_COLOR)), guiAlpha),
        Fade({ 43, 43, 43, 255 }, guiAlpha));
    //--------------------------------------------------------------------
}

bool GuiWindowBoxModified(Rectangle bounds, const char* title)
{
    // Window title bar height (including borders)
    // NOTE: This define is also used by GuiMessageBox() and GuiTextInputBox()
#if !defined(RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT)
#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT        24
#endif

//GuiState state = guiState;
    bool clicked = false;

    int statusBarHeight = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;

    Rectangle statusBar = { bounds.x, bounds.y, bounds.width, (float)statusBarHeight };
    if (bounds.height < statusBarHeight * 2.0f) bounds.height = statusBarHeight * 2.0f;

    Rectangle windowPanel = { bounds.x, bounds.y + (float)statusBarHeight - 1, bounds.width, bounds.height - (float)statusBarHeight + 1 };
    Rectangle closeButtonRec = { statusBar.x + statusBar.width - GuiGetStyle(STATUSBAR, BORDER_WIDTH) - 20,
                                 statusBar.y + statusBarHeight / 2.0f - 18.0f / 2.0f, 18, 18 };

    // Update control
    //--------------------------------------------------------------------
    // NOTE: Logic is directly managed by button
    //--------------------------------------------------------------------

    // Draw control
    //--------------------------------------------------------------------
    GuiStatusBar(statusBar, title); // Draw window header as status bar
    GuiPanelModified(windowPanel, NULL);    // Draw window base

    // Draw window close button
    int tempBorderWidth = GuiGetStyle(BUTTON, BORDER_WIDTH);
    int tempTextAlignment = GuiGetStyle(BUTTON, TEXT_ALIGNMENT);
    GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
#if defined(RAYGUI_NO_ICONS)
    clicked = GuiButtonRound(closeButtonRec, "x");
#else
    clicked = GuiButtonRound(closeButtonRec, GuiIconText(ICON_CROSS_SMALL, NULL));
#endif
    GuiSetStyle(BUTTON, BORDER_WIDTH, tempBorderWidth);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, tempTextAlignment);
    //--------------------------------------------------------------------

    return clicked;
}

void GuiLineModified(Rectangle bounds, const char* text)
{
#if !defined(RAYGUI_LINE_ORIGIN_SIZE)
#define RAYGUI_LINE_MARGIN_TEXT  12
#endif
#if !defined(RAYGUI_LINE_TEXT_PADDING)
#define RAYGUI_LINE_TEXT_PADDING  4
#endif

    GuiState state = guiState;

    Color color = Fade(GetColor(GuiGetStyle(DEFAULT, (state == STATE_DISABLED) ? BORDER_COLOR_DISABLED : LINE_COLOR)), guiAlpha);
    Color color_text = { 179, 179, 179, 255 };

    // Draw control
    //--------------------------------------------------------------------
    if (text == NULL) GuiDrawRectangle(RAYGUI_CLITERAL(Rectangle) { bounds.x, bounds.y + bounds.height / 2, bounds.width, 1 }, 0, BLANK, color);
    else
    {
        Rectangle textBounds = { 0 };
        textBounds.width = (float)GetTextWidth(text);
        textBounds.height = bounds.height;
        textBounds.x = bounds.x + RAYGUI_LINE_MARGIN_TEXT;
        textBounds.y = bounds.y;

        // Draw line with embedded text label: "--- text --------------"
        GuiDrawRectangle(RAYGUI_CLITERAL(Rectangle) { bounds.x, bounds.y + bounds.height / 2, RAYGUI_LINE_MARGIN_TEXT - RAYGUI_LINE_TEXT_PADDING, 1 }, 0, BLANK, color);
        GuiDrawText(text, textBounds, TEXT_ALIGN_LEFT, color_text);
        GuiDrawRectangle(RAYGUI_CLITERAL(Rectangle) { bounds.x + 12 + textBounds.width + 4, bounds.y + bounds.height / 2, bounds.width - textBounds.width - RAYGUI_LINE_MARGIN_TEXT - RAYGUI_LINE_TEXT_PADDING, 1 }, 0, BLANK, color);
    }
    //--------------------------------------------------------------------
}

void GuiGroupBoxModified(Rectangle bounds, const char* text)
{
#if !defined(RAYGUI_GROUPBOX_LINE_THICK)
#define RAYGUI_GROUPBOX_LINE_THICK     1
#endif

    GuiState state = guiState;

    // Draw control
    //--------------------------------------------------------------------
    GuiDrawRectangle(RAYGUI_CLITERAL(Rectangle) { bounds.x, bounds.y, RAYGUI_GROUPBOX_LINE_THICK, bounds.height }, 0, BLANK, Fade(GetColor(GuiGetStyle(DEFAULT, (state == STATE_DISABLED) ? BORDER_COLOR_DISABLED : LINE_COLOR)), guiAlpha));
    GuiDrawRectangle(RAYGUI_CLITERAL(Rectangle) { bounds.x, bounds.y + bounds.height - 1, bounds.width, RAYGUI_GROUPBOX_LINE_THICK }, 0, BLANK, Fade(GetColor(GuiGetStyle(DEFAULT, (state == STATE_DISABLED) ? BORDER_COLOR_DISABLED : LINE_COLOR)), guiAlpha));
    GuiDrawRectangle(RAYGUI_CLITERAL(Rectangle) { bounds.x + bounds.width - 1, bounds.y, RAYGUI_GROUPBOX_LINE_THICK, bounds.height }, 0, BLANK, Fade(GetColor(GuiGetStyle(DEFAULT, (state == STATE_DISABLED) ? BORDER_COLOR_DISABLED : LINE_COLOR)), guiAlpha));

    GuiLineModified(RAYGUI_CLITERAL(Rectangle) { bounds.x, bounds.y - GuiGetStyle(DEFAULT, TEXT_SIZE) / 2, bounds.width, (float)GuiGetStyle(DEFAULT, TEXT_SIZE) }, text);
    //--------------------------------------------------------------------
}

void DrawRectangleFloat(float x, float y, float width, float height, Color color) {
    DrawRectangleLinesEx({ x, y, width, height }, 1, color);
}

void DrawRectangleLinesExNeg(Vector2 position, Vector2 size, float lineThick, Color color)
{
    float x1 = position.x;
    float y1 = position.y;
    float x2 = position.x + size.x;
    float y2 = position.y;
    float x3 = position.x + size.x;
    float y3 = position.y + size.y;
    float x4 = position.x;
    float y4 = position.y + size.y;

    // Ensure that the lines do not intersect in the center.
    DrawLineEx({ x1, y1 }, { x2, y2 }, lineThick, color);
    DrawLineEx({ x2, y2 }, { x3, y3 }, lineThick, color);
    DrawLineEx({ x3, y3 }, { x4, y3 }, lineThick, color);
    DrawLineEx({ x4, y3 }, { x1, y1 }, lineThick, color);

    // Draw the remaining two lines.
    DrawLineEx({ x1, y1 }, { x4, y4 }, lineThick, color);
    DrawLineEx({ x2, y2 }, { x3, y4 }, lineThick, color);
}

Rectangle fixNegativeRect(Rectangle rect)
{
    Rectangle newRect = rect;

    if (newRect.width < 0)
    {
        newRect.x += newRect.width;
        newRect.width = -newRect.width;
    }

    if (newRect.height < 0)
    {
        newRect.y += newRect.height;
        newRect.height = -newRect.height;
    }

    return newRect;
}

void clear_frames(std::vector<Image>& frames, std::string& frame_panel) {
    frame_panel = "";
    if (!frames.empty()) {
        for (Image& img : frames)
            UnloadImage(img);
        frames.clear();
    }
}

void delete_frame(std::vector<Image>& frames, std::string& frame_panel, int index) {
    frame_panel = "";
    std::vector<Image> new_list;
    int i = 0;
    std::string pnl_temp = "";
    for (const Image& img : frames) {
        if (i != index) {
            pnl_temp += (new_list.empty() ? "" : "\n") + (std::string)"frame " + std::to_string(new_list.size() + 1);
            new_list.push_back(img);
        }
        else
            UnloadImage(img);
        i++;
    }
    frame_panel = pnl_temp;
    frames = new_list;
}

void get_image_frames(Image image, std::vector<Image>& frames, std::string& frame_panel) {
    int frame_count = (image.height > 0 && image.width > 0 ? image.height / image.width : 0);
    int frame_offset = 0;
    if (frame_count > 0 && image.height >= image.width) {
        for (int i = 0; i < frame_count; i++) {
            frame_panel += (frames.empty() ? "" : "\n") + (std::string)"frame " + std::to_string(frames.size() + 1);
            Image temp_image = ImageCopy(image);
            ImageCrop(&temp_image, { 0, (float)frame_offset, (float)image.width, (float)image.width});
            frames.push_back(temp_image);
            frame_offset += image.width;
        }
    }
}

void get_gif_frames(Image image, std::vector<Image>& frame_list, std::string& frame_panel, int gif_frames) {
    int frame_offset = 0;
    if (gif_frames > 0 && image.width >= image.height) {
        Texture2D txt_temp = LoadTextureFromImage(image);
        for (int i = 0; i < gif_frames; i++) {
            frame_panel += (frame_list.empty() ? "" : "\n") + (std::string)"frame " + std::to_string(frame_list.size() + 1);
            frame_offset = image.width * image.height * 4 * i;
            UpdateTexture(txt_temp, ((unsigned char*)image.data) + frame_offset);
            Image img_temp = LoadImageFromTexture(txt_temp);
            frame_list.push_back(img_temp);
        }
        UnloadTexture(txt_temp);
    }
}

bool ColorEquals(Color color1, Color color2) {
    return color1.r == color2.r && color1.g == color2.g && color1.b == color2.b && color1.a == color2.a;
}

float ColorDistance(Color a, Color b) {
    float dr = (float)a.r - (float)b.r;
    float dg = (float)a.g - (float)b.g;
    float db = (float)a.b - (float)b.b;
    float da = (float)a.a - (float)b.a;
    return std::sqrt(dr * dr + dg * dg + db * db + da * da);
}

void fill_area(Image& image, Color color, Vector2 pos, float tolerance) {

    if (ColorEquals(color, current_color))
        return;

    // Create a queue of positions to fill
    std::queue<Vector2> fillQueue;

    // Add the clicked position to the queue
    fillQueue.push(pos);

    // While there are still positions to fill
    while (!fillQueue.empty()) {
        // Get the next position to fill
        Vector2 fillPos = fillQueue.front();
        fillQueue.pop();

        // If the position is within the bounds of the image and the pixel at
        // that position is within the tolerance range of the clicked color,
        // replace it with the replacement color
        if (fillPos.x >= 0 && fillPos.x < image.width &&
            fillPos.y >= 0 && fillPos.y < image.height &&
            ColorDistance(GetImageColor(image, fillPos.x, fillPos.y), color) <= tolerance)
        {
            ImageDrawPixel(&image, fillPos.x, fillPos.y, current_color);

            // Add the neighboring positions to the queue
            fillQueue.push({ fillPos.x + 1, fillPos.y });
            fillQueue.push({ fillPos.x - 1, fillPos.y });
            fillQueue.push({ fillPos.x, fillPos.y + 1 });
            fillQueue.push({ fillPos.x, fillPos.y - 1 });
        }
    }
}

void draw_pixel_grid(Image image, std::vector<Image>& images, int index, float dimensions, Vector2 pos) {
    float pixelSize = dimensions / image.width;
    if (selected_area) //draw selected area
        DrawRectangleLinesExNeg({ originX, originY },{ recX, recY }, 1, RAYWHITE);
    for (int i = 0; i < image.width; i++) {
        for (int j = 0; j < image.width; j++) {
            float posX = pos.x + (i * pixelSize);
            float posY = pos.y + (j * pixelSize);
            if (CheckCollisionPointRec(GetMousePosition(), {(float)posX, (float)posY, (float)pixelSize, (float)pixelSize})) {
                std::string pos_text = std::to_string(i) + ":" + std::to_string(j);
                GuiLabel({ 325, (float)ScreenHeight - 97, 20, 20 }, pos_text.c_str());
                if (!snipping) //drawing selected area, dont draw pixel rectangle 
                    DrawRectangleFloat(posX, posY, pixelSize, pixelSize, RAYWHITE);
                if (erasing) //drawing eraser icon
                    GuiDrawIcon(ICON_RUBBER, GetMouseX() - 3, GetMouseY() - 30, 2, RAYWHITE);
                else if (picking) //drawing color picker icon
                    GuiDrawIcon(ICON_COLOR_PICKER, GetMouseX() - 3, GetMouseY() - 30, 2, RAYWHITE);
                else if (snipping) //drawing cropping icon
                    GuiDrawIcon(ICON_CROP, GetMouseX() - 3, GetMouseY() - 30, 2, RAYWHITE);
                else if (filling)
                    GuiDrawIcon(ICON_COLOR_BUCKET, GetMouseX() - 3, GetMouseY() - 30, 2, RAYWHITE);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { //get selected area origin or draw it
                    if (!selected_area || (snipping && !button_down && !filling)) {
                        originX = posX;
                        originY = posY;
                        realX = i;
                        realY = j;
                    }
                    if (filling && !in_dialog) {
                        fill_area(image, GetImageColor(image, i, j), { (float)i, (float)j }, tolerance);
                        update_texture = true;
                    }
                    else if (selected_area) {
                        Rectangle rec1 = fixNegativeRect({ originX, originY, recX, recY });
                        if (CheckCollisionPointRec(GetMousePosition(), rec1) && !snipping && !picking && !filling) {
                            Rectangle rec2 = fixNegativeRect({ realX, realY, realW, realH });
                            ImageDrawRectangle(&image, rec2.x, rec2.y, rec2.width, rec2.height, (!erasing ? current_color : BLANK));
                            update_texture = true;
                        }
                        selected_area = false;
                    }
                }
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !in_dialog) {
                    button_down = true;
                    if (!picking && !snipping && !filling) {
                        ImageDrawPixel(&image, i, j, (!erasing ? current_color : BLANK));
                        images[index] = image;
                        update_texture = true;
                    }
                    else if (picking) {
                        Color clr = GetImageColor(image, i, j);
                        if (clr.a != 0)
                            current_color = clr;
                        picking = false;
                    }
                    else if (snipping) {
                        recX = pixelSize * ((posX - originX) / pixelSize) + (pixelSize * ((posX - originX) / pixelSize) < 0 ? 0 : pixelSize);
                        recY = pixelSize * ((posY - originY) / pixelSize) + (pixelSize * ((posY - originY) / pixelSize) < 0 ? 0 : pixelSize);
                        selected_area = true;
                        realW = i - realX + (pixelSize * ((posX - originX) / pixelSize) < 0 ? 0 : 1);
                        realH = j - realY + (pixelSize * ((posY - originY) / pixelSize) < 0 ? 0 : 1);
                    }
                }
                else
                    button_down = false;
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !in_dialog) {
                    if (selected_action < image_cache.size() - 1 && !image_cache.empty()) { //if undone actions and edit, remove extras
                        std::vector<Image> imgs_temp;
                        actions = "";
                        int n = 0;
                        for (Image& img : image_cache) {
                            if (n < selected_action) {
                                actions += (imgs_temp.empty() ? "Action " + std::to_string(1) : "\nAction " + std::to_string(imgs_temp.size() + 1));
                                imgs_temp.push_back(img);
                            }
                            else
                                UnloadImage(img);
                            n++;
                        }
                        image_cache.clear();
                        image_cache = imgs_temp;
                    }
                    if (image_cache.size() >= 50) { //action history handling
                        std::vector<Image> temp_imgs;
                        bool first = true;
                        actions = "";
                        for (Image& img : image_cache) { //copy all except the first image
                            if (first) {
                                first = false;
                                UnloadImage(img);
                            }
                            else {
                                actions += (temp_imgs.empty() ? "Action " + std::to_string(1) : "\nAction " + std::to_string(temp_imgs.size() + 1));
                                temp_imgs.push_back(img);
                            }
                        }
                        actions += "\nAction 50";
                        temp_imgs.push_back(ImageCopy(image));
                        image_cache.clear();
                        image_cache = temp_imgs;
                        selected_action = 49;
                        update_cancel = true;
                    }
                    else { //normal copying of the edited image
                        actions += (image_cache.empty() ? "Action " + std::to_string(1) : "\nAction " + std::to_string(image_cache.size() + 1));
                        image_cache.push_back(ImageCopy(image));
                        selected_action = image_cache.size() - 1;
                        update_cancel = true;
                    }
                }
            }
        }
    }
}

void export_animation(std::vector<Image> images, const char* path) {
    int max_width = 0;
    int max_height = 0;
    int height_sum = 0;
    int image_count = 0;
    int resize_count = 0;
    for (const Image& image : images) {
        if (image.width > max_width) {
            max_width = image.width;
            resize_count++;
        }
        if (image.height > max_height) {
            max_height = image.height;
            resize_count++;
        }
    }
    Image animation = GenImageColor(max_width, max_height * images.size(), BLANK);
    for (const Image& image : images) {
        Image temp_image = ImageCopy(image);
        if (resize_count > 2)
            ImageResize(&temp_image, max_width, max_height);
        ImageDraw(&animation, temp_image, { 0, 0, (float)max_width, (float)max_height }, { 0, (float)(image_count * max_height), (float)max_width, (float)max_height }, WHITE);
        UnloadImage(temp_image);
        image_count++;
    }
    ExportImage(animation, path);
    UnloadImage(animation);
}

int main()
{
    Image icon = LoadImage("ico_umbr.png");

    InitWindow(1200, 600, "MCSprite");
    SetWindowMinSize(1200, 600); 
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowState(FLAG_VSYNC_HINT);
    SetWindowIcon(icon);
    GuiLoadStyle("midnight.rgs");

    Image strip = { 0 };
    int gif_frames = 0;

    Texture temp_txtr = { 0 };
    bool unsupported_file_dialog = false;
    bool file_dialog = false;
    bool tolerance_dialog = false;
    float _tolerance = 0;
    int index, selected = -1, preview_cache = -1, newheight = 0;
    std::string frames = "";
    std::vector<Image> imageFrames;

    bool empty_image_dialog = false;
    int pixels_int = 1;

    std::vector<Image> animationFrames;
    std::string frames_a = "";
    int index_a, selected_a = -1, preview_cache_a = -1;

    //generate the alpha texture backround
    Image img_temp_ = GenImageGradientH(400, 400, BLANK, BLANK);
    for (int y = 0; y < 400; y += 8)
    {
        for (int x = 0; x < 400; x += 8)
        {
            if ((x / 8 + y / 8) % 2 == 0)
                ImageDrawRectangle(&img_temp_, x, y, 8, 8, { 127, 127, 127, 255 });
        }
    }
    Texture alpha_texture = LoadTextureFromImage(img_temp_);
    UnloadImage(img_temp_);

    while (!WindowShouldClose()) {

        if (IsWindowResized()) {
            if (!IsWindowFullscreen()) {
                ScreenWidth = GetScreenWidth();
                ScreenHeight = GetScreenHeight();
            }
            else {
                ScreenWidth = GetMonitorWidth(GetCurrentMonitor());
                ScreenHeight = GetMonitorHeight(GetCurrentMonitor());
            }
            if ((GetScreenHeight() - 600) < GetScreenWidth() - 1200) {
                scale_up = (GetScreenHeight() - 600);
            }
            else
                scale_up = 0;
        }

        if (animating && !animationFrames.empty()) {
            if (selected_a != -1 && selected_a != animationFrames.size())
                selected_a = count;
            timer += GetFrameTime();
            if (timer >= 0.1f)
            {
                timer = 0.0f;
                count++;
            }
            if (selected_a != -1 && selected_a == animationFrames.size()) {
                count = 0;
                selected_a = 0;
            }
        }
        else if (animating && animationFrames.empty() || selected_a == -1)
            animating = false;

        ClearBackground(dark);

        //Dummy Rectangles//
        DrawRectangle(320, ScreenHeight - 55, ScreenWidth - 635, 60, right_dark);
        DrawRectangle((float)(ScreenWidth - (400 + scale_up)) / 2, (float)(ScreenHeight - (400 + scale_up)) / 2, (float)400 + scale_up, (float)400 + scale_up, (selected == -1 || imageFrames.empty() ? bottom_dark : WHITE));
        DrawRectangle((float)ScreenWidth - 320, 0, 320, (float)ScreenHeight, right_dark);
        DrawRectangle(0, 0, 320, (float)ScreenHeight, right_dark);
        DrawRectangle(ScreenWidth - 320, 393, 320, 35, { 38, 38, 38, 255 });
        //----------------//

        if (IsKeyPressed(KEY_F11)) {
            if (!IsWindowFullscreen()) {
                MaximizeWindow();
                ToggleFullscreen();
            }
            else
                ToggleFullscreen();
        }

        //------------------------------------------------//
        //---------------Left Side Widgets----------------//
        //------------------------------------------------//

        //Image Frames
        if (GuiButtonRound({ 10, 10, 140, 30 }, GuiIconText(ICON_FILE_OPEN, "Open Image")) && !unsupported_file_dialog && !empty_image_dialog) {
            file_dialog = !IsWindowFullscreen();
            if (IsWindowFullscreen())
                ToggleFullscreen();
        }
        GuiGroupBoxModified({ 10, 60, 140, (float)ScreenHeight - 230 }, "Image Frames");
        selected = GuiListView({ 15, 70, 130, (float)ScreenHeight - 245 }, (frames == "" ? NULL : frames.c_str()), &index, selected);
        GuiGroupBoxModified({ 10, (float)ScreenHeight - 160, 140, 152 }, "Actions");
        if (GuiButtonRound({ 15, (float)ScreenHeight - 150, 130, 30 }, "delete frame") && selected != -1 && !imageFrames.empty()) {
            int indx_temp = index;
            delete_frame(imageFrames, frames, selected);
            selected -= 1;
            index = indx_temp;
        }
        if (GuiButtonRound({ 15, (float)ScreenHeight - 115, 130, 30 }, "clear frames") && !imageFrames.empty()) {
            clear_frames(imageFrames, frames);
            selected = -1;
        }
        if (GuiButtonRound({ 15, (float)ScreenHeight - 80, 130, 30 }, "include frame") && selected != -1 && !imageFrames.empty()) {
            frames_a += (!animationFrames.empty() ? "\nframe " : "frame ") + std::to_string(animationFrames.size() + 1);
            animationFrames.push_back(ImageCopy(imageFrames[selected]));
        }
        if (GuiButtonRound({ 15, (float)ScreenHeight - 45, 130, 30 }, "include all") && !imageFrames.empty()) {
            for (const Image img_frame : imageFrames) {
                frames_a += (!animationFrames.empty() ? "\nframe " : "frame ") + std::to_string(animationFrames.size() + 1);
                animationFrames.push_back(ImageCopy(img_frame));
            }
            update_texture = true;
        }
        if (selected != -1 && !imageFrames.empty()) {
            if (preview_cache != selected || ScreenHeight != newheight || update_texture) {
                UnloadTexture(temp_txtr);
                temp_txtr = LoadTextureFromImage(imageFrames[selected]);
                temp_txtr.width = 400 + scale_up;
                temp_txtr.height = 400 + scale_up;
                alpha_texture.width = 400 + scale_up;
                alpha_texture.height = 400 + scale_up;
                update_texture = false;
                selected_area = false;
            }
            DrawTexture(alpha_texture, (ScreenWidth - alpha_texture.width) / 2, (ScreenHeight - alpha_texture.height) / 2, WHITE);
            DrawTexture(temp_txtr, (ScreenWidth - temp_txtr.width) / 2, (ScreenHeight - temp_txtr.height) / 2, WHITE);
            if (CheckCollisionPointRec(GetMousePosition(), { (float)(ScreenWidth - temp_txtr.width) / 2, (float)(ScreenHeight - temp_txtr.height) / 2, (float)temp_txtr.width, (float)temp_txtr.height }) || selected_area)
                draw_pixel_grid(imageFrames[selected], imageFrames, selected, temp_txtr.width, { (float)((ScreenWidth - temp_txtr.width) / 2), (float)((ScreenHeight - temp_txtr.height) / 2) });
        }

        //Animation Frames
        GuiGroupBoxModified({ 170, 60, 140, (float)ScreenHeight - 230 }, "Animation Frames");
        selected_a = GuiListView({ 175, 70, 130, (float)ScreenHeight - 245 }, (frames_a == "" ? NULL : frames_a.c_str()), &index_a, selected_a);
        if (selected_a != -1 && !animationFrames.empty()) {
            if (preview_cache_a != selected_a || ScreenHeight != newheight || update_texture) {
                UnloadTexture(temp_txtr);
                temp_txtr = LoadTextureFromImage(animationFrames[selected_a]);
                temp_txtr.width = 400 + scale_up;
                temp_txtr.height = 400 + scale_up;
                update_texture = false;
                selected_area = false;
            }
            DrawTexture(temp_txtr, (ScreenWidth - temp_txtr.width) / 2, (ScreenHeight - temp_txtr.height) / 2, WHITE);
        }
        GuiGroupBoxModified({ 170, (float)ScreenHeight - 160, 140, 152 }, "Actions");
        if (GuiButtonRound({ 175, (float)ScreenHeight - 150, 130, 30 }, "delete frame") && selected_a != -1 && !animationFrames.empty()) {
            delete_frame(animationFrames, frames_a, selected_a);
            selected_a -= 1;
        }
        if (GuiButtonRound({ 175, (float)ScreenHeight - 115, 130, 30 }, "clear frames") && !animationFrames.empty()) {
            clear_frames(animationFrames, frames_a);
            selected_a = -1;
        }
        if (GuiButtonRound({ 175, (float)ScreenHeight - 80, 130, 30 }, "move up") && selected_a > 0 && !animationFrames.empty()) {
            const Image temp = animationFrames[selected_a - 1];
            animationFrames[selected_a - 1] = animationFrames[selected_a];
            animationFrames[selected_a] = temp;
            selected_a -= 1;
        }
        if (GuiButtonRound({ 175, (float)ScreenHeight - 45, 130, 30 }, "move down") && animationFrames.size() - 1 >= selected_a + 1 && !animationFrames.empty()) {
            const Image temp = animationFrames[selected_a + 1];
            animationFrames[selected_a + 1] = animationFrames[selected_a];
            animationFrames[selected_a] = temp;
            selected_a += 1;
        }
        if (selected != -1) {
            std::string bounds_text = "[" + std::to_string(imageFrames[selected].width) + "x" + std::to_string(imageFrames[selected].height) + "]";
            GuiLabel({ 325, (float)ScreenHeight - 77, 20, 20 }, bounds_text.c_str());
        }

        //action cache clearing
        if ((preview_cache != selected || selected_a != -1) && !image_cache.empty()) {
            for (const Image& img : image_cache)
                UnloadImage(img);
            image_cache.clear();
            actions = "";
        }

        if (selected_action != cache_action && !update_cancel) {
            UnloadImage(imageFrames[selected]);
            imageFrames[selected] = ImageCopy(image_cache[selected_action]);
            update_texture = true;
        }

        //caches and updating
        if (preview_cache != selected) {
            selected_a = -1;
            animating = false;
        }
        else if (preview_cache_a != selected_a)
            selected = -1;
        preview_cache_a = selected_a;
        preview_cache = selected;
        newheight = ScreenHeight;
        cache_action = selected_action;
        update_cancel = false;

        if (GuiButtonRound({ 168, 10, 140, 30 }, GuiIconText(ICON_TEXT_NOTES, "Blank Image")) && !in_dialog) empty_image_dialog = true;

        //------------------------------------------------//
        //---------------Right Side Widgets---------------//
        //------------------------------------------------//

        //color picker and image tools
        GuiGroupBoxModified({ (float)ScreenWidth - 315, 10, 310, 315 }, "Color Picker");
        current_color = GuiColorPicker({ (float)ScreenWidth - 306, 22, 270, 270 }, "", current_color);
        alpha_amount = GuiColorBarAlpha({ (float)ScreenWidth - 306, 299, 295, 20 }, NULL, alpha_amount); current_color.a = alpha_amount * 255;
        GuiGroupBoxModified({ (float)ScreenWidth - 315, 334, 310, 52 }, "Image Tools");
        if (GuiButtonRound({ (float)ScreenWidth - 306, 344, 35, 35 }, "#23#")) { erasing = false; picking = false; snipping = false; filling = false; }
        if (GuiButtonRound({ (float)ScreenWidth - 266, 344, 35, 35 }, "#28#")) { erasing = true; picking = false; snipping = false; filling = false; }
        if (GuiButtonRound({ (float)ScreenWidth - 226, 344, 35, 35 }, "#27#")) { erasing = false; picking = true; snipping = false; filling = false; }
        if (GuiButtonRound({ (float)ScreenWidth - 186, 344, 35, 35 }, "#36#")) { erasing = false; picking = false; snipping = true; filling = false; }
        if (GuiButtonRound({ (float)ScreenWidth - 146, 344, 35, 35 }, "#29#")) { erasing = false; picking = false; snipping = false; filling = true; }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(GetMousePosition(), { (float)ScreenWidth - 300, 398, 120, 30 }))
                history = true;
            else if (CheckCollisionPointRec(GetMousePosition(), { (float)ScreenWidth - 140, 398, 120, 30 }))
                history = false;
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z) && selected_action > 0)
            selected_action--;
        else if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Y) && image_cache.size() - 1 > selected_action)
            selected_action++;

        //action history
        if (history) {
            DrawRectangleRounded({ (float)ScreenWidth - 300, 398, 120, 55 }, 0.5, 12, right_dark);
            selected_action = GuiListView({ (float)ScreenWidth - 315, 435, 310, (float)ScreenHeight - 440 }, (actions == "" ? NULL : actions.c_str()), &action_index, selected_action);
            if (selected_action == -1 && actions != "" && !image_cache.empty())
                selected_action = image_cache.size() - 1;
        }
        else if (!history) {
            DrawRectangleRounded({ (float)ScreenWidth - 140, 398, 120, 55 }, 0.5, 12, right_dark);
        }

        GuiLabel({ (float)ScreenWidth - 115, 388, 120, 50 }, "Color Pallete");
        GuiLabel({ (float)ScreenWidth - 280, 388, 120, 50 }, "Action History");

        //------------------------------------------------//
        //-----------------Image Preview------------------//
        //------------------------------------------------//

        GuiGroupBoxModified({ (float)(ScreenWidth - (410 + scale_up)) / 2, (float)(ScreenHeight - (420 + scale_up)) / 2, (float)410 + scale_up, (float)415 + scale_up }, "Image Preview");

        //------------------------------------------------//
        //-------------------Bottom Bar-------------------//
        //------------------------------------------------//

        if (PlayerButton({ (float)(ScreenWidth - 34) / 2, (float)ScreenHeight - 45, 35, 35 }, (!animating ? "#131#" : "#132#"), selected_a != -1 && !animationFrames.empty())) {
            if (selected_a != -1 && !animationFrames.empty()) {
                animating = !animating;
                count = (selected_a != -1 ? count = selected_a : count = 0);
            }
        }
        if (PlayerButton({ (float)(ScreenWidth - 129) / 2, (float)ScreenHeight - 45, 34, 34 }, "#129#", selected_a != -1 && !animationFrames.empty())) {
            if (selected_a > 0 && !animationFrames.empty())
                selected_a -= 1;
        }
        if (PlayerButton({ (float)(ScreenWidth + 60) / 2, (float)ScreenHeight - 45, 35, 35 }, "#134#", selected_a != -1 && !animationFrames.empty())) {
            if (selected_a != -1 && selected_a < animationFrames.size() - 1 && !animationFrames.empty())
                selected_a += 1;
        }

        //lines//
        GuiLine({ 0, 47, 320, 0 }, NULL);
        DrawLine( 320, 0, 320, (float)ScreenHeight, linecolor);
        DrawLine( 160, 47, 160, (float)ScreenHeight, linecolor);
        DrawLine( 320, ScreenHeight - 55, ScreenWidth - 320, ScreenHeight - 55, linecolor);
        //GuiLine({ (float)GetScreenWidth() - 170, 58, 170, 0}, NULL);
        DrawLine((float)ScreenWidth - 320, 0, (float)ScreenWidth - 320, (float)ScreenHeight, linecolor);
        DrawLine(ScreenWidth - 320, 393, ScreenWidth, 393, linecolor);
        //-----//

        //------------------------------------------------//
        //------------Bucket Tolerance Dialog-------------//
        //------------------------------------------------//

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && CheckCollisionPointRec(GetMousePosition(), { (float)ScreenWidth - 146, 344, 35, 35 }) && !in_dialog)
                tolerance_dialog = true;
        if (tolerance_dialog) {
            if (GuiWindowBoxModified({ (float)(ScreenWidth - 250) / 2, (float)(ScreenHeight - 170) / 2,  250, 170 }, "Bucket fill tolerance")) {
                tolerance_dialog = false;
            }
            float tolerance_ = GuiSlider({ (float)(ScreenWidth - 200) / 2, (float)(ScreenHeight - 35) / 2, 200, 35 }, "", "", _tolerance, 0, 50);
            _tolerance = tolerance_;
            tolerance = (int)tolerance_;
            GuiDrawText(std::to_string(tolerance).c_str(),{ (float)(((ScreenWidth - 52.5 + (tolerance != 100 ? tolerance >= 10 ? 47.5 : 50 : 45))) / 2), (float)(ScreenHeight + 45) / 2, 200, 55 }, 0, RAYWHITE);
            in_dialog = true;
        }

        //------------------------------------------------//
        //--------------Invalid File Dialog---------------//
        //------------------------------------------------//

        if (unsupported_file_dialog) {
            if (GuiWindowBoxModified({ (float)(ScreenWidth - 350) / 2, (float)(ScreenHeight - 200) / 2, 350, 200 }, "#205# Invalid file format")) {
                unsupported_file_dialog = false;
            }
            if (GuiButtonRound({ (float)(ScreenWidth - 45) / 2, (float)(ScreenHeight + 100) / 2, 45, 30 }, "ok")) {
                unsupported_file_dialog = false;
            }
            GuiDrawText("The selected file must be a PNG image or GIF", { (float)(ScreenWidth - 250) / 2, (float)(ScreenHeight - 25) / 2 }, 10, RAYWHITE);
            in_dialog = true;
        }

        //------------------------------------------------//
        //---------------Blank Image Dialog---------------//
        //------------------------------------------------//

        if (empty_image_dialog) {
            in_dialog = true;
            if (GuiWindowBoxModified({ (float)(ScreenWidth - 350) / 2, (float)(ScreenHeight - 200) / 2, 350, 200 }, "#176# Blank Image")) {
                empty_image_dialog = false;
            }
            float pixels = GuiSlider({ (float)(ScreenWidth - 200) / 2, (float)(ScreenHeight - 100) / 2, 200, 30 }, "", "", pixels_int, 1, 64);
            pixels_int = (int)pixels;
            std::string tempS = std::to_string(pixels_int) + " X " + std::to_string(pixels_int);
            GuiDrawText(tempS.c_str(), { (float)(ScreenWidth - (pixels >= 10 ? 45 : 30)) / 2, (float)(ScreenHeight - 25) / 2 }, 3, RAYWHITE);
            if (GuiButtonRound({ (float)(ScreenWidth - 240) / 2, (float)(ScreenHeight + 100) / 2, 100, 35 }, "add")) {
                empty_image_dialog = false;
                Image img_blank = GenImageGradientH(pixels_int, pixels_int, BLANK, BLANK);
                animating = false;
                if (frames == "") {
                    imageFrames.push_back(img_blank);
                    frames += "frame 1";
                }
                else {
                    imageFrames.push_back(img_blank);
                    frames += "\nframe " + std::to_string(imageFrames.size());
                }
                selected = imageFrames.size() - 1;
            }
            if (GuiButtonRound({ (float)(ScreenWidth + 30) / 2, (float)(ScreenHeight + 100) / 2, 100, 35 }, "cancel")) {
                empty_image_dialog = false;
            }
        }

        //------------------------------------------------//
        //--------------File Dialog Drawing---------------//
        //------------------------------------------------//

        if (file_dialog) {
            char file_name[512] = { 0 };
            in_dialog = true;
            int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load texture file", file_name, "*.png;*.gif", "Image Files (*.png, *.gif)");
            if (result == 1) {
                if (IsFileExtension(file_name, ".png")) {
                    UnloadImage(strip);
                    strip = LoadImage(file_name);
                    get_image_frames(strip, imageFrames, frames);
                    selected = imageFrames.size() - 1;
                    file_dialog = false;
                }
                else if (IsFileExtension(file_name, ".gif")) {
                    UnloadImage(strip);
                    strip = LoadImageAnim(file_name, &gif_frames);
                    get_gif_frames(strip, imageFrames, frames, gif_frames);
                    selected = imageFrames.size() - 1;
                    file_dialog = false;
                }
            }
            if (result >= 0 && result != 1) {
                file_dialog = false;
                    unsupported_file_dialog = true;
            }
        }

        if (IsFileDropped()) {
            FilePathList files = LoadDroppedFiles();
            if (count == 1) {
                if (IsFileExtension(*files.paths, ".png")) {
                    UnloadImage(strip);
                    strip = LoadImage(*files.paths);
                    get_image_frames(strip, imageFrames, frames);
                    selected = imageFrames.size() - 1;
                }
                else if (IsFileExtension(*files.paths, ".gif")) {
                    UnloadImage(strip);
                    strip = LoadImageAnim(*files.paths, &gif_frames);
                    get_gif_frames(strip, imageFrames, frames, gif_frames);
                    selected = imageFrames.size() - 1;
                }
                else
                    unsupported_file_dialog = true;
                UnloadDroppedFiles(files);
            }
            else
                UnloadDroppedFiles(files);
        }

        if (IsKeyPressed(KEY_ENTER) && !animationFrames.empty()) {
            if (IsWindowFullscreen()) {
                ToggleFullscreen();
            }
            else {
                char file_location[1024] = { 0 };
                int result = GuiFileDialog(DIALOG_SAVE_FILE, "Save animation strip image...", file_location, "*.png", "Animation strip (*.png)");
                if (result == 1) {
                    export_animation(animationFrames, file_location);
                }
            }
        }

        if (!file_dialog && !unsupported_file_dialog && !empty_image_dialog && !tolerance_dialog && in_dialog) in_dialog = false;

        BeginDrawing();
        EndDrawing();
    }

    UnloadTexture(alpha_texture);
    UnloadImage(icon);
    UnloadImage(strip);
    UnloadTexture(temp_txtr);
    for (Image& img_t : imageFrames)
        UnloadImage(img_t);
    for (Image& img_t_ : animationFrames)
        UnloadImage(img_t_);
    for (Image& _img : image_cache)
        UnloadImage(_img);

    CloseWindow();

    return 0;
}