#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>

#include "raylib.h"
#include "resource.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SCREEN_W 900
#define SCREEN_H 650
#define STAR_COUNT 150
#define G0 9.80665f

static Font uiFont;

typedef enum { STATE_MENU, STATE_PLANET_SELECT, STATE_SIMULATION } GameState;

typedef struct Planet {
    const char* name;
    const char* tagline;
    float mu;              // gravitational parameter, m^3/s^2
    float radius;           // meters
    float rho0;              // surface air density, kg/m^3 (0 = airless)
    float scaleHeight;        // atmospheric scale height, meters
    float spaceLine;           // altitude counted as "cleared the atmosphere"
    const char* spaceLineLabel;
    Color skyLow;             // color near the surface
    Color skyHigh;            // color at/above spaceLine
    Color groundColor;
    Color bodyBase;           // real surface/body color (for the selection icon)
    Color bodyDetail;         // secondary shade for texture (oceans, craters, dust)
} Planet;

typedef struct Rocket {
    float dryMass;
    float fuelMass;
    float thrust;
    float isp;
    float dragCoeff;
    float crossArea;
} Rocket;

typedef struct Star { float x, y, brightness; } Star;

/*definitions of the planets*/
static const Planet PLANETS[3] = {
    {
        "EARTH", "Thick nitrogen-oxygen atmosphere creates significant aerodynamic drag during ascent.",
        3.986004418e14f, 6371000.0f,
        1.225f, 8500.0f,
        100000.0f, "Karman Line - Edge of Space (100 km)",
        {107,179,245,255}, {5,5,15,255}, {80,60,40,255},
        {40,90,160,255}, {70,150,70,255}
    },
    {
        "MARS", "Thin atmosphere, about a third of Earth's gravity.",
        4.282837e13f, 3389500.0f,
        0.020f, 11100.0f,
        80000.0f, "Edge of Martian Atmosphere (80 km)",
        {210,140,100,255}, {10,5,10,255}, {169,74,42,255},
        {193,68,14,255}, {120,45,15,255}
    },
    {
        "MOON", "No atmosphere at all - space starts at the ground.",
        4.9048695e12f, 1737400.0f,
        0.0f, 1.0f,
        500.0f, "Lunar Surface - Already In Vacuum",
        {20,20,30,255}, {0,0,5,255}, {150,150,150,255},
        {185,185,188,255}, {120,120,124,255}
    }
};

float GravityAt(const Planet* p, float altitude)
{
    float r = p->radius + altitude;
    return p->mu / (r * r);
}

float AirDensityAt(const Planet* p, float altitude)
{
    if (p->rho0 <= 0.0f) return 0.0f;
    if (altitude < 0) altitude = 0;
    return p->rho0 * expf(-altitude / p->scaleHeight);
}

Color ColorLerpManual(Color a, Color b, float t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    Color out;
    out.r = (unsigned char)(a.r + (b.r - a.r) * t);
    out.g = (unsigned char)(a.g + (b.g - a.g) * t);
    out.b = (unsigned char)(a.b + (b.b - a.b) * t);
    out.a = 255;
    return out;
}

Color GetSkyColor(const Planet* p, float altitude)
{
    float t = fminf(fmaxf(altitude / p->spaceLine, 0.0f), 1.0f);
    return ColorLerpManual(p->skyLow, p->skyHigh, t);
}

int DrawCenteredText(const char* text, int centerX, int y, int fontSize, Color color)
{
    Vector2 size = MeasureTextEx(uiFont, text, (float)fontSize, 1.5f);
    DrawTextEx(uiFont, text, (Vector2) { centerX - size.x / 2, (float)y }, (float)fontSize, 1.5f, color);
    return (int)size.x;
}

void DrawTextF(const char* text, int x, int y, int fontSize, Color color)
{
    DrawTextEx(uiFont, text, (Vector2) { (float)x, (float)y }, (float)fontSize, 1.5f, color);
}

/* Displays the text line by line so it stays within the maximum width, then returns the height used to help position the next content. */
int DrawWrappedText(const char* text, int x, int y, int maxWidth, int fontSize, int lineSpacing, Color color)
{
    char buffer[256];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char line[256] = "";
    char* word = strtok(buffer, " ");
    int curY = y;
    int linesDrawn = 0;

    while (word != NULL)
    {
        char testLine[256];
        if (line[0] == '\0') snprintf(testLine, sizeof(testLine), "%s", word);
        else snprintf(testLine, sizeof(testLine), "%s %s", line, word);

        Vector2 size = MeasureTextEx(uiFont, testLine, (float)fontSize, 1.0f);
        if (size.x > (float)maxWidth && line[0] != '\0')
        {
            DrawTextF(line, x, curY, fontSize, color);
            curY += lineSpacing;
            linesDrawn++;
            snprintf(line, sizeof(line), "%s", word);
        }
        else
        {
            snprintf(line, sizeof(line), "%s", testLine);
        }
        word = strtok(NULL, " ");
    }
    if (line[0] != '\0')
    {
        DrawTextF(line, x, curY, fontSize, color);
        linesDrawn++;
    }
    return linesDrawn * lineSpacing;
}

/* Draws a shape of each planet */
void DrawPlanetIcon(int planetIndex, const Planet* p, int cx, int cy, float radius)
{
    DrawCircle(cx, cy, radius, p->bodyBase);

    if (planetIndex == 0) /* Earth: ocean base + green continents + white cloud swirls */
    {
        DrawCircle(cx - radius * 0.3f, cy - radius * 0.2f, radius * 0.42f, p->bodyDetail);
        DrawCircle(cx + radius * 0.35f, cy + radius * 0.15f, radius * 0.30f, p->bodyDetail);
        DrawCircle(cx + radius * 0.1f, cy - radius * 0.45f, radius * 0.18f, p->bodyDetail);
        DrawCircle(cx - radius * 0.45f, cy + radius * 0.35f, (float)(radius * 0.22f), (Color) { 255, 255, 255, 160 });
        DrawCircle(cx + radius * 0.2f, cy + radius * 0.4f, radius * 0.15f, (Color) { 255, 255, 255, 140 });
    }
    else if (planetIndex == 1) /* Mars: rusty base + darker dust/basin patches */
    {
        DrawCircle(cx - radius * 0.25f, cy + radius * 0.2f, radius * 0.35f, p->bodyDetail);
        DrawCircle(cx + radius * 0.3f, cy - radius * 0.25f, radius * 0.22f, p->bodyDetail);
        DrawCircle(cx + radius * 0.1f, cy + radius * 0.4f, radius * 0.15f, p->bodyDetail);
    }
    else /* Moon: grey base + darker mare patches + small crater dots */
    {
        DrawCircle(cx - radius * 0.3f, cy - radius * 0.1f, radius * 0.30f, p->bodyDetail);
        DrawCircle(cx + radius * 0.25f, cy + radius * 0.3f, radius * 0.22f, p->bodyDetail);
        DrawCircle(cx - radius * 0.5f, cy + radius * 0.35f, radius * 0.10f, (Color) { 100, 100, 104, 255 });
        DrawCircle(cx + radius * 0.4f, cy - radius * 0.35f, radius * 0.08f, (Color) { 100, 100, 104, 255 });
    }

    DrawCircleLines(cx, cy, radius, (Color) { 255, 255, 255, 60 });
}

/* Draws a rocket styled to match the app icon: orange nose, white body,
   light-blue window, blue fins, layered flame when thrusting. */
void DrawRocketShip(float cx, float topY, bool thrusting)
{
    float bodyW = 26.0f;
    float noseH = 24.0f;
    float bodyH = 50.0f;

    float bodyTop = topY + noseH;
    float bodyBottom = bodyTop + bodyH;

    /* Nose cone - orange, matching the icon */
    DrawTriangle(
        (Vector2) {
        cx - bodyW / 2, bodyTop
    },
        (Vector2) {
        cx + bodyW / 2, bodyTop
    },
        (Vector2) {
        cx, topY
    },
        (Color) {
        245, 130, 45, 255
    }
    );
    DrawTriangleLines(
        (Vector2) {
        cx - bodyW / 2, bodyTop
    },
        (Vector2) {
        cx + bodyW / 2, bodyTop
    },
        (Vector2) {
        cx, topY
    },
        (Color) {
        180, 90, 20, 255
    }
    );

    /* Main body - off-white */
    Color hullColor = thrusting ? (Color) { 250, 250, 252, 255 } : (Color) { 225, 225, 230, 255 };
    DrawRectangle((int)(cx - bodyW / 2), (int)bodyTop, (int)bodyW, (int)bodyH, hullColor);
    DrawRectangleLines((int)(cx - bodyW / 2), (int)bodyTop, (int)bodyW, (int)bodyH, (Color) { 160, 160, 170, 255 });

    /* Light-blue accent stripe, like the icon's upper band */
    DrawRectangle((int)(cx - bodyW / 2), (int)(bodyTop + 4), (int)bodyW, 8, (Color) { 110, 195, 235, 255 });

    /* Window - light blue circle with darker ring */
    float windowY = bodyTop + bodyH * 0.42f;
    DrawCircle((int)cx, (int)windowY, 7.0f, (Color) { 170, 225, 245, 255 });
    DrawCircleLines((int)cx, (int)windowY, 7.0f, (Color) { 40, 110, 150, 255 });

    /* Fins - blue, angled outward at the base */
    DrawTriangle(
        (Vector2) {
        cx - bodyW / 2, bodyBottom - 14
    },
        (Vector2) {
        cx - bodyW / 2 - 14, bodyBottom + 10
    },
        (Vector2) {
        cx - bodyW / 2, bodyBottom
    },
        (Color) {
        60, 140, 205, 255
    }
    );
    DrawTriangle(
        (Vector2) {
        cx + bodyW / 2, bodyBottom - 14
    },
        (Vector2) {
        cx + bodyW / 2 + 14, bodyBottom + 10
    },
        (Vector2) {
        cx + bodyW / 2, bodyBottom
    },
        (Color) {
        60, 140, 205, 255
    }
    );

    /* Engine base - dark strip where the flame emerges */
    DrawRectangle((int)(cx - bodyW / 2 + 3), (int)bodyBottom, (int)(bodyW - 6), 6, (Color) { 70, 70, 80, 255 });

    if (thrusting)
    {
        float flicker = (float)(rand() % 20);
        float flameLen = 45.0f + flicker;

        /* Outer flame - wide, deep red-orange */
        DrawTriangle(
            (Vector2) {
            cx - 16, bodyBottom + 4
        },
            (Vector2) {
            cx + 16, bodyBottom + 4
        },
            (Vector2) {
            cx, bodyBottom + 4 + flameLen
        },
            (Color) {
            235, 70, 20, 220
        }
        );
        /* Mid flame - bright orange */
        DrawTriangle(
            (Vector2) {
            cx - 11, bodyBottom + 6
        },
            (Vector2) {
            cx + 11, bodyBottom + 6
        },
            (Vector2) {
            cx, bodyBottom + 6 + flameLen * 0.75f
        },
            (Color) {
            255, 150, 30, 235
        }
        );
        /* Inner flame - hot yellow */
        DrawTriangle(
            (Vector2) {
            cx - 6, bodyBottom + 6
        },
            (Vector2) {
            cx + 6, bodyBottom + 6
        },
            (Vector2) {
            cx, bodyBottom + 6 + flameLen * 0.5f
        },
            (Color) {
            255, 230, 110, 245
        }
        );
        /* White-hot core right at the nozzle */
        DrawTriangle(
            (Vector2) {
            cx - 3, bodyBottom + 6
        },
            (Vector2) {
            cx + 3, bodyBottom + 6
        },
            (Vector2) {
            cx, bodyBottom + 6 + flameLen * 0.28f
        },
            (Color) {
            255, 255, 230, 255
        }
        );

        /* Scattered spark particles for extra intensity */
        for (int i = 0; i < 5; i++)
        {
            float sx = cx + (float)((rand() % 26) - 13);
            float sy = bodyBottom + 10 + (float)(rand() % (int)flameLen);
            float sr = 1.0f + (float)(rand() % 3);
            DrawCircle((int)sx, (int)sy, sr, (Color) { 255, 200, 90, 180 });
        }

        /* Soft outer glow behind the flame */
        DrawCircle((int)cx, (int)(bodyBottom + 10), 22.0f, (Color) { 255, 140, 40, 40 });
    }
}

/* Finishes drawing on the virtual canvas, then scales it to fit the actual window while keeping 
the original proportions. This prevents stretching, with black bars filling any extra space. */
void EndFrameAndPresent(RenderTexture2D target)
{
    EndTextureMode();

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float scale = fminf((float)screenW / SCREEN_W, (float)screenH / SCREEN_H);

    Rectangle src = { 0, 0, (float)target.texture.width, -(float)target.texture.height };
    Rectangle dst = {
        (screenW - SCREEN_W * scale) * 0.5f,
        (screenH - SCREEN_H * scale) * 0.5f,
        SCREEN_W * scale,
        SCREEN_H * scale
    };

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(target.texture, src, dst, (Vector2) { 0, 0 }, 0.0f, WHITE);
    EndDrawing();
}

/* This is what lets the font and icon travel inside the exe
   instead of needing to sit next to it as separate files. */
unsigned char* LoadEmbeddedResource(int resourceId, int* outSize)
{
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) { *outSize = 0; return NULL; }
    HGLOBAL hData = LoadResource(hModule, hRes);
    if (!hData) { *outSize = 0; return NULL; }
    *outSize = (int)SizeofResource(hModule, hRes);
    return (unsigned char*)LockResource(hData);
}

int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "AstroLaunch Simulator");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    RenderTexture2D target = LoadRenderTexture(SCREEN_W, SCREEN_H);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    /* Window icon - loaded from the PNG baked into the exe, not a disk file */
    int iconDataSize = 0;
    unsigned char* iconData = LoadEmbeddedResource(IDR_ICON_PNG_DATA, &iconDataSize);
    if (iconData != NULL)
    {
        Image iconImg = LoadImageFromMemory(".png", iconData, iconDataSize);
        SetWindowIcon(iconImg);
        UnloadImage(iconImg);
    }

    SetTargetFPS(60);

    /* Font - loaded from the TTF baked into the exe, not C:/Windows/Fonts */
    int fontDataSize = 0;
    unsigned char* fontData = LoadEmbeddedResource(IDR_FONT_DATA, &fontDataSize);
    if (fontData != NULL)
        uiFont = LoadFontFromMemory(".ttf", fontData, fontDataSize, 32, NULL, 0);
    if (uiFont.texture.id == 0) uiFont = GetFontDefault(); /* fallback if embedded font fails to load */

    GameState state = STATE_MENU;
    int selectedPlanet = 0;

    Rocket rocketTemplate = { 25000.0f, 75000.0f, 1500000.0f, 300.0f, 0.5f, 10.0f };
    Rocket rocket = rocketTemplate;

    float altitude = 0.0f, velocity = 0.0f, fuel = rocket.fuelMass;
    bool launched = false, reachedSpace = false, reachedEscape = false;
    float massFlowRate = rocket.thrust / (rocket.isp * G0);

    Star stars[STAR_COUNT];
    for (int i = 0; i < STAR_COUNT; i++)
    {
        stars[i].x = (float)(rand() % SCREEN_W);
        stars[i].y = (float)(rand() % SCREEN_H);
        stars[i].brightness = 100 + (rand() % 155);
    }

    float menuPulse = 0.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        menuPulse += dt;

        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        /*STATE: MENU*/
        if (state == STATE_MENU)
        {
            if (IsKeyPressed(KEY_ENTER)) state = STATE_PLANET_SELECT;

            BeginTextureMode(target);
            ClearBackground((Color) { 8, 8, 20, 255 });
            for (int i = 0; i < STAR_COUNT; i++)
            {
                unsigned char b = (unsigned char)stars[i].brightness;
                DrawPixel((int)stars[i].x, (int)stars[i].y, (Color) { 255, 255, 255, b });
            }
            DrawCenteredText("ORBIXION", SCREEN_W / 2, 220, 42, RAYWHITE);
            DrawCenteredText("Launch Beyond Boundaries!", SCREEN_W / 2, 275, 18, LIGHTGRAY);

            float alpha = (sinf(menuPulse * 3.0f) + 1.0f) / 2.0f;
            Color promptColor = (Color){ 255,255,255,(unsigned char)(120 + alpha * 135) };
            DrawCenteredText("Press ENTER to begin", SCREEN_W / 2, 400, 22, promptColor);

            DrawCenteredText("Built by Hajer Chetoui with C + raylib", SCREEN_W / 2, SCREEN_H - 40, 14, GRAY);
            DrawCenteredText("F11 = Fullscreen", SCREEN_W / 2, SCREEN_H - 20, 12, (Color) { 150, 150, 160, 255 });
            EndFrameAndPresent(target);
            continue;
        }

        /*STATE: PLANET SELECT*/
        if (state == STATE_PLANET_SELECT)
        {
            if (IsKeyPressed(KEY_RIGHT)) selectedPlanet = (selectedPlanet + 1) % 3;
            if (IsKeyPressed(KEY_LEFT))  selectedPlanet = (selectedPlanet + 2) % 3;
            if (IsKeyPressed(KEY_ONE)) selectedPlanet = 0;
            if (IsKeyPressed(KEY_TWO)) selectedPlanet = 1;
            if (IsKeyPressed(KEY_THREE)) selectedPlanet = 2;

            if (IsKeyPressed(KEY_ENTER))
            {
                rocket = rocketTemplate;
                altitude = 0.0f; velocity = 0.0f; fuel = rocket.fuelMass;
                launched = false; reachedSpace = false; reachedEscape = false;
                state = STATE_SIMULATION;
            }

            BeginTextureMode(target);
            ClearBackground((Color) { 12, 12, 25, 255 });
            DrawCenteredText("CHOOSE YOUR LAUNCH SITE", SCREEN_W / 2, 50, 30, RAYWHITE);
            DrawCenteredText("LEFT / RIGHT to choose  -  ENTER to launch", SCREEN_W / 2, 90, 16, LIGHTGRAY);

            int cardW = 260, cardH = 400, gap = 30;
            int totalW = cardW * 3 + gap * 2;
            int startX = SCREEN_W / 2 - totalW / 2;

            for (int i = 0; i < 3; i++)
            {
                const Planet* p = &PLANETS[i];
                int cx = startX + i * (cardW + gap);
                int cy = 140;
                bool selected = (i == selectedPlanet);

                Color panelColor = selected ? (Color) { 40, 40, 60, 255 } : (Color) { 22, 22, 35, 255 };
                DrawRectangle(cx, cy, cardW, cardH, panelColor);
                DrawRectangleLines(cx, cy, cardW, cardH, selected ? RAYWHITE : (Color) { 80, 80, 90, 255 });

                DrawPlanetIcon(i, p, cx + cardW / 2, cy + 85, 48.0f);
                DrawCenteredText(p->name, cx + cardW / 2, cy + 155, 22, RAYWHITE);

                float g = GravityAt(p, 0);
                float vesc = sqrtf(2.0f * p->mu / p->radius);

                DrawTextF(TextFormat("Surface g: %.2f m/s^2", g), cx + 16, cy + 195, 14, LIGHTGRAY);
                DrawTextF(TextFormat("Escape v: %.0f m/s", vesc), cx + 16, cy + 217, 14, LIGHTGRAY);
                DrawTextF(p->rho0 > 0 ? "Dense atmosphere" : "No atmosphere", cx + 16, cy + 239, 14,
                    p->rho0 > 0 ? (Color) { 150, 220, 255, 255 } : (Color) { 255, 180, 120, 255 });

                DrawWrappedText(p->tagline, cx + 16, cy + 272, cardW - 32, 13, 17, GRAY);

                if (selected)
                    DrawCenteredText("SELECTED", cx + cardW / 2, cy + cardH - 30, 16, (Color) { 120, 255, 180, 255 });
            }

            EndFrameAndPresent(target);
            continue;
        }

        /* STATE: SIMULATION */
        const Planet* planet = &PLANETS[selectedPlanet];

        if (IsKeyPressed(KEY_ESCAPE)) { state = STATE_PLANET_SELECT; continue; }

        bool thrusting = IsKeyDown(KEY_SPACE) && fuel > 0.0f;

        if (IsKeyPressed(KEY_R))
        {
            altitude = 0.0f; velocity = 0.0f; fuel = rocket.fuelMass;
            launched = false; reachedSpace = false; reachedEscape = false;
        }

        int substeps = 8;
        float subDt = dt / substeps;
        for (int s = 0; s < substeps; s++)
        {
            if (altitude <= 0.0f && !thrusting) { velocity = 0.0f; continue; }
            launched = true;
            float currentMass = rocket.dryMass + fuel;

            float thrustForce = 0.0f;
            if (thrusting)
            {
                thrustForce = rocket.thrust;
                fuel -= massFlowRate * subDt;
                if (fuel < 0.0f) fuel = 0.0f;
            }

            float weight = currentMass * GravityAt(planet, altitude);
            float rho = AirDensityAt(planet, altitude);
            float dragForce = 0.5f * rho * velocity * fabsf(velocity) * rocket.dragCoeff * rocket.crossArea;

            float netForce = thrustForce - weight - dragForce;
            float acceleration = netForce / currentMass;

            velocity += acceleration * subDt;
            altitude += velocity * subDt;
            if (altitude < 0.0f) { altitude = 0.0f; velocity = 0.0f; }
        }

        float escapeVelocity = sqrtf(2.0f * planet->mu / planet->radius);
        if (altitude >= planet->spaceLine) reachedSpace = true;
        if (velocity >= escapeVelocity) reachedEscape = true;

        float currentMass = rocket.dryMass + fuel;
        float deltaVRemaining = rocket.isp * G0 * logf(currentMass / rocket.dryMass);

        float visibleRange = fmaxf(3000.0f, altitude * 1.3f + 500.0f);
        float pixelsPerMeter = (SCREEN_H - 100) / visibleRange;
        float groundY = SCREEN_H - 40.0f;
        float rocketScreenY = groundY - (altitude * pixelsPerMeter) - 30.0f;

        BeginTextureMode(target);
        Color sky = GetSkyColor(planet, altitude);
        ClearBackground(sky);

        float starAlpha = fminf(fmaxf((altitude - planet->spaceLine * 0.4f) / (planet->spaceLine * 0.6f + 1.0f), 0.0f), 1.0f);
        if (planet->rho0 <= 0.0f) starAlpha = 1.0f;
        for (int i = 0; i < STAR_COUNT; i++)
        {
            unsigned char b = (unsigned char)(stars[i].brightness * starAlpha);
            DrawPixel((int)stars[i].x, (int)stars[i].y, (Color) { 255, 255, 255, b });
        }

        float groundAlpha = fminf(fmaxf(1.0f - altitude / (planet->spaceLine * 0.3f + 2000.0f), 0.0f), 1.0f);
        if (groundAlpha > 0.0f)
        {
            Color gcol = planet->groundColor;
            gcol.a = (unsigned char)(255 * groundAlpha);
            DrawRectangle(0, (int)groundY, SCREEN_W, SCREEN_H - (int)groundY, gcol);
        }

        float lineY = groundY - (planet->spaceLine * pixelsPerMeter);
        if (lineY > -20 && lineY < SCREEN_H)
        {
            DrawLine(0, (int)lineY, SCREEN_W, (int)lineY, (Color) { 255, 255, 255, 120 });
            DrawTextF(planet->spaceLineLabel, 10, (int)lineY - 18, 14, (Color) { 255, 255, 255, 180 });
        }

        float rx = SCREEN_W / 2.0f;
        DrawRocketShip(rx, rocketScreenY, thrusting);

        DrawRectangle(10, 10, 270, 210, (Color) { 20, 20, 30, 190 });
        DrawRectangleLines(10, 10, 270, 210, (Color) { 255, 255, 255, 80 });
        DrawTextF(TextFormat("Launch site: %s", planet->name), 22, 18, 16, (Color) { 180, 220, 255, 255 });
        DrawTextF(TextFormat("Altitude: %.0f m", altitude), 22, 42, 18, RAYWHITE);
        DrawTextF(TextFormat("Velocity: %.1f m/s", velocity), 22, 64, 18, RAYWHITE);
        DrawTextF(TextFormat("Fuel: %.0f / %.0f kg", fuel, rocket.fuelMass), 22, 86, 18, RAYWHITE);
        DrawTextF(TextFormat("Mass: %.0f kg", currentMass), 22, 108, 18, RAYWHITE);
        DrawTextF(TextFormat("Delta-v left: %.0f m/s", deltaVRemaining), 22, 130, 18, (Color) { 150, 220, 255, 255 });
        DrawTextF(TextFormat("Gravity here: %.2f m/s^2", GravityAt(planet, altitude)), 22, 152, 15, (Color) { 180, 180, 190, 255 });

        float spaceProgress = fminf(altitude / planet->spaceLine, 1.0f);
        float escProgress = fminf(velocity / escapeVelocity, 1.0f);
        DrawTextF("To edge of atmosphere:", 22, 172, 13, LIGHTGRAY);
        DrawRectangle(22, 187, 244, 6, (Color) { 60, 60, 70, 255 });
        DrawRectangle(22, 187, (int)(244 * spaceProgress), 6, (Color) { 90, 200, 255, 255 });
        DrawTextF("To escape velocity:", 22, 197, 13, LIGHTGRAY);
        DrawRectangle(22, 212, 244, 6, (Color) { 60, 60, 70, 255 });
        DrawRectangle(22, 212, (int)(244 * escProgress), 6, (Color) { 255, 170, 80, 255 });

        if (!launched)
            DrawCenteredText("Hold SPACE to launch", SCREEN_W / 2, SCREEN_H / 2, 22, (Color) { 255, 255, 255, 220 });
        if (fuel <= 0.0f && launched)
            DrawCenteredText("OUT OF FUEL", SCREEN_W / 2, 40, 22, (Color) { 255, 90, 90, 255 });
        if (reachedSpace)
            DrawCenteredText(TextFormat("CLEARED %s", planet->name), SCREEN_W / 2, 70, 20, (Color) { 120, 255, 180, 255 });
        if (reachedEscape)
            DrawCenteredText("ESCAPE VELOCITY REACHED", SCREEN_W / 2, 100, 20, (Color) { 255, 215, 90, 255 });

        DrawTextF("R = Reset   ESC = Change planet", SCREEN_W - 300, 10, 15, (Color) { 255, 255, 255, 180 });

        EndFrameAndPresent(target);
    }

    UnloadFont(uiFont);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

