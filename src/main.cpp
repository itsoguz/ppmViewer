#include <iostream>
#include <vector>

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QPushButton>

#include <SDL3/SDL.h>

int WINDOW_WIDTH = 900;
int WINDOW_HEIGHT = 600;

SDL_Event event;

bool quit = false;

int main(int argc, char *argv[]){
    QApplication app(argc, argv);

    QMainWindow* mainWindow = new QMainWindow();

    mainWindow->setWindowTitle(".ppm Image Viewer");
    mainWindow->resize(400, 200);

    QPushButton *openButton = new QPushButton("Open Image");
    openButton->setStyleSheet("font-size: 24px; color: white; background-color: green;");
    mainWindow->setCentralWidget(openButton);
    mainWindow->show();

    int picWidth = 0;
    int picHeight = 0;

    QString filePath;

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "Problem Initializing SDL..";
        return -1;
    }

    // Create SDL window and renderer as nullptr initially
    SDL_Window* sdlWindow = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* imageTexture = nullptr;

    QObject::connect(openButton, &QPushButton::clicked, [&]() {
        filePath = QFileDialog::getOpenFileName(mainWindow, "Choose a .ppm Image File", "", "Image Files (*.ppm)");
        if(!filePath.isEmpty()){
            qDebug() << filePath << "\n";

            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly))
                return;

            QTextStream in(&file);

            // 1. Read Magic Number
            QByteArray line = file.readLine().trimmed();
            if (line != "P6") {
                qDebug() << "Not a P6 file!";
                return;
            }

            // 2. Read Dimensions (Ignoring comments)
            while (!file.atEnd()) {
                line = file.readLine().trimmed();
                if (line.isEmpty() || line.startsWith('#'))
                    continue;

                QList<QByteArray> parts = line.split(' ');
                if (parts.size() >= 2) {
                    picWidth = parts[0].toInt();
                    picHeight = parts[1].toInt();
                    break;
                }
            }

            // 3. Read Max Color Value
            line = file.readLine().trimmed();
            int maxColor = line.toInt();

            qDebug() << "Dimensions:" << picWidth << "x" << picHeight;
            qDebug() << "Max Color Value:" << maxColor;

            // The file pointer is now exactly at the start of the binary pixels.
            std::vector<unsigned char> pixelData(picWidth * picHeight * 3);
            file.read(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
            file.close();

            // Create SDL window now with correct dimensions
            if (!sdlWindow) {
                sdlWindow = SDL_CreateWindow(".ppm Image", picWidth, picHeight, 0);
                renderer = SDL_CreateRenderer(sdlWindow, NULL);

                if(!sdlWindow){
                    std::cout << "Window Creation Failed...";
                    SDL_Quit();
                    return;
                }
            } else {
                // Resize existing window
                SDL_SetWindowSize(sdlWindow, picWidth, picHeight);
            }

            // Update window dimensions
            WINDOW_WIDTH = picWidth;
            WINDOW_HEIGHT = picHeight;

            // 4. Update SDL Texture
            if (imageTexture)
                SDL_DestroyTexture(imageTexture);

            SDL_Surface* surface = SDL_CreateSurfaceFrom(picWidth, picHeight, SDL_PIXELFORMAT_RGB24, pixelData.data(), 3 * picWidth);
            imageTexture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);

            qDebug() << "Image file loaded on screen";
        }
    });

    //Main Loop
    while(!quit){
        // 1. Keep Qt Alive
        app.processEvents();

        // 2. Handle SDL Events (only if window exists)
        if (sdlWindow) {
            while(SDL_PollEvent(&event)) {
                if(event.type == SDL_EVENT_QUIT) {
                    quit = true;
                }
            }

            // 3. Render
            SDL_SetRenderDrawColor(renderer, 0, 32, 32, 255);
            SDL_RenderClear(renderer);

            if (imageTexture) {
                SDL_FRect destRect = { 0, 0, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)};
                SDL_RenderTexture(renderer, imageTexture, NULL, &destRect);
            }

            SDL_RenderPresent(renderer);
        }

        SDL_Delay(16); //approx 60 FPS
    }

    if (imageTexture)
        SDL_DestroyTexture(imageTexture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (sdlWindow)
        SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
    return app.exec();
}
