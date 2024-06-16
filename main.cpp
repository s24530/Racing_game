#include <chrono>
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <cmath>


constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

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

vect_t operator-(const vect_t a, const vect_t b) {
    vect_t ret = a;
    ret.v.x -= b.v.x;
    ret.v.y -= b.v.y;
    return ret;
}

class Car {

private:
    SDL_Rect rect;
    vect_t position = { 0, 0 };
    vect_t velocity = { 0, 0 };
    vect_t acceleration = { 0, 0 };
    double angle = 0.;
    double accelerationValue = 0.0;

    SDL_Texture* texture;
public:

    Car(int x, int y, SDL_Texture* tex) : texture(tex) {
        position.v.x = x;
        position.v.y = y;
        rect = { x, y, 25, 50 };
    }


    void accelerate(double value) {
        accelerationValue = value;
    }

    void decelerate(double value) {
        accelerationValue = -value*0.8;
    }

    //Turning the vehicle
    void turnLeft(double value) { angle -= value; }
    void turnRight(double value) { angle += value; }

    void draw(SDL_Renderer* renderer) const {
        SDL_RenderCopyEx(renderer, texture, nullptr, &rect, angle, nullptr, SDL_FLIP_NONE);
    }

    void update(double dt) {
        acceleration.v.y = -accelerationValue * std::cos(angle * M_PI / 180.0);
        acceleration.v.x = accelerationValue * std::sin(angle * M_PI / 180.0);


        //Physics using acceleration and velocity to determin position
        this->position = this->position + (this->velocity * dt) + (this->acceleration * dt * dt * 0.5);
        this->velocity = this->velocity + (this->acceleration * dt);
        this->velocity = this->velocity * 0.99;

        //Collision with bound so player can't go out of bounds
        if (position.v.x < 0) {
            position.v.x = 0;
            velocity.v.x = -velocity.v.x;
        }
        if (position.v.x + rect.w > WINDOW_WIDTH) {
            position.v.x = WINDOW_WIDTH - rect.w;
            velocity.v.x = -velocity.v.x;
        }
        if (position.v.y < 0) {
            position.v.y = 0;
            velocity.v.y = -velocity.v.y;
        }
        if (position.v.y + rect.h > WINDOW_HEIGHT) {
            position.v.y = WINDOW_HEIGHT - rect.h;
            velocity.v.y = -velocity.v.y;
        }

        //Update player rect based on the calculated position
        this->rect.x = static_cast<int>(this->position.v.x);
        this->rect.y = static_cast<int>(this->position.v.y);
    }


    //Handling car collision with each other

    bool checkCollision(const Car& other) const {
        return SDL_HasIntersection(&this->rect, &other.rect);
    }

    void handleCollision(Car& other) {

        vect_t temp = this->velocity;
        this->velocity = other.velocity;
        other.velocity = temp;

        vect_t displacement = this->position - other.position;
        double distance = std::sqrt(displacement.v.x * displacement.v.x + displacement.v.y * displacement.v.y);
        double overlap = 0.5 * (distance - (this->rect.w + other.rect.w) / 2);

        this->position.v.x -= overlap * (this->position.v.x - other.position.v.x) / distance;
        this->position.v.y -= overlap * (this->position.v.y - other.position.v.y) / distance;

        other.position.v.x += overlap * (this->position.v.x - other.position.v.x) / distance;
        other.position.v.y += overlap * (this->position.v.y - other.position.v.y) / distance;
    }
};


int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!init(window, renderer)) {
        std::cerr << "Failed to initialize SDL." << std::endl;
        return 1;
    }

    //Loading textures for cars and track
    std::vector<SDL_Texture*> textures;
    SDL_Texture* trackTexture = loadTexture("resources/track.bmp", renderer);
    if (!trackTexture) return 1;
    textures.push_back(trackTexture);

    SDL_Texture* car1Texture = loadTexture("resources/car1.bmp", renderer);
    if (!car1Texture) return 1;
    textures.push_back(car1Texture);

    SDL_Texture* car2Texture = loadTexture("resources/car2.bmp", renderer);
    if (!car2Texture) return 1;
    textures.push_back(car2Texture);

    bool quit = false;
    SDL_Event event;

    // Create player cars
    std::vector<Car> cars = {
        Car(100, 100, car1Texture),
        Car(200, 100, car2Texture)
    };

    // 60 fps animation
    double dt = 1./60.;


    //Game loop
    while (!quit) {
        //Even loop
        while (SDL_PollEvent(&event)) {
           switch(event.type) {
               case SDL_QUIT:
                quit = true;
               break;
               //Key press handle for car movement
               case SDL_KEYDOWN :
                     if (event.key.keysym.scancode == SDL_SCANCODE_W) cars[0].accelerate(50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_S) cars[0].decelerate(50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_A) cars[0].turnLeft(5);
                     if (event.key.keysym.scancode == SDL_SCANCODE_D) cars[0].turnRight(5);
                     if (event.key.keysym.scancode == SDL_SCANCODE_UP) cars[1].accelerate(50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) cars[1].decelerate(50.);
                     if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) cars[1].turnLeft(5);
                     if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) cars[1].turnRight(5);
               break;
                case SDL_KEYUP :
                    if (event.key.keysym.scancode == SDL_SCANCODE_W) cars[0].accelerate(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_S) cars[0].decelerate(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_A) cars[0].turnLeft(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_D) cars[0].turnRight(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_UP) cars[1].accelerate(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) cars[1].decelerate(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) cars[1].turnLeft(0);
                    if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) cars[1].turnLeft(0);
               break;
               default:
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
        }

        //Collision checking for both cars
        if (cars[0].checkCollision(cars[1])) {
            cars[0].handleCollision(cars[1]);
        }
        if (cars[1].checkCollision(cars[0])) {
            cars[1].handleCollision(cars[0]);
        }

        //Rendering cars
        for (auto& car : cars) {
            car.draw(renderer);
        }

        // Update screen
        SDL_RenderPresent(renderer);
    }

    cleanup(window, renderer, textures);
    return 0;
}