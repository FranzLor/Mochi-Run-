/*******************************************************************************************
*
*   Mochi, Run - 2D Scroller
*
*   @author Franz Lor
*
*   @copyright (c) 2023 by Franz Lor, all rights reserved
*
*   License:
*   This software is provided 'as-is', without any express or implied warranty. In no event 
*   will the authors be held liable for any damages arising from the use of this software.
*   
*   Permission is granted to anyone to use this software for any purpose, including 
*   commercial applications, and to alter it and redistribute it freely, subject to the 
*   following restriction:
*   
*   1. The origin of this software must not be misrepresented; you must not claim that you 
*   wrote the original software. If you use this software in a product, an acknowledgment 
*   in the product documentation would be appreciated but is not required.
*   
*   2. Altered source versions must be plainly marked as such, and must not be misrepresented 
*   as being the original software.
*   
*   3. This notice may not be removed or altered from any source distribution.
*
********************************************************************************************/

#include <cstdlib>
#include <ctime>
#include "raylib.h"

//game states
enum GameState {
    INTRO,
    COUNTDOWN,
    GAMEPLAY,
    GAMEOVER
};

//try again state, yes or no
enum TryAgainState {
    YES,
    NO
};

//Mochi main animation data
struct AnimationData {
    Rectangle rec;
    Vector2 pos;
    int frame;
    float updateTime;
    float runningTime;
};

//Enemy drone properties
struct Enemy {
    Texture2D texture;
    Vector2 position;
    float speed;
    bool active;
    int frameCount;
    float frameTime;
    int currentFrame;
    float frameTimer;
    
};

//Enemy drone main animation data
struct Drone {
    Texture2D texture;
    int frameCount;
    float frameTime;
};

//Health pick up properties
struct HealthPickup {
    Texture2D texture;
    Vector2 position;
    float speed;
};

//Health system properties
struct HealthSystem {
    Texture2D heartTexture;
    int maxHealth;
    int currentHealth;
};

//Collision animation data
struct ImpactAnimation {
    Texture2D texture;
    int frameCount;   
    float frameTime;  
    int currentFrame; 
    float frameTimer; 
    Vector2 position; 
    bool active;      
};

//update animation state of running time
AnimationData updateAnimData(AnimationData data, float deltaTime, int maxFrame) {
    data.runningTime += deltaTime;
    if (data.runningTime >= data.updateTime) {
        data.runningTime = 0.0;
        //update animation frame
        data.rec.x = data.frame * data.rec.width;
        data.frame++;
        //reset frame
        if (data.frame > maxFrame) {
            data.frame = 0;
        }
    }
    return data;
}

//checks player on ground
bool isOnGround(AnimationData data ,int windowHeight){
    return data.pos.y >= windowHeight - data.rec.height;
}

//draw player health at the top left of the screen
void DrawPlayerHealth(HealthSystem healthSystem, bool isInGracePeriod) {
    int spacing = 10;
    int heartWidth = healthSystem.heartTexture.width;

    for (int i = 0; i < healthSystem.currentHealth; i++) {
        Vector2 heartPosition = {static_cast<float>(10 + i * (heartWidth + spacing)), 10.0f};
        //alternate upon collision
        Color textColor = isInGracePeriod ? RED : WHITE;
        DrawTexture(healthSystem.heartTexture, heartPosition.x, heartPosition.y, textColor);
    }
}

//checks for collision (player | health pickup)
bool CheckCollisionPlayerHealthPickup(AnimationData player, HealthPickup healthPickup) {
    return CheckCollisionRecs(
        //position of both player and health pickup
        (Rectangle){player.pos.x, player.pos.y, static_cast<float>(player.rec.width), static_cast<float>(player.rec.height)},
        (Rectangle){healthPickup.position.x, healthPickup.position.y, static_cast<float>(healthPickup.texture.width), static_cast<float>(healthPickup.texture.height)}
    );
}




//MAIN
int main() {
    //window dimensions
    const int screenWidth = 700;
    const int screenHeight = 300;

    //init window
    InitWindow(screenWidth, screenHeight, "Mochi, Run!");

    //init audio
    InitAudioDevice();

    //initialize game state to INTRO
    GameState gameState = INTRO;

    //initialize intro state time
    double introTimer = GetTime();
    //intro prompt animation
    float minScale = 1.0f;
    float maxScale = 1.15f;
    float scaleSpeed = 0.3f;
    float textScale = minScale;
    bool scalingUp = true;

    
    //initialize countdown state time
    double countdownTimer = GetTime();
    //countdown timer var
    int countdownValue = 4;
    int fontSize = 60;

    //initialize try again state
    TryAgainState tryAgainState = YES;
    bool tryAgainSelected = true;

    //initialize the player score and game time
    int playerScore = 0;
    double gameTime = 0.0;

    //initialize game velocity
    int velocity{0};

    //Mochi (player) properties
    //Mochi texture (for intro)
    Texture2D mochiIntroTexture = LoadTexture("textures/mochi_intro.png");
    //Mochi textures (for gameplay)
    Texture2D mochiTexture = LoadTexture("textures/mochi_running.png");
    Texture2D mochiJumpTexture = LoadTexture("textures/mochi_jump.png");
    //Mochi rec
    Rectangle playerRect = { 150.0f, screenHeight - mochiTexture.height - 20.0f, mochiTexture.width / 4, mochiTexture.height };


    //Mochi properties
    AnimationData mochiData;
    //texture
    mochiData.rec.width = mochiTexture.width/4;
    mochiData.rec.height = mochiTexture.height;
    mochiData.rec.x = 0;
    mochiData.rec.y = 0;
    //pos
    mochiData.pos.x = 150 - mochiData.rec.width/2;
    mochiData.pos.y = screenHeight - mochiData.rec.height;
    //animation
    mochiData.frame = 0;
    mochiData.updateTime = 1.0/16.0;
    mochiData.runningTime = 0.0;

    //gravity (pixel/frame/frame)/s
    const int gravity{1'600};
    //Mochi Air Bool
    bool isInAir{};
    //Mochi jump velocity 
    const int jumpVelocity{-580};

    //health pick up properties
    ///health pick up textures
    Texture2D healthPickupTexture = LoadTexture("textures/health.png");
    //store health pick up array
    const int maxHealthPickups = 10;
    HealthPickup healthPickups[maxHealthPickups] = {0};
    //init grace period
    double gracePeriodRemaining = 0.0;
    // Add a grace period timer
    double gracePeriodTimer = 0.0;
    //time before user | drone collision counts again (sec)
    const double gracePeriodDuration = 1.5;
    //initialize var to control the spawning of health
    float healthSpawnTimer = 0.0f;
    //before one spawns
    float healthSpawnRate = 15.0f;
    //alternate ground | air spawns
    bool spawnOnGround = true;
    
    // Initialize the player's health system
    HealthSystem playerHealth;
    playerHealth.heartTexture = LoadTexture("textures/mochi_health.png");
    playerHealth.maxHealth = 3;
    playerHealth.currentHealth = 3;

    //initialize drone types
    Drone drones[4];
    //drone 1
    drones[0].texture = LoadTexture("textures/drone1.png");
    drones[0].frameCount = 4;
    drones[0].frameTime = 1.0 / 10.0;
    //drone 2
    drones[1].texture = LoadTexture("textures/drone2.png");
    drones[1].frameCount = 8;
    drones[1].frameTime = 1.0 / 15.0;
    //drone 3
    drones[2].texture = LoadTexture("textures/drone3.png");
    drones[2].frameCount = 4;
    drones[2].frameTime = 1.0 / 10.0;
    //drone 4
    drones[3].texture = LoadTexture("textures/drone4.png");
    drones[3].frameCount = 4;
    drones[3].frameTime = 1.0 / 10.0;

    //collision box for drones
    Rectangle droneCollisionRectangles[4];
    for (int i = 0; i < 4; i++) {
        droneCollisionRectangles[i] = (Rectangle){0};

        //adjust width and height to make the collision box smaller
        //multiplier for width size
        droneCollisionRectangles[i].width = drones[i].texture.width / drones[i].frameCount * 0.5f;
        //multiplier for height size
        droneCollisionRectangles[i].height = drones[i].texture.height * 0.5f;
    }

    //enemy amount|frequency
    //max on screen before despawn
    const int maxEnemies = 10;
    //store enemy instances
    Enemy enemies[maxEnemies];
    
    //enemy spawn time (sec)
    //ground enemies
    const float minGroundEnemySpawnTime = 2.0f;  
    const float maxGroundEnemySpawnTime = 8.0f;
    //air enemy
    const float minAirEnemySpawnTime = 8.0f;
    const float maxAirEnemySpawnTime = 12.0f;
    //ground and air enemy initial time spawn
    float groundEnemySpawnTimer = 0.0f;
    float airEnemySpawnTimer = 0.0f;

    //player to enemy collision impact properties
    ImpactAnimation impactAnim;
    //texture
    impactAnim.texture = LoadTexture("textures/impact.png");
    //animation
    impactAnim.frameCount = 8;
    impactAnim.frameTime = 1.0 / 16.0;
    impactAnim.currentFrame = 0;
    impactAnim.frameTimer = 0.0;
    //initialize impact animation pos, update this on collision
    impactAnim.position = (Vector2){0, 0};
    //default state
    impactAnim.active = false;
    
    //background | foreground textures
    Texture2D background = LoadTexture("textures/background.png");
    Texture2D foreground = LoadTexture("textures/foreground.png");
    //initialize parallax
    float bgX{};
    float fgX{};

    //sounds
    //jump sound
    Sound jumpSound = LoadSound("sfx/jump.wav");
    //impact sound
    Sound impactSound = LoadSound("sfx/impact.wav");
    //health pickup sound
    Sound eatSound = LoadSound("sfx/eat.wav");
    //intro alarm sound (intro)
    Sound alarmSound = LoadSound("sfx/alarm.wav");
    //angry cat meow sound (intro)
    Sound angrySound = LoadSound("sfx/angry.wav");
    //game over sound
    Sound meowSound = LoadSound("sfx/meow.wav");

    //music soundtrack
    //intro
    Music menuSong = LoadMusicStream("sfx/menu.ogg");
    //gameplay
    Music soundTrack = LoadMusicStream("sfx/soundtrack.ogg");

    //FPS
    SetTargetFPS(60);
    //main window and game loop
    while (!WindowShouldClose()) {
        //update timer
        double currentTime = GetTime();

        //Switch game states
        switch (gameState) {
            //intro state
            case INTRO: {
                //intro song
                SetMusicVolume(menuSong, 0.5f);
                PlayMusicStream(menuSong);
                UpdateMusicStream(menuSong);

                //Calculate the elapsed time for the intro state
                double introElapsed = GetTime() - introTimer;

                // Check if the intro state has been running for less than 2 seconds
                if (introElapsed < 2.0) {
                    //
                    DrawText("Made by Franz", screenWidth / 2 - MeasureText("Made by Franz", 40) / 2, screenHeight / 2 - 40, 40, SKYBLUE);
                } else {
                    //increases scale factor of prompt
                    float scaleFactorBackground = 2.0;
                    float scaleFactorForeground = 2.0;
                    
                    Vector2 bgPos;
                    bgPos.x = 0;
                    bgPos.y = screenHeight - background.height * scaleFactorBackground;
                    DrawTextureEx(background, bgPos, 0.0, scaleFactorBackground, WHITE);

                    //adjust the position foreground texture
                    Vector2 fgPos;
                    fgPos.x = -130; // Adjust the x-coordinate to move it horizontally
                    fgPos.y = screenHeight - foreground.height * scaleFactorForeground; // Keep the same y-coordinate
                    DrawTextureEx(foreground, fgPos, 0.0, scaleFactorForeground, WHITE);

                    //draws the intro foreground
                    Vector2 introPos = {10.0f, static_cast<float>(screenHeight - mochiIntroTexture.height - 10)};

                    //draws the Mochi shadow (only in intro)
                    Color shadowColor = (Color){0, 0, 0, 150}; // Adjust the alpha value for transparency
                    Vector2 shadowCenter;
                    shadowCenter.x = introPos.x + mochiIntroTexture.width / 2 - 115;
                    shadowCenter.y = screenHeight - 20;
                    int shadowWidth = mochiIntroTexture.width;
                    int shadowHeight = 7;
                    DrawEllipse((int)shadowCenter.x, (int)shadowCenter.y, shadowWidth, shadowHeight, shadowColor);
                    //draws Mochi intro texture
                    DrawTexture(mochiIntroTexture, (int)introPos.x, (int)introPos.y, RAYWHITE);

                    //Title text
                    //set outline color
                    Color outlineColor = BLACK;
                    //set the text color
                    Color textColor = MAGENTA;
                    //calculate the text width
                    int textWidth = MeasureText("Mochi, Run!", 60);
                    //gets the position of the text
                    int textX = screenWidth / 2 - textWidth / 2;
                    int textY = screenHeight / 2 - 40;
                    //draws the text with an outline
                    int outlineSize = 3;
                    for (int i = -outlineSize; i <= outlineSize; i++) {
                        for (int j = -outlineSize; j <= outlineSize; j++) {
                            if (i != 0 || j != 0) {
                                DrawText("Mochi, Run!", textX + i, textY + j, 60, outlineColor);
                            }
                        }
                    }
                    //draws the main text over the outline
                    DrawText("Mochi, Run!", textX, textY, 60, textColor);
                    //prompt bool animation
                    if (scalingUp) {
                        textScale += scaleSpeed * GetFrameTime();
                        if (textScale >= maxScale) {
                            scalingUp = false;
                        }
                    } else {
                        textScale -= scaleSpeed * GetFrameTime();
                        if (textScale <= minScale) {
                            scalingUp = true;
                        }
                    }

                    //draws the "Press [SPACE] to Start" text with the current scale
                    DrawText("Press [SPACE] to START", (screenWidth - 355) - MeasureText("Press [SPACE] to START", 20 * textScale) / 2, screenHeight - 25, 20 * textScale, RAYWHITE);
            
                    //check for key press to transition to the countdown state
                    if (IsKeyDown(KEY_SPACE)) {
                        StopMusicStream(menuSong);
                        gameState = COUNTDOWN;
                    }

                    //exit button (works throughout prog)
                    if (IsKeyDown(KEY_ESCAPE)) {
                        break;
                        return 0;
                    }
                }
                break;
            }

            //countdown state
            case COUNTDOWN: {
                //calculate countdown time
                double countdownElapsed = currentTime - countdownTimer;
                
                //countdown greater than 1 sec
                if (countdownElapsed >= 1.0) {
                    countdownValue--;
                    if (countdownValue >= 0) {
                        countdownTimer = currentTime;
                    }
                }

                //delay before showing run text
                double runDelayElapsed = countdownElapsed - 1;
                double runDelay = 1.0;
                
                //countdown less than 0
                if (countdownValue < 0) {
                    //draws run text after countdonw
                    if (runDelayElapsed < runDelay) {  
                        DrawText("Run!", screenWidth / 2 - MeasureText("Run!", fontSize) / 2, screenHeight / 2 - fontSize / 2, fontSize, RED);
                    } else {
                        gameState = GAMEPLAY;

                        //reset game variables
                        mochiData.pos.x = 150 - mochiData.rec.width / 2;
                        mochiData.pos.y = screenHeight - mochiData.rec.height;
                        gameTime = 0.0;
                        playerScore = 0;
                        gracePeriodRemaining = 0.0;
                        playerHealth.currentHealth = playerHealth.maxHealth;
                        gracePeriodRemaining = 0.0;
                        //enemy drones
                        for (int i = 0; i < maxEnemies; i++) {
                            enemies[i].active = false;
                        }
                        //health pickups
                        for (int i = 0; i < maxHealthPickups; i++) {
                            healthPickups[i].texture.id = 0;
                        }
                    }
                    PlaySound(angrySound);
                } else {
                    //draws countdown
                    DrawText(TextFormat("%d", countdownValue), screenWidth / 2 - MeasureText(TextFormat("%d", countdownValue), fontSize) / 2, screenHeight / 2 - fontSize / 2, fontSize, MAGENTA);
                    PlaySound(alarmSound);
                }
                break;
            }

            //gameplay state, main gameplay
            case GAMEPLAY: {
                //sound control
                SetMusicVolume(soundTrack, 0.2f);  // Adjust the volume as needed
                PlayMusicStream(soundTrack);
                UpdateMusicStream(soundTrack);

                //delta time
                const float dT{GetFrameTime()};
                //game time | score
                gameTime += GetFrameTime();
                playerScore = (int)(gameTime * 1000);


                //background scroll, reset for infinite effect
                bgX -= 40 * dT;
                if (bgX <= -background.width * 2.8f) {
                    bgX = 0.0f;
                }
                //foreground scroll
                fgX -= 120 * dT;
                if (fgX <= -foreground.width * 1.2f) {
                    fgX = 0.0f;
                }

                //draw backgrounds
                Vector2 bg1Pos{bgX, 0.0f};
                DrawTextureEx(background, bg1Pos, 0.0f, 2.8f, WHITE);
                //second background, reset
                Vector2 bg2Pos = {bgX + background.width * 2.8f, 0.0f};
                DrawTextureEx(background, bg2Pos, 0.0f, 2.8f, WHITE);

                //draw foregrounds
                Vector2 fg1Pos{fgX, -18.0f};
                DrawTextureEx(foreground, fg1Pos, 0.0f, 1.2f, WHITE);
                //second foreground, reset
                Vector2 fg2Pos = {fgX + foreground.width * 1.2f, -18.0f};
                DrawTextureEx(foreground, fg2Pos, 0.0f, 1.2f, WHITE);

                //Update mochi position
                mochiData.pos.y += velocity * dT;

                //update Mochi animation frame (not in air)
                if (!isInAir) {
                    mochiData = updateAnimData(mochiData, dT, 4);
                }

                //Mochi ground check
                if (isOnGround(mochiData, screenHeight)) {
                    //Mochi on ground
                    velocity = 0;
                    isInAir = false;

                    //set the Y position to the exact position of the ground after landing
                    mochiData.pos.y = screenHeight - mochiData.rec.height; 
                } else {
                    //Mochi in air
                    velocity += gravity * dT;
                    isInAir = true;
                }
        
                //jump check 
                if (IsKeyPressed(KEY_SPACE) && !isInAir) {
                    velocity += jumpVelocity;
                    //use jump texture when jumping
                    mochiData.rec.width = mochiJumpTexture.width / 4;
                    //reset y pos
                    mochiData.pos.y = screenHeight - mochiData.rec.height;

                    PlaySound(jumpSound);
                } else {
                     //use running texture when not jumping
                    mochiData.rec.width = mochiTexture.width / 4;
                
                }

                //draw Mochi, alternating running |  jumping textures
                DrawTextureRec(isInAir ? mochiJumpTexture : mochiTexture, mochiData.rec, mochiData.pos, WHITE);

                //update health pickups time
                healthSpawnTimer += GetFrameTime();
                
                //spawn health pickups
                if (healthSpawnTimer >= healthSpawnRate) {
                    //reset the spawn timer after initial
                    healthSpawnTimer = 0.0f;

                    //find an available health pickup slot in the array
                    for (int i = 0; i < maxHealthPickups; i++) {
                        if (healthPickups[i].texture.id == 0) {
                            //initialize a new health pickup
                            healthPickups[i].texture = healthPickupTexture;
                            healthPickups[i].speed = 200;
                            if (spawnOnGround) {
                                //spawn on the ground
                                healthPickups[i].position = (Vector2){screenWidth, screenHeight - (healthPickupTexture.height + 10)};
                            } else {
                                //spawn in the air
                                healthPickups[i].position = (Vector2){screenWidth, screenHeight - (healthPickupTexture.height + 120)};
                            }
                            break;
                        }
                    }

                    //randomize next spawn rate (15 | 30 seconds)
                    healthSpawnRate = GetRandomValue(15, 30);
                    //alternate between ground and air spawns
                    spawnOnGround = !spawnOnGround;
                }

                //update the position of active health pickups
                for (int i = 0; i < maxHealthPickups; i++) {
                    if (healthPickups[i].texture.id != 0) {
                        healthPickups[i].position.x -= healthPickups[i].speed * GetFrameTime();
                    }
                }

                //check for collisions with health pickups and collect them
                for (int i = 0; i < maxHealthPickups; i++) {
                    if (healthPickups[i].texture.id != 0) {
                        if (CheckCollisionPlayerHealthPickup(mochiData, healthPickups[i])) {
                            if (playerHealth.currentHealth < playerHealth.maxHealth) {
                                //increase player's health by 1
                                playerHealth.currentHealth++;

                                //eat sound effect
                                PlaySound(eatSound);
                            }
                            //set the health pickup invisible
                            healthPickups[i].texture.id = 0;
                        }
                    }
                }

                //draws health pickups
                for (int i = 0; i < maxHealthPickups; i++) {
                    if (healthPickups[i].texture.id != 0) {
                        DrawTexture(healthPickupTexture, healthPickups[i].position.x, healthPickups[i].position.y, WHITE);
                    }
                }

                //checks for collisions player and enemy collsions during grace period
                if (gracePeriodRemaining > 0.0) {
                    gracePeriodRemaining -= GetFrameTime();
                } else {
                    for (int i = 0; i < maxEnemies; i++) {
                        if (enemies[i].active) {
                            if (CheckCollisionRecs(
                                (Rectangle){mochiData.pos.x, mochiData.pos.y, mochiData.rec.width, mochiData.rec.height},
                                (Rectangle){enemies[i].position.x, enemies[i].position.y, droneCollisionRectangles[3].width, droneCollisionRectangles[3].height})) {
                                //out of lives
                                if (playerHealth.currentHealth <= 0) {
                                    gameState = GAMEOVER;

                                    //play meow sound
                                    PlaySound(meowSound);
                                } else {
                                    //decrease player health
                                    PlaySound(impactSound);
                                    playerHealth.currentHealth--;

                                    //set the grace period remaining time to 1.5 sec
                                    gracePeriodRemaining = gracePeriodDuration;

                                    //update the impact animation position to the collision point
                                    impactAnim.position = (Vector2){enemies[i].position.x, mochiData.pos.y};
                                    impactAnim.active = true;

                                    //turn the collided drone invisible
                                    enemies[i].active = false;
                                }
                            }
                        }
                    }
                }
                //during impact, animation frames
                if (impactAnim.active) {
                    impactAnim.frameTimer += GetFrameTime();
                    if (impactAnim.frameTimer >= impactAnim.frameTime) {
                        impactAnim.frameTimer = 0.0;
                        impactAnim.currentFrame++;
                        //deactivate impact animation
                        if (impactAnim.currentFrame >= impactAnim.frameCount) {
                            impactAnim.currentFrame = 0;
                            impactAnim.active = false;
                        }
                    }
                }

                //update enemy spawning timers
                groundEnemySpawnTimer += GetFrameTime();

                //spawns ground enemies randomly
                if (groundEnemySpawnTimer >= GetRandomValue(minGroundEnemySpawnTime, maxGroundEnemySpawnTime)) {
                    for (int i = 0; i < maxEnemies; i++) {
                        if (!enemies[i].active) {
                            int droneType = GetRandomValue(0, 2);
                            enemies[i].texture = drones[droneType].texture;
                            enemies[i].frameCount = drones[droneType].frameCount;
                            enemies[i].frameTime = drones[droneType].frameTime;
                            enemies[i].currentFrame = 0;
                            enemies[i].frameTimer = 0.0f;

                            //ground pos 1 (on ground)
                            int groundPosition1 = screenHeight - enemies[i].texture.height;
                            //ground pos 2 (slightly above)
                            int groundPosition2 = screenHeight - enemies[i].texture.height - 45;
                            //selected ground pos 1 or 2
                            int selectedPosition = GetRandomValue(0, 1);
                            
                            if (selectedPosition == 0) {
                                enemies[i].position = (Vector2){screenWidth, groundPosition1};
                            } else {
                                enemies[i].position = (Vector2){screenWidth, groundPosition2};
                            }

                            enemies[i].speed = GetRandomValue(400, 800);
                            enemies[i].active = true;
                            //reset ground enemy timer
                            groundEnemySpawnTimer = 0.0f;
                            break;
                        }
                    }
                }

                //updates enemy spawning timers
                airEnemySpawnTimer += GetFrameTime();
                //spawn air enemies randomly
                if (airEnemySpawnTimer >= GetRandomValue(minAirEnemySpawnTime, maxAirEnemySpawnTime)) {
                    for (int i = 0; i < maxEnemies; i++) {
                        if (!enemies[i].active) {
                            //uses only drone 4 texture
                            enemies[i].texture = drones[3].texture;
                            enemies[i].frameCount = drones[3].frameCount;
                            enemies[i].currentFrame = 0;
                            enemies[i].frameTimer = 0.0f;
                            enemies[i].position = (Vector2) { screenWidth, (screenHeight - 145) - enemies[i].texture.height / 2 };
                            enemies[i].speed = GetRandomValue(200, 300);
                            enemies[i].active = true;
                            airEnemySpawnTimer = 0.0f;
                            break;
                        }
                    }
                }

                //update the position and animation frames of active enemies
                for (int i = 0; i < maxEnemies; i++) {
                    if (enemies[i].active) {
                        enemies[i].position.x -= enemies[i].speed * GetFrameTime();
                        
                        //check if the enemy is out of the screen
                        if (enemies[i].position.x + enemies[i].texture.width < 0) {
                            enemies[i].active = false;
                        }

                        enemies[i].frameTimer += GetFrameTime();
                        if (enemies[i].frameTimer >= enemies[i].frameTime) {
                            enemies[i].frameTimer = 0.0f;
                            enemies[i].currentFrame++;
                            if (enemies[i].currentFrame >= enemies[i].frameCount) {
                                enemies[i].currentFrame = 0;
                            }
                        }
                    }
                }

                //draws active enemies
                for (int i = 0; i < maxEnemies; i++) {
                    if (enemies[i].active) {
                        if (enemies[i].frameCount > 0) {
                            float frameWidth = static_cast<float>(enemies[i].texture.width) / enemies[i].frameCount;
                            float frameHeight = static_cast<float>(enemies[i].texture.height);

                            DrawTexturePro(enemies[i].texture,
                                (Rectangle) { static_cast<float>(enemies[i].currentFrame) * frameWidth, 0, frameWidth, frameHeight },
                                (Rectangle) { static_cast<float>(enemies[i].position.x), static_cast<float>(enemies[i].position.y), frameWidth, frameHeight }, Vector2{ 0, 0 }, 0, WHITE);
                        }
                    }
                }

                //draws impact animation if it's active
                if (impactAnim.active) {
                    float frameWidth = (float)impactAnim.texture.width / impactAnim.frameCount;
                    float frameHeight = (float)impactAnim.texture.height;
            
                    //draws the current frame of the impact animation at its position
                    DrawTexturePro(impactAnim.texture,
                        (Rectangle){(float)impactAnim.currentFrame * frameWidth, 0, frameWidth, frameHeight},
                        (Rectangle){impactAnim.position.x, impactAnim.position.y, frameWidth, frameHeight},
                        Vector2{0, 0},
                        0,
                        WHITE);
                }

                //draws player health at the top left of the screen
                DrawPlayerHealth(playerHealth, gracePeriodRemaining > 0.0);

                //score (conversion)
                int seconds = (int)gameTime;
                int tenthsOfASecond = (int)((gameTime - seconds) * 10);
                //draws the scorein "00000" format
                DrawText(TextFormat("%05d%01d", seconds, tenthsOfASecond), screenWidth - 100, 10, 20, MAGENTA);
                break;
            }

            //game over state
            case GAMEOVER: {
                //stops gameplay music
                StopMusicStream(soundTrack);

                //reset the player's position after death 
                velocity = 0;
                isInAir = false;

                //draws game over text
                DrawText("Meow Over", screenWidth / 2 - MeasureText("Meow Over", 70) / 2, screenHeight / 2 - 130, 70, RED);

                //grabs score thats converted
                int seconds = (int)gameTime;
                int tenthsOfASecond = (int)((gameTime - seconds) * 10);
                //draws score below game over text
                DrawText(TextFormat("Score: %05d%01d", seconds, tenthsOfASecond), screenWidth / 2 - MeasureText("Score: 00000", 20) / 2, screenHeight / 2 - 40, 20, WHITE);

                //draw try again prompt
                DrawText("Try Again?", screenWidth / 2 - MeasureText("Try Again?", 20) / 2, screenHeight / 2 + 40, 25, WHITE);
                //input handling, W (up) | S (down) on prompt
                if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
                    tryAgainSelected = true;
                } else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
                    tryAgainSelected = false;
                }
                //draws yes and no options with highlighitng
                if (tryAgainSelected) {
                    //main selection yes
                    DrawText("> Yes <", screenWidth / 2 - MeasureText("> Yes <", 20) / 2, screenHeight / 2 + 80, 20, GREEN);
                    DrawText("No", screenWidth / 2 - MeasureText("No", 20) / 2, screenHeight / 2 + 110, 20, WHITE);
                } else {
                    //main selection no
                    DrawText("Yes", screenWidth / 2 - MeasureText("Yes", 20) / 2, screenHeight / 2 + 80, 20, WHITE);
                    DrawText("> No <", screenWidth / 2 - MeasureText("> No <", 20) / 2, screenHeight / 2 + 110, 20, RED);
                }
                //check for user input in prompt
                if (IsKeyPressed(KEY_W)) {
                    tryAgainState = YES;
                } else if (IsKeyPressed(KEY_S)) {
                    tryAgainState = NO;
                } else if (IsKeyPressed(KEY_ENTER)) {
                    //try again yes
                    if (tryAgainState == YES) {
                       //reset game variables
                        gameState = COUNTDOWN;
                        countdownTimer = GetTime();
                        countdownValue = 3;
                        gameTime = 0.0;
                        playerScore = 0;
                        playerHealth.currentHealth = playerHealth.maxHealth;
                        gracePeriodRemaining = 0.0;
                        //reset the player position
                        mochiData.pos.x = 150 - mochiData.rec.width / 2;
                        mochiData.pos.y = screenHeight - mochiData.rec.height;
                        //clear out enemy and health pickup data
                        for (int i = 0; i < maxEnemies; i++) {
                            enemies[i].active = false;
                        }
                        for (int i = 0; i < maxHealthPickups; i++) {
                            healthPickups[i].texture.id = 0;
                        }
                        // Reset the impact animation
                        impactAnim.active = false;
                    //try again no
                    } else if (tryAgainState == NO) {
                        //return to the main menu or intro
                        //reset intro timer
                        countdownTimer = GetTime();
                        countdownValue = 3;
                        gameState = INTRO;
                    }
                }
                break;
            }
        }
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
    }
    //unload textures
    //mochi
    UnloadTexture(mochiIntroTexture);
    UnloadTexture(mochiTexture);
    UnloadTexture(mochiJumpTexture);
    //health
    UnloadTexture(healthPickupTexture);
    //background | foreground
    UnloadTexture(background);
    UnloadTexture(foreground);
    
    //unloading all drone textures
    for (int i = 0; i < 4; i++) {
        UnloadTexture(drones[i].texture);
    }

    //unload the sound
    UnloadSound(jumpSound);
    UnloadSound(impactSound);
    UnloadSound(eatSound);
    UnloadSound(alarmSound);
    UnloadSound(meowSound);
    UnloadSound(angrySound);

    //unload music
    UnloadMusicStream(menuSong);
    UnloadMusicStream(soundTrack);

    //close window and audio
    CloseAudioDevice();
    CloseWindow();

    return 0;
}