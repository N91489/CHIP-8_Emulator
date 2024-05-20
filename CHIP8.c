#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>

#define GRID_WIDTH 64
#define GRID_HEIGHT 32
#define MEMORY_SIZE 4096
#define FONT_SIZE 80
#define MEMORY_STARTING_ADDRESS 0x200

typedef struct
{
    uint8_t Memory[MEMORY_SIZE];
    uint8_t Display[GRID_WIDTH * GRID_HEIGHT];
    uint16_t PC;
    uint16_t I;
    uint16_t Stack[16];
    uint8_t SP;
    uint8_t Delay_Timer;
    uint8_t Sound_Timer;
    uint8_t V[16];
    uint8_t Key[16];

} CHIP8_CPU;

const uint8_t Chip8Font[FONT_SIZE] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

CHIP8_CPU Chip8;

void InitializeChip8()
{
    // Initalize ProgramCounter, IndexRegister, StackPointer, DelayTimer, SoundTimer
    Chip8.PC = MEMORY_STARTING_ADDRESS;
    Chip8.I = 0;
    Chip8.SP = 0;
    Chip8.Delay_Timer = 0;
    Chip8.Sound_Timer = 0;

    // Initalize Stack, V-Registers & Keys
    for (int i = 0; i < 16; i++)
    {
        Chip8.V[i] = 0;
        Chip8.Stack[i] = 0;
        Chip8.Key[i] = 0;
    }

    // Initalize Memory
    for (int i = 0; i < MEMORY_SIZE; i++)
    {
        Chip8.Memory[i] = 0;
    }

    // Loading Font to Memory Address (0x000 – 0x050)
    for (int i = 0; i < FONT_SIZE; i++)
    {
        Chip8.Memory[0x000 + i] = Chip8Font[i];
    }
}

void ClearDisplay()
{
    // (Initalize) Clear Display
    for (int i = 0; i < (GRID_WIDTH * GRID_HEIGHT); i++)
    {
        Chip8.Display[i] = 0;
    }
}

void DrawSprite(uint8_t X, uint8_t Y, uint8_t N)
{
    // Clear the collision flag
    Chip8.V[0xF] = 0;

    // Iterate over each line of the sprite
    for (int line = 0; line < N; line++)
    {
        for (int bit = 0; bit < 8; bit++)
        {
            // Check if the current bit is set in the sprite byte
            if (((Chip8.Memory[Chip8.I + line]) & (0x80 >> bit)) != 0)
            {
                // Check for collision
                if (Chip8.Display[((Chip8.V[Y] + line) % GRID_HEIGHT) * GRID_WIDTH + ((Chip8.V[X] + bit) % GRID_WIDTH)] == 1)
                {
                    Chip8.V[0xF] = 1;
                }

                // XOR the bit on the display
                Chip8.Display[((Chip8.V[Y] + line) % GRID_HEIGHT) * GRID_WIDTH + ((Chip8.V[X] + bit) % GRID_WIDTH)] ^= 1;
            }
        }
    }
}

bool LoadROM(const char *file)
{
    FILE *ROM = fopen(file, "rb");

    // Can't Find ROM
    if (ROM == NULL)
    {
        printf("Couldn't Find ROM\n");
        fclose(ROM);
        return 1;
    }

    // Checking File Extension
    if (strcmp(&file[strlen(file) - 4], ".ch8") != 0)
    {
        printf("Not [.ch8] File Cant Load ROM\n");
        fclose(ROM);
        return 1;
    }

    // Check ROM Size
    fseek(ROM, 0, SEEK_END);
    int ROMSize = ftell(ROM);
    fseek(ROM, 0, SEEK_SET);

    // ROM Larger Then Memory
    if (ROMSize > (MEMORY_SIZE - MEMORY_STARTING_ADDRESS))
    {
        printf("ROM Size: %db\n", ROMSize);
        printf("ROM Too Large To Load Into CHIP-8, ROM Size Should be Less Than %db\n", (MEMORY_SIZE - MEMORY_STARTING_ADDRESS));
        fclose(ROM);
        return 1;
    }

    // Loading ROM Into Memory
    else
    {
        int data = fread(&Chip8.Memory[MEMORY_STARTING_ADDRESS], sizeof(uint8_t), ROMSize, ROM);

        // Couldnt Load ROM
        if (data != ROMSize)
        {
            printf("Error During Loading The ROM");
            fclose(ROM);
            return 1;
        }

        // ROM Loaded Succesfully,
        else
        {
            printf("ROM Size: %db\n", ROMSize);
            fclose(ROM);
            return 0;
        }
    }
}

void ExecuteInstructions()
{
    // Combine 2 Byte From Memory To Make One Opcode
    uint16_t opcode = Chip8.Memory[Chip8.PC] << 8 | Chip8.Memory[Chip8.PC + 1];

    // 0x0
    if ((opcode & 0xF000) == 0x0000)
    {
        // 00E0 - Clear Display
        if ((opcode & 0x00FF) == 0x00E0)
        {
            ClearDisplay();
            Chip8.PC += 2;
        }
        // 00EE - Return
        else if ((opcode & 0x00FF) == 0x00EE)
        {
            Chip8.SP--;
            Chip8.PC = Chip8.Stack[Chip8.SP];
            Chip8.PC += 2;
        }
    }

    // 1NNN - Goto NNN
    if ((opcode & 0xF000) == 0x1000)
    {
        Chip8.PC = (opcode & 0x0FFF);
    }

    // 2NNN - Calls subroutine at NNN
    if ((opcode & 0xF000) == 0x2000)
    {
        Chip8.Stack[Chip8.SP] = Chip8.PC;
        Chip8.SP++;
        Chip8.PC = (opcode & 0x0FFF);
    }

    // 3XNN - SKIP Instruction if(Vx == NN)
    if ((opcode & 0xF000) == 0x3000)
    {
        if (Chip8.V[((opcode & 0x0F00) >> 8)] == (opcode & 0x00FF))
        {
            Chip8.PC += 4;
        }
        else
        {
            Chip8.PC += 2;
        }
    }

    // 4XNN - SKIP Instruction if(Vx != NN)
    if ((opcode & 0xF000) == 0x4000)
    {
        if (Chip8.V[((opcode & 0x0F00) >> 8)] != (opcode & 0x00FF))
        {
            Chip8.PC += 4;
        }
        else
        {
            Chip8.PC += 2;
        }
    }

    // 5XY0 - SKIP Instruction if(Vx == Vy)
    if ((opcode & 0xF000) == 0x5000)
    {
        if (Chip8.V[((opcode & 0x0F00) >> 8)] == Chip8.V[((opcode & 0x00F0) >> 4)])
        {
            Chip8.PC += 4;
        }
        else
        {
            Chip8.PC += 2;
        }
    }

    // 6XNN - SET Vx = NN
    if ((opcode & 0xF000) == 0x6000)
    {
        Chip8.V[((opcode & 0x0F00) >> 8)] = (opcode & 0x00FF);
        Chip8.PC += 2;
    }

    // 7XNN - ADD Vx += NN
    if ((opcode & 0xF000) == 0x7000)
    {
        Chip8.V[((opcode & 0x0F00) >> 8)] += (opcode & 0x00FF);
        Chip8.PC += 2;
    }

    // 0x8
    if ((opcode & 0xF000) == 0x8000)
    {
        // 8XY0 - SET Vx = Vy
        if ((opcode & 0x000F) == 0x0000)
        {
            Chip8.V[((opcode & 0x0F00) >> 8)] = Chip8.V[((opcode & 0x00F0) >> 4)];
            Chip8.PC += 2;
        }

        // 8XY1 - SET Vx |= Vy
        else if ((opcode & 0x000F) == 0x0001)
        {
            Chip8.V[((opcode & 0x0F00) >> 8)] |= Chip8.V[((opcode & 0x00F0) >> 4)];
            Chip8.PC += 2;
        }

        // 8XY2 - SET Vx &= Vy
        else if ((opcode & 0x000F) == 0x0002)
        {
            Chip8.V[((opcode & 0x0F00) >> 8)] &= Chip8.V[((opcode & 0x00F0) >> 4)];
            Chip8.PC += 2;
        }

        // 8XY3 - SET Vx ^= Vy
        else if ((opcode & 0x000F) == 0x0003)
        {
            Chip8.V[((opcode & 0x0F00) >> 8)] ^= Chip8.V[((opcode & 0x00F0) >> 4)];
            Chip8.PC += 2;
        }

        // 8XY4 - SET Vx += Vy
        else if ((opcode & 0x000F) == 0x0004)
        {
            // Check OverFlow
            if ((Chip8.V[((opcode & 0x0F00) >> 8)]) + (Chip8.V[((opcode & 0x00F0) >> 4)]) > 255)
            {
                Chip8.V[0xF] = 1;
            }
            else
            {
                Chip8.V[0xF] = 0;
            }

            Chip8.V[((opcode & 0x0F00) >> 8)] += Chip8.V[((opcode & 0x00F0) >> 4)];
            Chip8.PC += 2;
        }

        // 8XY5 - SET Vx -= Vy
        else if ((opcode & 0x000F) == 0x0005)
        {
            // Check UnderFlow
            if (Chip8.V[((opcode & 0x0F00) >> 8)] > Chip8.V[((opcode & 0x00F0) >> 4)])
            {
                Chip8.V[0xF] = 1;
            }
            else
            {
                Chip8.V[0xF] = 0;
            }

            Chip8.V[((opcode & 0x0F00) >> 8)] -= Chip8.V[((opcode & 0x00F0) >> 4)];
            Chip8.PC += 2;
        }

        // 8XY6 - SET Vx >>= 1
        else if ((opcode & 0x000F) == 0x0006)
        {
            // Flag Set
            Chip8.V[0xF] = (Chip8.V[((opcode & 0x0F00) >> 8)]) & 0x01;

            Chip8.V[((opcode & 0x0F00) >> 8)] >>= 1;
            Chip8.PC += 2;
        }

        // 8XY7 - SET Vx = Vy - Vx
        else if ((opcode & 0x000F) == 0x0007)
        {
            // Check UnderFlow
            if (Chip8.V[((opcode & 0x00F0) >> 4)] > Chip8.V[((opcode & 0x0F00) >> 8)])
            {
                Chip8.V[0xF] = 1;
            }
            else
            {
                Chip8.V[0xF] = 0;
            }

            Chip8.V[((opcode & 0x0F00) >> 8)] -= Chip8.V[((opcode & 0x00F0) >> 4)];
            Chip8.PC += 2;
        }

        // 8XYE - SET Vx <<= 1
        else if ((opcode & 0x000F) == 0x000E)
        {
            // Flag Set
            Chip8.V[0xF] = (Chip8.V[((opcode & 0x0F00) >> 8)]) >> 7;

            Chip8.V[((opcode & 0x0F00) >> 8)] <<= 1;
            Chip8.PC += 2;
        }
    }

    // 9XY0 - SKIP Instruction if(Vx != Vy)
    if ((opcode & 0xF000) == 0x9000)
    {
        if (Chip8.V[((opcode & 0x0F00) >> 8)] != Chip8.V[((opcode & 0x00F0) >> 4)])
        {
            Chip8.PC += 4;
        }
        else
        {
            Chip8.PC += 2;
        }
    }

    // ANNN - SET I = NNN
    if ((opcode & 0xF000) == 0xA000)
    {
        Chip8.I = (opcode & 0x0FFF);
        Chip8.PC += 2;
    }

    // BNNN - SET PC = V0 + NNN
    if ((opcode & 0xF000) == 0xB000)
    {
        Chip8.PC = Chip8.V[0x0] + (opcode & 0x0FFF);
        Chip8.PC += 2;
    }

    // CXNN - SET Vx = rand(0-255) & NN
    if ((opcode & 0xF000) == 0xC000)
    {
        Chip8.V[((opcode & 0x0F00) >> 8)] = (rand() % 0xFF) & (opcode & 0x00FF);
        Chip8.PC += 2;
    }

    // DXYN - DISPLAY draw(Vx, Vy, N)
    if ((opcode & 0xF000) == 0xD000)
    {
        DrawSprite(((opcode & 0x0F00) >> 8), ((opcode & 0x00F0) >> 4), (opcode & 0x000F));
        Chip8.PC += 2;
    }

    // 0xE
    if ((opcode & 0xF000) == 0xE000)
    {
        // EX9E - SKIP if(key[Vx] == 1)
        if ((opcode & 0x00FF) == 0x009E)
        {
            if (Chip8.Key[((opcode & 0x0F00) >> 8)] == 1)
            {
                Chip8.PC += 2;
            }
        }

        // EXA1 - SKIP if(key[Vx] != 1)
        if ((opcode & 0x00FF) == 0x00A1)
        {
            if (Chip8.Key[((opcode & 0x0F00) >> 8)] != 1)
            {
                Chip8.PC += 2;
            }
        }
    }

    // 0xF
    if ((opcode & 0xF000) == 0xF000)
    {
        // FX07 - SET Vx = Delay_Timer
        if ((opcode & 0x00FF) == 0x0007)
        {
            Chip8.V[((opcode & 0x0F00) >> 8)] = Chip8.Delay_Timer;
            Chip8.PC += 2;
        }

        // FX0A - AWAIT EXEC UNTIL if(AnyKey == 1) & Store (AnyKey == 1) = Vx
        if ((opcode & 0x00FF) == 0x000A)
        {
            for (int i = 0; i < 16; i++)
            {
                if (Chip8.Key[i] == 1)
                {
                    Chip8.V[((opcode & 0x0F00) >> 8)] = i;
                    Chip8.PC += 2;
                    break;
                }
            }
        }

        // FX15 - SET Delay_Timer = Vx
        if ((opcode & 0x00FF) == 0x0015)
        {
            Chip8.Delay_Timer = Chip8.V[((opcode & 0x0F00) >> 8)];
            Chip8.PC += 2;
        }

        // FX18 - SET Sound_Timer = Vx
        if ((opcode & 0x00FF) == 0x0018)
        {
            Chip8.Sound_Timer = Chip8.V[((opcode & 0x0F00) >> 8)];
            Chip8.PC += 2;
        }

        // FX1E - SET I += Vx
        if ((opcode & 0x00FF) == 0x001E)
        {
            Chip8.I += Chip8.V[((opcode & 0x0F00) >> 8)];
            Chip8.PC += 2;
        }

        // FX29 - SET I = Sprite_Address of Vx
        if ((opcode & 0x00FF) == 0x0029)
        {
            Chip8.I = Chip8.V[((opcode & 0x0F00) >> 8)] * 0x5;
            Chip8.PC += 2;
        }

        // FX33 - BCD of Vx At I[0] = BCD(100), I[1] = BCD(10), I[2] = BCD(1)
        if ((opcode & 0x00FF) == 0x0033)
        {
            Chip8.Memory[Chip8.I] = Chip8.V[((opcode & 0x0F00) >> 8)] % 10;
            Chip8.Memory[Chip8.I + 1] = ((Chip8.V[((opcode & 0x0F00) >> 8)]) / 10) % 10;
            Chip8.Memory[Chip8.I + 2] = (Chip8.V[((opcode & 0x0F00) >> 8)] / 100) % 10;
            Chip8.PC += 2;
        }

        // FX55 - SET Memory[I + i] = V[i]
        if ((opcode & 0x00FF) == 0x0055)
        {
            for (int i = 0; i < ((opcode & 0x0F00) >> 8); i++)
            {
                Chip8.Memory[(Chip8.I + i)] = Chip8.V[i];
            }
            Chip8.PC += 2;
        }

        // FX65 - SET V[i] = Memory[I + i]
        if ((opcode & 0x00FF) == 0x0065)
        {
            for (int i = 0; i < ((opcode & 0x0F00) >> 8); i++)
            {
                Chip8.V[i] = Chip8.Memory[(Chip8.I + i)];
            }
            Chip8.PC += 2;
        }

        // Timers
        if (Chip8.Delay_Timer > 0)
        {
            Chip8.Delay_Timer--;
        }
        if (Chip8.Sound_Timer > 0)
        {
            Chip8.Sound_Timer--;
        }
    }
}

void Run()
{
    // Setting RunTime Variables
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    bool run = true;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    // Getting Screen Resolution
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    int SCREEN_WIDTH = 640;
    int SCREEN_HEIGHT = 320;

    // Uniform Scaling Cell According to Resolution
    int CELL_WIDTH = SCREEN_WIDTH / GRID_WIDTH;
    int CELL_HEIGHT = SCREEN_HEIGHT / GRID_HEIGHT;
    int CELL_SIZE = (CELL_WIDTH > CELL_HEIGHT) ? CELL_HEIGHT : CELL_WIDTH;

    // Setting Window, Renderer & Audio
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

    // Main Loop
    while (run)
    {
        // Event Handling
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                run = false;
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_1:
                    Chip8.Key[0x1] = 1;
                    break;

                case SDLK_2:
                    Chip8.Key[0x2] = 1;
                    break;

                case SDLK_3:
                    Chip8.Key[0x3] = 1;
                    break;

                case SDLK_4:
                    Chip8.Key[0xC] = 1;
                    break;

                case SDLK_q:
                    Chip8.Key[0x4] = 1;
                    break;

                case SDLK_w:
                    Chip8.Key[0x5] = 1;
                    break;

                case SDLK_e:
                    Chip8.Key[0x6] = 1;
                    break;

                case SDLK_r:
                    Chip8.Key[0xD] = 1;
                    break;

                case SDLK_a:
                    Chip8.Key[0x7] = 1;
                    break;

                case SDLK_s:
                    Chip8.Key[0x8] = 1;
                    break;

                case SDLK_d:
                    Chip8.Key[0x9] = 1;
                    break;

                case SDLK_f:
                    Chip8.Key[0xE] = 1;
                    break;

                case SDLK_z:
                    Chip8.Key[0xA] = 1;
                    break;

                case SDLK_x:
                    Chip8.Key[0x0] = 1;
                    break;

                case SDLK_c:
                    Chip8.Key[0xB] = 1;
                    break;

                case SDLK_v:
                    Chip8.Key[0xF] = 1;
                    break;

                default:
                    printf("Key Not For Emulator\n");
                    break;
                }
            }

            if (event.type == SDL_KEYUP)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_1:
                    Chip8.Key[0x1] = 0;
                    break;

                case SDLK_2:
                    Chip8.Key[0x2] = 0;
                    break;

                case SDLK_3:
                    Chip8.Key[0x3] = 0;
                    break;

                case SDLK_4:
                    Chip8.Key[0xC] = 0;
                    break;

                case SDLK_q:
                    Chip8.Key[0x4] = 0;
                    break;

                case SDLK_w:
                    Chip8.Key[0x5] = 0;
                    break;

                case SDLK_e:
                    Chip8.Key[0x6] = 0;
                    break;

                case SDLK_r:
                    Chip8.Key[0xD] = 0;
                    break;

                case SDLK_a:
                    Chip8.Key[0x7] = 0;
                    break;

                case SDLK_s:
                    Chip8.Key[0x8] = 0;
                    break;

                case SDLK_d:
                    Chip8.Key[0x9] = 0;
                    break;

                case SDLK_f:
                    Chip8.Key[0xE] = 0;
                    break;

                case SDLK_z:
                    Chip8.Key[0xA] = 0;
                    break;

                case SDLK_x:
                    Chip8.Key[0x0] = 0;
                    break;

                case SDLK_c:
                    Chip8.Key[0xB] = 0;
                    break;

                case SDLK_v:
                    Chip8.Key[0xF] = 0;
                    break;

                default:
                    printf("Key Not For Emulator\n");
                    break;
                }
            }
        }

        ExecuteInstructions();

        // Clear Renderer
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render Cells
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int i = 0; i < GRID_WIDTH; i++)
        {
            for (int j = 0; j < GRID_HEIGHT; j++)
            {
                if (Chip8.Display[j * GRID_WIDTH + i] != 0)
                {
                    SDL_Rect cell = {i * CELL_SIZE, j * CELL_SIZE, CELL_SIZE, CELL_SIZE};
                    SDL_RenderFillRect(renderer, &cell);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / 60);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char **argv)
{
    if (argc > 2 || argc == 1)
    {
        printf("Usage: %s <file_path_name> \n", argv[0]);
    }
    else
    {
        InitializeChip8();
        ClearDisplay();

        if (LoadROM(argv[1]) == 0)
        {
            Run();
        }
    }
    return 0;
}