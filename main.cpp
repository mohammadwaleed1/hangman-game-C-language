#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QPainter>
#include <vector>
#include <queue>
#include <random>

using namespace std;

// -------------------- Maze Cell Types --------------------
#define WALL 0
#define PASSAGE 1
#define VISITED 2
#define PATH 3

// -------------------- Maze Class --------------------
class Maze {
public:
    int width, height;
    int sx, sy;        // Start at corner
    int ex, ey;        // End at opposite corner
    vector<vector<int>> grid;

    Maze(int w = 21, int h = 21) : width(w), height(h) {
        sx = 0; sy = 0;                // TOP-LEFT START
        ex = width - 1; ey = height - 1;  // BOTTOM-RIGHT END
        grid.resize(height, vector<int>(width, WALL));
    }

    bool isInside(int x, int y) {
        return x >= 0 && y >= 0 && x < width && y < height;
    }

    // -------------------- Maze Generation (Prim’s Algorithm) --------------------
    void generatePrim() {
        grid.assign(height, vector<int>(width, WALL));

        // Force corners to be open
        grid[sy][sx] = PASSAGE;
        grid[ey][ex] = PASSAGE;

        // Prim's works best starting from (1,1)
        int px = 1, py = 1;
        if (width > 2 && height > 2)
            grid[py][px] = PASSAGE;

        vector<pair<int,int>> walls;
        random_device rd;
        mt19937 gen(rd());

        walls.push_back({px+1, py});
        walls.push_back({px, py+1});

        while (!walls.empty()) {
            uniform_int_distribution<> dis(0, walls.size() - 1);
            int idx = dis(gen);
            auto [wx, wy] = walls[idx];
            walls.erase(walls.begin() + idx);

            if (!isInside(wx, wy)) continue;

            if (grid[wy][wx] == WALL) {
                int passages = 0;

                if (isInside(wx+1, wy) && grid[wy][wx+1] == PASSAGE) passages++;
                if (isInside(wx-1, wy) && grid[wy][wx-1] == PASSAGE) passages++;
                if (isInside(wx, wy+1) && grid[wy+1][wx] == PASSAGE) passages++;
                if (isInside(wx, wy-1) && grid[wy-1][wx] == PASSAGE) passages++;

                if (passages == 1) {
                    grid[wy][wx] = PASSAGE;

                    walls.push_back({wx+1, wy});
                    walls.push_back({wx-1, wy});
                    walls.push_back({wx, wy+1});
                    walls.push_back({wx, wy-1});
                }
            }
        }

        // Always ensure start corner connects
        if (width > 1) grid[0][1] = PASSAGE;
        if (height > 1) grid[1][0] = PASSAGE;

        // Always ensure end corner connects
        if (width > 1) grid[ey][ex-1] = PASSAGE;
        if (height > 1) grid[ey-1][ex] = PASSAGE;
    }

    // -------------------- BFS Shortest Path --------------------
    bool findShortestPathBFS() {
        queue<pair<int,int>> q;
        vector<vector<bool>> visited(height, vector<bool>(width,false));
        vector<vector<pair<int,int>>> parent(height, vector<pair<int,int>>(width, {-1,-1}));

        q.push({sx, sy});
        visited[sy][sx] = true;

        int dx[4] = {1,-1,0,0};
        int dy[4] = {0,0,1,-1};

        while(!q.empty()) {
            auto [x,y] = q.front(); q.pop();
            if(x==ex && y==ey) break;

            for(int i=0;i<4;i++){
                int nx=x+dx[i], ny=y+dy[i];
                if(nx>=0 && ny>=0 && nx<width && ny<height &&
                    !visited[ny][nx] && grid[ny][nx]==PASSAGE){
                    visited[ny][nx]=true;
                    if(!(nx==sx && ny==sy) && !(nx==ex && ny==ey))
                        grid[ny][nx]=VISITED;
                    parent[ny][nx] = {x,y};
                    q.push({nx,ny});
                }
            }
        }

        if(!visited[ey][ex]) return false;

        int cx = ex, cy = ey;
        while(!(cx==sx && cy==sy)){
            if(!(cx==sx && cy==sy) && !(cx==ex && cy==ey))
                grid[cy][cx] = PATH;
            auto [px, py] = parent[cy][cx];
            cx = px; cy = py;
        }

        return true;
    }
};

// -------------------- Maze Drawing Widget --------------------
class MazeWidget : public QWidget {
    Q_OBJECT
public:
    MazeWidget(QWidget* parent = nullptr) : QWidget(parent), maze(nullptr) {}
    Maze* maze;

protected:
    void paintEvent(QPaintEvent*) override {
        if (!maze) return;

        QPainter p(this);
        int cellSize = 20;

        for(int y=0; y<maze->height; y++){
            for(int x=0; x<maze->width; x++){
                if(x==maze->sx && y==maze->sy)
                    p.setBrush(Qt::magenta);
                else if(x==maze->ex && y==maze->ey)
                    p.setBrush(Qt::red);
                else {
                    switch(maze->grid[y][x]){
                    case WALL:     p.setBrush(QColor(0,102,204)); break;
                    case PASSAGE:  p.setBrush(Qt::white); break;
                    case VISITED:  p.setBrush(QColor(102,204,255)); break;
                    case PATH:     p.setBrush(Qt::yellow); break;
                    }
                }
                p.drawRect(x*cellSize, y*cellSize, cellSize, cellSize);
            }
        }
    }
};

// -------------------- Main Window --------------------
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
    void goToLevelScreen(){ stacked->setCurrentWidget(levelPage); }
    void startEasy(){ startMazeLevel(21); }
    void startMedium(){ startMazeLevel(31); }
    void startHard(){ startMazeLevel(41); }

    void solveMaze(){
        maze.findShortestPathBFS();
        mazeWidget->update();
        btnExit->setVisible(true);
    }

private:
    Maze maze;
    QStackedWidget *stacked;
    QWidget *titlePage, *levelPage, *mazePage;
    MazeWidget *mazeWidget;
    QPushButton *btnSolve;
    QPushButton *btnExit;

    // -------------------- Title Screen --------------------
    void setupTitleScreen(){
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

    // -------------------- Level Screen --------------------
    void setupLevelScreen(){
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

    // -------------------- Maze Screen --------------------
    void setupMazeScreen(){
        mazePage = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout();

        mazeWidget = new MazeWidget();
        mazeWidget->maze = &maze;

        btnSolve = new QPushButton("Solve Maze");
        btnExit = new QPushButton("Exit Game");
        btnExit->setVisible(false);

        btnSolve->setStyleSheet("font-size: 18px; padding: 8px; background-color: purple; color: white;");
        btnExit->setStyleSheet("font-size: 18px; padding: 8px; background-color: red; color: white;");

        btnSolve->setFixedWidth(350);
        btnExit->setFixedWidth(350);

        layout->addWidget(mazeWidget, 0, Qt::AlignCenter);
        layout->addSpacing(15);
        layout->addWidget(btnSolve, 0, Qt::AlignCenter);
        layout->addWidget(btnExit, 0, Qt::AlignCenter);
        layout->addStretch();

        mazePage->setLayout(layout);
        stacked->addWidget(mazePage);

        connect(btnSolve, &QPushButton::clicked, this, &Window::solveMaze);

        // ⭐ FIX: Go back to Home instead of closing
        connect(btnExit, &QPushButton::clicked, this, [this](){
            stacked->setCurrentWidget(titlePage);
        });
    }

    // -------------------- AUTO-RESIZE FOR MAZE --------------------
    void startMazeLevel(int size){
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

// -------------------- MAIN --------------------
int main(int argc, char *argv[]){
    QApplication app(argc, argv);
    Window w;
    w.show();
    return app.exec();
}

#include "main.moc"
