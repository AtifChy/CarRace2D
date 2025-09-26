#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <GL/freeglut_std.h>
#include <GL/gl.h>

const double PI = 3.1416;
const int WIDTH = 1200;
const int HEIGHT = 800;
const int MAX_ENEMIES = 4;

// random number generation setup
static std::random_device rd;
static std::mt19937 gen(rd());

// alias for random distributions
using RandInt = std::uniform_int_distribution<>;
using RandReal = std::uniform_real_distribution<>;

// random color generator
RandReal colorDist(0.0, 1.0);

// game state
int finishTimeMs = 0; // store finish time so timer can stop

bool gameOver = false;
bool paused = false;
bool gameFinished = false; // Indicates if the game is finished

// game settings
bool isCollisionEnabled = true;

// timing for start/finish lines
int gameStartTimeMs = 0;
const int START_LINE_SHOW_MS = 2000; // show start line for 2 seconds
const int FINISH_LINE_AT_MS = 60000; // show finish line after 60 seconds

// game variables
double playerX = 0.0;
double playerY = -0.75;

double roadWidth = 0.9;
double carWidth = 0.16;
double carHeight = 0.2;
double margin = (roadWidth - carWidth) / 2;

enum class SceneryType { GRASS, DESERT };
static SceneryType currentScenery = SceneryType::DESERT;
static int scenerayIntervalMS = 10000; // 20 seconds

// #region Scenery

// ----- Grass -----
std::vector<std::pair<double, double>> leftGrassBlades;
std::vector<std::pair<double, double>> rightGrassBlades;

void initGrass() {
    leftGrassBlades.clear();
    rightGrassBlades.clear();
    int numBlades = 200;
    leftGrassBlades.reserve(numBlades);
    rightGrassBlades.reserve(numBlades);
    RandReal xDist(-1.0, -roadWidth / 2 - 0.05);
    RandReal yDist(-1.0, 1.0);
    for (int i = 0; i < numBlades; ++i) {
        leftGrassBlades.emplace_back(xDist(gen), yDist(gen));
        rightGrassBlades.emplace_back(-xDist(gen), yDist(gen));
    }
}

void drawGrass() {
    glColor3ub(46, 111, 64);

    glBegin(GL_QUADS);
    glVertex2d(-1.0, -1.0);
    glVertex2d(-roadWidth / 2, -1.0);
    glVertex2d(-roadWidth / 2, 1.0);
    glVertex2d(-1.0, 1.0);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2d(roadWidth / 2, -1.0);
    glVertex2d(1.0, -1.0);
    glVertex2d(1.0, 1.0);
    glVertex2d(roadWidth / 2, 1.0);
    glEnd();

    glColor3ub(104, 186, 127);
    for (const auto &blade : leftGrassBlades) {
        glBegin(GL_LINES);
        glVertex2d(blade.first, blade.second);
        glVertex2d(blade.first + 0.01, blade.second + 0.03);
        glEnd();
    }
    for (const auto &blade : rightGrassBlades) {
        glBegin(GL_LINES);
        glVertex2d(blade.first, blade.second);
        glVertex2d(blade.first - 0.01, blade.second + 0.03);
        glEnd();
    }
}

void updateGrass() {
    for (auto &[x, y] : leftGrassBlades) {
        y -= 0.01;
        if (y < -1.0) y = 1.0;
    }
    for (auto &[x, y] : rightGrassBlades) {
        y -= 0.01;
        if (y < -1.0) y = 1.0;
    }
}

// ----- Desert -----
struct Cactus {
    double x, y;
    double size;
};
std::vector<Cactus> leftCt;
std::vector<Cactus> rightCt;

void drawCactus(double x, double y, double size) {
    glColor3ub(34, 139, 34);

    // Draw circular base of the cactus
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(x, y);               // Center point
    for (int i = 0; i <= 12; ++i) { // 12 segments for smooth circle
        double angle = 2.0 * PI * i / 12;
        double dx = size * 0.3 * cos(angle);
        double dy = size * 0.3 * sin(angle);
        glVertex2d(x + dx, y + dy);
    }
    glEnd();

    // Draw spikes (starburst effect)
    glBegin(GL_LINES);
    for (int i = 0; i < 12; ++i) {
        double angle = 2.0 * PI * i / 12;
        double dx = size * 0.35 * cos(angle);
        double dy = size * 0.35 * sin(angle);
        glVertex2d(x, y);
        glVertex2d(x + dx, y + dy);
    }
    glEnd();
}

void initDesert() {
    leftCt.clear();
    rightCt.clear();
    int num = 10;
    leftCt.reserve(num);
    rightCt.reserve(num);
    RandReal xLeft(-1.0, -roadWidth / 2 - 0.05);
    RandReal xRight(roadWidth / 2 + 0.05, 1.0);
    RandReal yDist(-1.0, 1.0);
    RandReal sDist(0.05, 0.2);
    for (int i = 0; i < num; ++i) {
        leftCt.push_back({xLeft(gen), yDist(gen), sDist(gen)});
        rightCt.push_back({xRight(gen), yDist(gen), sDist(gen)});
    }
}

void drawDesert() {
    glColor3ub(237, 201, 175);
    glBegin(GL_QUADS); // left sand
    glVertex2d(-1.0, -1.0);
    glVertex2d(-roadWidth / 2, -1.0);
    glVertex2d(-roadWidth / 2, 1.0);
    glVertex2d(-1.0, 1.0);
    glEnd();
    glBegin(GL_QUADS); // right sand
    glVertex2d(roadWidth / 2, -1.0);
    glVertex2d(1.0, -1.0);
    glVertex2d(1.0, 1.0);
    glVertex2d(roadWidth / 2, 1.0);
    glEnd();
    for (const auto &c : leftCt) {
        drawCactus(c.x, c.y, c.size);
    }
    for (const auto &c : rightCt) {
        drawCactus(c.x, c.y, c.size);
    }
}

void updateDesert() {
    for (auto &c : leftCt) {
        c.y -= 0.01;
        if (c.y < -1.0) c.y = 1.0;
    }
    for (auto &c : rightCt) {
        c.y -= 0.01;
        if (c.y < -1.0) c.y = 1.0;
    }
}

// Scenery
void initScenery(SceneryType t) {
    switch (t) {
    case SceneryType::GRASS:
        initGrass();
        break;
    case SceneryType::DESERT:
        initDesert();
        break;
    }
}

void drawScenery() {
    switch (currentScenery) {
    case SceneryType::GRASS:
        drawGrass();
        break;
    case SceneryType::DESERT:
        drawDesert();
        break;
    }
}

void updateScenery() {
    switch (currentScenery) {
    case SceneryType::GRASS:
        updateGrass();
        break;
    case SceneryType::DESERT:
        updateDesert();
        break;
    }
}

int lastScenerySwitchTime = 0;
void autoSwitchScenery() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (now - lastScenerySwitchTime >= scenerayIntervalMS) {
        lastScenerySwitchTime = now;
        int next = (static_cast<int>(currentScenery) + 1) % 2;
        currentScenery = static_cast<SceneryType>(next);
        initScenery(currentScenery);
    }
}

// #endregion

// #region Road

double laneOffset = 0.0;
double roadScroll = 0.0;        // non-wrapping scroll accumulator for anchored features
double startScroll0 = 0.0;      // roadScroll value at game start
double finishScroll0 = 0.0;     // roadScroll value when finish line spawns
bool finishLineSpawned = false; // indicates if finish line has been spawned
std::vector<double> lanes;
RandInt laneDist;

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

    // start/finish lines
    int now = glutGet(GLUT_ELAPSED_TIME);
    int elapsed = now - gameStartTimeMs;

    auto drawCheckeredLine = [&](double baseY, double height, double cellW) {
        double left = -roadWidth / 2.0;
        int cells = static_cast<int>(roadWidth / cellW) + 1;
        for (int i = 0; i < cells; ++i) {
            if (i % 2 == 0)
                glColor3d(1, 1, 1);
            else
                glColor3d(0, 0, 0);
            double x0 = left + i * cellW;
            double x1 = x0 + cellW;
            glBegin(GL_QUADS);
            glVertex2d(x0, baseY);
            glVertex2d(x1, baseY);
            glVertex2d(x1, baseY + height);
            glVertex2d(x0, baseY + height);
            glEnd();
        }
    };

    // Show start line only for a short time at game start
    if (elapsed <= START_LINE_SHOW_MS) {
        double startY = -0.6 + (roadScroll - startScroll0);
        drawCheckeredLine(startY, 0.06, 0.06);
    }

    // Show finish line after a certain amount of time
    if (elapsed >= FINISH_LINE_AT_MS) {
        if (!finishLineSpawned) {
            finishLineSpawned = true;
            finishScroll0 = roadScroll; // remember spawn scroll position
        }
        double finishY = 0.7 + (roadScroll - finishScroll0);
        drawCheckeredLine(finishY, 0.06, 0.06);
    }
}

void updateRoad() {
    laneOffset -= 0.02; // moves road lane markings down
    if (laneOffset < -0.4) laneOffset = 0.0;

    roadScroll -= 0.02; // non-wrapping scroll for anchored features
}

// #endregion Road

// #region Car

enum class CarType { SEDAN, SUV, TRACK };

void drawRoundedRect(double x, double y, double w, double h, double radius, int segments = 12) {
    double left = x - w * 0.5;
    double right = x + w * 0.5;
    double top = y + h * 0.75;
    double bottom = y - h;

    // Corner centers
    double cx[4] = {right - radius, left + radius, left + radius, right - radius};
    double cy[4] = {top - radius, top - radius, bottom + radius, bottom + radius};

    // Angles for each corner (in radians)
    double start[4] = {0, PI / 2, PI, 3 * PI / 2};

    glBegin(GL_POLYGON);

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j <= segments; j++) {
            double theta = start[i] + (PI / 2) * (double)j / segments;
            double vx = cx[i] + radius * cos(theta);
            double vy = cy[i] + radius * sin(theta);
            glVertex2d(vx, vy);
        }
    }

    glEnd();
}

void drawCar(double x, double y, double r, double g, double b, CarType type = CarType::SEDAN) {
    double w = carWidth;
    double h = carHeight;

    switch (type) {
    case CarType::SEDAN: {
        glColor3d(r, g, b);
        // Body
        glBegin(GL_QUADS);
        glVertex2d(x - w * 0.5, y - h);
        glVertex2d(x + w * 0.5, y - h);
        glVertex2d(x + w * 0.5, y + h * 0.75);
        glVertex2d(x - w * 0.5, y + h * 0.75);
        glEnd();

        // Roof
        glColor3d(r * 0.8, g * 0.8, b * 0.8);
        glBegin(GL_QUADS);
        glVertex2d(x - w * 0.4, y - h * 0.75);
        glVertex2d(x + w * 0.4, y - h * 0.75);
        glVertex2d(x + w * 0.4, y + h * 0.2);
        glVertex2d(x - w * 0.4, y + h * 0.2);
        glEnd();

        // Windshield
        glColor3d(0.5, 0.8, 1);
        glBegin(GL_QUADS);
        glVertex2d(x - w * 0.4, y + h * 0.15);
        glVertex2d(x + w * 0.4, y + h * 0.15);
        glVertex2d(x + w * 0.35, y + h * 0.5);
        glVertex2d(x - w * 0.35, y + h * 0.5);
        glEnd();

        // Headlights
        glColor3d(1, 1, 0);
        glBegin(GL_QUADS); // left front
        glVertex2d(x - w * 0.2, y + h * 0.80);
        glVertex2d(x - w * 0.5, y + h * 0.80);
        glVertex2d(x - w * 0.5, y + h * 0.75);
        glVertex2d(x - w * 0.2, y + h * 0.75);
        glEnd();
        glBegin(GL_QUADS); // right front
        glVertex2d(x + w * 0.2, y + h * 0.80);
        glVertex2d(x + w * 0.5, y + h * 0.80);
        glVertex2d(x + w * 0.5, y + h * 0.75);
        glVertex2d(x + w * 0.2, y + h * 0.75);
        glEnd();

        glColor3d(1, 0, 0);
        glBegin(GL_QUADS); // left rear
        glVertex2d(x - w * 0.2, y - h * 1.05);
        glVertex2d(x - w * 0.5, y - h * 1.05);
        glVertex2d(x - w * 0.5, y - h);
        glVertex2d(x - w * 0.2, y - h);
        glEnd();
        glBegin(GL_QUADS); // right rear
        glVertex2d(x + w * 0.2, y - h * 1.05);
        glVertex2d(x + w * 0.5, y - h * 1.05);
        glVertex2d(x + w * 0.5, y - h);
        glVertex2d(x + w * 0.2, y - h);
        glEnd();

        // Wheels
        glColor3d(0.1, 0.1, 0.1);
        glBegin(GL_QUADS); // left front
        glVertex2d(x - w * 0.5, y + h * 0.6);
        glVertex2d(x - w * 0.5, y + h * 0.2);
        glVertex2d(x - w * 0.55, y + h * 0.2);
        glVertex2d(x - w * 0.55, y + h * 0.6);
        glEnd();
        glBegin(GL_QUADS); // right front
        glVertex2d(x + w * 0.5, y + h * 0.6);
        glVertex2d(x + w * 0.5, y + h * 0.2);
        glVertex2d(x + w * 0.55, y + h * 0.2);
        glVertex2d(x + w * 0.55, y + h * 0.6);
        glEnd();
        glBegin(GL_QUADS); // left rear
        glVertex2d(x - w * 0.5, y - h * 0.9);
        glVertex2d(x - w * 0.5, y - h * 0.5);
        glVertex2d(x - w * 0.55, y - h * 0.5);
        glVertex2d(x - w * 0.55, y - h * 0.9);
        glEnd();
        glBegin(GL_QUADS); // right rear
        glVertex2d(x + w * 0.5, y - h * 0.9);
        glVertex2d(x + w * 0.5, y - h * 0.5);
        glVertex2d(x + w * 0.55, y - h * 0.5);
        glVertex2d(x + w * 0.55, y - h * 0.9);
        glEnd();
        break;
    }

    case CarType::SUV: {
        glColor3d(r, g, b);
        // Body
        drawRoundedRect(x, y, w, h * 1.1, w * 0.1);

        // Roof
        glColor3d(r * 0.8, g * 0.8, b * 0.8);
        glBegin(GL_QUADS);
        glVertex2d(x - w * 0.45, y - h * 0.75);
        glVertex2d(x + w * 0.45, y - h * 0.75);
        glVertex2d(x + w * 0.45, y + h * 0.3);
        glVertex2d(x - w * 0.45, y + h * 0.3);
        glEnd();

        // Windshield
        glColor3d(0.5, 0.8, 1);
        glBegin(GL_QUADS);
        glVertex2d(x - w * 0.45, y + h * 0.25);
        glVertex2d(x + w * 0.45, y + h * 0.25);
        glVertex2d(x + w * 0.4, y + h * 0.5);
        glVertex2d(x - w * 0.4, y + h * 0.5);
        glEnd();

        // Headlights
        glColor3d(1, 1, 0);
        glBegin(GL_QUADS); // left front
        glVertex2d(x - w * 0.3, y + h * 0.80);
        glVertex2d(x - w * 0.5, y + h * 0.80);
        glVertex2d(x - w * 0.5, y + h * 0.7);
        glVertex2d(x - w * 0.3, y + h * 0.7);
        glEnd();
        glBegin(GL_QUADS); // right front
        glVertex2d(x + w * 0.3, y + h * 0.80);
        glVertex2d(x + w * 0.5, y + h * 0.80);
        glVertex2d(x + w * 0.5, y + h * 0.7);
        glVertex2d(x + w * 0.3, y + h * 0.7);
        glEnd();

        glColor3d(1, 0, 0);
        glBegin(GL_QUADS); // left rear
        glVertex2d(x - w * 0.3, y - h * 1.1);
        glVertex2d(x - w * 0.5, y - h * 1.1);
        glVertex2d(x - w * 0.5, y - h);
        glVertex2d(x - w * 0.3, y - h);
        glEnd();
        glBegin(GL_QUADS); // right rear
        glVertex2d(x + w * 0.3, y - h * 1.1);
        glVertex2d(x + w * 0.5, y - h * 1.1);
        glVertex2d(x + w * 0.5, y - h);
        glVertex2d(x + w * 0.3, y - h);
        glEnd();

        // Wheels
        glColor3d(0.1, 0.1, 0.1);
        glBegin(GL_QUADS); // left front
        glVertex2d(x - w * 0.5, y + h * 0.6);
        glVertex2d(x - w * 0.5, y + h * 0.2);
        glVertex2d(x - w * 0.55, y + h * 0.2);
        glVertex2d(x - w * 0.55, y + h * 0.6);
        glEnd();
        glBegin(GL_QUADS); // right front
        glVertex2d(x + w * 0.5, y + h * 0.6);
        glVertex2d(x + w * 0.5, y + h * 0.2);
        glVertex2d(x + w * 0.55, y + h * 0.2);
        glVertex2d(x + w * 0.55, y + h * 0.6);
        glEnd();
        glBegin(GL_QUADS); // left rear
        glVertex2d(x - w * 0.5, y - h * 0.9);
        glVertex2d(x - w * 0.5, y - h * 0.5);
        glVertex2d(x - w * 0.55, y - h * 0.5);
        glVertex2d(x - w * 0.55, y - h * 0.9);
        glEnd();
        glBegin(GL_QUADS); // right rear
        glVertex2d(x + w * 0.5, y - h * 0.9);
        glVertex2d(x + w * 0.5, y - h * 0.5);
        glVertex2d(x + w * 0.55, y - h * 0.5);
        glVertex2d(x + w * 0.55, y - h * 0.9);
        glEnd();
        break;
    }

    case CarType::TRACK: {
        glColor3d(r, g, b);
        // Body
        glBegin(GL_QUADS);
        glVertex2d(x - w * 0.5, y - h);
        glVertex2d(x + w * 0.5, y - h);
        glVertex2d(x + w * 0.5, y + h * 0.75);
        glVertex2d(x - w * 0.5, y + h * 0.75);
        glEnd();

        // Roof
        glColor3d(r * 0.8, g * 0.8, b * 0.8);
        glBegin(GL_QUADS);
        glVertex2d(x - w * 0.4, y - h * 0.75);
        glVertex2d(x + w * 0.4, y - h * 0.75);
        glVertex2d(x + w * 0.4, y + h * 0.35);
        glVertex2d(x - w * 0.4, y + h * 0.35);
        glEnd();

        // Trunk
        glColor3d(r * 0.2, g * 0.6, b * 0.9);
        glBegin(GL_QUADS);
        glVertex2d(x - w * 0.45, y - h * 0.9);
        glVertex2d(x + w * 0.45, y - h * 0.9);
        glVertex2d(x + w * 0.45, y - h * 0.05);
        glVertex2d(x - w * 0.45, y - h * 0.05);
        glEnd();

        // Windshield
        glColor3d(0.5, 0.8, 1);
        glBegin(GL_QUADS);
        glVertex2d(x - w * 0.4, y + h * 0.35);
        glVertex2d(x + w * 0.4, y + h * 0.35);
        glVertex2d(x + w * 0.35, y + h * 0.6);
        glVertex2d(x - w * 0.35, y + h * 0.6);
        glEnd();

        // Headlights (front) and taillights (rear)
        glColor3d(1, 1, 0);
        glBegin(GL_QUADS); // left front
        glVertex2d(x - w * 0.2, y + h * 0.80);
        glVertex2d(x - w * 0.5, y + h * 0.80);
        glVertex2d(x - w * 0.5, y + h * 0.75);
        glVertex2d(x - w * 0.2, y + h * 0.75);
        glEnd();
        glBegin(GL_QUADS); // right front
        glVertex2d(x + w * 0.2, y + h * 0.80);
        glVertex2d(x + w * 0.5, y + h * 0.80);
        glVertex2d(x + w * 0.5, y + h * 0.75);
        glVertex2d(x + w * 0.2, y + h * 0.75);
        glEnd();

        glColor3d(1, 0, 0);
        glBegin(GL_QUADS); // left rear
        glVertex2d(x - w * 0.2, y - h * 1.05);
        glVertex2d(x - w * 0.5, y - h * 1.05);
        glVertex2d(x - w * 0.5, y - h);
        glVertex2d(x - w * 0.2, y - h);
        glEnd();
        glBegin(GL_QUADS); // right rear
        glVertex2d(x + w * 0.2, y - h * 1.05);
        glVertex2d(x + w * 0.5, y - h * 1.05);
        glVertex2d(x + w * 0.5, y - h);
        glVertex2d(x + w * 0.2, y - h);
        glEnd();

        // Wheels
        glColor3d(0.1, 0.1, 0.1);
        glBegin(GL_QUADS); // left front
        glVertex2d(x - w * 0.5, y + h * 0.6);
        glVertex2d(x - w * 0.5, y + h * 0.2);
        glVertex2d(x - w * 0.55, y + h * 0.2);
        glVertex2d(x - w * 0.55, y + h * 0.6);
        glEnd();
        glBegin(GL_QUADS); // right front
        glVertex2d(x + w * 0.5, y + h * 0.6);
        glVertex2d(x + w * 0.5, y + h * 0.2);
        glVertex2d(x + w * 0.55, y + h * 0.2);
        glVertex2d(x + w * 0.55, y + h * 0.6);
        glEnd();
        glBegin(GL_QUADS); // left rear
        glVertex2d(x - w * 0.5, y - h * 0.9);
        glVertex2d(x - w * 0.5, y - h * 0.5);
        glVertex2d(x - w * 0.55, y - h * 0.5);
        glVertex2d(x - w * 0.55, y - h * 0.9);
        glEnd();
        glBegin(GL_QUADS); // right rear
        glVertex2d(x + w * 0.5, y - h * 0.9);
        glVertex2d(x + w * 0.5, y - h * 0.5);
        glVertex2d(x + w * 0.55, y - h * 0.5);
        glVertex2d(x + w * 0.55, y - h * 0.9);
        glEnd();
        break;
    }
    }
}

// #endregion Car

// #region Bridge

struct Bridge {
    double y;            // Y position of the bridge
    double height;       // Height of the bridge structure
    double shadowOffset; // Offset for shadow effect
    bool active;         // Whether bridge is active/visible
};

Bridge bridge;
const double BRIDGE_HEIGHT = 0.6;
const double BRIDGE_WIDTH = 2.0; // Spans full window width (left to right)
int lastBridgeSpawnTime = 0;
const int BRIDGE_SPAWN_INTERVAL_MS = 8000; // Spawn every 8 seconds

void initBridge() {
    // Initialize single bridge as inactive
    bridge = {2.0, BRIDGE_HEIGHT, 0.02, false};
    lastBridgeSpawnTime = glutGet(GLUT_ELAPSED_TIME);
}

void spawnBridge() {
    // Only spawn if current bridge is inactive
    if (!bridge.active) {
        bridge.y = 1.5;                                             // Spawn above visible area
        bridge.height = BRIDGE_HEIGHT + RandReal(-0.02, 0.02)(gen); // Slight height variation
        bridge.shadowOffset = 0.02;
        bridge.active = true;
    }
}

void drawBridge(const Bridge &bridge) {
    if (!bridge.active) return;

    double bridgeY = bridge.y;
    double bridgeLeft = -1.0; // Full screen width from left edge
    double bridgeRight = 1.0; // Full screen width to right edge
    double bridgeTop = bridgeY + bridge.height;
    double bridgeBottom = bridgeY;

    // Draw shadow first (darker, slightly offset)
    glColor3d(0.1, 0.1, 0.1);
    glBegin(GL_QUADS);
    glVertex2d(bridgeLeft + bridge.shadowOffset, bridgeBottom - bridge.shadowOffset);
    glVertex2d(bridgeRight + bridge.shadowOffset, bridgeBottom - bridge.shadowOffset);
    glVertex2d(bridgeRight + bridge.shadowOffset, bridgeTop - bridge.shadowOffset);
    glVertex2d(bridgeLeft + bridge.shadowOffset, bridgeTop - bridge.shadowOffset);
    glEnd();

    // Draw main bridge structure (concrete gray) - spans full screen width
    glColor3d(0.3, 0.3, 0.3);
    glBegin(GL_QUADS);
    glVertex2d(bridgeLeft, bridgeBottom);
    glVertex2d(bridgeRight, bridgeBottom);
    glVertex2d(bridgeRight, bridgeTop);
    glVertex2d(bridgeLeft, bridgeTop);
    glEnd();

    // Draw bridge railings on top and bottom edges
    double railingHeight = 0.02;

    // Top railing (front edge)
    glColor3d(0.7, 0.7, 0.7);
    glBegin(GL_QUADS);
    glVertex2d(bridgeLeft, bridgeTop);
    glVertex2d(bridgeRight, bridgeTop);
    glVertex2d(bridgeRight, bridgeTop + railingHeight);
    glVertex2d(bridgeLeft, bridgeTop + railingHeight);
    glEnd();

    // Bottom railing (back edge)
    glBegin(GL_QUADS);
    glVertex2d(bridgeLeft, bridgeBottom - railingHeight);
    glVertex2d(bridgeRight, bridgeBottom - railingHeight);
    glVertex2d(bridgeRight, bridgeBottom);
    glVertex2d(bridgeLeft, bridgeBottom);
    glEnd();

    // Draw a single dashed lane marking at the center
    glColor3d(1, 1, 0.8);
    double markingHeight = 0.02; // thickness of the line
    double dashLength = 0.1;     // length of each dash
    double dashGap = 0.1;        // gap between dashes
    double centerY = (bridgeTop + bridgeBottom) / 2.0;

    for (double x = bridgeLeft; x < bridgeRight; x += dashLength + dashGap) {
        glBegin(GL_QUADS);
        glVertex2d(x, centerY - markingHeight / 2);
        glVertex2d(x + dashLength, centerY - markingHeight / 2);
        glVertex2d(x + dashLength, centerY + markingHeight / 2);
        glVertex2d(x, centerY + markingHeight / 2);
        glEnd();
    }
}

void drawBridge() { drawBridge(bridge); }

void updateBridge() {
    if (gameFinished) return;

    // Check if it's time to spawn a new bridge
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (now - lastBridgeSpawnTime >= BRIDGE_SPAWN_INTERVAL_MS) {
        // Random chance to spawn bridge (70% probability)
        if (RandReal(0.0, 1.0)(gen) < 0.7) {
            spawnBridge();
        }
        lastBridgeSpawnTime = now;
    }

    if (bridge.active) {
        bridge.y -= 0.01; // Move bridge down with road

        // Deactivate bridge when it goes off screen
        if (bridge.y < -1.5) {
            bridge.active = false;
        }
    }
}

// #endregion Bridge

// #region Explosion

struct Particle {
    double x, y;        // Position
    double vx, vy;      // Velocity
    double r, g, b;     // Color
    double lifetime;    // Remaining lifetime
    double maxLifetime; // Initial lifetime
    bool active;
};

struct Explosion {
    double x, y; // Explosion center
    std::vector<Particle> particles;
    bool active;
    int maxParticles;
};

Explosion explosion;
const int MAX_EXPLOSION_PARTICLES = 20;
const double EXPLOSION_DURATION = 1.0; // seconds

void initExplosion() {
    explosion.active = false;
    explosion.maxParticles = MAX_EXPLOSION_PARTICLES;
    explosion.particles.reserve(MAX_EXPLOSION_PARTICLES);
}

void createExplosion(double x, double y) {
    explosion.x = x;
    explosion.y = y;
    explosion.active = true;
    explosion.particles.clear();

    // Create particles
    RandReal angleDist(0.0, 2.0 * PI);
    RandReal speedDist(0.1, 0.3);
    RandReal lifetimeDist(0.5, 1.0);

    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; ++i) {
        Particle p;
        double angle = angleDist(gen);
        double speed = speedDist(gen);

        p.x = x;
        p.y = y;
        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed;

        // Hot colors: red, orange, yellow
        if (i % 3 == 0) {
            p.r = 1.0;
            p.g = 0.0;
            p.b = 0.0; // Red
        } else if (i % 3 == 1) {
            p.r = 1.0;
            p.g = 0.5;
            p.b = 0.0; // Orange
        } else {
            p.r = 1.0;
            p.g = 1.0;
            p.b = 0.0; // Yellow
        }

        p.lifetime = lifetimeDist(gen);
        p.maxLifetime = p.lifetime;
        p.active = true;

        explosion.particles.push_back(p);
    }
}

void updateExplosion() {
    if (!explosion.active) return;

    bool anyActive = false;
    for (auto &p : explosion.particles) {
        if (!p.active) continue;

        // Update position
        p.x += p.vx * 0.016; // Assuming ~60 FPS
        p.y += p.vy * 0.016;

        // Apply gravity
        p.vy -= 0.5 * 0.016;

        // Update lifetime
        p.lifetime -= 0.016;
        if (p.lifetime <= 0) {
            p.active = false;
        } else {
            anyActive = true;
        }
    }

    // Deactivate explosion when all particles are done
    if (!anyActive) {
        explosion.active = false;
    }
}

void drawExplosion() {
    if (!explosion.active) return;

    for (const auto &p : explosion.particles) {
        if (!p.active) continue;

        // Fade out over time
        double alpha = p.lifetime / p.maxLifetime;
        glColor3d(p.r * alpha, p.g * alpha, p.b * alpha);

        // Draw particle as small square
        double size = 0.02 * alpha; // Shrink over time
        glBegin(GL_QUADS);
        glVertex2d(p.x - size, p.y - size);
        glVertex2d(p.x + size, p.y - size);
        glVertex2d(p.x + size, p.y + size);
        glVertex2d(p.x - size, p.y + size);
        glEnd();
    }
}

// #endregion Explosion

// #region Score

int64_t score = 0;

void updateScore() {
    if (!gameFinished) {
        score += 1;
    }
}

void drawText(double x, double y, const std::string &text) {
    glRasterPos2d(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

void drawScore() {
    glColor3d(1, 1, 1);
    std::string scoreText = "Score:" + std::to_string(score);
    drawText(-0.95, 0.9, scoreText);
}

void drawTimer() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    int elapsedMs = now - gameStartTimeMs;
    int totalSeconds = elapsedMs / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);

    glColor3d(1, 1, 1);
    // place at top-right corner
    drawText(0.8, 0.9, std::string("Time:") + buf);
}

// #endregion Score

// #region Enemy

struct EnemyCar {
    double x, y;
    CarType type;
    double r, g, b;
    bool active;
};
std::array<EnemyCar, MAX_ENEMIES> enemies;

void initEnemies() {
    RandInt typeDist(0, 2);
    for (size_t i = 0; i < enemies.size(); ++i) {
        enemies[i].y = 1.2 + i * 0.5;
        enemies[i].x = lanes[laneDist(gen)];
        enemies[i].r = colorDist(gen);
        enemies[i].g = colorDist(gen);
        enemies[i].b = colorDist(gen);
        enemies[i].type = static_cast<CarType>(typeDist(gen));
        enemies[i].active = true;
    }
}

void drawEnemies() {
    for (const auto &enemy : enemies) {
        if (enemy.active) {
            drawCar(enemy.x, enemy.y, enemy.r, enemy.g, enemy.b, enemy.type);
        }
    }
}

bool checkCollision(double x1, double y1, double x2, double y2) {
    if (!isCollisionEnabled) return false;
    return (std::abs(x1 - x2) * 2 < (2 * carWidth) * 1.05) && (std::abs(y1 - y2) * 2 < (2 * carHeight) * 1.05);
}

void updateEnemies() {
    if (gameFinished) return; // Stop updating enemies once the game is finished

    for (auto &enemy : enemies) {
        if (!enemy.active) continue;
        enemy.y -= 0.01;
        if (enemy.y < -1.4) {
            enemy.y = 1.4;
            enemy.x = lanes[laneDist(gen)];
        }

        if (checkCollision(playerX, playerY, enemy.x, enemy.y)) {
            // Create explosion at collision point
            double explosionX = (playerX + enemy.x) / 2.0;
            double explosionY = (playerY + enemy.y) / 2.0;
            createExplosion(explosionX, explosionY);

            // reset game
            std::cout << "Game Over!!\n";
            gameOver = true;
            break;
        }
    }
}

// #endregion Enemy

void resetGame() {
    playerX = 0.0;
    playerY = -0.75;
    gameOver = false;
    paused = false;
    score = 0;
    laneOffset = 0;
    initEnemies();
    // Reset the start time so start/finish lines schedule restarts as well
    gameStartTimeMs = glutGet(GLUT_ELAPSED_TIME);

    // Reset non-wrapping scroll anchors so lines don't reappear on laneOffset wrap
    roadScroll = 0.0;
    startScroll0 = roadScroll;
    finishLineSpawned = false;
    finishScroll0 = 0.0;
}

void drawGameOverOverlay() {
    glColor3d(1, 0.2, 0.2);
    drawText(-0.3, 0.05, "GAME OVER");
    glColor3d(1, 1, 1);
    drawText(-0.4, -0.05, "Press Enter to Restart");
}

void keyboardNormal(unsigned char key, int x, int y) {
    if (key == 13 && gameOver) { // Enter key
        resetGame();
    } else if (key == 27) { // Esc key
        exit(0);
    }

    glutPostRedisplay();
}

void init() {
    initScenery(currentScenery);
    initRoad();
    initEnemies();
    initBridge();
    initExplosion();
    gameStartTimeMs = glutGet(GLUT_ELAPSED_TIME);
    // Initialize non-wrapping scroll anchors so lines don't reappear on laneOffset wrap
    roadScroll = 0.0;
    startScroll0 = roadScroll;
    finishLineSpawned = false;
}

void display() {
    glClearColor(0.53, 0.81, 0.92, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    drawScenery();
    drawRoad();
    drawCar(playerX, playerY, 0.2, 0.3, 0.9);
    drawEnemies();
    drawBridge();
    drawExplosion();
    drawScore();
    drawTimer();

    if (gameOver) {
        drawGameOverOverlay();
    }

    glFlush();
}

void keyboardSpecial(int key, int x, int y) {
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
    if (!gameOver && !gameFinished) {
        autoSwitchScenery();
        updateScenery();
        updateRoad();
        updateBridge();
        updateEnemies();
        updateScore();

        // Check if the player has crossed the finish line
        if (finishLineSpawned && roadScroll - finishScroll0 <= -1.6) {
            std::cout << "Congratulations! You finished the race!\n";
            gameFinished = true;
        }
    }

    updateExplosion();

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
    glutSpecialFunc(keyboardSpecial);
    glutKeyboardFunc(keyboardNormal);
    glutTimerFunc(30, update, 0);

    glutMainLoop();
    return 0;
}
