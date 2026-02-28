/**
* Author: Kareena Joseph 
* Assignment: Pong Clone
* Date due: 02/28/2026
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "CS3113/cs3113.h"
#include <math.h>

// Screen and timing configuration
constexpr int SCREEN_WIDTH  = 1000,
              SCREEN_HEIGHT = 600,
              FPS           = 60;

AppStatus gAppStatus = RUNNING;

float gPreviousTicks = 0.0f;

// Play area (whole screen)
constexpr float COURT_LEFT   = 0.0f,
                COURT_TOP    = 0.0f,
                COURT_RIGHT  = (float)SCREEN_WIDTH,
                COURT_BOTTOM = (float)SCREEN_HEIGHT;

// Assets
// P2 uses the same paddle texture as P1 (only one wii char paddle image that I got from google).
constexpr char P1_TEX_PATH[]   = "assets/game/wii_1.png",
               BALL_TEX_PATH[] = "assets/game/tennis_ball.png";

// Movement
// These are the speeds I tuned until it felt like actual a smooth game
constexpr float PADDLE_SPEED = 520.0f,
                BALL_SPEED_X = 430.0f,
                BALL_SPEED_Y = 240.0f;

// Everything is center-positioned (pos is the center)
Vector2 gP1Pos {0.0f, 0.0f},
        gP2Pos {0.0f, 0.0f},
        gP1Size{0.0f, 0.0f},
        gP2Size{0.0f, 0.0f};

// Requirement 3: 3 balls and only use the active ones
struct Ball
{
    Vector2 pos{0.0f, 0.0f};   // center position 
    Vector2 size{0.0f, 0.0f};  // width/height
    Vector2 vel{0.0f, 0.0f};
    bool    active = false;
};

Ball    gBalls[3];
int     gBallCount = 1;
Vector2 gBallSize{0.0f, 0.0f};

Vector2 gP1Move{0.0f, 0.0f},
        gP2Move{0.0f, 0.0f};

// Requirement 2: Single-player toggle (T)
// 2-player (P2 uses arrow keys).
// P2 becomes a simple COMP that follows the ball.
bool gSinglePlayer = false;

// Requirement 4: Game over
constexpr int WIN_SCORE = 5;
int  gLeftScore  = 0;
int  gRightScore = 0;
bool gGameOver   = false;

// Helper function: box-to-box collision
// If the overlap on BOTH axes is negative, we collided.
static bool isColliding(const Vector2 *posA, const Vector2 *sizeA,
                        const Vector2 *posB, const Vector2 *sizeB)
{
    float xDistance = fabs(posA->x - posB->x) - ((sizeA->x + sizeB->x) / 2.0f);
    float yDistance = fabs(posA->y - posB->y) - ((sizeA->y + sizeB->y) / 2.0f);

    if (xDistance < 0.0f && yDistance < 0.0f) return true;

    return false;
}

static void renderObject(const Texture2D *texture, const Vector2 *position,
                         const Vector2 *scale)
{
    Rectangle textureArea = {
        0.0f, 0.0f,
        static_cast<float>(texture->width),
        static_cast<float>(texture->height)
    };
    Rectangle destinationArea = {
        position->x,
        position->y,
        static_cast<float>(scale->x),
        static_cast<float>(scale->y)
    };
    Vector2 originOffset = {
        static_cast<float>(scale->x) / 2.0f,
        static_cast<float>(scale->y) / 2.0f
    };
    DrawTexturePro(
        *texture,
        textureArea, destinationArea, originOffset,
        0.0f, WHITE
    );
}
// note which direction it starts going.
static void ResetBall(Ball &b, bool serveRight)
{
    // Center of the court
    b.pos = { (COURT_LEFT + COURT_RIGHT) / 2.0f, (COURT_TOP + COURT_BOTTOM) / 2.0f };
    b.vel.x = serveRight ? BALL_SPEED_X : -BALL_SPEED_X;
    //Y positive is default, then ApplyBallCount will flip it for variety.
    b.vel.y = BALL_SPEED_Y;
}

// Activate exactly 1–3 balls but only update/render what's active.
static void ApplyBallCount(int count)
{
    if (count < 1) count = 1;
    if (count > 3) count = 3;

    gBallCount = count;

    // Turn on first N balls
    for (int i = 0; i < 3; i++)
        gBalls[i].active = (i < gBallCount);

    bool serveRight = (gLeftScore == gRightScore);
    for (int i = 0; i < gBallCount; i++)
    {
        ResetBall(gBalls[i], serveRight);

        gBalls[i].vel.y = (i % 2 == 0) ? BALL_SPEED_Y : -BALL_SPEED_Y;
        gBalls[i].pos.y += i * 28.0f;
    }
}

// Note: Restart match using R
static void ResetGame()
{
    gLeftScore = 0;
    gRightScore = 0;
    gGameOver = false;

    gP1Pos.y = (COURT_TOP + COURT_BOTTOM) / 2.0f;
    gP2Pos.y = (COURT_TOP + COURT_BOTTOM) / 2.0f;

    ApplyBallCount(gBallCount);
}

Texture2D gP1Texture{};
Texture2D gP2Texture{};
Texture2D gBallTexture{};


constexpr float PADDLE_SCALE = 0.18f; // smaller paddle
constexpr float BALL_SCALE   = 0.08f; // smaller tennis ball

void initialise();
void processInput();
void update();
void render();
void shutdown();

void initialise()
{
    // Create window and load all required textures
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Project 2 - Pong Variation");

    // Load textures (P2 uses the same texture as P1)
    gP1Texture   = LoadTexture(P1_TEX_PATH);
    gP2Texture   = gP1Texture;
    gBallTexture = LoadTexture(BALL_TEX_PATH);

    SetTargetFPS(FPS);

    // Setup paddles
    gP1Size = { gP1Texture.width * PADDLE_SCALE, gP1Texture.height * PADDLE_SCALE };
    gP1Pos  = { COURT_LEFT + (gP1Size.x / 2.0f) + 10.0f, (COURT_TOP + COURT_BOTTOM) / 2.0f };

    gP2Size = { gP2Texture.width * PADDLE_SCALE, gP2Texture.height * PADDLE_SCALE };
    gP2Pos  = { COURT_RIGHT - (gP2Size.x / 2.0f) - 10.0f, (COURT_TOP + COURT_BOTTOM) / 2.0f };

    // Setup balls 
    gBallSize = { gBallTexture.width * BALL_SCALE, gBallTexture.height * BALL_SCALE };

    for (int i = 0; i < 3; i++)
    {
        gBalls[i].size   = gBallSize;
        gBalls[i].pos    = { (COURT_LEFT + COURT_RIGHT) / 2.0f, (COURT_TOP + COURT_BOTTOM) / 2.0f };
        gBalls[i].vel    = { 0.0f, 0.0f };
        gBalls[i].active = false;
    }

    ApplyBallCount(1);

    gPreviousTicks = (float)GetTime();
}

void processInput()
{
    gP1Move = {0.0f, 0.0f};
    gP2Move = {0.0f, 0.0f};

    if (WindowShouldClose()) gAppStatus = TERMINATED;

    // Toggle 1-player / 2-player mode
    if (IsKeyPressed(KEY_T))
    {
        gSinglePlayer = !gSinglePlayer;
    }

    // Requirement 3: Change number of balls any time
    if (IsKeyPressed(KEY_ONE))   ApplyBallCount(1);
    if (IsKeyPressed(KEY_TWO))   ApplyBallCount(2);
    if (IsKeyPressed(KEY_THREE)) ApplyBallCount(3);

    // Requirement 4: Restart after game over
    if (gGameOver && IsKeyPressed(KEY_R))
    {
        ResetGame();
    }

    // Player 1: W/S
    if      (IsKeyDown(KEY_W)) gP1Move.y = -1.0f;
    else if (IsKeyDown(KEY_S)) gP1Move.y =  1.0f;

    // Player 2:
    // - 2-player mode: Up/Down controls
    // - single-player: simple COMP tracks the ball
    if (!gSinglePlayer)
    {
        if      (IsKeyDown(KEY_UP))   gP2Move.y = -1.0f;
        else if (IsKeyDown(KEY_DOWN)) gP2Move.y =  1.0f;
    }
    else
    {
        // The COMP should just follow the ball directly.
        if (gBalls[0].pos.y < gP2Pos.y)           gP2Move.y = -1.0f;
        else if (gBalls[0].pos.y > gP2Pos.y)      gP2Move.y =  1.0f;
        else                                      gP2Move.y =  0.0f;
    }
}

void update()
{
    // Delta time
    float ticks = (float)GetTime();
    float dt = ticks - gPreviousTicks;
    gPreviousTicks = ticks;

    if (gGameOver) return;

    // Update paddles
    gP1Pos.y += PADDLE_SPEED * gP1Move.y * dt;
    gP2Pos.y += PADDLE_SPEED * gP2Move.y * dt;

    float Wii_half1 = gP1Size.y / 2.0f;
    if (gP1Pos.y - Wii_half1 < COURT_TOP)    gP1Pos.y = COURT_TOP + Wii_half1;
    if (gP1Pos.y + Wii_half1 > COURT_BOTTOM) gP1Pos.y = COURT_BOTTOM - Wii_half1;

    float Wii_half2 = gP2Size.y / 2.0f;
    if (gP2Pos.y - Wii_half2 < COURT_TOP)    gP2Pos.y = COURT_TOP + Wii_half2;
    if (gP2Pos.y + Wii_half2 > COURT_BOTTOM) gP2Pos.y = COURT_BOTTOM - Wii_half2;

    // Update balls + wall bounce
    // Only active balls get updated (inactive ones are basically "not in the game").
    for (int i = 0; i < 3; i++)
    {
        if (!gBalls[i].active) continue;

        Ball &b = gBalls[i];

        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;

        // Top/bottom bounce
        float halfH = b.size.y / 2.0f;
        if (b.pos.y - halfH < COURT_TOP)
        {
            b.pos.y = COURT_TOP + halfH;
            b.vel.y *= -1.0f;
        }
        if (b.pos.y + halfH > COURT_BOTTOM)
        {
            b.pos.y = COURT_BOTTOM - halfH;
            b.vel.y *= -1.0f;
        }

        // Paddle collisions (box-to-box)
        // Only bounce if the ball is moving toward the paddle, so it doesn't get stuck inside.
        if (isColliding(&b.pos, &b.size, &gP1Pos, &gP1Size) && b.vel.x < 0.0f)
        {
            b.pos.x = gP1Pos.x + (gP1Size.x / 2.0f) + (b.size.x / 2.0f);
            b.vel.x *= -1.0f;
        }
        if (isColliding(&b.pos, &b.size, &gP2Pos, &gP2Size) && b.vel.x > 0.0f)
        {
            b.pos.x = gP2Pos.x - (gP2Size.x / 2.0f) - (b.size.x / 2.0f);
            b.vel.x *= -1.0f;
        }

        // Scoring
        float halfW = b.size.x / 2.0f;
        if (b.pos.x + halfW < COURT_LEFT)
        {
            // Ball go past P1 -> P2 gets a point, then re-serve the ball back into play.
            gRightScore++;
            ResetBall(b, false);
        }
        else if (b.pos.x - halfW > COURT_RIGHT)
        {
            // Ball go past P2 -> P1 gets a point, then re-serve.
            gLeftScore++;
            ResetBall(b,true);
        }
    }

    // Win condition
    if (gLeftScore >= WIN_SCORE || gRightScore >= WIN_SCORE)
    {
        gGameOver = true;
    }
}

void render()
{
    BeginDrawing();
    ClearBackground(Color{80, 190, 110, 255});

    // Center line (classic Pong vibes)
    DrawRectangle(SCREEN_WIDTH / 2 - 3, 0, 6, SCREEN_HEIGHT, Color{255,255,255,120});

    renderObject(&gP1Texture, &gP1Pos, &gP1Size);
    renderObject(&gP2Texture, &gP2Pos, &gP2Size);
    for (int i = 0; i < 3; i++)
        if (gBalls[i].active) renderObject(&gBallTexture, &gBalls[i].pos, &gBalls[i].size);

    EndDrawing();
}

void shutdown()
{
    if (gP1Texture.id != 0) UnloadTexture(gP1Texture);
    if (gP2Texture.id != 0 && gP2Texture.id != gP1Texture.id) UnloadTexture(gP2Texture);
    if (gBallTexture.id != 0) UnloadTexture(gBallTexture);

    CloseWindow();
}

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();
    return 0;
}
