/*******************************************************************************************
*
*   Window Font Atlas
*
*   MODULE USAGE:
*       #define GUI_WINDOW_FONT_ATLAS_IMPLEMENTATION
*       #include "gui_window_font_atlas.h"
*
*       INIT: GuiWindowFontAtlasState state = InitGuiWindowFontAtlas();
*       DRAW: GuiWindowFontAtlas(&state);
*
*   LICENSE: Propietary License
*
*   Copyright (c) 2023 raylib technologies. All Rights Reserved.
*
*   Unauthorized copying of this file, via any medium is strictly prohibited
*   This project is proprietary and confidential unless the owner allows
*   usage in any other form by expresely written permission.
*
**********************************************************************************************/

#include "raylib.h"

// WARNING: raygui implementation is expected to be defined before including this header
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"

#ifndef GUI_WINDOW_FONT_ATLAS_H
#define GUI_WINDOW_FONT_ATLAS_H

typedef struct {
    Vector2 anchor;
    
    bool windowActive;

    bool btnLoadFontPressed;
    bool btnLoadCharsetPressed;
    bool fontGenSizeEditMode;
    int fontGenSizeValue;

    bool btnExportFontAtlasPressed;
    bool btnCropAtlasPressed;

    bool selectWhiteRecActive;
    bool compressImageDataActive;
    bool compressRecDataActive;
    bool compressGlyphDataActive;

    int selectedCharset;

    // Custom state variables (depend on development software)
    // NOTE: This variables should be added manually if required
    Texture2D texFont;
    Rectangle fontWhiteRec;

    int *externalCodepointList;        // External charset codepoints loaded from UTF-8 file
    unsigned int externalCodepointListCount;

} GuiWindowFontAtlasState;

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
GuiWindowFontAtlasState InitGuiWindowFontAtlas(void);
void GuiWindowFontAtlas(GuiWindowFontAtlasState *state);

#ifdef __cplusplus
}
#endif

#endif // GUI_WINDOW_FONT_ATLAS_H

/***********************************************************************************
*
*   GUI_WINDOW_FONT_ATLAS IMPLEMENTATION
*
************************************************************************************/
#if defined(GUI_WINDOW_FONT_ATLAS_IMPLEMENTATION)

#include "raygui.h"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
extern Rectangle texShapesRec;

// Basic charset (95 codepoints)
static const char *charsetBasic = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
// Default charset: ISO-8859-15 (213 codepoints)
static const char *charsetDefault = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~¡¢£€¥Š§š©ª«¬®¯°±²³Žµ¶·ž¹º»ŒœŸ¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ";

static Rectangle fontAtlasRec = { 0 };
static Vector2 fontAtlasPosition = { 0 };
static Vector2 prevFontAtlasPosition = { 0 };
static Vector2 fontAtlasOffset = { 0 };
static float fontAtlasScale = 1.0f;
static bool panningMode = false;
static Rectangle fontWhiteRecScreen = { 0 };
static Vector2 fontWhiteRecStartPos = { 0 };
static bool prevSelectWhiteRecActive = false;
static int prevFontGenSizeValue = 10;
static int prevSelectedCharset = 0;

// Custom font variables
// NOTE: They have to be global to be used bys tyle export functions
static Font customFont = { 0 };             // Custom font
static bool customFontLoaded = false;       // Custom font loaded flag (from font file or style file)
static char inFontFileName[512] = { 0 };    // Input font file name (required for font reloading on atlas regeneration)

static int *codepointList = NULL;           // Custom codepoint list
static int codepointListCount = 0;          // Custom codepoint list count

//----------------------------------------------------------------------------------
// Internal Module Functions Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
GuiWindowFontAtlasState InitGuiWindowFontAtlas(void)
{
    GuiWindowFontAtlasState state = { 0 };

    state.anchor = (Vector2){ 12, 48 };
    
    state.windowActive = false;

    state.btnLoadFontPressed = false;
    state.btnLoadCharsetPressed = false;
    state.fontGenSizeEditMode = false;
    state.fontGenSizeValue = 10;
    state.btnExportFontAtlasPressed = false;
    state.btnCropAtlasPressed = false;
    state.selectWhiteRecActive = false;

    state.compressImageDataActive = true;
    state.compressRecDataActive = true;
    state.compressGlyphDataActive = true;

    // Custom variables initialization
    state.texFont = (Texture2D){ 0 };
    state.fontWhiteRec = texShapesRec;
    state.selectedCharset = 0;
    state.externalCodepointList = NULL;
    state.externalCodepointListCount = 0;

    codepointList = LoadCodepoints(charsetBasic, &codepointListCount);

    return state;
}

void GuiWindowFontAtlas(GuiWindowFontAtlasState *state)
{
    if (state->windowActive)
    {
        // Update logic
        //--------------------------------------------------------------------------------------------------
        // Reload font and generate new atlas at new size when required
        if ((inFontFileName[0] != '\0') &&                 // Check an external font file is provided (not internal custom one)
            !state->fontGenSizeEditMode &&                          // Check the spinner text editing has finished
            (prevFontGenSizeValue != state->fontGenSizeValue))      // Check selected size actually changed
        {
            UnloadFont(customFont);
            customFont = LoadFontEx(inFontFileName, state->fontGenSizeValue, codepointList, codepointListCount);
            GuiSetFont(customFont);
            customFontLoaded = true;
        }

        Vector2 mousePosition = GetMousePosition();

        if (IsKeyPressed(KEY_SPACE)) state->selectWhiteRecActive = !state->selectWhiteRecActive;

        if (!prevSelectWhiteRecActive && state->selectWhiteRecActive)
        {
            fontWhiteRecScreen.x = fontAtlasRec.x + state->fontWhiteRec.x*fontAtlasScale;
            fontWhiteRecScreen.y = fontAtlasRec.y + state->fontWhiteRec.y*fontAtlasScale,
                fontWhiteRecScreen.width = state->fontWhiteRec.width*fontAtlasScale;
            fontWhiteRecScreen.height = state->fontWhiteRec.height*fontAtlasScale;
        }

        if (state->selectWhiteRecActive && CheckCollisionPointRec(mousePosition, (Rectangle){ state->anchor.x, state->anchor.y + 64, 724, 532 - 64 }))
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                fontWhiteRecStartPos = mousePosition;

                fontWhiteRecScreen.x = mousePosition.x;
                fontWhiteRecScreen.y = mousePosition.y;
            }
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            {
                fontWhiteRecScreen.width = mousePosition.x - fontWhiteRecStartPos.x;
                fontWhiteRecScreen.height = mousePosition.y - fontWhiteRecStartPos.y;

                // Take care of rectangles drawn in different directions
                if (fontWhiteRecScreen.width < 0)
                {
                    fontWhiteRecScreen.x = mousePosition.x;
                    fontWhiteRecScreen.width *= -1;
                }

                if (fontWhiteRecScreen.height < 0)
                {
                    fontWhiteRecScreen.y = mousePosition.y;
                    fontWhiteRecScreen.height *= -1;
                }
            }

            state->fontWhiteRec.x = (fontWhiteRecScreen.x - fontAtlasPosition.x - state->texFont.width*fontAtlasScale/2)/fontAtlasScale + state->texFont.width;
            state->fontWhiteRec.y = (fontWhiteRecScreen.y - fontAtlasPosition.y - state->texFont.height*fontAtlasScale/2)/fontAtlasScale + state->texFont.height;
            state->fontWhiteRec.width = fontWhiteRecScreen.width/fontAtlasScale;
            state->fontWhiteRec.height = fontWhiteRecScreen.height/fontAtlasScale;
            if (state->fontWhiteRec.x < 0) state->fontWhiteRec.x = 0;
            if (state->fontWhiteRec.y < 0) state->fontWhiteRec.y = 0;

            if (IsKeyPressed(KEY_ENTER))
            {
                state->selectWhiteRecActive = false;
                prevSelectWhiteRecActive = false;
            }
        }
        else
        {
            if ((state->fontWhiteRec.width <= 0) || (state->fontWhiteRec.height <= 0)) state->fontWhiteRec = (Rectangle){ 0 };

            fontAtlasScale += GetMouseWheelMove();
            if (fontAtlasScale < 1.0f) fontAtlasScale = 1.0f;
            else if (fontAtlasScale > 16.0f) fontAtlasScale = 16.0f;

            // Calculate font atlas rectangle (considering transformations)
            fontAtlasRec = (Rectangle){ fontAtlasPosition.x - state->texFont.width*fontAtlasScale/2,
                fontAtlasPosition.y - state->texFont.height*fontAtlasScale/2,
                state->texFont.width*fontAtlasScale, state->texFont.height*fontAtlasScale };

            // Font atlas panning with mouse logic
            if (CheckCollisionPointRec(GetMousePosition(), fontAtlasRec))
            {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    panningMode = true;
                    fontAtlasOffset = GetMousePosition();
                    prevFontAtlasPosition = fontAtlasPosition;
                }
            }
            if (panningMode)
            {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                {
                    fontAtlasPosition.x = prevFontAtlasPosition.x + (GetMouseX() - fontAtlasOffset.x);
                    fontAtlasPosition.y = prevFontAtlasPosition.y + (GetMouseY() - fontAtlasOffset.y);
                }

                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) panningMode = false;
            }

            if (IsKeyPressed(KEY_F))
            {
                fontAtlasScale = 1.0f;
                fontAtlasPosition.x = state->anchor.x + 724/2;
                fontAtlasPosition.y = state->anchor.y + 532/2;
                prevFontAtlasPosition = fontAtlasPosition;

                prevSelectWhiteRecActive = false; 
                state->selectWhiteRecActive = false;

                prevFontGenSizeValue = state->fontGenSizeValue;
            }

            if (prevSelectedCharset != state->selectedCharset)
            {
                if (state->selectedCharset != 2) UnloadCodepoints(codepointList);

                if (state->selectedCharset == 0) codepointList = LoadCodepoints(charsetBasic, &codepointListCount);
                else if (state->selectedCharset == 1) codepointList = LoadCodepoints(charsetDefault, &codepointListCount);
                else if (state->selectedCharset == 2)
                {
                    if (state->externalCodepointList != NULL)
                    {
                        codepointList = state->externalCodepointList;
                        codepointListCount = state->externalCodepointListCount;
                    }
                }
            }
        }
        //----------------------------------------------------------------------------------------------------------------------

        // Draw window logic
        //--------------------------------------------------------------------------------------------------
        state->windowActive = !GuiWindowBox((Rectangle){ state->anchor.x, state->anchor.y, 724, 532 }, "#30# Font Atlas Generation");

        // White rectangle selection border
        if (state->selectWhiteRecActive) DrawRectangleLinesEx((Rectangle){ state->anchor.x, state->anchor.y + 64, 724, 532 - 64 }, 4, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_PRESSED)));

        // Draw font atlas view
        BeginScissorMode(state->anchor.x + 1, state->anchor.y + 24 + 40, 724 - 2, 532 - 65);
            DrawRectangleRec(fontAtlasRec, BLACK);
            DrawRectangleLinesEx(fontAtlasRec, 1.0f, RED);
            DrawTexturePro(state->texFont, (Rectangle){ 0, 0, state->texFont.width, state->texFont.height }, fontAtlasRec, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

            if (state->selectWhiteRecActive)
            {
                DrawRectangleLinesEx(fontWhiteRecScreen, 1.0f, RED);

                // Draw values for convenience
                DrawTextEx(GuiGetFont(), TextFormat("[%i, %i]", (int)state->fontWhiteRec.x, (int)state->fontWhiteRec.y), 
                    (Vector2){ fontWhiteRecScreen.x - 20, fontWhiteRecScreen.y - 20 }, GuiGetStyle(DEFAULT, TEXT_SIZE), 
                    GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
                DrawTextEx(GuiGetFont(), TextFormat("[%i, %i]", (int)state->fontWhiteRec.width, (int)state->fontWhiteRec.height),
                    (Vector2){ fontWhiteRecScreen.x + fontWhiteRecScreen.width - 20, fontWhiteRecScreen.y + fontWhiteRecScreen.height + 20 }, 
                    GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
            }
            else
            {
                DrawRectangleRec((Rectangle){ 
                    fontAtlasRec.x + state->fontWhiteRec.x*fontAtlasScale, 
                        fontAtlasRec.y + state->fontWhiteRec.y*fontAtlasScale,
                        state->fontWhiteRec.width*fontAtlasScale, state->fontWhiteRec.height*fontAtlasScale }, 
                    GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
            }
        EndScissorMode();

        GuiLine((Rectangle){ state->anchor.x + 0, state->anchor.y + 24 + 40 - 2, 724, 2 }, NULL);
        
        GuiEnableTooltip();
        GuiSetTooltip("Load font file");
        state->btnLoadFontPressed = GuiButton((Rectangle){ state->anchor.x + 12, state->anchor.y + 32, 24, 24 }, "#30#");

        if (!FileExists(inFontFileName)) GuiDisable();
        prevFontGenSizeValue = state->fontGenSizeValue;
        if (GuiSpinner((Rectangle){ state->anchor.x + 108, state->anchor.y + 32, 96, 24 }, "Gen Size: ", &state->fontGenSizeValue, 0, 100, state->fontGenSizeEditMode)) state->fontGenSizeEditMode = !state->fontGenSizeEditMode;
        
        GuiSetTooltip("Regenerate font atlas");
        if (GuiButton((Rectangle){ state->anchor.x + 210, state->anchor.y + 32, 80, 24 }, "#142#Regen"))
        {
            // Load font file
            Font tempFont = LoadFontEx(inFontFileName, state->fontGenSizeValue, codepointList, codepointListCount);

            if (tempFont.texture.id > 0)
            {
                if (customFontLoaded) UnloadFont(customFont);   // Unload previously loaded font
                customFont = tempFont;

                GuiSetFont(customFont);
                customFontLoaded = true;

                // Reset shapes texture and rectangle
                SetShapesTexture((Texture2D){ 0 }, (Rectangle){ 0 });
            }
        }
        GuiEnable();

        GuiSetTooltip("Save font atlas image");
        if (GuiButton((Rectangle){ state->anchor.x + 210 + 80 + 4, state->anchor.y + 32, 24, 24 }, "#12#"))
        {
            Image image = LoadImageFromTexture(state->texFont);
            ExportImage(image, "font_file.png");
            UnloadImage(image);
        }

        DrawLine(state->anchor.x + 294 + 24 + 12, state->anchor.y + 24, state->anchor.x + 294 + 24 + 12, state->anchor.y + 24 + 40, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));

        GuiSetTooltip("Load font charset file");
        state->btnLoadCharsetPressed = GuiButton((Rectangle){ state->anchor.x + 340, state->anchor.y + 32, 24, 24 }, "#31#");

        if (!FileExists(inFontFileName)) GuiDisable();
        prevSelectedCharset = state->selectedCharset;
        GuiSetTooltip("Select font charset");
        GuiLabel((Rectangle){ state->anchor.x + 342 + 38, state->anchor.y + 32, 60, 24 }, "Charset: ");
        GuiComboBox((Rectangle){ state->anchor.x + 378 + 56, state->anchor.y + 32, 128, 24 }, (state->externalCodepointList != NULL)? "Basic;ISO-8859-15;Custom" : "Basic;ISO-8859-15", &state->selectedCharset);
        GuiEnable();

        DrawLine(state->anchor.x + 574, state->anchor.y + 24, state->anchor.x + 574, state->anchor.y + 24 + 40, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));

        GuiSetTooltip("Toggle shapes rectangle selection (SPACE)");
        prevSelectWhiteRecActive = state->selectWhiteRecActive;
        GuiLabel((Rectangle){ state->anchor.x + 578 + 8, state->anchor.y + 32, 74, 24 }, "Shapes rec: ");
        GuiToggle((Rectangle){ state->anchor.x + 578 + 82, state->anchor.y + 32, 24, 24 }, "#80#", &state->selectWhiteRecActive);
        GuiSetTooltip("Clear shapes rectangle");
        if (GuiButton((Rectangle){ state->anchor.x + 578 + 82 + 24 + 4, state->anchor.y + 32, 24, 24 }, "#79#"))
        {
            state->fontWhiteRec = (Rectangle){ 0 };

            // Reset shapes texture and rectangle
            SetShapesTexture((Texture2D){ 0 }, (Rectangle){ 0 });
        }

        /*
        GuiSetTooltip("Font atlas height crop (%2)");
        if (GuiButton((Rectangle){ state->anchor.x + 312, state->anchor.y + 32, 24, 24 }, "#38#"))
        {
            // Crop atlas texture by half vertically
            // WARNING: This is a hack to avoid textures too big, GenImageFontAtlas() calculates size very conservatively
            Image image = LoadImageFromTexture(state->texFont);
            ImageCrop(&image, (Rectangle){ 0, 0, state->texFont.width, state->texFont.height/2 });
            UnloadTexture(state->texFont);
            state->texFont = LoadTextureFromImage(image);
            UnloadImage(image);
        }
        */
        GuiSetTooltip(NULL);

        //GuiToggle((Rectangle){ state->anchor.x + 360, state->anchor.y + 32, 24, 24 }, "#178#", &state->compressImageDataActive);
        //GuiToggle((Rectangle){ state->anchor.x + 360 + 24 + 4, state->anchor.y + 32, 24, 24 }, "#179#", &state->compressRecDataActive);
        //GuiToggle((Rectangle){ state->anchor.x + 360 + 48 + 8, state->anchor.y + 32, 24, 24 }, "#180#", &state->compressGlyphDataActive);

        GuiStatusBar((Rectangle){ state->anchor.x + 0, state->anchor.y + 531, 217, 24 }, TextFormat("File: %s [%s]", GetFileName(inFontFileName), FileExists(inFontFileName)? "LOADED" : "NOT AVAILABLE"));
        GuiStatusBar((Rectangle){ state->anchor.x + 216, state->anchor.y + 531, 145, 24 }, TextFormat("Codepoints: %i", GuiGetFont().glyphCount));
        GuiStatusBar((Rectangle){ state->anchor.x + 360, state->anchor.y + 531, 161, 24 }, TextFormat("Atlas Size: %ix%i", state->texFont.width, state->texFont.height));
        GuiStatusBar((Rectangle){ state->anchor.x + 520, state->anchor.y + 531, 204, 24 }, 
            TextFormat("White rec: [%i, %i, %i, %i]", (int)state->fontWhiteRec.x, (int)state->fontWhiteRec.y, (int)state->fontWhiteRec.width, (int)state->fontWhiteRec.height));

        //DrawText(TextFormat("Atlas TOP-LEFT: %i, %i", (int)(fontAtlasPosition.x - state->texFont.width*fontAtlasScale/2), (int)(fontAtlasPosition.y - state->texFont.height*fontAtlasScale/2)), 10, 10, 30, RED);
        //DrawCircleV(fontAtlasPosition, 4, MAROON);
        //DrawCircle((int)(fontAtlasPosition.x - state->texFont.width*fontAtlasScale/2), (int)(fontAtlasPosition.y - state->texFont.height*fontAtlasScale/2), 4, MAROON);
        //--------------------------------------------------------------------------------------------------
    }
    else
    {
        fontAtlasScale = 1.0f;
        fontAtlasPosition.x = state->anchor.x + 724/2;
        fontAtlasPosition.y = state->anchor.y + 532/2;
        prevFontAtlasPosition = fontAtlasPosition;

        prevSelectWhiteRecActive = false; 
        state->selectWhiteRecActive = false;

        prevFontGenSizeValue = state->fontGenSizeValue;
    }
}

#endif // GUI_WINDOW_FONT_ATLAS_IMPLEMENTATION