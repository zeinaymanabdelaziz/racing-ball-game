#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cfloat>  
// #include <GL/glu.h>  
#include <OpenGL/glu.h> 

using namespace std;

enum GameScreen { GAME, END_SCREEN };
GameScreen currentScreen = GAME;

int selectedEndOption = 0; 
int winnerIndex = -1;           // -1 = none, 0 = Player 1, 1 = Player 2
string playerNames[2] = {"Player 1", "Player 2"};
int playerScores[2] = {0, 0};

int countdownSeconds = 120;     // 2-minute timer for 1-player mode
int startTime = 0;              
bool timeExpired = false;       // Whether the player lost due to time

int gameMode = 1;
bool gameWon = false;
vector<pair<float, float>> claimedCheckpoints;

extern void setCamera(int playerIndex, int x, int y, int w, int h);
extern void drawPlayers();

const int NUM_PLAYERS = 2;
const float tileSize = 3.0f;

const float PLATFORM_HEIGHT = 5.0f;     
const float TRACK_SURFACE_HEIGHT = 1.0f;
const float PLAYER_HEIGHT_ABOVE_SURFACE = 0.5f;


struct TrackSegment {
    float x, z;         
    float angle;        
    float width;        
    bool isCheckpoint;
};

vector<TrackSegment> trackSegments;

struct Player {
    float x, y, z;
    float velX, velY, velZ;
    float rotX, rotZ;
    float facingAngle = 0.0f;
    int colorIndex;
    float respawnX, respawnZ, respawnY;
    int health = 100;
    int lives = 3;
    float regenCooldown = 0.0f;
    int lapsCompleted = 0;
};

Player players[NUM_PLAYERS];

float colorMap[4][3] = {
    {1, 0, 0}, 
    {0, 1, 0},  
    {0, 0, 1},  
    {1, 1, 0}  
};

struct FloatingText {
    string text;
    float life;         // Remaining time in seconds
    float r, g, b;
    int playerIndex;    // 0 or 1
};

vector<FloatingText> floatingTexts;

struct Pickup {
    float x, z;
    int value; // e.g., 10, 20, 30, 40, 50
    bool collected;
};
std::vector<Pickup> pickups;

GLuint loadTexture(const char* filename) {
    // Load image and generate OpenGL texture
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "Failed To Load Texture: " << filename << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

GLuint heartTexture;

void drawPlayerHealthBar(int viewPlayerIndex) {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    int barWidth = 100;
    int barHeight = 12;
    int margin = 10;
    int spacing = (gameMode == 1) ? 2 : 25;
    int x = ((gameMode == 2 || gameMode == 3 || gameMode == 4) && viewPlayerIndex == 1)
            ? w - barWidth - margin
            : ((gameMode == 2 || gameMode == 3 || gameMode == 4) ? w / 2 - barWidth + 485 : w - barWidth - margin);

    int y = h - barHeight - margin;

    Player& p = players[viewPlayerIndex];
    float healthPercent = std::max(0, p.health) / 100.0f;

    // Set up 2D drawing
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Draw "Health:" label
    void* font = GLUT_BITMAP_HELVETICA_12;
    std::string label = "Health:";
    int labelWidth = label.length() * 8;

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(x - labelWidth - spacing, y + 2);
    for (char c : label) glutBitmapCharacter(font, c);

    // Draw background bar (gray)
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + barWidth, y);
    glVertex2i(x + barWidth, y + barHeight);
    glVertex2i(x, y + barHeight);
    glEnd();

    // Draw red health fill
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + (int)(barWidth * healthPercent), y);
    glVertex2i(x + (int)(barWidth * healthPercent), y + barHeight);
    glVertex2i(x, y + barHeight);
    glEnd();

    // Draw heart textures for lives
    int heartSize = 20;
    int baseX = x;
    int baseY = y - heartSize - 6;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, heartTexture);
    glColor3f(1.0f, 1.0f, 1.0f);  // Use white for original texture color

    for (int i = 0; i < p.lives; ++i) {
        int xPos = baseX + i * (heartSize + spacing);
        int yPos = baseY;

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2i(xPos, yPos);
        glTexCoord2f(1.0f, 0.0f); glVertex2i(xPos + heartSize, yPos);
        glTexCoord2f(1.0f, 1.0f); glVertex2i(xPos + heartSize, yPos + heartSize);
        glTexCoord2f(0.0f, 1.0f); glVertex2i(xPos, yPos + heartSize);
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);

}


void generateTrack() {
    trackSegments.clear();
    
    const int numSegments = 1600;      // Doubled for more detail and smoothness
    const float totalLength = 1500.0f; // Tripled track length
    const float segmentLength = totalLength / numSegments;
    
    float x = 0.0f, z = 0.0f;
    float angle = 0.0f;
    
    for (int i = 0; i < numSegments; ++i) {
        float t = (float)i / numSegments;
        bool isCheckpoint = (i % (numSegments/15) == 0 && i > 0);
        
        if (t > 0.15f && t < 0.3f) {
            angle += 0.4f;
        } else if (t > 0.35f && t < 0.5f) {
            angle -= 0.6f;
        } else if (t > 0.55f && t < 0.7f) {
            angle += 0.8f;
        } else if (t > 0.75f) {
            angle -= 0.5f;
        }
        
        trackSegments.push_back({x, z, angle, 3.0f, isCheckpoint});
        
        float rad = angle * 3.14159f / 180.0f;
        x += segmentLength * sin(rad);
        z += segmentLength * -cos(rad);
    }
    
    players[0].lapsCompleted = 0;
    players[1].lapsCompleted = 0;

    players[0].x = trackSegments[0].x;
    players[0].z = trackSegments[0].z;
    players[0].respawnX = players[0].x;
    players[0].respawnZ = players[0].z;
    players[0].respawnY = TRACK_SURFACE_HEIGHT + PLAYER_HEIGHT_ABOVE_SURFACE;
    players[0].y = players[0].respawnY;  // Set initial Y position
    players[0].facingAngle = 180.0f;
    
    if (gameMode == 2 || gameMode == 3 || gameMode == 4) {
        players[1].x = trackSegments[0].x - 1.0f;
        players[1].z = trackSegments[0].z;
        players[1].respawnX = players[1].x;
        players[1].respawnZ = players[1].z;
        players[1].respawnY = TRACK_SURFACE_HEIGHT + PLAYER_HEIGHT_ABOVE_SURFACE;
        players[1].y = players[1].respawnY;  // Set initial Y position
        players[1].facingAngle = 180.0f;
    }

    // Generate treasure pickups for Treasure Hunt mode
    if (gameMode == 4) {
        pickups.clear();
        for (int i = 50; i < trackSegments.size(); i += 50) {
            float x = trackSegments[i].x;
            float z = trackSegments[i].z;
            int value = (rand() % 4 + 2) * 10;  // Random treasure value between 20 and 50
            pickups.push_back({x, z, value, false});
        }
    }

}

void drawTile(float x, float z, float angle, bool isCheckpoint) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(angle, 0.0f, 1.0f, 0.0f);
    glScalef(tileSize * 2.0f, 0.2f, tileSize);

    if (isCheckpoint)
        glColor3f(0.0f, 1.0f, 0.0f);
    else
        glColor3f(1.0f, 1.0f, 1.0f);

    glutSolidCube(1.0f);
    glPopMatrix();

    if (isCheckpoint) {
        glPushMatrix();
        glTranslatef(x, 1.2f, z);
        glColor3f(1.0f, 1.0f, 1.0f);
        glutSolidCube(0.5);
        glPopMatrix();
    }
}

void drawArrow(float x, float y, float z, float angle) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(180.0f - angle, 0.0f, 1.0f, 0.0f);

    glScalef(5.5f, 0.1f, 6.5f); // Scale to create a flat, wide arrow shape

    glColor3f(0.05f, 0.05f, 0.2f);

    // Draw three chevrons spaced along the Z-axis
    const float spacing = 0.7f;  // gap between them
    for (int i = 0; i < 3; ++i) {
        float zOff = -i * spacing;
        glBegin(GL_TRIANGLES);
            glVertex3f( 0.0f, 0.0f,  0.8f + zOff);  // tip
            glVertex3f(-0.4f, 0.0f,  0.0f + zOff);  // left corner
            glVertex3f( 0.4f, 0.0f,  0.0f + zOff);  // right corner
        glEnd();
    }

    glPopMatrix();
}


void drawTrackArrowsForPlayer(int playerIndex) {
    const size_t arrowStartIndex = 10;  // Skip first 10 segments (start of visible track)
    for (size_t i = arrowStartIndex; i < trackSegments.size(); i += 25) {
        const auto& seg = trackSegments[i];
        drawArrow(seg.x, 1.05f, seg.z, seg.angle);
    }
}


void drawTrack() {
    glPushMatrix();
    glColor3f(0.6f, 0.9f, 0.7f);
    glTranslatef(0.0f, -0.5f, 0.0f);  
    glScalef(500.0f, 1.0f, 500.0f);  
    glutSolidCube(1.0);
    glPopMatrix();
    
    // Draw the smooth track
    for (size_t i = 0; i < trackSegments.size() - 1; i++) {
        const auto& seg1 = trackSegments[i];
        const auto& seg2 = trackSegments[i+1];
        
        // Draw track segment
        glColor3f(1.0f, 1.0f, 1.0f);
        if (seg1.isCheckpoint) glColor3f(0.0f, 1.0f, 0.0f);
        
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= 10; j++) {
            float t = j / 10.0f;
            float x = seg1.x * (1-t) + seg2.x * t;
            float z = seg1.z * (1-t) + seg2.z * t;
            float angle = seg1.angle * (1-t) + seg2.angle * t;
            
            float perpAngle = angle + 90.0f;
            float rad = perpAngle * 3.14159f / 180.0f;
            float dx = sin(rad) * seg1.width;
            float dz = cos(rad) * seg1.width;
            
            glVertex3f(x + dx, 1.0f, z - dz);  // Top surface
            glVertex3f(x - dx, 1.0f, z + dz);  // Top surface
            glVertex3f(x + dx, 0.0f, z - dz);  // Bottom surface
            glVertex3f(x - dx, 0.0f, z + dz);  // Bottom surface
        }
        glEnd();
        
        // Draw checkpoint marker
        if (seg1.isCheckpoint) {
            glPushMatrix();
            glTranslatef(seg1.x, 1.5f, seg1.z);
            glColor3f(1.0f, 1.0f, 1.0f);
            glutSolidCube(0.5);
            glPopMatrix();
        }

    }
}

void drawPickups() {
    for (const auto& p : pickups) {
        if (p.collected) continue;

        glPushMatrix();
        glTranslatef(p.x, 1.2f, p.z);
        if (p.value == 10) glColor3f(1, 1, 1);
        else if (p.value == 20) glColor3f(0, 1, 0);
        else if (p.value == 30) glColor3f(0, 1, 1);
        else if (p.value == 40) glColor3f(1, 0.5f, 0);
        else glColor3f(1, 0, 1);

        glutSolidSphere(0.3, 12, 12);
        glPopMatrix();
    }
}

void drawPlayers(int viewPlayerIndex) {
    int playerCount = (gameMode == 1) ? 1 : 2;
    for (int i = 0; i < playerCount; ++i) {
        Player& p = players[i];
        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glRotatef(p.facingAngle, 0.0f, 1.0f, 0.0f);
        glColor3fv(colorMap[p.colorIndex]);
        glutSolidSphere(0.5, 20, 20);
        glPopMatrix();
    }

    drawPlayerHealthBar(viewPlayerIndex);

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);

    int xOffset = (gameMode == 1 || viewPlayerIndex == 0) ? (w / ((gameMode == 2 || gameMode == 3 || gameMode == 4) ? 2 : 1)) - 110 : w - 110;
    int yOffset = h - 40;
    Player& p = players[viewPlayerIndex];
    
    // Floating text stays
    int player0MsgY = h / 2 + 80;
    int player1MsgY = h / 2 + 80;
    for (const auto& txt : floatingTexts) {
        if (txt.playerIndex != viewPlayerIndex) continue;
        int centerX = (gameMode == 1 || txt.playerIndex == 0) ? w / 4 : (3 * w) / 4;
        int& centerY = (txt.playerIndex == 0) ? player0MsgY : player1MsgY;
        glColor3f(txt.r, txt.g, txt.b);
        glRasterPos2i(centerX - (txt.text.length() * 5), centerY);
        for (char c : txt.text)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        centerY -= 30;
    }

    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}


void setCamera(int playerIndex, int x, int y, int w, int h) {
    glViewport(x, y, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float)w / h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Player& p = players[playerIndex];
    float rad = p.facingAngle * 3.14159f / 180.0f;
    float camX = p.x - sin(rad) * 8.0f;
    float camY = p.y + 4.0f;
    float camZ = p.z - cos(rad) * 8.0f;

    gluLookAt(camX, camY, camZ, p.x, p.y + 1.0f, p.z, 0, 1, 0);
}

void updatePhysics() {
    if (gameWon || currentScreen == END_SCREEN) return;

    int playerCount = (gameMode == 1) ? 1 : 2;
    for (int i = 0; i < playerCount; ++i) {
        Player& p = players[i];
        bool onTrack = false;
        float minDistSq = FLT_MAX;
        int closestSeg = 0;

        // Find closest segment
        for (size_t j = 0; j < trackSegments.size(); j++) {
            float dx = p.x - trackSegments[j].x;
            float dz = p.z - trackSegments[j].z;
            float distSq = dx * dx + dz * dz;
            if (distSq < minDistSq) {
                minDistSq = distSq;
                closestSeg = j;
            }
        }

        const auto& seg = trackSegments[closestSeg];
        float dx = p.x - seg.x;
        float dz = p.z - seg.z;
        float distToCenter = sqrt(dx * dx + dz * dz);

        if (distToCenter < seg.width && p.y <= TRACK_SURFACE_HEIGHT + 2.0f) {
            onTrack = true;

            // Checkpoint update
            if (seg.isCheckpoint) {
                p.respawnX = seg.x;
                p.respawnZ = seg.z;
                p.respawnY = TRACK_SURFACE_HEIGHT + PLAYER_HEIGHT_ABOVE_SURFACE;

                if (!(seg.x == trackSegments[0].x && seg.z == trackSegments[0].z)) {
                    bool alreadyClaimed = false;
                    for (auto& cp : claimedCheckpoints) {
                        if (fabs(cp.first - seg.x) < 0.1f && fabs(cp.second - seg.z) < 0.1f) {
                            alreadyClaimed = true;
                            break;
                        }
                    }

                    if (!alreadyClaimed) {
                        claimedCheckpoints.push_back({seg.x, seg.z});
                        playerScores[i] += 20;

                        floatingTexts.push_back({
                            "+20 pts!",
                            2.0f,
                            1.0f, 1.0f, 0.0f,
                            i
                        });

                        if (gameMode == 2 || gameMode == 3 || gameMode == 4) {
                            int other = (i == 0 ? 1 : 0);
                            floatingTexts.push_back({
                                "Too Late!",
                                2.0f,
                                1.0f, 0.3f, 0.3f,
                                other
                            });
                        }
                    }
                }
            }

            // Win Condition: Must reach end and have enough score
            const int MIN_SCORE_TO_WIN = 40;
            // In Timed/Tresure-Hunt modes, loop player back to start after finishing track
            if ((gameMode == 3 || gameMode == 4) && closestSeg == trackSegments.size() - 1) {
                const auto& startSeg = trackSegments[0];
                p.x = startSeg.x + (i == 0 ? 0.0f : -1.0f);
                p.z = startSeg.z + 2.0f; 
                p.y = TRACK_SURFACE_HEIGHT + PLAYER_HEIGHT_ABOVE_SURFACE;
                p.velX = p.velY = p.velZ = 0;
                p.facingAngle = 180.0f;
                p.respawnX = p.x;
                p.respawnZ = p.z;
                p.respawnY = p.y;
                p.lapsCompleted++;

                claimedCheckpoints.clear(); 

                floatingTexts.push_back({
                    "Looped!",
                    2.0f,
                    0.8f, 0.8f, 1.0f,
                    i
                });
            }


            // GameMode 1 or 2: Check win condition at end
            if ((gameMode == 1 || gameMode == 2) &&
                closestSeg == trackSegments.size() - 1 && !gameWon) {
                
                const int MIN_SCORE_TO_WIN = 40;
                if (playerScores[i] >= MIN_SCORE_TO_WIN) {
                    gameWon = true;
                    winnerIndex = i;
                    currentScreen = END_SCREEN;
                } else {
                    floatingTexts.push_back({
                        "Need 40+ Points to Win!",
                        3.0f,
                        1.0f, 0.5f, 0.0f,
                        i
                    });
                }
            }
        }


        // Apply gravity and damage when off track
        if (!onTrack) {
            p.y -= 1.0f;

            if (p.y > -20 && p.health > 0) {
                p.health -= 1;
            }
        }

        p.x += p.velX;
        p.z += p.velZ;
        p.velX *= 0.9f;
        p.velZ *= 0.9f;

        if (gameMode == 4) {
            for (auto& pUp : pickups) {
                if (pUp.collected) continue;
                float dx = p.x - pUp.x;
                float dz = p.z - pUp.z;
                if (dx * dx + dz * dz < 1.0f) {
                    pUp.collected = true;
                    playerScores[i] += pUp.value;

                    floatingTexts.push_back({
                        "+" + to_string(pUp.value) + " pts!",
                        2.0f,
                        1.0f, 1.0f, 0.0f,
                        i
                    });
                }
            }
        }

        // Cooldown tick
        p.regenCooldown -= 0.016f;
        if (p.regenCooldown < 0.0f) p.regenCooldown = 0.0f;

        if (p.health <= 0 && !gameWon) {
            p.lives--;
            if (p.lives > 0) {
                // Respawn with full health
                p.x = p.respawnX;
                p.z = p.respawnZ;
                p.y = p.respawnY;
                p.velX = p.velY = p.velZ = 0;
                p.health = 100;
            } else {
                // No lives left = Game Over
                gameWon = true;
                winnerIndex = (gameMode == 1) ? 1 : (i == 0 ? 1 : 0); // Winner is the other player or nobody in solo
                currentScreen = END_SCREEN;
                return;
            }
        } else if (p.y < -20) {
            // Only respawn if fell, health remains as is
            p.x = p.respawnX;
            p.z = p.respawnZ;
            p.y = p.respawnY;
            p.velX = p.velY = p.velZ = 0;
        }

    }

    // Update floating messages
    for (auto it = floatingTexts.begin(); it != floatingTexts.end(); ) {
        it->life -= 0.016f;
        if (it->life <= 0)
            it = floatingTexts.erase(it);
        else
            ++it;
    }

    // Basic physical collision response between players
    if ((gameMode == 2 || gameMode == 3 || gameMode == 4) && !gameWon) {
        Player& p1 = players[0];
        Player& p2 = players[1];

        float dx = p2.x - p1.x;
        float dz = p2.z - p1.z;
        float distSq = dx * dx + dz * dz;
        float minDist = 1.0f;

        if (distSq < minDist * minDist && distSq > 0.0001f) {
            float dist = sqrt(distSq);
            float nx = dx / dist;
            float nz = dz / dist;
            float overlap = minDist - dist;

            float impactForce = 0.2f;
            p1.velX -= nx * impactForce;
            p1.velZ -= nz * impactForce;
            p2.velX += nx * impactForce;
            p2.velZ += nz * impactForce;

            float correction = 0.5f * overlap;
            p1.x -= nx * correction;
            p1.z -= nz * correction;
            p2.x += nx * correction;
            p2.z += nz * correction;
        }
    }
    // Timed Score Attack Mode: check for time expiration
    if ((gameMode == 3  || gameMode == 4) && !gameWon && !timeExpired) {
        int now = glutGet(GLUT_ELAPSED_TIME);
        int elapsedSeconds = (now - startTime) / 1000;

        if (elapsedSeconds >= countdownSeconds) {
            timeExpired = true;
            currentScreen = END_SCREEN;
            gameWon = true;

            // Determine winner by score
            if (playerScores[0] > playerScores[1]) {
                winnerIndex = 0;
            } else if (playerScores[1] > playerScores[0]) {
                winnerIndex = 1;
            } else {
                winnerIndex = -1; // tie
            }
        }
    }

}


bool keyState[256] = {false};
bool specialState[256] = {false};

void keyUp(unsigned char key, int, int) { keyState[key] = false; }
void specialUp(int key, int, int) { specialState[key] = false; }


void display() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    if (currentScreen == END_SCREEN) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Determine background color based on end reason
        if (gameMode == 1 && players[0].health <= 0) {
            glClearColor(0.3f, 0.0f, 0.0f, 1.0f);  
        } else if (gameMode == 1 && timeExpired) {
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  
        } if (winnerIndex == -1) {
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f); 
        } else if (players[winnerIndex].health <= 0) {
            glClearColor(0.3f, 0.0f, 0.0f, 1.0f);
        } else {
            glClearColor(0.0f, 0.1f, 0.3f, 1.0f);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, w, 0, h);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        void* font = GLUT_BITMAP_HELVETICA_18;
        string winText;

        if (gameMode == 1) {
            if (timeExpired) {
                winText = "Time's Up! You Lose.";
            } else if (players[0].health <= 0) {
                winText = "Game Over! You Lost All Your Health.";
            } else {
                winText = playerNames[0] + " Wins!";
            }
        } else {
            if (players[winnerIndex].health <= 0) {
                winText = "Game Over! " + playerNames[winnerIndex == 0 ? 1 : 0] + " Lost All Their Health.";
            } else {
                if (winnerIndex == -1)
                    winText = "It's a Tie!";
                else
                    winText = playerNames[winnerIndex] + " Wins!";

            }
        }
        glColor3f(1.0f, 1.0f, 1.0f);
        int textWidth = 0;
        for (char c : winText)
            textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, c);

        glRasterPos2i(w / 2 - textWidth / 2, h / 2 + 40);

        for (char c : winText) glutBitmapCharacter(font, c);

        // Final Scores line (centered below winText)
        string finalScores;
        if (gameMode == 1) {
            finalScores = playerNames[0] + ": " + to_string(playerScores[0]) + " pts";
        } else {
            finalScores = playerNames[0] + ": " + to_string(playerScores[0]) + " pts   |   " +
                        playerNames[1] + ": " + to_string(playerScores[1]) + " pts";
        }

        int scoreWidth = 0;
        for (char c : finalScores)
            scoreWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, c);

        int finalScoreY = h / 2 + 10;

        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2i(w / 2 - scoreWidth / 2, finalScoreY);
        for (char c : finalScores)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);


        string options[2] = {"Play Again", "Exit"};
        int optionYBase = h / 2 - 20;  
        int optionSpacing = 40;        

        for (int i = 0; i < 2; ++i) {
            int y = optionYBase - i * optionSpacing;
            if (i == selectedEndOption) {
                glColor3f(1.0f, 1.0f, 0.0f);
                glRasterPos2i(w / 2 - 60, y);
                glutBitmapCharacter(font, '>');
                glRasterPos2i(w / 2 - 40, y);
            } else {
                glColor3f(0.8f, 0.8f, 0.8f);
                glRasterPos2i(w / 2 - 40, y);
            }
            for (char c : options[i]) glutBitmapCharacter(font, c);
        }


        glutSwapBuffers();
        return;
    }

    // ===== Gradient Background =====
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
        glColor3f(0.2f, 0.4f, 0.8f);
        glVertex2f(0.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glColor3f(0.05f, 0.05f, 0.2f);
        glVertex2f(1.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    // ===== Game Rendering =====
    if (gameMode == 1) {
        setCamera(0, 0, 0, w, h);
        glDisable(GL_LIGHTING);
        drawTrack();
        drawTrackArrowsForPlayer(0);
        glEnable(GL_LIGHTING);
        drawPlayers(0);  
    } else if (gameMode == 2 || gameMode == 3 || gameMode == 4) {
        // Player 1 View
        setCamera(0, 0, 0, w / 2, h);
        glDisable(GL_LIGHTING);
        drawTrack();
        drawTrackArrowsForPlayer(0); 
        drawPickups();
        glEnable(GL_LIGHTING);
        drawPlayers(0);  // Left side

        // Player 2 View
        setCamera(1, w / 2, 0, w / 2, h);
        glDisable(GL_LIGHTING);
        drawTrack();
        drawTrackArrowsForPlayer(1); 
        drawPickups();
        glEnable(GL_LIGHTING);
        drawPlayers(1);  // Right side

        // Split screen line
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, w, 0, h);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 1.0f); 
        glLineWidth(4.0f); 
        glBegin(GL_LINES);
        glVertex2f(w / 200.0f, 0);
        glVertex2f(w / 200.0f, h);
        glEnd();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    // ===== Score and Timer Overlay =====
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    void* font = GLUT_BITMAP_HELVETICA_18;
    glColor3f(1.0f, 1.0f, 1.0f);

    if (gameMode == 1) {
        if (!gameWon && !timeExpired) {
            int now = glutGet(GLUT_ELAPSED_TIME);
            int remaining = countdownSeconds - (now - startTime) / 1000;
            string timeLabel = "Time Left: " + to_string(max(0, remaining)) + "s";
            glRasterPos2i(10, h - 45);
            for (char c : timeLabel) glutBitmapCharacter(font, c);
        }

        string label = playerNames[0] + ": " + to_string(playerScores[0]) + " pts";
        glRasterPos2i(10, h - 20);
        for (char c : label) glutBitmapCharacter(font, c);

    } else if (gameMode == 2 || gameMode == 3 || gameMode == 4) {
        // Shared timer calculation
        int now = glutGet(GLUT_ELAPSED_TIME);
        int remaining = countdownSeconds - (now - startTime) / 1000;
        remaining = max(0, remaining);

        // Player 1 score
        string label1 = playerNames[0] + ": " + to_string(playerScores[0]) + " pts";
        glRasterPos2i(10, h - 20);
        for (char c : label1) glutBitmapCharacter(font, c);

        // Player 1 laps (for mode 3 and 4)
        if (gameMode == 3 || gameMode == 4) {
            string lapLabel1 = "Laps: " + to_string(players[0].lapsCompleted);
            glRasterPos2i(10, h - 70);
            for (char c : lapLabel1) glutBitmapCharacter(font, c);
        }

        // Player 1 timer
        if ((gameMode == 3 || gameMode == 4) && !gameWon && !timeExpired) {
            string timeLabel1 = "Time Left: " + to_string(remaining) + "s";
            glRasterPos2i(10, h - 45);
            for (char c : timeLabel1) glutBitmapCharacter(font, c);
        }

        // Player 2 score
        string label2 = playerNames[1] + ": " + to_string(playerScores[1]) + " pts";
        glRasterPos2i(w / 2 + 10, h - 20);
        for (char c : label2) glutBitmapCharacter(font, c);

        // Player 2 laps (for mode 3 and 4)
        if (gameMode == 3 || gameMode == 4) {
            string lapLabel2 = "Laps: " + to_string(players[1].lapsCompleted);
            glRasterPos2i(w / 2 + 10, h - 70);
            for (char c : lapLabel2) glutBitmapCharacter(font, c);
        }

        // Player 2 timer
        if ((gameMode == 3 || gameMode == 4) && !gameWon && !timeExpired) {
            string timeLabel2 = "Time Left: " + to_string(remaining) + "s";
            glRasterPos2i(w / 2 + 10, h - 45);
            for (char c : timeLabel2) glutBitmapCharacter(font, c);
        }
    }


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

void timer(int) {
    float speed = 0.12f;

    // Player 1 (WASD)
    if (keyState['a']) players[0].facingAngle += 2.0f;
    if (keyState['d']) players[0].facingAngle -= 2.0f;
    if (keyState['w']) {
        float rad = players[0].facingAngle * 3.14159f / 180.0f;
        players[0].velX += sin(rad) * speed;
        players[0].velZ += cos(rad) * speed;
    }
    if (keyState['s']) {
        float rad = players[0].facingAngle * 3.14159f / 180.0f;
        players[0].velX -= sin(rad) * speed;
        players[0].velZ -= cos(rad) * speed;
    }

    // Player 2 (Arrow keys)
    if (gameMode == 2 || gameMode == 3 || gameMode == 4) {
        if (specialState[GLUT_KEY_LEFT]) players[1].facingAngle += 2.0f;
        if (specialState[GLUT_KEY_RIGHT]) players[1].facingAngle -= 2.0f;
        if (specialState[GLUT_KEY_UP]) {
            float rad = players[1].facingAngle * 3.14159f / 180.0f;
            players[1].velX += sin(rad) * speed;
            players[1].velZ += cos(rad) * speed;
        }
        if (specialState[GLUT_KEY_DOWN]) {
            float rad = players[1].facingAngle * 3.14159f / 180.0f;
            players[1].velX -= sin(rad) * speed;
            players[1].velZ -= cos(rad) * speed;
        }
    }

    if ((gameMode == 1 || gameMode == 3) && !gameWon && !timeExpired) {
        int now = glutGet(GLUT_ELAPSED_TIME);
        int elapsedSeconds = (now - startTime) / 1000;

        if (elapsedSeconds >= countdownSeconds) {
            timeExpired = true;
            currentScreen = END_SCREEN;

            if (gameMode == 1) {
                winnerIndex = 1; // solo mode: player loses if timer runs out
            } else if (gameMode == 3) {
                // Proper score-based evaluation
                if (playerScores[0] > playerScores[1]) {
                    winnerIndex = 0;
                } else if (playerScores[1] > playerScores[0]) {
                    winnerIndex = 1;
                } else {
                    winnerIndex = -1; // tie
                }
            }

            gameWon = true;
        }
    }


    updatePhysics();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyUnified(unsigned char key, int, int) {
    if (currentScreen == END_SCREEN) {
        if (key == 13) { // Enter key
            if (selectedEndOption == 0) {
                // Relaunch the player selection screen
                char* args[] = {(char*)"./screen", NULL};
                execv(args[0], args);
            } else if (selectedEndOption == 1) {
                exit(0);
            }
        }
        return;
    }

    keyState[key] = true; // Normal gameplay keys like WASD
}

void specialUnified(int key, int, int) {
    if (currentScreen == END_SCREEN) {
        if (key == GLUT_KEY_UP || key == GLUT_KEY_DOWN) {
            selectedEndOption = (selectedEndOption + 1) % 2;
            glutPostRedisplay();
        }
        return;
    }

    specialState[key] = true; // Arrow keys for player 2
}

int main(int argc, char** argv) {
    
    if (argc > 1) {
        int argMode = atoi(argv[1]);
        if (argMode >= 1 && argMode <= 4) gameMode = argMode;
        if (argc > 2) players[0].colorIndex = atoi(argv[2]);
        if (argc > 3 && (gameMode == 2 || gameMode == 3 || gameMode == 4)) players[1].colorIndex = atoi(argv[3]);
        if (argc > 4) playerNames[0] = argv[4];
        if (argc > 5 && (gameMode == 2 || gameMode == 3 || gameMode == 4)) playerNames[1] = argv[5];
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 600);
    glutCreateWindow("RollRush Game");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_FLAT);  
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  

    glEnable(GL_FOG);
    GLfloat fogColor[4] = {0.5f, 0.8f, 1.0f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 80.0f);
    glFogf(GL_FOG_END, 150.0f);

    generateTrack();

    if (gameMode == 1 || gameMode == 3)
        startTime = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutKeyboardUpFunc(keyUp);
    glutKeyboardFunc(keyUnified);
    glutSpecialFunc(specialUnified);
    glutSpecialUpFunc(specialUp);
    glutTimerFunc(0, timer, 0);

    heartTexture = loadTexture("heart.png");

    glutMainLoop();
    return 0;
}
