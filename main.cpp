#include <array>
#include <cmath>
#include <print>
#include <random>
#include <vector>

#include <GL/freeglut_std.h>
#include <GL/gl.h>

#define PI 3.14159265358979323846
#define WIDTH 1200
#define HEIGHT 800
#define MAX_ENEMIES 5

using RandInt = std::uniform_int_distribution<>;
using RandReal = std::uniform_real_distribution<>;

// random number generation setup
static std::random_device rd;
static std::mt19937 gen(rd());

// random color generator
RandReal colorDist(0.0, 1.0);

// game state
bool gameOver = false;
bool paused = false;

// game settings
bool isCollisionEnabled = false;

// game variables
double playerX = 0.0;
double playerY = -0.75;

double roadWidth = 0.9;
double carWidth = 0.16;
double carHeight = 0.2;
double margin = (roadWidth - carWidth) / 2;

double laneOffset = 0.0;
std::vector<double> lanes;
RandInt laneDist;

struct EnemyCar {
    double x, y;
    double width, height;
    double r, g, b;
    bool active;
};
std::array<EnemyCar, MAX_ENEMIES> enemies;

void initRoad() {
    int numLanes = static_cast<int>(roadWidth / carWidth);
    lanes.reserve(numLanes);
    double laneSpacing = roadWidth / numLanes;
    double startX = -roadWidth / 2 + laneSpacing / 2;
    for (int i = 0; i < numLanes; ++i) {
        lanes.push_back(startX + i * laneSpacing);
    }
    laneDist = RandInt(0, numLanes - 1);
}

void drawRoad() {
    // road
    glColor3d(0.2, 0.2, 0.2);
    glBegin(GL_QUADS);
    glVertex2d(-roadWidth / 2, -1.0);
    glVertex2d(roadWidth / 2, -1.0);
    glVertex2d(roadWidth / 2, 1.0);
    glVertex2d(-roadWidth / 2, 1.0);
    glEnd();

    // road borders
    glColor3d(0.8, 0.8, 0.8);

    glBegin(GL_QUADS);
    glVertex2d(-roadWidth / 2 - 0.02, -1.0);
    glVertex2d(-roadWidth / 2, -1.0);
    glVertex2d(-roadWidth / 2, 1.0);
    glVertex2d(-roadWidth / 2 - 0.02, 1.0);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2d(roadWidth / 2, -1.0);
    glVertex2d(roadWidth / 2 + 0.02, -1.0);
    glVertex2d(roadWidth / 2 + 0.02, 1.0);
    glVertex2d(roadWidth / 2, 1.0);
    glEnd();

    for (double y = -1.4; y < 1.4; y += 0.1) {
        glColor3d(1, 0.85, 0.2);

        glBegin(GL_QUADS);
        glVertex2d(-roadWidth / 2 - 0.02, y + laneOffset);
        glVertex2d(-roadWidth / 2, y + laneOffset);
        glVertex2d(-roadWidth / 2, y + 0.05 + laneOffset);
        glVertex2d(-roadWidth / 2 - 0.02, y + 0.05 + laneOffset);
        glEnd();

        glBegin(GL_QUADS);
        glVertex2d(roadWidth / 2, y + laneOffset);
        glVertex2d(roadWidth / 2 + 0.02, y + laneOffset);
        glVertex2d(roadWidth / 2 + 0.02, y + 0.05 + laneOffset);
        glVertex2d(roadWidth / 2, y + 0.05 + laneOffset);
        glEnd();
    }

    // lane markings
    glColor3d(1, 1, 1);
    for (double y = -1.4; y < 1.4; y += 0.2) {
        glBegin(GL_QUADS);
        glVertex2d(-0.01, y + laneOffset);
        glVertex2d(0.01, y + laneOffset);
        glVertex2d(0.01, y + 0.1 + laneOffset);
        glVertex2d(-0.01, y + 0.1 + laneOffset);
        glEnd();
    }
}

void updateRoad() {
    laneOffset -= 0.02; // moves road lane markings down
    if (laneOffset < -0.4) laneOffset = 0.0;
}

void drawCircle(double cx, double cy, double r, int num_segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(cx, cy);
    for (int i = 0; i < num_segments; i++) {
        double theta = 2 * PI * double(i) / double(num_segments);
        double x = r * cos(theta);
        double y = r * sin(theta);
        glVertex2d(x + cx, y + cy);
    }
    glEnd();
}

void drawCar(double x, double y, double r, double g, double b) {
    glColor3d(r, g, b);

    // Body
    glBegin(GL_QUADS);
    glVertex2d(x - carWidth * 0.5, y - carHeight);
    glVertex2d(x + carWidth * 0.5, y - carHeight);
    glVertex2d(x + carWidth * 0.5, y + carHeight * 0.75);
    glVertex2d(x - carWidth * 0.5, y + carHeight * 0.75);
    glEnd();

    // Roof
    glColor3d(r * 0.8, g * 0.8, b * 0.8);
    glBegin(GL_QUADS);
    glVertex2d(x - carWidth * 0.4, y - carHeight * 0.75);
    glVertex2d(x + carWidth * 0.4, y - carHeight * 0.75);
    glVertex2d(x + carWidth * 0.4, y + carHeight * 0.2);
    glVertex2d(x - carWidth * 0.4, y + carHeight * 0.2);
    glEnd();

    // Wind Shield
    glColor3d(0.5, 0.8, 1);
    glBegin(GL_QUADS);
    glVertex2d(x - carWidth * 0.4, y + carHeight * 0.15);
    glVertex2d(x + carWidth * 0.4, y + carHeight * 0.15);
    glVertex2d(x + carWidth * 0.35, y + carHeight * 0.5);
    glVertex2d(x - carWidth * 0.35, y + carHeight * 0.5);
    glEnd();

    // Headlights
    glColor3d(1, 1, 0);

    // left top
    glBegin(GL_QUADS);
    glVertex2d(x - carWidth * 0.2, y + carHeight * 0.80);
    glVertex2d(x - carWidth * 0.5, y + carHeight * 0.80);
    glVertex2d(x - carWidth * 0.5, y + carHeight * 0.75);
    glVertex2d(x - carWidth * 0.2, y + carHeight * 0.75);
    glEnd();

    // right top
    glBegin(GL_QUADS);
    glVertex2d(x + carWidth * 0.2, y + carHeight * 0.80);
    glVertex2d(x + carWidth * 0.5, y + carHeight * 0.80);
    glVertex2d(x + carWidth * 0.5, y + carHeight * 0.75);
    glVertex2d(x + carWidth * 0.2, y + carHeight * 0.75);
    glEnd();

    glColor3d(1, 0, 0);

    // left bottom
    glBegin(GL_QUADS);
    glVertex2d(x - carWidth * 0.2, y - carHeight * 1.05);
    glVertex2d(x - carWidth * 0.5, y - carHeight * 1.05);
    glVertex2d(x - carWidth * 0.5, y - carHeight);
    glVertex2d(x - carWidth * 0.2, y - carHeight);
    glEnd();

    // left bottom
    glBegin(GL_QUADS);
    glVertex2d(x + carWidth * 0.2, y - carHeight * 1.05);
    glVertex2d(x + carWidth * 0.5, y - carHeight * 1.05);
    glVertex2d(x + carWidth * 0.5, y - carHeight);
    glVertex2d(x + carWidth * 0.2, y - carHeight);
    glEnd();

    // Wheels
    glColor3d(0.1, 0.1, 0.1);

    // left top
    glBegin(GL_QUADS);
    glVertex2d(x - carWidth * 0.5, y + carHeight * 0.6);
    glVertex2d(x - carWidth * 0.5, y + carHeight * 0.2);
    glVertex2d(x - carWidth * 0.55, y + carHeight * 0.2);
    glVertex2d(x - carWidth * 0.55, y + carHeight * 0.6);
    glEnd();

    // right top
    glBegin(GL_QUADS);
    glVertex2d(x + carWidth * 0.5, y + carHeight * 0.6);
    glVertex2d(x + carWidth * 0.5, y + carHeight * 0.2);
    glVertex2d(x + carWidth * 0.55, y + carHeight * 0.2);
    glVertex2d(x + carWidth * 0.55, y + carHeight * 0.6);
    glEnd();

    // left bottom
    glBegin(GL_QUADS);
    glVertex2d(x - carWidth * 0.5, y - carHeight * 0.9);
    glVertex2d(x - carWidth * 0.5, y - carHeight * 0.5);
    glVertex2d(x - carWidth * 0.55, y - carHeight * 0.5);
    glVertex2d(x - carWidth * 0.55, y - carHeight * 0.9);
    glEnd();

    // right bottom
    glBegin(GL_QUADS);
    glVertex2d(x + carWidth * 0.5, y - carHeight * 0.9);
    glVertex2d(x + carWidth * 0.5, y - carHeight * 0.5);
    glVertex2d(x + carWidth * 0.55, y - carHeight * 0.5);
    glVertex2d(x + carWidth * 0.55, y - carHeight * 0.9);
    glEnd();
}

void initEnemies() {
    for (size_t i = 0; i < enemies.size(); ++i) {
        enemies[i].width = carWidth;
        enemies[i].height = carHeight;
        enemies[i].y = 1.2 + i * 0.5;
        enemies[i].x = lanes[laneDist(gen)];
        enemies[i].r = colorDist(gen);
        enemies[i].g = colorDist(gen);
        enemies[i].b = colorDist(gen);
        enemies[i].active = true;
    }
}

void drawEnemies() {
    for (const auto &enemy : enemies) {
        if (enemy.active) {
            drawCar(enemy.x, enemy.y, enemy.r, enemy.g, enemy.b);
        }
    }
}

bool checkCollision(double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2) {
    if (!isCollisionEnabled) return false;
    return (std::abs(x1 - x2) * 2 < (w1 + w2)) && (std::abs(y1 - y2) * 2 < (h1 + h2));
}

void updateEnemies() {
    for (auto &enemy : enemies) {
        if (!enemy.active) continue;
        enemy.y -= 0.01;
        if (enemy.y < -1.4) {
            enemy.y = 1.4;
            enemy.x = lanes[laneDist(gen)];
        }

        if (checkCollision(playerX, playerY, carWidth, carHeight, enemy.x, enemy.y, enemy.width, enemy.height)) {
            // reset game
            std::println("Game Over!!");
            playerX = 0.0;
            playerY = -0.75;
            initEnemies();
            break;
        }
    }
}

void init() {
    initRoad();
    initEnemies();
}

void display() {
    glClearColor(0.53, 0.81, 0.92, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    drawRoad();
    drawCar(playerX, playerY, 0.2, 0.3, 0.9);
    drawEnemies();

    glFlush();
}

void keyboard(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT && playerX > -margin) {
        playerX -= margin / 10;
    } else if (key == GLUT_KEY_RIGHT && playerX < margin) {
        playerX += margin / 10;
    } else if (key == GLUT_KEY_UP && playerY < 1.0) {
        playerY += 0.05;
    } else if (key == GLUT_KEY_DOWN && playerY > -1.0) {
        playerY -= 0.05;
    }

    glutPostRedisplay();
}

void update(int value) {
    updateRoad();
    updateEnemies();

    glutPostRedisplay();
    glutTimerFunc(30, update, 0);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_MULTISAMPLE);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Car Race");

    // enable anti-aliasing
    glEnable(GLUT_MULTISAMPLE | GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    init();

    glutDisplayFunc(display);
    glutSpecialFunc(keyboard);
    glutTimerFunc(30, update, 0);

    glutMainLoop();
    return 0;
}
