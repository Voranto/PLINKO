
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm> 
#include <iostream> 
#include <vector> 
using namespace std;

using namespace sf;
using namespace std;
using namespace std::chrono;

//variables constantes
const int frameheight = 1000;
const int framewidth = 1200;
const float ballradius = 15.0f;
const float staticballradius = 10.0f;
const int substeps = 10;
const Vector2f gravity(0, 1000);
const float friction = 0.6f;
const int numberoflevels = 16;
const float separation = 45;



int Random(int min, int max) {
    int val = min + rand() % max;
    return val;
}

class MoneyRect {
public:
    Vector2f oldpos;
    Vector2f pos;
    Vector2f vel = Vector2f(0,0);
    float width;
    float height;
    Vector2f opposite = Vector2f(0, -1000);
    int idx;
    MoneyRect(Vector2f pos2, float wid, float hei, int id) : pos(pos2), oldpos(pos2), width(wid), height(hei), idx(id) {}

    void update(float dt) {
        if (pos.y < oldpos.y){
            pos.y = oldpos.y;
            vel.y = 0;
        }
        else {
            if (vel.y != 0 and pos.y >= oldpos.y) {
                pos.y += vel.y*dt;
                vel.y += opposite.y*dt;
            }
        }
    }
};

class StaticBall {
public:
    Vector2f pos;
    float radius = staticballradius;
    Color color = Color(255, 255, 255, 255);
    StaticBall(Vector2f position) : pos(position) {}
};

class MovingBall {
public:
    Vector2f pos;
    Vector2f oldpos = Vector2f(0,0);
    Vector2f vel;
    float radius = ballradius;
    Color color = Color(255, 0, 0, 255);
    bool render = true;
    MovingBall(Vector2f pos2, Vector2f vel) : pos(pos2), vel(vel) {}

    void update(float dt) {
        vel += gravity * dt;
        pos += vel * dt;
    }

    void CheckConstraints() {
        Vector2f poscenter = pos + Vector2f(radius, radius);
        if (poscenter.x > framewidth - ballradius) {
            vel.x *= -1;
            pos.x = framewidth - 2 * radius;
        }
        if (poscenter.x <  0) {
            vel.x *= -1;
            pos.x = 0;
        }
    }

    void checkcollision(float dt, StaticBall& ball2) {
        Vector2f vec = pos + Vector2f(radius,radius) - ball2.pos - Vector2f(ball2.radius, ball2.radius);
        Vector2f perpendicular = pos + Vector2f(radius, radius) - ball2.pos - Vector2f(ball2.radius, ball2.radius);
        float dist2 = vec.x * vec.x + vec.y * vec.y;
        Vector2f n = vec / sqrt(dist2);
        if (dist2 < (radius + ball2.radius)* (radius + ball2.radius) and (vel.x * perpendicular.x < 0 or vel.y * perpendicular.y < 0)) {

            Vector2f overlap = n * (radius + ball2.radius -sqrt(dist2));
            pos += overlap;

            Vector2f r = vel - 2 * (vel.x * n.x + vel.y * n.y) * n;
            
            // Combine the reflected normal velocity and tangential velocity
            if (vel.y > 10 or n.y > 0.2){
                vel = r * friction;
            }
            else {
                float modulegravity = gravity.x * gravity.x + gravity.y * gravity.y;
                vel += Vector2f(modulegravity * perpendicular.x, perpendicular.y * n.y) / float(10000* numberoflevels + (numberoflevels-1));
            }
            


            
        }
    }
};



vector<MovingBall> MovingBallList;
vector<StaticBall> StaticBallList;
vector<MoneyRect> MoneyRectList;
vector<MoneyRect> Previous;
vector<Color> colorslist = { Color(255,0,0),Color(255,63.75,0), Color(255,127.5,0), Color(255,191.25,0), Color(255,255,0),Color(255,255,63.75),Color(255,255,127.5),Color(255,255,191.25),Color(255,255,255) };
vector<float> percentagelist = { 100,30,10,3,2,1.5f ,0.2f ,0.1f ,0.1f};
int main()
{
    //AUDIO RENDERING
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("main.wav"))return -1;
    sf::Sound clicksound;
    clicksound.setBuffer(buffer);
    sf::SoundBuffer buffer2;
    if (!buffer2.loadFromFile("win.wav"))return -1;
    sf::Sound winsound;
    winsound.setBuffer(buffer2);
    float initialmoney = 10000;
    float moneyperball = 50;

    cout << "How much starting money do you desire(default is 10000): ";
    cin >> initialmoney;
    cout << "How much starting money per ball do you desire(default is 50): ";
    cin >> moneyperball;
    //TEXT RENDERING
    Font font;
    if (!font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf")) {
        cerr << "Error loading font\n";
        return -1;
    }
    Text percentage;
    percentage.setFont(font);
    percentage.setFillColor(Color(0, 0, 0, 255));
    percentage.setCharacterSize(20);
    Text moneytext;
    moneytext.setFont(font);
    moneytext.setFillColor(Color(255, 255, 255, 255));
    moneytext.setCharacterSize(60);
    Text increasemoney;
    increasemoney.setFont(font);
    increasemoney.setFillColor(Color(255, 255, 255, 255));
    increasemoney.setCharacterSize(30);
    increasemoney.setPosition(Vector2f(0,70));


    sf::RenderWindow window(sf::VideoMode(framewidth,frameheight),"PLINKO");
    Event event;
    Clock clock;

    //building static balls
    for (int i = 2; i < numberoflevels +2; i++){
        for (int j = 0; j <= i ; j++){
            float separacion_inicial = framewidth/2 -  i * ballradius*2;
            float separacion_horizontal = j * ballradius*4;
            StaticBallList.emplace_back(StaticBall(Vector2f(separacion_inicial + separacion_horizontal, separation*(i-1))));
        }
    }
    int coloridx = 0;
    int colordirection = 1;
    //building rect list
    for (int x = 0; x < numberoflevels + 1; x++) {
        MoneyRectList.emplace_back(MoneyRect(Vector2f(framewidth / 2 - (numberoflevels + 2) * ballradius * 2 + x * ballradius * 4.02 + 2.5 * ballradius, numberoflevels * separation + 2 * ballradius + 10), ballradius * 4 , 40, coloridx));
        coloridx += colordirection;
        if (coloridx == colorslist.size() - 1) { colordirection = -1; }

    }
    float money = initialmoney;
    window.setFramerateLimit(60);
    while (window.isOpen()) {
        
        float deltaTime = clock.restart().asSeconds();
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::MouseButtonPressed and event.mouseButton.button == sf::Mouse::Left and money > moneyperball) //Mouse button Pressed
            {
                float xvel = Random(0, 30);
                while (xvel == 15){
                    xvel = Random(0, 30);
                }
                //play click sound
                clicksound.play();
                MovingBallList.emplace_back(MovingBall(Vector2f(framewidth/2 - ballradius + staticballradius + xvel - 15, 0), Vector2f(0, 0)));
                money -= moneyperball;
            }
            else if (event.type == sf::Event::MouseButtonPressed and event.mouseButton.button == sf::Mouse::Left and money > moneyperball) {
                cout << "YOU ARE OUT OF MONEY";
            }
        }
        window.clear();
        
        float subDelta = deltaTime / substeps;
        for (int t = 0; t < substeps; t++) {
            for (auto& ball : MovingBallList) {
                
                for (auto& otherball : StaticBallList) {
                    ball.checkcollision(subDelta, otherball);
                }
                ball.update(subDelta);
                ball.CheckConstraints();
                if (ball.pos.y + 2 * ballradius > numberoflevels * separation + 2 * ballradius + 10 and ball.render == true) {

                    int idx = floor((ball.pos.x - 97.5)/ 60.28235);
                    if (idx < 0 or idx > MoneyRectList.size() - 1) { ball.render = false; continue; }
                    MoneyRectList[idx].vel.y = 200;
                    MoneyRectList[idx].pos.y = MoneyRectList[idx].oldpos.y;
                    if (idx > 8) { idx = MoneyRectList.size() - idx -1 ; }
                    money += moneyperball * percentagelist[idx];
                    winsound.play();
                    
                    if (Previous.size() >= 5) {
                        Previous.pop_back();
                    }
                    Previous.insert(Previous.begin(),MoneyRectList[idx]);
                    ball.render = false;
                }

            }
            //update moneyblocks
            for (auto& block : MoneyRectList) {
                block.update(subDelta);
            }
        }
        

        CircleShape circle{ ballradius };
        for (auto& ball : MovingBallList) {
            if(ball.render == true){
            circle.setPosition(ball.pos);
            circle.setFillColor(ball.color);
            window.draw(circle);
            }
        }
         circle.setRadius(staticballradius);
        for (auto& ball : StaticBallList) {
            circle.setPosition(ball.pos);
            circle.setFillColor(ball.color);
            window.draw(circle);
        }

        RectangleShape rectangle(sf::Vector2f(ballradius*4-5, 40));
        vector<Color> colorslist = { Color(255,0,0),Color(255,63.75,0), Color(255,127.5,0), Color(255,191.25,0), Color(255,255,0),Color(255,255,63.75),Color(255,255,127.5),Color(255,255,191.25),Color(255,255,255) };
        coloridx = 0;
        colordirection = 1;
        for (int x = 0; x < numberoflevels+1; x++) {
            rectangle.setPosition(MoneyRectList[x].pos);
            
            rectangle.setFillColor(colorslist[coloridx]);
            string currpercentage = to_string(percentagelist[coloridx]);
            for (int c = currpercentage.size() - 1;c > -1;c--) {
                
                if (currpercentage[c] == to_string(0)) {
                    currpercentage.erase(c);
                }
                else {
                    break;
                }
            }
            percentage.setString(currpercentage + "x");
            percentage.setPosition(Vector2f(MoneyRectList[x].pos.x , MoneyRectList[x].pos.y + 10));
            coloridx+= colordirection;
            if (coloridx == colorslist.size() - 1) { colordirection = -1; }
            window.draw(rectangle);
            window.draw(percentage);

            
        }
        rectangle.setSize(Vector2f(ballradius*8-5,75));
        for (int p = 0; p < Previous.size(); p++) {
            rectangle.setPosition(Vector2f(framewidth - Previous[p].width*2 -10, 400 - Previous[p].height * p * 2));
            percentage.setString(to_string(percentagelist[Previous[p].idx]) + "x");
            percentage.setPosition(Vector2f(framewidth - Previous[p].width * 2 - 10, 400 - Previous[p].height * p * 2));
            rectangle.setFillColor(colorslist[Previous[p].idx]);
            window.draw(rectangle);
            window.draw(percentage);
        }

        moneytext.setString("Money: " + to_string(money));
        string percentageincrease = to_string(money / initialmoney * 100);
        for (int c = percentageincrease.size() - 1;c > -1;c--) {

            if (percentageincrease[c] == to_string(0)) {
                percentageincrease.erase(c);
            }
            else {
                break;
            }
        }
        increasemoney.setString("Increase: " + to_string(int(money - initialmoney)) + "(" + percentageincrease + "%)");

        if (money - initialmoney> 0) { increasemoney.setFillColor(Color(0, 255, 0, 255)); }
        else if (money - initialmoney < 0) { increasemoney.setFillColor(Color(255, 0, 0, 255)); }
        else { increasemoney.setFillColor(Color(255, 255, 255, 255)); }
        window.draw(increasemoney);
        window.draw(moneytext);
        window.display();
    }
    
    return 0;
}
