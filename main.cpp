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

    window = SDL_CreateWindow("Racing Game", 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
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
    SDL_Rect carRect;
    SDL_Rect finishLine = {470,100,10,50};
    vect_t position = { 0, 0 };
    vect_t velocity = { 0, 0 };
    vect_t acceleration = { 0, 0 };
    double angle = 90.;
    double accelerationValue = 0.0;
    std::vector<SDL_Rect> trackBounds = {
        {170, 160, 385, 5},
        {170, 160, 5, 314},
        {557, 160, 5, 314},
        {355, 298, 5, 302}
    };
    int timesPassedFinishLine = 0;

    SDL_Texture* texture;

public:

    Car(int x, int y, SDL_Texture* tex) : texture(tex) {
        position.v.x = x;
        position.v.y = y;
        carRect = { x, y, 20, 40 };
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
        SDL_RenderCopyEx(renderer, texture, nullptr, &carRect, angle, nullptr, SDL_FLIP_NONE);
    }

    void update(double dt) {
        acceleration.v.y = -accelerationValue * std::cos(angle * M_PI / 180.0);
        acceleration.v.x = accelerationValue * std::sin(angle * M_PI / 180.0);


        //Physics using acceleration and velocity to determin position
        this->position = this->position + (this->velocity * dt) + (this->acceleration * dt * dt * 0.5);
        this->velocity = this->velocity + (this->acceleration * dt);
        this->velocity = this->velocity * 0.99;

        //Collision with map trackBound so player can't go out of trackBounds
        if (position.v.x < 0) {
            position.v.x = 0;
            velocity.v.x = -velocity.v.x;
        }
        if (position.v.x + carRect.w > WINDOW_WIDTH) {
            position.v.x = WINDOW_WIDTH - carRect.w;
            velocity.v.x = -velocity.v.x;
        }
        if (position.v.y < 0) {
            position.v.y = 0;
            velocity.v.y = -velocity.v.y;
        }
        if (position.v.y + carRect.h > WINDOW_HEIGHT) {
            position.v.y = WINDOW_HEIGHT - carRect.h;
            velocity.v.y = -velocity.v.y;
        }


        //Handle collision with track bounds
        for (const auto& trackBound : trackBounds) {
            if (SDL_HasIntersection(&carRect, &trackBound)) {
                //Determine the side of the collision

                //From the left
                if (position.v.x + carRect.w > trackBound.x && position.v.x < trackBound.x) {
                    position.v.x = trackBound.x - carRect.w;
                    velocity.v.x = -velocity.v.x * 0.5; 
                }

                //From the right
                if (position.v.x < trackBound.x + trackBound.w && position.v.x + carRect.w > trackBound.x + trackBound.w) {
                    position.v.x = trackBound.x + trackBound.w;
                    velocity.v.x = -velocity.v.x * 0.5; 
                }

                //From above
                if (position.v.y + carRect.h > trackBound.y && position.v.y < trackBound.y) {
                    position.v.y = trackBound.y - carRect.h;
                    velocity.v.y = -velocity.v.y * 0.5; 
                }

                //From below
                if (position.v.y < trackBound.y + trackBound.h && position.v.y + carRect.h > trackBound.y + trackBound.h) {
                    position.v.y = trackBound.y + trackBound.h;
                    velocity.v.y = -velocity.v.y * 0.5; 
                }
            }
        }

        //Update player rect based on the calculated position
        this->carRect.x = static_cast<int>(this->position.v.x);
        this->carRect.y = static_cast<int>(this->position.v.y);
    }


    //Handling car collision with each other
    bool checkCollision(const Car& other) const {
        return SDL_HasIntersection(&this->carRect, &other.carRect);
    }

    void handleCollision(Car& other) {

        vect_t temp = this->velocity;
        this->velocity = other.velocity;
        other.velocity = temp;

        vect_t displacement = this->position - other.position;
        double distance = std::sqrt(displacement.v.x * displacement.v.x + displacement.v.y * displacement.v.y);
        double overlap = 0.5 * (distance - (this->carRect.w + other.carRect.w) / 2);

        this->position.v.x -= overlap * (this->position.v.x - other.position.v.x) / distance;
        this->position.v.y -= overlap * (this->position.v.y - other.position.v.y) / distance;

        other.position.v.x += overlap * (this->position.v.x - other.position.v.x) / distance;
        other.position.v.y += overlap * (this->position.v.y - other.position.v.y) / distance;
    }


    //Handling crossing the finsh line
    bool checkFinishLine() const {
        return SDL_HasIntersection(&this->carRect, &finishLine);
    }

    void passedFinishLine() {
        timesPassedFinishLine += 1;
    }

    int getTimesPassed() const {
        return timesPassedFinishLine;
    }

    //Used to stop cars after they cross the finish line
    void stop() {
        velocity = {0, 0};
        acceleration = {0, 0};
        accelerationValue = 0.0;
    }
};

//Print winner message for players
void printWinner(SDL_Renderer* renderer, SDL_Texture* winnerTexture) {
    SDL_Rect dstRect = { 0 , 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderCopy(renderer, winnerTexture, nullptr, &dstRect);
}


int main(int argc, char* argv[]) {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    if (!init(window, renderer)) {
        std::cerr << "Failed to initialize SDL." << std::endl;
        return 1;
    }

    //Loading textures for cars and track
    std::vector<SDL_Texture *> textures;
    SDL_Texture *trackTexture = loadTexture("resources/track.bmp", renderer);
    if (!trackTexture) return 1;
    textures.push_back(trackTexture);

    SDL_Texture *car1Texture = loadTexture("resources/car1.bmp", renderer);
    if (!car1Texture) return 1;
    textures.push_back(car1Texture);

    SDL_Texture *car2Texture = loadTexture("resources/car2.bmp", renderer);
    if (!car2Texture) return 1;
    textures.push_back(car2Texture);

    SDL_Texture *winnerTexture = loadTexture("resources/winner1.bmp", renderer);
    if (!winnerTexture) return 1;
    textures.push_back(winnerTexture);

    bool quit = false;
    bool raceFinished = false;
    SDL_Event event;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);;

    // Create player cars
    std::vector<Car> cars = {
            Car(370, 60, car1Texture),
            Car(370, 110, car2Texture)
    };

    // 60 fps animation
    double dt = 1. / 60.;


    //Game loop
    while (!quit) {
        //Even loop
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_QUIT)
                quit = true;

        }

        cars[0].accelerate(0);
        cars[0].decelerate(0);
        cars[0].turnLeft(0);
        cars[0].turnRight(0);
        cars[1].accelerate(0);
        cars[1].decelerate(0);
        cars[1].turnLeft(0);
        cars[1].turnLeft(0);
        //Key press handle for car movement
        if (!raceFinished) {
            if (keys[SDL_SCANCODE_W]) cars[0].accelerate(50.);
            if (keys[SDL_SCANCODE_S]) cars[0].decelerate(50.);
            if (keys[SDL_SCANCODE_A]) cars[0].turnLeft(1);
            if (keys[SDL_SCANCODE_D]) cars[0].turnRight(1);
            if (keys[SDL_SCANCODE_UP]) cars[1].accelerate(50.);
            if (keys[SDL_SCANCODE_DOWN]) cars[1].decelerate(50.);
            if (keys[SDL_SCANCODE_LEFT]) cars[1].turnLeft(1);
            if (keys[SDL_SCANCODE_RIGHT]) cars[1].turnRight(1);
        }


        if (keys[SDL_SCANCODE_ESCAPE]) quit = true;




        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw track
        SDL_RenderCopy(renderer, trackTexture, nullptr, nullptr);

        // Update cars
        for (auto &car: cars) {
            car.update(dt);
        }

        //Checking if player passed the finish line 2 lines
        //-> using bigger value due to the amount of collision between rects
        if (!raceFinished) {
            if (cars[0].checkFinishLine()) {
                if (cars[0].getTimesPassed() < 2) {
                    cars[0].passedFinishLine();
                } else {
                    raceFinished = true;
                    winnerTexture = loadTexture("resources/winner1.bmp", renderer);
                    if (!winnerTexture) return 1;
                    textures.push_back(winnerTexture);
                    for (auto &car: cars) car.stop();
                }
            }
            if (cars[1].checkFinishLine()) {
                if (cars[1].getTimesPassed() < 40) {
                    cars[1].passedFinishLine();
                } else {
                    raceFinished = true;
                    winnerTexture = loadTexture("resources/winner2.bmp", renderer);
                    if (!winnerTexture) return 1;
                    textures.push_back(winnerTexture);
                    for (auto &car: cars) car.stop();
                }
            }
        }

        //Collision checking for both cars
        if (cars[0].checkCollision(cars[1])) {
            cars[0].handleCollision(cars[1]);
        }
        if (cars[1].checkCollision(cars[0])) {
            cars[1].handleCollision(cars[0]);
        }

        //Rendering cars
        for (auto &car: cars) {
            car.draw(renderer);
        }

        //Print winner message
        if (raceFinished) {
            printWinner(renderer, winnerTexture);
        }

        // Update screen
        SDL_RenderPresent(renderer);

    }


    cleanup(window, renderer, textures);
    return 0;
}