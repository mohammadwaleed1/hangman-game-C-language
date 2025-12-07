#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QPainter>
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>

using namespace std;

#define WALL 0
#define PASSAGE 1
#define VISITED 2
#define PATH 3

//Maze Class
class Maze {
public:
    int width, height;
    int sx, sy;
    int ex, ey;
    vector<vector<int>> grid;

    Maze(int w = 21, int h = 21) : width(w), height(h) {
        sx = 0; sy = 0;// Start top-left
        ex = width - 1; ey = height - 1;  // End bottom-right
        grid.resize(height, vector<int>(width, WALL));
    }

    bool isInside(int x, int y) {
        return x >= 0 && y >= 0 && x < width && y < height;
    }

    void addWalls(int x, int y, vector<pair<int,int>> &walls) {
        if (isInside(x+1, y)) walls.push_back(make_pair(x+1, y));
        if (isInside(x-1, y)) walls.push_back(make_pair(x-1, y));
        if (isInside(x, y+1)) walls.push_back(make_pair(x, y+1));
        if (isInside(x, y-1)) walls.push_back(make_pair(x, y-1));
    }

    void generatePrim() {
        // Fill maze with walls
        grid.assign(height, vector<int>(width, WALL));

        // Start selecting from (1,1)
        int px = 1, py = 1;
        grid[py][px] = PASSAGE;

        vector<pair<int,int>> walls;
        addWalls(px, py, walls);

        srand(time(NULL));

        while (!walls.empty()) {
            int idx = rand() % walls.size();
            int wx = walls[idx].first;
            int wy = walls[idx].second;
            walls.erase(walls.begin() + idx);

            if (!isInside(wx, wy)) continue;

            // Count neighboring passages
            int count = 0;
            if (isInside(wx+1, wy) && grid[wy][wx+1] == PASSAGE) count++;
            if (isInside(wx-1, wy) && grid[wy][wx-1] == PASSAGE) count++;
            if (isInside(wx, wy+1) && grid[wy+1][wx] == PASSAGE) count++;
            if (isInside(wx, wy-1) && grid[wy-1][wx] == PASSAGE) count++;

            // select only if exactly 1 neighbor is passage
            if (count == 1) {
                grid[wy][wx] = PASSAGE;
                addWalls(wx, wy, walls);
            }
        }

        // Ensure start and end are open
        grid[sy][sx] = PASSAGE;
        if (isInside(sx+1, sy)) grid[sy][sx+1] = PASSAGE;
        grid[ey][ex] = PASSAGE;
        if (isInside(ex-1, ey)) grid[ey][ex-1] = PASSAGE;
    }
    // BFS Shortest Path
    bool findShortestPathBFS() {
        // Queue stores x, y, and previous coordinates
        struct Node {
            int x, y, px, py;
        };

        queue<Node> q;
        q.push({sx, sy, -1, -1}); // start

        // 2D visited array
        vector<vector<bool>> visited(height, vector<bool>(width,false));

        visited[sy][sx] = true;

        // Directions: right, left, down, up
        int dx[4] = {1,-1,0,0};
        int dy[4] = {0,0,1,-1};

        // To store the path back
        vector<vector<pair<int,int>>> cameFrom(height, vector<pair<int,int>>(width, make_pair(-1,-1)));

        while (!q.empty()) {
            Node current = q.front(); q.pop();

            if (current.x == ex && current.y == ey) break;

            for (int i=0; i<4; i++) {
                int nx = current.x + dx[i];
                int ny = current.y + dy[i];

                if (nx>=0 && ny>=0 && nx<width && ny<height &&
                    !visited[ny][nx] && grid[ny][nx] == PASSAGE) {
                    visited[ny][nx] = true;

                    // Mark as visited
                    if (!(nx==sx && ny==sy) && !(nx==ex && ny==ey))
                        grid[ny][nx] = VISITED;

                    // Store previous cell for path backtracking
                    cameFrom[ny][nx] = make_pair(current.x, current.y);

                    q.push({nx, ny, current.x, current.y});
                }
            }
        }

        // If end not reached
        if (!visited[ey][ex]) return false;

        // Backtrack path
        int cx = ex, cy = ey;
        while (!(cx == sx && cy == sy)) {
            if (!(cx==sx && cy==sy) && !(cx==ex && cy==ey))
                grid[cy][cx] = PATH;

            int px = cameFrom[cy][cx].first;
            int py = cameFrom[cy][cx].second;
            cx = px;
            cy = py;
        }

        return true;
    }

};

// Maze Widget
class MazeWidget : public QWidget {
    Q_OBJECT
public:
    MazeWidget(QWidget* parent=nullptr) : QWidget(parent), maze(nullptr) {}
    Maze* maze;

protected:
    void paintEvent(QPaintEvent*) override {
        if (!maze) return;

        QPainter p(this);
        int cellSize = 20;

        for (int y=0; y<maze->height; y++) {
            for (int x=0; x<maze->width; x++) {
                if (x==maze->sx && y==maze->sy) p.setBrush(Qt::magenta);
                else if (x==maze->ex && y==maze->ey) p.setBrush(Qt::red);
                else {
                    switch(maze->grid[y][x]) {
                    case WALL: p.setBrush(QColor(0,102,204)); break;
                    case PASSAGE: p.setBrush(Qt::white); break;
                    case VISITED: p.setBrush(QColor(102,204,255)); break;
                    case PATH: p.setBrush(Qt::yellow); break;
                    }
                }
                p.drawRect(x*cellSize, y*cellSize, cellSize, cellSize);
            }
        }
    }
};

// Main Window
class Window : public QWidget {
    Q_OBJECT
public:
    Window(QWidget *parent=nullptr) : QWidget(parent), maze(21,21) {
        stacked = new QStackedWidget(this);
        setupTitleScreen();
        setupLevelScreen();
        setupMazeScreen();

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(stacked);
        setLayout(mainLayout);

        stacked->setCurrentWidget(titlePage);
        setWindowTitle("Maze Game");
        resize(460, 540);
    }

private slots:
    void goToLevelScreen() { stacked->setCurrentWidget(levelPage); }
    void startEasy() { startMazeLevel(21); }
    void startMedium() { startMazeLevel(31); }
    void startHard() { startMazeLevel(41); }
    void solveMaze() { maze.findShortestPathBFS(); mazeWidget->update(); btnExit->setVisible(true); }
    void backToHome() { stacked->setCurrentWidget(titlePage); }

private:
    Maze maze;
    QStackedWidget *stacked;
    QWidget *titlePage, *levelPage, *mazePage;
    MazeWidget *mazeWidget;
    QPushButton *btnSolve, *btnExit;

    //Title Screen
    void setupTitleScreen() {
        titlePage = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout();

        QLabel *title = new QLabel("Maze Game");
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 40px; font-weight: bold; color: orange;");

        QPushButton *play = new QPushButton("Play");
        QPushButton *exit = new QPushButton("Exit");

        play->setStyleSheet("font-size: 20px; padding: 10px; background-color: green; color: white;");
        exit->setStyleSheet("font-size: 20px; padding: 10px; background-color: red; color: white;");

        layout->addStretch();
        layout->addWidget(title);
        layout->addSpacing(30);
        layout->addWidget(play);
        layout->addWidget(exit);
        layout->addStretch();
        titlePage->setLayout(layout);

        stacked->addWidget(titlePage);

        connect(play, &QPushButton::clicked, this, &Window::goToLevelScreen);
        connect(exit, &QPushButton::clicked, this, &QWidget::close);
    }

    //  Level Screen
    void setupLevelScreen() {
        levelPage = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout();

        QLabel *label = new QLabel("Select Level");
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 30px; font-weight: bold; color: darkblue;");

        QPushButton *easy = new QPushButton("Easy");
        QPushButton *medium = new QPushButton("Medium");
        QPushButton *hard = new QPushButton("Hard");

        easy->setStyleSheet("font-size: 18px; padding: 8px; background-color: lightgreen;");
        medium->setStyleSheet("font-size: 18px; padding: 8px; background-color: yellow;");
        hard->setStyleSheet("font-size: 18px; padding: 8px; background-color: orange;");

        layout->addStretch();
        layout->addWidget(label);
        layout->addSpacing(20);
        layout->addWidget(easy);
        layout->addWidget(medium);
        layout->addWidget(hard);
        layout->addStretch();

        levelPage->setLayout(layout);
        stacked->addWidget(levelPage);

        connect(easy, &QPushButton::clicked, this, &Window::startEasy);
        connect(medium, &QPushButton::clicked, this, &Window::startMedium);
        connect(hard, &QPushButton::clicked, this, &Window::startHard);
    }

    // Maze Screen
    void setupMazeScreen() {
        mazePage = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout();

        mazeWidget = new MazeWidget();
        mazeWidget->maze = &maze;

        btnSolve = new QPushButton("Solve Maze");
        btnExit = new QPushButton("Exit Game");
        btnExit->setVisible(false);

        btnSolve->setStyleSheet("font-size: 18px; padding: 8px; background-color: purple; color: white;");
        btnExit->setStyleSheet("font-size: 18px; padding: 8px; background-color: red; color: white;");

        layout->addWidget(mazeWidget, 0, Qt::AlignCenter);
        layout->addSpacing(15);
        layout->addWidget(btnSolve, 0, Qt::AlignCenter);
        layout->addWidget(btnExit, 0, Qt::AlignCenter);
        layout->addStretch();

        mazePage->setLayout(layout);
        stacked->addWidget(mazePage);

        connect(btnSolve, &QPushButton::clicked, this, &Window::solveMaze);
        connect(btnExit, &QPushButton::clicked, this, &Window::backToHome);
    }

    void startMazeLevel(int size) {
        maze = Maze(size, size);
        maze.generatePrim();
        mazeWidget->maze = &maze;

        int cell = 20;
        int mazeW = maze.width * cell;
        int mazeH = maze.height * cell;

        mazeWidget->setFixedSize(mazeW, mazeH);
        mazeWidget->update();

        int windowW = mazeW + 80;
        int windowH = mazeH + 200;

        this->setMinimumSize(windowW, windowH);
        this->resize(windowW, windowH);

        btnSolve->setFixedWidth(windowW * 0.6);
        btnExit->setFixedWidth(windowW * 0.6);

        stacked->setCurrentWidget(mazePage);
        btnExit->setVisible(false);
        update();
    }
};

//main
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Window w;
    w.show();
    return app.exec();
}

#include "main.moc"
