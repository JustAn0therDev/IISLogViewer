#define CLAY_IMPLEMENTATION
#include "include/clay.h"
#include "renderers/raylib/clay_renderer_raylib.c"
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

const int FONT_ID_BODY_16 = 0;
const Clay_Color FOREGROUND_COLOR = {255,255,255,255};
const Clay_Color BACKGROUND_COLOR = {0,0,140,255};
const Clay_BorderElementConfig BORDER = { .width = { .left = 5, .right = 5, .top = 5, .bottom = 5  }, .color = FOREGROUND_COLOR };
int searchBarIsInFocus = 0;
char searchString[2048] = { 0 };
int searchStringIndex = 0;

#ifdef _WIN32
#define LINE_DELIMITER "\r\n"
#else
#define LINE_DELIMITER "\n"
#endif

#define CELL_DELIMITER ' '
#define CELL_CHAR_LIMIT 10

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

void RenderTextComponent(Clay_String text) {
    CLAY_AUTO_ID({
                     .layout = {
                         .padding = CLAY_PADDING_ALL(16),
                         .sizing = { .width = CLAY_SIZING_GROW(100), .height = CLAY_SIZING_GROW(0) },
                         .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER },
                     },
                 }) {
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({
                                             .fontId = FONT_ID_BODY_16,
                                             .fontSize = 16,
                                             .textColor = FOREGROUND_COLOR
                                         }));
    }
}

char* ReadFileContent(FILE* file) {
    if (file == 0) {
        puts("Unable to open file with the provided path.");
        exit(1);
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *fileBuffer = calloc(size, sizeof(char));
    
    if (fileBuffer == 0) {
        puts("Unable to allocate memory for file.");
        exit(1);
    }
    
    fread(fileBuffer, sizeof(*fileBuffer), size, file);
    
    if (fileBuffer == 0) {
        puts("Unable to read content from file.");
        exit(1);
    }
    
    return fileBuffer;
}

// NOTES: For now, this considers every four metadata lines of an IIS log file, so we
// end up allocating more memory than we actually need. Its ok for this to stay for now, it'll hardly be
// that much more memory anyway.
size_t CountCellsInFile(char* string) {
    size_t count = 0;
    
    while (*string != '\0') {
        int isACellInTheMiddleOrEndOfTheLine = *string == CELL_DELIMITER || *string == '\n';
        if (isACellInTheMiddleOrEndOfTheLine) {
            count++;
        }
        
        string++;
    }
    
    return count;
}

void HandleFocusInteraction(Clay_ElementId clayElementId, Clay_PointerData pointerData, intptr_t userData) {
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        searchBarIsInFocus = 1;
        printf("Search bar in focus: %i\n", searchBarIsInFocus);
    }
}

int ConvertShiftKey(int key) {
    if (key == KEY_EQUAL)
        return 43;
    if (key == KEY_SEMICOLON)
        return 58;
    if (key == KEY_COMMA)
        return 60;
    if (key == KEY_PERIOD)
        return 62;
    if (key == KEY_SLASH)
        return 63;
    if (key == KEY_MINUS)
        return 95;
    if (key == KEY_GRAVE)
        return 96;
    if (key == KEY_LEFT_BRACKET)
        return 123;
    if (key == KEY_BACKSLASH)
        return 124;
    if (key == KEY_RIGHT_BRACKET)
        return 125;

    return 0;
}

char* strstr_insensitive(char* haystack, char* needle) {
    assert(haystack != 0);
    assert(needle != 0);

    char* foundPtr = 0;

    char* haystackPtr = haystack;
    char* needlePtr = needle;

    while (*haystackPtr != '\0') {
        if (tolower(*haystackPtr) == tolower(*needlePtr)) {
            if (foundPtr == 0) {
                foundPtr = haystackPtr;
            }

            needlePtr++;

            if (*needlePtr == '\0') {
                break;
            }
        } else if (foundPtr != 0) {
            foundPtr = 0;
            needlePtr = needle;
        }

        haystackPtr++;
    }

    return foundPtr;
}

int main(void) {
    Clay_Raylib_Initialize(1600, 900, "IIS Log Viewer", FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    
    uint64_t clayRequiredMemory = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(clayRequiredMemory, malloc(clayRequiredMemory));
    Clay_Initialize(clayMemory,(Clay_Dimensions){.width = GetScreenWidth(), .height = GetScreenHeight()},(Clay_ErrorHandler){HandleClayErrors});
    
    Font fonts[1];
    fonts[FONT_ID_BODY_16] = LoadFontEx("resources/Roboto-Regular.ttf", 48, 0, 400);
    
    SetTextureFilter(fonts[FONT_ID_BODY_16].texture, TEXTURE_FILTER_BILINEAR);
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);
    
    FILE* file = fopen("../example_log.txt", "r"); // TODO: read the file path from args
    char* logFileContent = ReadFileContent(file);
    size_t logFileContentSize = strlen(logFileContent);
    size_t cellsInFile = CountCellsInFile(logFileContent);
    char** arrayOfCellBuffers = calloc(sizeof(char*), cellsInFile);
    
    while (!WindowShouldClose()) {
        int arrayOfCellBuffersIndex = 0;
        int numberOfValidLinesInFile = 0;
        int numberOfCells = 0;
        
        Clay_SetLayoutDimensions((Clay_Dimensions){.width = GetScreenWidth(), .height = GetScreenHeight()});
        
        Vector2 mousePosition = GetMousePosition();
        Vector2 scrollDelta = GetMouseWheelMoveV();
        Clay_SetPointerState((Clay_Vector2){mousePosition.x, mousePosition.y},IsMouseButtonDown(0));
        Clay_UpdateScrollContainers(false, (Clay_Vector2){scrollDelta.x, scrollDelta.y}, GetFrameTime());
        
        Clay_BeginLayout();
        
        CLAY(CLAY_ID("OuterContainer"), { 
                 .layout = { 
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                     .padding = {1,1,1,1},
                     .childGap = 10
                 },
                 .backgroundColor = BACKGROUND_COLOR
             }) {
            
            CLAY(CLAY_ID("SearchBar"), {
                     .layout = {
                         .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50)}
                     },
                     .border = BORDER,
                     .cornerRadius = CLAY_CORNER_RADIUS(10)
                 }) {
                
                Clay_OnHover(HandleFocusInteraction, (intptr_t)0);
                
                if (searchBarIsInFocus) {
                    int keyPressed = GetKeyPressed();
                    
                    if (keyPressed == KEY_ENTER) {
                        searchBarIsInFocus = 0;
                    } else if (keyPressed == KEY_BACKSPACE) {
                        if (searchStringIndex > 0) {
                            searchString[--searchStringIndex] = 0;
                        }
                    } else if (keyPressed != 0) {
                        if (IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_LEFT_SHIFT)) {
                            keyPressed = ConvertShiftKey(keyPressed);
                        }
                        
                        if (keyPressed != 0)
                            searchString[searchStringIndex++] = (char)keyPressed;
                    }
                }
				
                Clay_String claySearchString = { .chars = searchString, .length = strlen(searchString) };
                RenderTextComponent(claySearchString);
            }
            
            CLAY(CLAY_ID("Table"), {
                     .layout = {
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }
                     },
                     .border = BORDER,
                     .cornerRadius = CLAY_CORNER_RADIUS(10),
                 }) {
                // header
                CLAY(CLAY_ID("TableHeader"), {
                         .layout = {
                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
                             .sizing = { .width = CLAY_SIZING_GROW(100), .height = CLAY_SIZING_FIXED(0) },
                             .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                             .childGap = 5
                         },
                         .border = { .width = { .bottom = 5  }, .color = FOREGROUND_COLOR },
                     }) {
                    RenderTextComponent(CLAY_STRING("date"));
                    RenderTextComponent(CLAY_STRING("time"));
                    RenderTextComponent(CLAY_STRING("s-ip"));
                    RenderTextComponent(CLAY_STRING("cs-method"));
                    RenderTextComponent(CLAY_STRING("cs-uri-stem"));
                    RenderTextComponent(CLAY_STRING("cs-uri-query"));
                    RenderTextComponent(CLAY_STRING("s-port"));
                    RenderTextComponent(CLAY_STRING("cs-username"));
                    RenderTextComponent(CLAY_STRING("c-ip"));
                    RenderTextComponent(CLAY_STRING("cs(UserAgent)"));
                    RenderTextComponent(CLAY_STRING("cs(Referer)"));
                    RenderTextComponent(CLAY_STRING("sc-status"));
                    RenderTextComponent(CLAY_STRING("sc-substatus"));
                    RenderTextComponent(CLAY_STRING("sc-win32-status"));
                    RenderTextComponent(CLAY_STRING("time-taken"));
                }
                
                CLAY(CLAY_ID("TableLines"), {
                         .layout = {
                             .layoutDirection = CLAY_TOP_TO_BOTTOM,
                             .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                             .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }
                         },
                         .cornerRadius = CLAY_CORNER_RADIUS(10),
                         .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() }
                     }) {
                    
                    // lines
                    char* fileContentCpy = calloc(sizeof(char), logFileContentSize + 1);
                    strcpy(fileContentCpy, logFileContent);
                    char* line = strtok(fileContentCpy, LINE_DELIMITER);
                    
                    while (line != 0) {
                        if (line[0] != '#') {
                            numberOfValidLinesInFile++;

                            Clay_String cells[15] = { 0 };
                            int cellsIndex = 0;
                            int cellBufferIndex = 0;
                            
                            arrayOfCellBuffers[arrayOfCellBuffersIndex] = calloc(sizeof(char), CELL_CHAR_LIMIT + 1);
                            
                            char* fullCellValue = calloc(sizeof(char), 1);
                            int fullCellValueIndex = 0;
                            
                            char* linePtr = line;

                            int isValidLine = 0;
                            int shouldCheckForValidLine = strlen(searchString) > 0;
                            
                            while (*linePtr != '\0') {
                                if (*linePtr != CELL_DELIMITER) {
                                    if (cellBufferIndex < CELL_CHAR_LIMIT) {
                                        arrayOfCellBuffers[arrayOfCellBuffersIndex][cellBufferIndex++] = *linePtr;
                                    }
                                    fullCellValue[fullCellValueIndex++] = *linePtr;
                                    fullCellValue = realloc(fullCellValue, fullCellValueIndex + 1);
                                    fullCellValue[fullCellValueIndex] = '\0';
                                } else {
                                    numberOfCells++;
                                    Clay_String clayString = { .chars = arrayOfCellBuffers[arrayOfCellBuffersIndex], .length = cellBufferIndex + 1 };
                                    cells[cellsIndex++] = clayString;

                                    if (shouldCheckForValidLine && strstr_insensitive(fullCellValue, searchString)) {
                                        isValidLine = 1;
                                    }

                                    cellBufferIndex = 0;
                                    arrayOfCellBuffers[++arrayOfCellBuffersIndex] = calloc(sizeof(char), CELL_CHAR_LIMIT + 1);
                                    free(fullCellValue);
                                    fullCellValue = calloc(sizeof(char), 1);
                                    fullCellValueIndex = 0;
                                }
                                
                                linePtr++;
                            }

                            numberOfCells++;
                            Clay_String clayString = { .chars = arrayOfCellBuffers[arrayOfCellBuffersIndex], .length = cellBufferIndex + 1 };
                            cells[cellsIndex++] = clayString;

                            if (shouldCheckForValidLine && strstr_insensitive(fullCellValue, searchString)) {
                                isValidLine = 1;
                            }

                            cellBufferIndex = 0;
                            arrayOfCellBuffers[++arrayOfCellBuffersIndex] = calloc(sizeof(char), CELL_CHAR_LIMIT + 1);

                            if (!shouldCheckForValidLine || (shouldCheckForValidLine && isValidLine)) {
                                CLAY_AUTO_ID({.layout = {
                                                    .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50) },
                                                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                                                },
                                                .border = { .width = { .bottom = 1 }, .color = FOREGROUND_COLOR },
                                            }) {
                                    for (int i = 0; i < cellsIndex; i++)
                                        RenderTextComponent(cells[i]);
                                }
                            }
                        }
                        line = strtok(0, LINE_DELIMITER);
                    }
                    
                    free(fileContentCpy);
                }
            }
            
            CLAY(CLAY_ID("SearchInfo"), {
                     .layout = {
                         .layoutDirection = CLAY_LEFT_TO_RIGHT
                     },
                 }) {
                char foundRecordsBuffer[2248] = { 0 };
                
                if (strcmp(searchString, "") == 0) {
                    sprintf(foundRecordsBuffer, "Found %i records", numberOfValidLinesInFile);
                } else {
                    sprintf(foundRecordsBuffer, "Found %i records for '%s'", numberOfValidLinesInFile, searchString);
                }
                
                Clay_String foundRecordsClayString = { .chars = foundRecordsBuffer, .length = strlen(foundRecordsBuffer) };
                RenderTextComponent(foundRecordsClayString);
            }
        }
        
        Clay_RenderCommandArray renderCommands = Clay_EndLayout();
        
        BeginDrawing();
        ClearBackground(BLACK);
        Clay_Raylib_Render(renderCommands, fonts);
        EndDrawing();
        
        for (int i = 0; i < numberOfCells; i++) {
            if (arrayOfCellBuffers[i] != 0)  {
                free(arrayOfCellBuffers[i]);
                arrayOfCellBuffers[i] = 0;
            }
        }
        
        arrayOfCellBuffersIndex = 0;
    }
    
    for (int i = 0; i < cellsInFile; i++) {
        if (arrayOfCellBuffers[i] != 0)  {
            free(arrayOfCellBuffers[i]);
            arrayOfCellBuffers[i] = 0;
        }
    }
    free(arrayOfCellBuffers);
    free(logFileContent);
    fclose(file);
    UnloadFont(fonts[FONT_ID_BODY_16]);
    Clay_Raylib_Close();
}
