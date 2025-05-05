#include <GLUT/glut.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <algorithm> 
#include<iostream>
using namespace std;// Needed for std::remove_if

// Constants
const int GRID_ROWS = 5;
const int GRID_COLS = 10;
const float CELL_SIZE = 2.0f;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int ENEMY_SPAWN_INTERVAL = 200;
const int MAX_BASE_HEALTH = 10;

// Global Variables
int selectedRow = 0;
int selectedCol = 0;
int resources = 50;
int frameCount = 0;
int baseHealth = MAX_BASE_HEALTH;
int wave = 1;
bool gameOver = false;
bool gridOccupied[GRID_ROWS][GRID_COLS] = { false };
bool transitionRendered = false;
int level = 1;
int score = 0;
int maxScore = 0;
bool levelTransition = false;
int instructionPage = 1; // Tracks the current instruction page (1 = Level 1, 2 = Level 2, 3 = Level 3)

// Enums and Structures
enum EnemyType { NORMAL, FAST, TANK };
enum TowerType { BASIC, SLOW, AREA };
enum GameState { WELCOME, INSTRUCTIONS, PLAYING, GAME_OVER };
GameState gameState = WELCOME; // Start with the welcome screen

struct Enemy {
    float x, z;
    float speed;
    int health;
    int maxHealth;
    bool alive;
    EnemyType type;
};

struct Tower {
    int row, col;
    int cooldown;
    int level;
    TowerType type;
    int health; // Lifespan of the tower
};

struct Bullet {
    float x, z;
    float speed;
    int damage;
    bool active;
};

// Vectors
std::vector<Enemy> enemies;
std::vector<Tower> towers;
std::vector<Bullet> bullets;

// Function Prototypes
void spawnEnemy();
void drawCube(float x, float y, float z, float size);
void drawText(const std::string& text, int x, int y);
void drawGrid();
void drawTerrain();
void drawHealthBar(float x, float z, int health, int maxHealth);
void updateAndDrawEnemies();
void updateAndDrawBullets();
void towersShoot();
void display();
void timer(int = 0);
void keyboard(int key, int x, int y);
void normalKey(unsigned char key, int x, int y);
void reshape(int w, int h);
void init();

// Initialize OpenGL state
void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f); // Sky blue background
    srand(static_cast<unsigned>(time(0)));
}

void spawnEnemy() {
    int row = rand() % GRID_ROWS;
    Enemy e;
    e.x = GRID_COLS * CELL_SIZE;
    e.z = row * CELL_SIZE;

    int type = rand() % 3;
    if (type == 0) {
        e.speed = 0.02f + 0.002f * level;
        e.health = 80 + 10 * level;
        e.type = NORMAL;
    }
    else if (type == 1) {
        e.speed = 0.05f + 0.003f * level;
        e.health = 50 + 8 * level;
        e.type = FAST;
    }
    else {
        e.speed = 0.01f + 0.001f * level;
        e.health = 150 + 20 * level;
        e.type = TANK;
    }

    e.maxHealth = e.health;
    e.alive = true;
    enemies.push_back(e);
    if (level == 2) { // Medium
        enemies.push_back(e); // Spawn an additional enemy
    } else if (level == 3) { // Difficult
        enemies.push_back(e);
        enemies.push_back(e); // Spawn two additional enemies
    }
}

void drawCube(float x, float y, float z, float size) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glutSolidCube(size);
    glPopMatrix();
}

void drawText(const std::string& text, int x, int y) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glRasterPos2i(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawTowerHealthBar(float x, float z, int health, int maxHealth) {
    float w = 1.0f * health / maxHealth;
    glPushMatrix();
    glTranslatef(x, 1.5f, z);
    glScalef(w, 0.1f, 0.1f);
    glColor3f(0.0f, 1.0f, 0.0f); // Green for tower health
    glutSolidCube(1.0f);
    glPopMatrix();
}

void printMatrix(GLenum matrixMode) {
    GLfloat matrix[16]; // OpenGL matrices are 4x4
    glGetFloatv(matrixMode, matrix); // Retrieve the matrix values

    // Print the matrix in a 4x4 format
    std::cout << "Matrix (" << (matrixMode == GL_MODELVIEW ? "Model-View" : 
                                (matrixMode == GL_PROJECTION ? "Projection" : "Texture")) << "):" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << matrix[i * 4 + j] << " ";
        }
        std::cout << std::endl;
    }
}

void drawGrid() {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            glColor3f(0.4f, 0.6f, 0.4f);
            if (r == selectedRow && c == selectedCol)
                glColor3f(0.7f, 0.7f, 0.2f);

            glPushMatrix();
            glTranslatef(c * CELL_SIZE, 0.1f, r * CELL_SIZE);
            glScalef(CELL_SIZE, 0.1f, CELL_SIZE);
            glutSolidCube(1.0);
            glPopMatrix();
        }
    }

    for (auto& t : towers) {
        float x = t.col * CELL_SIZE;
        float z = t.row * CELL_SIZE;
        glColor3f(0.8f, 0.2f, 0.2f);

        switch (t.type) {
        case BASIC:
            drawCube(x, 0.5f, z, 0.8f);
            break;
        case SLOW:
            glPushMatrix();
            glTranslatef(x, 0.5f, z);
            glRotatef(-90, 1, 0, 0);
            GLUquadric* quad;
            quad = gluNewQuadric();
            gluCylinder(quad, 0.4f, 0.4f, 1.0f, 20, 20);
            gluDeleteQuadric(quad);
            glPopMatrix();
            break;
        case AREA:
            drawCube(x, 0.5f, z, 1.0f);
            glColor3f(0.2f, 0.8f, 0.2f);
            drawCube(x, 1.5f, z, 0.5f);
            break;
        }
        drawTowerHealthBar(x, z, t.health, 300);
    }
}

void drawTerrain() {
    glColor3f(0.0f, 0.6f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(-50.0f, 0.0f, -50.0f);
    glVertex3f(-50.0f, 0.0f, GRID_ROWS * CELL_SIZE + 50.0f);
    glVertex3f(GRID_COLS * CELL_SIZE + 50.0f, 0.0f, GRID_ROWS * CELL_SIZE + 50.0f);
    glVertex3f(GRID_COLS * CELL_SIZE + 50.0f, 0.0f, -50.0f);
    glEnd();
}


void drawHealthBar(float x, float z, int health, int maxHealth) {
    float w = 1.0f * health / maxHealth;
    glPushMatrix();
    glTranslatef(x, 1.5f, z);
    glScalef(w, 0.1f, 0.1f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void updateAndDrawEnemies() {
    for (auto& e : enemies) {
        if (!e.alive) continue;

        e.x -= e.speed;
        if (e.x < 0) { // Enemy reaches the end of the grid
            e.alive = false;
            baseHealth--;
            if (baseHealth <= 0) { // Check if lives are over
                gameOver = true;
                if (score > maxScore) maxScore = score; // Update max score
                return; // Exit early since the game is over
            }
        }

        glPushMatrix();
        glTranslatef(e.x, 0.6f, e.z);
        switch (e.type) {
        case NORMAL:
            glColor3f(0.9f, 0.9f, 0.2f);
            glutSolidCube(1.0f);
            break;
        case FAST:
            glColor3f(0.2f, 0.9f, 0.2f);
            glutSolidSphere(0.5f, 20, 20);
            break;
        case TANK:
            glColor3f(0.9f, 0.2f, 0.2f);
            glRotatef(-90, 1, 0, 0);
            glutSolidCone(0.6f, 1.2f, 20, 20);
            break;
        }
        glPopMatrix();

        drawHealthBar(e.x, e.z, e.health, e.maxHealth);
    }
}

void updateAndDrawBullets() {
    for (size_t i = 0; i < bullets.size(); ++i) {
        Bullet& b = bullets[i];
        if (!b.active) continue;
        b.x += b.speed;

        glColor3f(1.0f, 1.0f, 1.0f);
        drawCube(b.x, 0.8f, b.z, 0.3f);

        for (auto& e : enemies) {
            if (!e.alive) continue;
            if (fabs(e.x - b.x) < 0.5f && fabs(e.z - b.z) < 0.5f) {
                e.health -= b.damage;
                if (e.health <= 0) {
                    e.alive = false;
                    resources += 10;
                    score++;
                }
                b.active = false;
                break;
            }
        }

        if (b.x > GRID_COLS * CELL_SIZE) b.active = false;
    }
}

void towersShoot() {
    for (auto& t : towers) {
        if (t.cooldown > 0) {
            t.cooldown--;
            continue;
        }

        Bullet b;
        b.x = t.col * CELL_SIZE + 1.0f;
        b.z = t.row * CELL_SIZE;
        b.speed = 0.1f + 0.01f * t.level;
        b.damage = 10 * t.level;
        b.active = true;

        bullets.push_back(b);
        t.cooldown = 60 - t.level * 10;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    if (gameState == WELCOME) {
        // Welcome Screen
        glDisable(GL_DEPTH_TEST);
        drawText("Welcome to 3D Tower Defense!", WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 + 50);
        drawText("Press 'I' for Instructions", WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2);
        drawText("Press 'S' to Start the Game", WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2 - 30);
        glEnable(GL_DEPTH_TEST);
    } else if (gameState == INSTRUCTIONS) {
        // Instructions Screen
        glDisable(GL_DEPTH_TEST);
        drawText("Instructions", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 100);

        if (instructionPage == 1) {
            drawText("Level 1: Easy", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 50);
            drawText("- Use Arrow Keys to Move", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 20);
            drawText("- Press 'P' to Place Slow Tower (10 resources)", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 10);
            drawText("- Defend your base from enemies!", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 40);
        } else if (instructionPage == 2) {
            drawText("Level 2: Medium", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 50);
            drawText("- Use Arrow Keys to Move", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 20);
            drawText("- Press 'P' to Place Slow Tower (10 resources)", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 10);
            drawText("- Press 'S' to Place Medium Tower (20 resources)", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 40);
            drawText("- Enemies are faster and stronger!", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 70);
        } else if (instructionPage == 3) {
            drawText("Level 3: Hard", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 50);
            drawText("- Use Arrow Keys to Move", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 20);
            drawText("- Press 'P' to Place Slow Tower (10 resources)", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 10);
            drawText("- Press 'S' to Place Medium Tower (20 resources)", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 40);
            drawText("- Press 'A' to Place Fastest Tower (30 resources)", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 70);
            drawText("- Survive the toughest enemies!", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 100);
        }

        drawText("Press 'N' for Next, 'B' for Back", WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 150);
        drawText("Press 'M' to Return to the Main Menu", WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 180);
        glEnable(GL_DEPTH_TEST);
    } else if (gameState == PLAYING) {
        // Gameplay
        gluLookAt(20, 15, 20,
            GRID_COLS * CELL_SIZE / 2, 0, GRID_ROWS * CELL_SIZE / 2,
            0, 1, 0);

        printMatrix(GL_MODELVIEW);
        drawTerrain();
        drawGrid();
        updateAndDrawEnemies();
        towersShoot();
        updateAndDrawBullets();

        // UI Overlay
        glDisable(GL_DEPTH_TEST);
        if (gameOver) {
            drawText("Game Over! Final Score: " + std::to_string(score),
                WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2);
            drawText("Max Score: " + std::to_string(maxScore),
                WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 40);
        } else if (levelTransition) {
            if (!transitionRendered) {
                drawText("Level " + std::to_string(level - 1) + " Complete!", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 20);
                drawText("Press 'N' to Start Level " + std::to_string(level), WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 20);
                transitionRendered = true; // Mark transition as rendered
            }
        } else {
            drawText("Base Health: " + std::to_string(baseHealth), 10, WINDOW_HEIGHT - 20);
            drawText("Wave: " + std::to_string(wave), 10, WINDOW_HEIGHT - 40);
            drawText("Resources: " + std::to_string(resources), 10, WINDOW_HEIGHT - 60);
            drawText("Level: " + std::to_string(level), 10, WINDOW_HEIGHT - 80);
            drawText("Score: " + std::to_string(score), 10, WINDOW_HEIGHT - 100);
            drawText("Tower Keys: P(10) S(20) A(30)", 10, WINDOW_HEIGHT - 140);
        }
        glEnable(GL_DEPTH_TEST);
    }

    glutSwapBuffers();
}

void timer(int) {
    if (!gameOver && !levelTransition) {
        frameCount++;
        if (frameCount % ENEMY_SPAWN_INTERVAL == 0) {
            for (int i = 0; i < wave; i++) {
                spawnEnemy();
            }
        }

        if (frameCount % 1000 == 0) {
            wave++;
        }

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [](const Enemy& e) { return !e.alive; }), enemies.end());

        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.active; }), bullets.end());

        // Check if all enemies are defeated to progress to the next level
        bool allEnemiesDefeated = true;
        for (auto& e : enemies) {
            if (e.alive) {
                allEnemiesDefeated = false;
                break;
            }
        }

        int maxWaves = (level == 1) ? 3 : (level == 2) ? 4 : 5;

        for (size_t i = 0; i < towers.size(); ++i) {
            towers[i].health--;
            if (towers[i].health <= 0) {
                gridOccupied[towers[i].row][towers[i].col] = false; // Free up the grid
                towers.erase(towers.begin() + i);
                --i; // Adjust index after removal
            }
        }

        if (wave > maxWaves) {
            baseHealth += 5; // Increase health by 5 per level
            enemies.clear(); // Clear enemies for the next level
            bullets.clear(); // Clear bullets
            towers.clear(); // Clear towers
            resources = 50;
            std::fill(&gridOccupied[0][0], &gridOccupied[0][0] + sizeof(gridOccupied), false); // Reset grid
            if (baseHealth <= 0) { // End game if health is less than or equal to 0
                gameOver = true;
                if (score > maxScore) {
                    maxScore = score; // Update max score
                }
            } else {
                level++;
                score += 100; // Add 100 points for changing the level
                wave = 1; // Reset wave count for the new level
                if (level > 3) {
                    gameOver = true;
                    if (score > maxScore) {
                        maxScore = score; // Update max score
                    }
                } else {
                    levelTransition = true; // Enter level transition state
                }
            }
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS
}

void keyboard(int key, int x, int y) {
    if (gameOver) return;

    switch (key) {
        case GLUT_KEY_UP:
            if (selectedRow > 0) selectedRow--;
            break;
        case GLUT_KEY_DOWN:
            if (selectedRow < GRID_ROWS - 1) selectedRow++;
            break;
        case GLUT_KEY_LEFT:
            if (selectedCol > 0) selectedCol--;
            break;
        case GLUT_KEY_RIGHT:
            if (selectedCol < GRID_COLS - 1) selectedCol++;
            break;

    }
    glutPostRedisplay();
}

void normalKey(unsigned char key, int x, int y) {
    if (gameState == WELCOME) {
        if (key == 'i' || key == 'I') {
            gameState = INSTRUCTIONS; // Go to instructions screen
        } else if (key == 's' || key == 'S') {
            gameState = PLAYING; // Start the game
        }
        return;
    }

    if (gameState == INSTRUCTIONS) {
        if (key == 'n' || key == 'N') {
            if (instructionPage < 3) instructionPage++; // Go to the next instruction page
        } else if (key == 'b' || key == 'B') {
            if (instructionPage > 1) instructionPage--; // Go to the previous instruction page
        } else if (key == 'm' || key == 'M') {
            gameState = WELCOME; // Return to the welcome screen
        }
        return;
    }

    if (gameOver) return;

    if (levelTransition && (key == 'n' || key == 'N')) {
        levelTransition = false; // Exit level transition state
        transitionRendered = false; // Reset the flag for the next transition
        wave = 1; // Reset wave count for the new level
        return;
    }

    // Tower placement logic (unchanged)
    if (level == 1) { // Easy: Only BASIC towers
        if ((key == 'p' || key == 'P') && resources >= 10) { // Slow Tower: 10 resources
            if (!gridOccupied[selectedRow][selectedCol]) {
                towers.push_back({selectedRow, selectedCol, 0, 1, SLOW,600});
                gridOccupied[selectedRow][selectedCol] = true;
                resources -= 10;
            }
        }
    } else if (level == 2) { // Medium: BASIC and SLOW towers
        if ((key == 'p' || key == 'P') && resources >= 10) { // Slow Tower: 10 resources
            if (!gridOccupied[selectedRow][selectedCol]) {
                towers.push_back({selectedRow, selectedCol, 0, 1, SLOW,600});
                gridOccupied[selectedRow][selectedCol] = true;
                resources -= 10;
            }
        } else if ((key == 's' || key == 'S') && resources >= 20) { // Medium Tower: 20 resources
            if (!gridOccupied[selectedRow][selectedCol]) {
                towers.push_back({selectedRow, selectedCol, 0, 2, BASIC,1000});
                gridOccupied[selectedRow][selectedCol] = true;
                resources -= 20;
            }
        }
    } else if (level == 3) { // Difficult: All towers
        if ((key == 'p' || key == 'P') && resources >= 10) { // Slow Tower: 10 resources
            if (!gridOccupied[selectedRow][selectedCol]) {
                towers.push_back({selectedRow, selectedCol, 0, 1, SLOW,600});
                gridOccupied[selectedRow][selectedCol] = true;
                resources -= 10;
            }
        } else if ((key == 's' || key == 'S') && resources >= 20) { // Medium Tower: 20 resources
            if (!gridOccupied[selectedRow][selectedCol]) {
                towers.push_back({selectedRow, selectedCol, 0, 2, BASIC,1000});
                gridOccupied[selectedRow][selectedCol] = true;
                resources -= 20;
            }
        } else if ((key == 'a' || key == 'A') && resources >= 30) { // Fastest Tower: 30 resources
            if (!gridOccupied[selectedRow][selectedCol]) {
                towers.push_back({selectedRow, selectedCol, 0, 3, AREA,1500});
                gridOccupied[selectedRow][selectedCol] = true;
                resources -= 30;
            }
        }
    }
}
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (float)w / h, 1, 100);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("3D Tower Defense Game");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(keyboard);
    glutKeyboardFunc(normalKey);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
}