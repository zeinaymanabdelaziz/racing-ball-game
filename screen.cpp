#include <GL/freeglut.h>
#include <string>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "stb_image.h"
using namespace std;

int screenState = 0;            // 0 = Main Menu, 1 = Multiplayer Submenu, 2 = Player Setup
int selectedOption = 0;

int gameMode = -1;              // 1 = SP, 2 = Race, 3 = Timed Score, 4 = Treasure Hunt
int currentPlayerIndex = 0;     // 0 = Player 1, 1 = Player 2
int colorSelectionIndex = 0;    

vector<string> playerNames = {"", ""};                              
vector<string> colorOptions = {"Red", "Green", "Blue", "Yellow"};   
int selectedColor[2] = {-1, -1};    // Indices into colorOptions chosen by each player.

bool typingName = true;          // true = typing name, false = displaying name
string nameInput = "";      

void drawText(float x, float y, const string& text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(font, c);
}

void drawEscHint() {
    glColor3f(0.8f, 0.8f, 0.8f);
    drawText(-0.95f, 0.95f, "Press ESC to Go Back");
}

void displayMenu() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glColor3f(1.0, 1.0, 0.0);
    drawText(-0.25f, 0.4f, "RollRush Game");

    // Draw Main Menu options with highlight on selected option
    if (screenState == 0) {
        vector<string> menuOptions = {"Single Player", "Multi-Player"};
        for (int i = 0; i < menuOptions.size(); ++i) {
            glColor3f(i == selectedOption ? 1.0 : 1.0,
                      i == selectedOption ? 0.0 : 1.0,
                      i == selectedOption ? 0.0 : 1.0);
            drawText(-0.25f, 0.2f - i * 0.2f, menuOptions[i]);
        }
    } else if (screenState == 1) {
        drawText(-0.25f, 0.3f, "Multi-Player Modes");

        // Mode title
        vector<string> subOptions = {"Race", "Timed Score Attack", "Treasure Hunt"};
        
        // Description below each mode
        vector<string> descriptions = {
            "Reach the end with 40+ points",
            "Score the most points in 2 minutes",
            "Collect hidden treasures before time runs out"
        };

        for (int i = 0; i < subOptions.size(); ++i) {
            float y = 0.1f - i * 0.25f;

            // Mode title
            glColor3f(i == selectedOption ? 1.0 : 1.0,
                    i == selectedOption ? 0.0 : 1.0,
                    i == selectedOption ? 0.0 : 1.0);
            drawText(-0.25f, y, subOptions[i]);

            // Description below each mode
            glColor3f(0.7f, 0.7f, 0.7f);
            drawText(-0.25f, y - 0.08f, descriptions[i], GLUT_BITMAP_HELVETICA_12);
        }
    }


    drawEscHint();
    glutSwapBuffers();
}

void displayPlayerSetup() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    string title = "Player " + to_string(currentPlayerIndex + 1) + " Setup";
    drawText(-0.3f, 0.6f, title);

    if (typingName) {
        drawText(-0.4f, 0.4f, "Enter Name: " + nameInput + "_");
    } else {
        drawText(-0.4f, 0.4f, "Name: " + playerNames[currentPlayerIndex]);
        drawText(-0.4f, 0.2f, "Choose Color:");

        float y = 0.0f;
        for (int i = 0; i < colorOptions.size(); ++i) {
            bool taken = (selectedColor[0] == i || selectedColor[1] == i);
            // Gray out taken colors for Player 2 in multiplayer modes
            if ((gameMode >= 2) && taken && currentPlayerIndex == 1){
                glColor3f(0.5f, 0.5f, 0.5f);
            } else {
                // Highlight selected color
                glColor3f(i == colorSelectionIndex ? 1.0f : 1.0f,
                          i == colorSelectionIndex ? 1.0f : 1.0f,
                          i == colorSelectionIndex ? 0.0f : 1.0f);
            }
            drawText(-0.3f, y - 0.2f * i, colorOptions[i]);
        }
    }

    drawEscHint();
    glutSwapBuffers();
}

void keySpecial(int key, int, int) {
    int maxOptions = (screenState == 0) ? 1 : 2;

    // Navigate menu options
    if (key == GLUT_KEY_UP) {
        selectedOption = (selectedOption - 1 + maxOptions + 1) % (maxOptions + 1);
        glutPostRedisplay();
    }
    if (key == GLUT_KEY_DOWN) {
        selectedOption = (selectedOption + 1) % (maxOptions + 1);
        glutPostRedisplay();
    }

    // Navigate color options during Player Setup
    if (screenState == 2 && !typingName) {
        if (key == GLUT_KEY_UP || key == GLUT_KEY_DOWN) {
            do {
                colorSelectionIndex = (key == GLUT_KEY_UP)
                    ? (colorSelectionIndex - 1 + colorOptions.size()) % colorOptions.size()
                    : (colorSelectionIndex + 1) % colorOptions.size();
            } while (gameMode != 1 && currentPlayerIndex == 1 && colorSelectionIndex == selectedColor[0]);
            glutPostRedisplay();
        }
    }
}

void keyNormal(unsigned char key, int, int) {
    // Handle Enter key for navigating menus
    if ((screenState == 0 || screenState == 1) && key == 13) {
        if (screenState == 0) {
            if (selectedOption == 0) {
                gameMode = 1;
                screenState = 2;
            } else {
                selectedOption = 0;
                screenState = 1;
            }
        } else if (screenState == 1) {
            gameMode = selectedOption + 2; // 0->2, 1->3, 2->4
            screenState = 2;
        }
        glutPostRedisplay();
        return;
    }

     // Handle ESC key for going back
    if (key == 27) { 
        if (screenState == 1) {
            screenState = 0;
            selectedOption = 1;
        } else if (screenState == 2) {
            screenState = (gameMode == 1) ? 0 : 1;
            selectedOption = 0;
            typingName = true;
            nameInput = "";
        }
        glutPostRedisplay();
        return;
    }

    // Handle name input and color selection
    if (screenState == 2) {
        if (typingName) {
            // Confirm name entry
            if (key == 13) {
                playerNames[currentPlayerIndex] = nameInput;
                nameInput = "";
                typingName = false;
                glutPostRedisplay();
            } 
            // Handle backspace
            else if (key == 8 && !nameInput.empty()) {
                nameInput.pop_back();
                glutPostRedisplay();
            } 
            // Add printable character to input
            else if (isprint(key)) {
                nameInput += key;
                glutPostRedisplay();
            }
        } else {
            // Confirm color selection and proceed
            if (key == 13) {
                int colorIndex = colorSelectionIndex;
                if ((gameMode >= 2) && currentPlayerIndex == 1 && colorIndex == selectedColor[0]) {
                    cout << "Color Already Taken by Player 1!" << endl;
                    return;
                }
                selectedColor[currentPlayerIndex] = colorIndex;

                // If multiplayer and Player 1 is done, switch to Player 2
                if ((gameMode >= 2) && currentPlayerIndex == 0) {
                    currentPlayerIndex = 1;
                    typingName = true;
                    nameInput = "";
                    colorSelectionIndex = 0;
                } else {
                    // Launch main game with execv
                    glutDestroyWindow(glutGetWindow());

                    string modeArg = to_string(gameMode);
                    string color1 = to_string(selectedColor[0]);
                    string color2 = to_string(selectedColor[1]);
                    string name1 = playerNames[0];
                    string name2 = playerNames[1];

                    char* args[7];
                    args[0] = (char *)"./main";
                    args[1] = (char *)modeArg.c_str();
                    args[2] = (char *)color1.c_str();
                    args[3] = (gameMode >= 2) ? (char *)color2.c_str() : NULL;
                    args[4] = (char *)name1.c_str();
                    args[5] = (gameMode >= 2) ? (char *)name2.c_str() : NULL;
                    args[6] = NULL;

                    execv(args[0], args);
                    perror("Failed to Launch Game");
                    exit(1);
                }
                glutPostRedisplay();
            }
        }
    }
}

void initGraphics() {
    glClearColor(0.0, 0.0, 0.4, 1.0);   // Set background color (dark blue)
    glMatrixMode(GL_PROJECTION);        // Switch to projection matrix
    glLoadIdentity();                   
    gluOrtho2D(-1, 1, -1, 1);           // Set 2D orthographic projection
    glMatrixMode(GL_MODELVIEW);         // Switch back to modelview matrix
}

void displayWrapper() {
    if (screenState < 2)
        displayMenu();          // Main menu or multiplayer submenu
    else
        displayPlayerSetup();   // Player name and color selection
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(700, 700);
    glutCreateWindow("RollRush Game");

    initGraphics();
    glutDisplayFunc(displayWrapper);
    glutSpecialFunc(keySpecial);
    glutKeyboardFunc(keyNormal);

    glutMainLoop();
    return 0;
}
