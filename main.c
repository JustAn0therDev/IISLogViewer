#define CLAY_IMPLEMENTATION
#include "include/clay.h"
#include "renderers/raylib/clay_renderer_raylib.c"
#include <stdio.h>
#include <ctype.h>

const int FONT_ID_BODY_16 = 0;
const Clay_BorderElementConfig BLACK_BORDER = { .width = { .left = 5, .right = 5, .top = 5, .bottom = 5  }, .color = {0,0,0,255} };

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
                         .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER }
                     }
                 }) {
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({
                                             .fontId = FONT_ID_BODY_16,
                                             .fontSize = 16,
                                             .textColor = { 0, 0, 0, 255 }
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

// NOTES: For now, this considers the four metadata lines of an IIS log file, so it
// allocates more than it actually needs. Its ok for this to stay for now, it'll hardly be
// that much more memory anyway.
size_t CountCellsInFile(char* string) {
    size_t count = 0;
    
    while (*string != '\0') {
        if (*string == CELL_DELIMITER) {
            count++;
        }
        
        string++;
    }
    
    return count;
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
    
    FILE* file = fopen("../example_log.txt", "r");
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
                 .backgroundColor = {255,255,255,255}
             }) {
            
            CLAY(CLAY_ID("SearchBar"), {
                     .layout = {
                         .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50)}
                     },
                     .border = BLACK_BORDER,
                     .cornerRadius = CLAY_CORNER_RADIUS(10)
                 }) {
                RenderTextComponent(CLAY_STRING("SEARCH"));
            }
            
            CLAY(CLAY_ID("Table"), {
                     .layout = {
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }
                     },
                     .border = BLACK_BORDER,
                     .cornerRadius = CLAY_CORNER_RADIUS(10),
                 }) {
                // header
                CLAY(CLAY_ID("TableHeader"), {
                         .layout = {
                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
                             .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(0) },
                             .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                             .childGap = 5
                         },
                         .border = { .width = { .bottom = 5  }, .color = {0,0,0,255} },
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
                             .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }
                         },
                         .border = { .width = { .bottom = 1, .left = 1, .right = 1  }, .color = {0,0,0,255} },
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
                            CLAY_AUTO_ID({.layout = {
                                                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                                 .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(0) },
                                                 .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                                                 .childGap = 5
                                             },
                                             .border = { .width = { .bottom = 1  }, .color = {0,0,0,255} },
                                         }) {
                                int cellBufferIndex = 0;
                                arrayOfCellBuffers[arrayOfCellBuffersIndex] = calloc(sizeof(char), CELL_CHAR_LIMIT + 1);
                                
                                char* linePtr = line;
                                
                                while (*linePtr != '\0') {
                                    if (*linePtr != CELL_DELIMITER) {
                                        if (cellBufferIndex < CELL_CHAR_LIMIT) {
                                            arrayOfCellBuffers[arrayOfCellBuffersIndex][cellBufferIndex++] = *linePtr;
                                        }
                                    } else {
                                        numberOfCells++;
                                        Clay_String clayString = { .chars = arrayOfCellBuffers[arrayOfCellBuffersIndex], .length = cellBufferIndex + 1 };
                                        RenderTextComponent(clayString);
                                        cellBufferIndex = 0;
                                        arrayOfCellBuffers[++arrayOfCellBuffersIndex] = calloc(sizeof(char), CELL_CHAR_LIMIT + 1);
                                    }
                                    
                                    linePtr++;
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
                char foundRecordsBuffer[200] = { 0 };
                sprintf(foundRecordsBuffer, "Found %i Records", numberOfValidLinesInFile);
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
    
    free(arrayOfCellBuffers);
    free(logFileContent);
    fclose(file);
    Clay_Raylib_Close();
}
