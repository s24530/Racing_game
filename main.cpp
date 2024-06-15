#include <chrono>
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <cmath>


const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

bool init(SDL_Window*& window, SDL_Renderer*& renderer) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Multiplayer Racing Game", 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        return false;
    }

    return true;
}

SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Texture* newTexture = nullptr;
    SDL_Surface* loadedSurface = SDL_LoadBMP(path.c_str());
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to load image " << path << "! SDL Error: " << SDL_GetError() << std::endl;
    } else {
        SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format,127, 127, 127));
        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (newTexture == nullptr) {
            std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        }
        SDL_FreeSurface(loadedSurface);
    }
    return newTexture;
}

void cleanup(SDL_Window* window, SDL_Renderer* renderer, std::vector<SDL_Texture*>& textures) {
    for (SDL_Texture* texture : textures) {
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


union vect_t {
    struct { double x; double y;} v;
};

vect_t operator+(const vect_t a, const vect_t b) {
    vect_t ret = a;
    ret.v.x += b.v.x;
    ret.v.y += b.v.y;
    return ret;
}
vect_t operator*(const vect_t a, const double b) {
    vect_t ret = a;
    ret.v.x *= b;
    ret.v.y *= b;
    return ret;
}

class Car {
public:
    SDL_Rect rect;
    vect_t position = {0,0} ;
    vect_t velocity = {0,0} ;
    vect_t acceleration = {0,0} ;

    SDL_Texture* texture;

    Car(int x, int y, SDL_Texture* tex) :  texture(tex) {
        position.v.x = x;
        position.v.y = y;
        rect = {x,y,50,100};
    }


    void moveUp(double value) { acceleration.v.y = value; }
    void moveDown(double value) { acceleration.v.y = value; }
    void moveLeft(double value) { acceleration.v.x = value;  }
    void moveRight(double value) { acceleration.v.x = value; }

    void draw(SDL_Renderer* renderer) {
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
    }
    void update(double dt) {
        this->position = this->position + (this->velocity * dt) + (this->acceleration * dt * dt * 0.5);
        this->velocity = this->velocity + (this->acceleration * dt);
        this->velocity = this->velocity * 0.99;
        this->rect.x = static_cast<int>(this->position.v.x);
        this->rect.y = static_cast<int>(this->position.v.y);
    }

};


int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!init(window, renderer)) {
        std::cerr << "Failed to initialize SDL." << std::endl;
        return 1;
    }

    std::vector<SDL_Texture*> textures;
    SDL_Texture* trackTexture = loadTexture("track.bmp", renderer);
    if (!trackTexture) return 1;
    textures.push_back(trackTexture);

    SDL_Texture* car1Texture = loadTexture("car1.bmp", renderer);
    if (!car1Texture) return 1;
    textures.push_back(car1Texture);

    SDL_Texture* car2Texture = loadTexture("car2.bmp", renderer);
    if (!car2Texture) return 1;
    textures.push_back(car2Texture);

    bool quit = false;
    SDL_Event event;

    // Create player cars
    std::vector<Car> cars = {
        Car(100, 100, car1Texture),
        Car(200, 100, car2Texture)
    };
    double dt = 1./60.;

    auto tp1 = std::chrono::system_clock::now();
    auto tp2 = std::chrono::system_clock::now();

    while (!quit) {
        tp2 = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();
        while (SDL_PollEvent(&event)) {
           switch(event.type) {
               case SDL_QUIT:
                quit = true;
               break;
               case SDL_KEYDOWN :
                     if (event.key.keysym.scancode == SDL_SCANCODE_W) cars[0].moveUp(-50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_S) cars[0].moveDown(50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_A) cars[0].moveLeft(-50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_D) cars[0].moveRight(50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_UP) cars[1].moveUp(-50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) cars[1].moveDown(50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) cars[1].moveLeft(-50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) cars[1].moveRight(50);
               break;
                case SDL_KEYUP :
                    if (event.key.keysym.scancode == SDL_SCANCODE_W) cars[0].moveUp(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_S) cars[0].moveDown(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_A) cars[0].moveLeft(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_D) cars[0].moveRight(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_UP) cars[1].moveUp(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) cars[1].moveDown(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) cars[1].moveLeft(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) cars[1].moveRight(0);
               break;
             }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw track
        SDL_RenderCopy(renderer, trackTexture, nullptr, nullptr);
        // Draw cars
        for (auto& car : cars) {
            car.update(dt);
            car.draw(renderer);
        }


        // Update screen
        SDL_RenderPresent(renderer);
    }

    cleanup(window, renderer, textures);
    return 0;
}
